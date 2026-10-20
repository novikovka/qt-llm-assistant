#include "mainwindow.h"

#include "llmclient.h"

#include <QAbstractTextDocumentLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QPushButton>
#include <QScrollBar>
#include <QTextBrowser>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QtMath>

namespace {
constexpr int kMaxInputLines = 6;  // выше 6 строк поле не растёт, появляется прокрутка
}

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
    , m_client(new LlmClient(this))
{
    setWindowTitle(QStringLiteral("Локальный ассистент (llama.cpp)"));

    // --- Создаём элементы интерфейса ---
    m_chatView = new QTextBrowser(this);
    m_chatView->setOpenExternalLinks(true);

    // [растущее поле] Многострочное поле вместо QLineEdit
    m_input = new QTextEdit(this);
    m_input->setAcceptRichText(false);  // вставленный текст — без шрифтов и цветов
    m_input->setPlaceholderText(
        QStringLiteral("Введите вопрос. Enter — отправить, Shift+Enter — новая строка"));
    m_input->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_input->installEventFilter(this);  // нажатия клавиш сначала попадут в eventFilter

    m_sendButton = new QPushButton(QStringLiteral("Отправить"), this);

    // --- Раскладываем их по окну ---
    // Нижняя строка: поле ввода растягивается, кнопка справа (прижата к низу)
    auto *inputRow = new QHBoxLayout;
    inputRow->addWidget(m_input);
    inputRow->addWidget(m_sendButton, 0, Qt::AlignBottom);  // [растущее поле]

    // Всё окно: сверху история (забирает всё свободное место), снизу строка ввода
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_chatView, 1);
    mainLayout->addLayout(inputRow);

    // --- Связываем действия пользователя с обработчиками ---
    connect(m_sendButton, &QPushButton::clicked, this, &MainWindow::onButtonClicked);

    // [растущее поле] Размер текста меняется, когда печатаем ИЛИ когда окно стало
    // уже/шире (строки переносятся иначе) — в обоих случаях пересчитываем высоту
    connect(m_input->document()->documentLayout(),
            &QAbstractTextDocumentLayout::documentSizeChanged,
            this, &MainWindow::adjustInputHeight);

    // --- Связываем сигналы сетевого слоя с окном ---
    connect(m_client, &LlmClient::chunkReceived, this, &MainWindow::onChunk);
    connect(m_client, &LlmClient::finished, this, &MainWindow::onFinished);
    connect(m_client, &LlmClient::stopped, this, &MainWindow::onStopped);
    connect(m_client, &LlmClient::errorOccurred, this, &MainWindow::onError);

    adjustInputHeight();  // начальная высота — одна строка
}

// [растущее поле]
bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_input && event->type() == QEvent::KeyPress) {
        auto *key = static_cast<QKeyEvent *>(event);
        const bool isEnter = key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter;

        if (isEnter && !(key->modifiers() & Qt::ShiftModifier)) {
            onSend();
            return true;  // «съели» нажатие: поле не вставит перенос строки
        }
        // Shift+Enter и все остальные клавиши — поле обработает как обычно
    }
    return QWidget::eventFilter(watched, event);
}

// [растущее поле]
void MainWindow::adjustInputHeight()
{
    // Высота текста в пикселях (уже с учётом переносов по ширине поля)
    const int textHeight = qCeil(m_input->document()->size().height());

    // Отступы документа сверху/снизу и рамка поля
    const int docMargin = qCeil(m_input->document()->documentMargin()) * 2;
    const int frame = m_input->frameWidth() * 2;
    const int line = m_input->fontMetrics().lineSpacing();

    const int minHeight = line + docMargin + frame;                   // 1 строка
    const int maxHeight = kMaxInputLines * line + docMargin + frame;  // 6 строк
    const int wanted = textHeight + frame;

    m_input->setFixedHeight(qBound(minHeight, wanted, maxHeight));

    // Прокрутка нужна только когда текст не помещается в 6 строк
    m_input->setVerticalScrollBarPolicy(wanted > maxHeight ? Qt::ScrollBarAsNeeded
                                                           : Qt::ScrollBarAlwaysOff);
}

// Одна кнопка — два действия
void MainWindow::onButtonClicked()
{
    if (m_busy)
        m_client->stop();  // идёт ответ — значит, это «Стоп»
    else
        onSend();          // иначе — «Отправить»
}

void MainWindow::onSend()
{
    const QString question = m_input->toPlainText().trimmed();  // [растущее поле: было text()]
    if (question.isEmpty() || m_busy)
        return;  // пустой вопрос не отправляем; пока ждём ответ — тоже

    // [растущее поле] В Markdown одиночный перенос склеивает строки;
    // "  \n" (два пробела + перенос) — принудительный перенос строки
    QString shown = question;
    shown.replace(QLatin1Char('\n'), QStringLiteral("  \n"));

    m_transcript += QStringLiteral("**Вы:** ") + shown + QStringLiteral("\n\n");
    m_input->clear();
    m_currentAnswer.clear();

    setBusy(true);
    m_client->ask(question);
}

// Каждый кусочек дописываем в конец и сразу показываем
void MainWindow::onChunk(const QString &text)
{
    m_currentAnswer += text;
    refreshChat();
}

// Ответ дописан до конца: переносим его в историю
void MainWindow::onFinished()
{
    commitAnswer();
    setBusy(false);
}

// Пользователь нажал «Стоп»
void MainWindow::onStopped()
{
    if (m_currentAnswer.isEmpty())
        m_transcript += QStringLiteral("*Остановлено до начала ответа.*\n\n");
    else
        commitAnswer(QStringLiteral("*(остановлено)*"));
    setBusy(false);
}

void MainWindow::onError(const QString &message)
{
    // Если модель успела что-то напечатать до ошибки — сохраняем это
    commitAnswer();
    m_transcript += QStringLiteral("**Ошибка:** ") + message + QStringLiteral("\n\n");
    setBusy(false);
}

// Общий кусок для onFinished, onStopped и onError
void MainWindow::commitAnswer(const QString &note)
{
    if (m_currentAnswer.isEmpty())
        return;

    m_transcript += QStringLiteral("**Модель:**\n\n") + m_currentAnswer + QStringLiteral("\n\n");
    if (!note.isEmpty())
        m_transcript += note + QStringLiteral("\n\n");
    m_currentAnswer.clear();
}

void MainWindow::setBusy(bool busy)
{
    m_busy = busy;
    m_input->setEnabled(!busy);
    // Кнопка всегда активна, меняется только надпись
    m_sendButton->setText(busy ? QStringLiteral("Стоп") : QStringLiteral("Отправить"));
    refreshChat();

    if (!busy)
        m_input->setFocus();  // сразу можно печатать следующий вопрос
}

void MainWindow::refreshChat()
{
    QString text = m_transcript;
    if (m_busy) {
        // Пока кусочков нет — «думает», дальше — растущий ответ
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
