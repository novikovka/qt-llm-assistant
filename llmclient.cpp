#include "llmclient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

LlmClient::LlmClient(QObject *parent)
    : QObject(parent)
    , m_manager(new QNetworkAccessManager(this))
    , m_url(QStringLiteral("http://localhost:8080/v1/chat/completions"))
{
}

void LlmClient::ask(const QString &question)
{
    // {"stream":true,"messages":[{"role":"user","content":"<вопрос>"}]}
    QJsonObject message;
    message.insert(QStringLiteral("role"), QStringLiteral("user"));
    message.insert(QStringLiteral("content"), question);

    QJsonObject body;
    body.insert(QStringLiteral("messages"), QJsonArray{message});
    body.insert(QStringLiteral("stream"), true);  // [этап 4] просим отвечать по кусочкам

    QNetworkRequest request(m_url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    m_buffer.clear();
    m_reply = m_manager->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));

    // [этап 4] readyRead — каждый раз, когда из сети пришла новая порция данных;
    // finished — когда соединение закрылось (ответ дочитан или ошибка)
    connect(m_reply, &QNetworkReply::readyRead, this, &LlmClient::onReadyRead);
    connect(m_reply, &QNetworkReply::finished, this, &LlmClient::onReplyFinished);
}

// [этап 4]
void LlmClient::onReadyRead()
{
    if (!m_reply)
        return;

    // Дописываем новые байты к тому, что осталось с прошлого раза
    m_buffer += m_reply->readAll();

    // Вынимаем из буфера все ЦЕЛЫЕ строки (заканчиваются на '\n').
    // Хвост без '\n' остаётся в буфере и дождётся следующей порции.
    qsizetype newline;
    while ((newline = m_buffer.indexOf('\n')) >= 0) {
        const QByteArray line = m_buffer.left(newline).trimmed();
        m_buffer.remove(0, newline + 1);
        processLine(line);
    }
}

// [этап 4]
void LlmClient::processLine(const QByteArray &line)
{
    // Нас интересуют только строки "data: ..."; пустые строки-разделители пропускаем
    if (!line.startsWith("data:"))
        return;

    const QByteArray payload = line.mid(5).trimmed();  // всё после "data:"
    if (payload == "[DONE]")
        return;  // конец потока; finished придёт, когда сервер закроет соединение

    // Достаём кусочек текста по пути choices[0].delta.content
    const QJsonDocument doc = QJsonDocument::fromJson(payload);
    const QString piece = doc.object()
                              .value(QStringLiteral("choices")).toArray()
                              .at(0).toObject()
                              .value(QStringLiteral("delta")).toObject()
                              .value(QStringLiteral("content")).toString();

    if (!piece.isEmpty())
        emit chunkReceived(piece);
}

// [этап 4]
void LlmClient::onReplyFinished()
{
    QNetworkReply *reply = m_reply;
    m_reply = nullptr;
    if (!reply)
        return;
    reply->deleteLater();  // освободить память после выхода из функции

    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(reply->errorString());
        return;
    }

    // Если последняя строка пришла без '\n', она ещё лежит в буфере
    if (!m_buffer.trimmed().isEmpty())
        processLine(m_buffer.trimmed());
    m_buffer.clear();

    emit finished();
}
