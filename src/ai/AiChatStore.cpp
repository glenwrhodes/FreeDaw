#include "AiChatStore.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QUuid>
#include <QFileInfo>
#include <juce_core/juce_core.h>

namespace OpenDaw {

// ── ChatSession serialization ───────────────────────────────────────────────

QJsonObject ChatSession::toJson() const
{
    QJsonObject obj;
    obj["id"] = id;
    obj["title"] = title;
    obj["created_at"] = createdAt.toString(Qt::ISODate);
    obj["updated_at"] = updatedAt.toString(Qt::ISODate);

    QJsonArray msgsArr;
    for (auto& msg : messages)
        msgsArr.append(msg.toJson());
    obj["messages"] = msgsArr;
    return obj;
}

ChatSession ChatSession::fromJson(const QJsonObject& obj)
{
    ChatSession s;
    s.id = obj["id"].toString();
    s.title = obj["title"].toString("New Chat");
    s.createdAt = QDateTime::fromString(obj["created_at"].toString(), Qt::ISODate);
    s.updatedAt = QDateTime::fromString(obj["updated_at"].toString(), Qt::ISODate);

    for (auto val : obj["messages"].toArray())
        s.messages.append(AiMessage::fromJson(val.toObject()));
    return s;
}

// ── AiChatStore ─────────────────────────────────────────────────────────────

AiChatStore::AiChatStore()
{
    createSession();
}

QString AiChatStore::createSession()
{
    ChatSession s;
    s.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    s.title = "New Chat";
    s.createdAt = QDateTime::currentDateTime();
    s.updatedAt = s.createdAt;

    sessions_.prepend(s);
    activeSessionId_ = s.id;
    return s.id;
}

void AiChatStore::deleteSession(const QString& id)
{
    sessions_.erase(
        std::remove_if(sessions_.begin(), sessions_.end(),
                       [&](const ChatSession& s) { return s.id == id; }),
        sessions_.end());

    if (activeSessionId_ == id) {
        if (sessions_.isEmpty())
            createSession();
        else
            activeSessionId_ = sessions_.first().id;
    }
}

void AiChatStore::setActiveSession(const QString& id)
{
    for (auto& s : sessions_) {
        if (s.id == id) {
            activeSessionId_ = id;
            return;
        }
    }
}

ChatSession* AiChatStore::activeSession()
{
    for (auto& s : sessions_) {
        if (s.id == activeSessionId_)
            return &s;
    }
    return nullptr;
}

const ChatSession* AiChatStore::activeSession() const
{
    for (auto& s : sessions_) {
        if (s.id == activeSessionId_)
            return &s;
    }
    return nullptr;
}

void AiChatStore::updateActiveMessages(const QVector<AiMessage>& msgs)
{
    if (auto* s = activeSession()) {
        s->messages = msgs;
        s->updatedAt = QDateTime::currentDateTime();
    }
}

void AiChatStore::setSessionTitle(const QString& id, const QString& title)
{
    for (auto& s : sessions_) {
        if (s.id == id) {
            s.title = title;
            return;
        }
    }
}

void AiChatStore::saveTo(const QString& jsonPath)
{
    if (jsonPath.isEmpty()) return;

    QJsonObject root;
    root["version"] = kFileVersion;
    root["active_session_id"] = activeSessionId_;

    QJsonArray sessionsArr;
    for (auto& s : sessions_) {
        if (s.messages.isEmpty()) continue;
        sessionsArr.append(s.toJson());
    }
    root["sessions"] = sessionsArr;

    QFile file(jsonPath);
    if (file.open(QIODevice::WriteOnly))
        file.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
}

void AiChatStore::loadFrom(const QString& jsonPath)
{
    sessions_.clear();
    activeSessionId_.clear();

    if (jsonPath.isEmpty()) {
        createSession();
        return;
    }

    QFile file(jsonPath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        createSession();
        return;
    }

    auto doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull() || !doc.isObject()) {
        createSession();
        return;
    }

    auto root = doc.object();
    activeSessionId_ = root["active_session_id"].toString();

    for (auto val : root["sessions"].toArray())
        sessions_.append(ChatSession::fromJson(val.toObject()));

    if (sessions_.isEmpty()) {
        createSession();
        return;
    }

    bool activeFound = false;
    for (auto& s : sessions_) {
        if (s.id == activeSessionId_) {
            activeFound = true;
            break;
        }
    }
    if (!activeFound)
        activeSessionId_ = sessions_.first().id;
}

void AiChatStore::reset()
{
    sessions_.clear();
    activeSessionId_.clear();
    createSession();
}

QString AiChatStore::chatFilePath(const juce::File& editFile)
{
    if (editFile == juce::File()) return {};
    auto dir = QString::fromStdString(editFile.getParentDirectory().getFullPathName().toStdString());
    auto baseName = QString::fromStdString(editFile.getFileNameWithoutExtension().toStdString());
    return dir + "/" + baseName + ".aichats.json";
}

} // namespace OpenDaw
