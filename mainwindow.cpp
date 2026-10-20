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
    , m_client(new LlmClient(this))
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

    // --- Связываем сигналы сетевого слоя с окном ---   [изменено на этапе 4]
    connect(m_client, &LlmClient::chunkReceived, this, &MainWindow::onChunk);
    connect(m_client, &LlmClient::finished, this, &MainWindow::onFinished);
    connect(m_client, &LlmClient::errorOccurred, this, &MainWindow::onError);
}

void MainWindow::onSend()
{
    const QString question = m_input->text().trimmed();
    if (question.isEmpty() || m_busy)
        return;  // пустой вопрос не отправляем; пока ждём ответ — тоже

    m_transcript += QStringLiteral("**Вы:** ") + question + QStringLiteral("\n\n");
    m_input->clear();
    m_currentAnswer.clear();  // [этап 4]

    setBusy(true);
    m_client->ask(question);
}

// [этап 4] Каждый кусочек дописываем в конец и сразу показываем
void MainWindow::onChunk(const QString &text)
{
    m_currentAnswer += text;
    refreshChat();
}

// [этап 4] Ответ дописан до конца: переносим его в историю
void MainWindow::onFinished()
{
    m_transcript += QStringLiteral("**Модель:**\n\n") + m_currentAnswer + QStringLiteral("\n\n");
    m_currentAnswer.clear();
    setBusy(false);
}

void MainWindow::onError(const QString &message)
{
    // [этап 4] Если модель успела что-то напечатать до ошибки — сохраняем это
    if (!m_currentAnswer.isEmpty())
        m_transcript += QStringLiteral("**Модель:**\n\n") + m_currentAnswer + QStringLiteral("\n\n");
    m_currentAnswer.clear();

    m_transcript += QStringLiteral("**Ошибка:** ") + message
                  + QStringLiteral("\n\n*Проверьте, что llama-server запущен на порту 8080.*\n\n");
    setBusy(false);
}

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
    if (m_busy) {
        // [этап 4] Пока кусочков нет — «думает», дальше — растущий ответ
        if (m_currentAnswer.isEmpty())
            text += QStringLiteral("*Модель думает…*\n\n");
        else
            text += QStringLiteral("**Модель:**\n\n") + m_currentAnswer + QStringLiteral("\n\n");
    }

    // setMarkdown превращает **жирный**, списки и код в оформленный текст
    m_chatView->setMarkdown(text);

    // Прокручиваем историю к последнему сообщению
    QScrollBar *bar = m_chatView->verticalScrollBar();
    bar->setValue(bar->maximum());
}
