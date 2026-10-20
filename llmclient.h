#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkReply;

// Сетевой слой: отправляет вопрос в llama-server и получает ответ ПОТОКОМ.
// Про окна и виджеты ничего не знает — только HTTP и JSON.
class LlmClient : public QObject
{
    Q_OBJECT

public:
    explicit LlmClient(QObject *parent = nullptr);

    // Отправить вопрос. Метод сразу возвращает управление,
    // ответ будет приходить кусочками через сигнал chunkReceived.
    void ask(const QString &question);

    // Прервать текущий ответ (кнопка «Стоп»)                          [этап 4, шаг 2]
    void stop();

signals:
    void chunkReceived(const QString &text);    // очередной кусочек ответа
    void finished();                            // ответ полностью получен
    void stopped();                             // ответ прерван пользователем [этап 4, шаг 2]
    void errorOccurred(const QString &message); // что-то пошло не так

private slots:
    void onReadyRead();       // пришла новая порция данных из сети
    void onReplyFinished();   // соединение закрыто

private:
    // Разобрать одну строку потока вида "data: {...}"
    void processLine(const QByteArray &line);
    // Перевести техническую ошибку в понятное сообщение            [этап 4, шаг 2]
    static QString friendlyError(QNetworkReply *reply);

    QNetworkAccessManager *m_manager = nullptr; // отправляет HTTP-запросы
    QUrl m_url;                                 // адрес llama-server

    QNetworkReply *m_reply = nullptr;  // текущий запрос (нужен, чтобы читать и останавливать)
    QByteArray m_buffer;               // недочитанный хвост: строка может прийти половинкой
};
