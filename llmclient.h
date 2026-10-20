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

signals:
    void chunkReceived(const QString &text);    // очередной кусочек ответа  [этап 4]
    void finished();                            // ответ полностью получен   [этап 4]
    void errorOccurred(const QString &message); // что-то пошло не так

private slots:
    void onReadyRead();       // пришла новая порция данных из сети  [этап 4]
    void onReplyFinished();   // соединение закрыто                  [этап 4]

private:
    // Разобрать одну строку потока вида "data: {...}"               [этап 4]
    void processLine(const QByteArray &line);

    QNetworkAccessManager *m_manager = nullptr; // отправляет HTTP-запросы
    QUrl m_url;                                 // адрес llama-server

    QNetworkReply *m_reply = nullptr;  // текущий запрос (нужен, чтобы читать и останавливать) [этап 4]
    QByteArray m_buffer;               // недочитанный хвост: строка может прийти половинкой  [этап 4]
};
