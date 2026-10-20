#pragma once

#include <QString>
#include <QWidget>

class QPushButton;
class QTextBrowser;
class QTextEdit;
class LlmClient;

// Главное окно чата: история диалога, строка ввода и кнопка «Отправить/Стоп»
class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    // Перехват клавиш в поле ввода: Enter — отправить, Shift+Enter — новая строка
    bool eventFilter(QObject *watched, QEvent *event) override;  // [растущее поле]

private slots:
    // Кнопка: «Отправить» в обычном режиме, «Стоп» во время ответа
    void onButtonClicked();
    // Отправка вопроса (кнопкой или Enter в поле ввода)
    void onSend();
    // Пришёл очередной кусочек ответа
    void onChunk(const QString &text);
    // Ответ полностью получен
    void onFinished();
    // Ответ прерван кнопкой «Стоп»
    void onStopped();
    // Запрос не удался
    void onError(const QString &message);
    // Подогнать высоту поля ввода под текст (от 1 до 6 строк)    [растущее поле]
    void adjustInputHeight();

private:
    // Перенести напечатанную часть ответа в историю (с пометкой, если нужно)
    void commitAnswer(const QString &note = QString());
    // Перерисовать историю диалога и прокрутить её вниз
    void refreshChat();
    // Включить/выключить режим «модель отвечает»
    void setBusy(bool busy);

    QTextBrowser *m_chatView = nullptr;   // история диалога (Markdown)
    QTextEdit *m_input = nullptr;         // поле ввода, растёт по тексту [растущее поле: было QLineEdit]
    QPushButton *m_sendButton = nullptr;  // кнопка «Отправить/Стоп»

    LlmClient *m_client = nullptr;        // связь с llama-server

    QString m_transcript;                 // завершённая часть диалога (Markdown)
    QString m_currentAnswer;              // ответ, который сейчас «печатается»
    bool m_busy = false;                  // ждём ли сейчас ответа
};
