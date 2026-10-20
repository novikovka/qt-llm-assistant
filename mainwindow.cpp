#include "mainwindow.h"

#include "llmclient.h"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QTextBrowser>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
    , m_client(new LlmClient(this))  // [новое на шаге 2]
{
    setWindowTitle(QStringLiteral("Локальный ассистент (llama.cpp)"));

    // --- Создаём элементы интерфейса ---
    m_chatView = new QTextBrowser(this);
    m_chatView->setOpenExternalLinks(true);

    m_input = new QLineEdit(this);
    m_input->setPlaceholderText(QStringLiteral("Введите вопрос и нажмите Enter"));

    m_sendButton = new QPushButton(QStringLiteral("Отправить"), this);

    // --- Раскладываем их по окну ---
    // Нижняя строка: поле ввода растягивается, кнопка справа
    auto *inputRow = new QHBoxLayout;
    inputRow->addWidget(m_input);
    inputRow->addWidget(m_sendButton);

    // Всё окно: сверху история (забирает всё свободное место), снизу строка ввода
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_chatView, 1);
    mainLayout->addLayout(inputRow);

    // --- Связываем действия пользователя с обработчиком ---
    connect(m_sendButton, &QPushButton::clicked, this, &MainWindow::onSend);
    connect(m_input, &QLineEdit::returnPressed, this, &MainWindow::onSend);

    // --- Связываем сигналы сетевого слоя с окном ---   [новое на шаге 2]
    connect(m_client, &LlmClient::answerReady, this, &MainWindow::onAnswer);
    connect(m_client, &LlmClient::errorOccurred, this, &MainWindow::onError);
}

void MainWindow::onSend()
{
    const QString question = m_input->text().trimmed();
    if (question.isEmpty() || m_busy)
        return;  // пустой вопрос не отправляем; пока ждём ответ — тоже

    m_transcript += QStringLiteral("**Вы:** ") + question + QStringLiteral("\n\n");
    m_input->clear();

    setBusy(true);             // [изменено на шаге 2: вместо заглушки]
    m_client->ask(question);   // [изменено на шаге 2: настоящий запрос]
}

// [новое на шаге 2]
void MainWindow::onAnswer(const QString &text)
{
    // Подпись на отдельной строке: если ответ начнётся со списка или кода,
    // Markdown не сломается
    m_transcript += QStringLiteral("**Модель:**\n\n") + text + QStringLiteral("\n\n");
    setBusy(false);
}

// [новое на шаге 2]
void MainWindow::onError(const QString &message)
{
    m_transcript += QStringLiteral("**Ошибка:** ") + message
                  + QStringLiteral("\n\n*Проверьте, что llama-server запущен на порту 8080.*\n\n");
    setBusy(false);
}

// [новое на шаге 2]
void MainWindow::setBusy(bool busy)
{
    m_busy = busy;
    m_input->setEnabled(!busy);
    m_sendButton->setEnabled(!busy);
    refreshChat();

    if (!busy)
        m_input->setFocus();  // сразу можно печатать следующий вопрос
}

void MainWindow::refreshChat()
{
    QString text = m_transcript;
    if (m_busy)
        text += QStringLiteral("*Модель думает…*\n\n");  // [новое на шаге 2]

    // setMarkdown превращает **жирный**, списки и код в оформленный текст
    m_chatView->setMarkdown(text);

    // Прокручиваем историю к последнему сообщению
    QScrollBar *bar = m_chatView->verticalScrollBar();
    bar->setValue(bar->maximum());
}
