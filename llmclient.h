#pragma once

#include <QObject>
#include <QString>
#include <QUrl>

class QNetworkAccessManager;

// Сетевой слой: отправляет вопрос в llama-server и возвращает ответ.
// Про окна и виджеты ничего не знает — только HTTP и JSON.
class LlmClient : public QObject
{
    Q_OBJECT

public:
    explicit LlmClient(QObject *parent = nullptr);

    // Отправить вопрос. Метод сразу возвращает управление,
    // результат придёт позже сигналом answerReady или errorOccurred.
    void ask(const QString &question);

signals:
    void answerReady(const QString &text);      // ответ модели целиком
    void errorOccurred(const QString &message); // что-то пошло не так

private:
    QNetworkAccessManager *m_manager = nullptr; // отправляет HTTP-запросы
    QUrl m_url;                                 // адрес llama-server
};
