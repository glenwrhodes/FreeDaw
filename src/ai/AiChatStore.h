#pragma once

#include "AiTypes.h"
#include <QString>
#include <QVector>
#include <QDateTime>

namespace juce { class File; }

namespace OpenDaw {

struct ChatSession {
    QString id;
    QString title;
    QDateTime createdAt;
    QDateTime updatedAt;
    QVector<AiMessage> messages;

    QJsonObject toJson() const;
    static ChatSession fromJson(const QJsonObject& obj);
};

class AiChatStore {
public:
    AiChatStore();

    QString createSession();
    void deleteSession(const QString& id);
    void setActiveSession(const QString& id);
    ChatSession* activeSession();
    const ChatSession* activeSession() const;
    QString activeSessionId() const { return activeSessionId_; }
    QVector<ChatSession>& sessions() { return sessions_; }
    const QVector<ChatSession>& sessions() const { return sessions_; }

    void updateActiveMessages(const QVector<AiMessage>& msgs);
    void setSessionTitle(const QString& id, const QString& title);

    void saveTo(const QString& jsonPath);
    void loadFrom(const QString& jsonPath);
    void reset();

    static QString chatFilePath(const juce::File& editFile);

private:
    static constexpr int kFileVersion = 1;
    QVector<ChatSession> sessions_;
    QString activeSessionId_;
};

} // namespace OpenDaw
