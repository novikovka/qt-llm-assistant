#pragma once

#include <QString>
#include <QWidget>

class QLineEdit;
class QPushButton;
class QTextBrowser;
class LlmClient;

// Главное окно чата: история диалога, строка ввода и кнопка «Отправить»
class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    // Вызывается по кнопке «Отправить» или по Enter в строке ввода
    void onSend();
    // Пришёл очередной кусочек ответа              [этап 4, вместо onAnswer]
    void onChunk(const QString &text);
    // Ответ полностью получен                       [этап 4]
    void onFinished();
    // Запрос не удался
    void onError(const QString &message);

private:
    // Перерисовать историю диалога и прокрутить её вниз
    void refreshChat();
    // Включить/выключить режим «модель отвечает»
    void setBusy(bool busy);

    QTextBrowser *m_chatView = nullptr;   // история диалога (Markdown)
    QLineEdit *m_input = nullptr;         // строка ввода вопроса
    QPushButton *m_sendButton = nullptr;  // кнопка «Отправить»

    LlmClient *m_client = nullptr;        // связь с llama-server

    QString m_transcript;                 // завершённая часть диалога (Markdown)
    QString m_currentAnswer;              // ответ, который сейчас «печатается» [этап 4]
    bool m_busy = false;                  // ждём ли сейчас ответа
};
