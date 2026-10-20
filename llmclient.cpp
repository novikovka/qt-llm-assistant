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
    // Собираем тот же JSON, что отправляли через curl:
    // {"messages":[{"role":"user","content":"<вопрос>"}]}
    QJsonObject message;
    message.insert(QStringLiteral("role"), QStringLiteral("user"));
    message.insert(QStringLiteral("content"), question);

    QJsonObject body;
    body.insert(QStringLiteral("messages"), QJsonArray{message});

    // Аналог -H "Content-Type: application/json"
    QNetworkRequest request(m_url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    // POST-запрос. Он асинхронный: окно продолжает работать, пока модель думает
    QNetworkReply *reply = m_manager->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));

    // Когда ответ придёт целиком, Qt вызовет эту лямбду
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater(); // освободить память после выхода из лямбды

        // Сервер не запущен, неверный адрес и т. п.
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(reply->errorString());
            return;
        }

        // Достаём текст по пути choices[0].message.content — как jq на этапе 2
        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        const QString content = doc.object()
                                    .value(QStringLiteral("choices")).toArray()
                                    .at(0).toObject()
                                    .value(QStringLiteral("message")).toObject()
                                    .value(QStringLiteral("content")).toString();

        if (content.isEmpty())
            emit errorOccurred(QStringLiteral("Сервер вернул ответ без текста"));
        else
            emit answerReady(content);
    });
}
