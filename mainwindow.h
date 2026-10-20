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
    // Ответ модели пришёл                              [новое на шаге 2]
    void onAnswer(const QString &text);
    // Запрос не удался                                 [новое на шаге 2]
    void onError(const QString &message);

private:
    // Перерисовать историю диалога и прокрутить её вниз
    void refreshChat();
    // Включить/выключить режим «модель думает»          [новое на шаге 2]
    void setBusy(bool busy);

    QTextBrowser *m_chatView = nullptr;   // история диалога (Markdown)
    QLineEdit *m_input = nullptr;         // строка ввода вопроса
    QPushButton *m_sendButton = nullptr;  // кнопка «Отправить»

    LlmClient *m_client = nullptr;        // связь с llama-server  [новое на шаге 2]

    QString m_transcript;                 // весь диалог в виде Markdown-текста
    bool m_busy = false;                  // ждём ли сейчас ответа [новое на шаге 2]
};
