#pragma once

#include "AiTypes.h"
#include "AiService.h"
#include "AiChatStore.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QKeyEvent>
#include <QTimer>
#include <QTextBrowser>
#include <QListWidget>

namespace OpenDaw {

class EditManager;
class AudioEngine;
class PluginScanner;

class AiChatWidget : public QWidget {
    Q_OBJECT
public:
    explicit AiChatWidget(EditManager* editMgr, AudioEngine* audioEngine,
                          PluginScanner* pluginScanner, QWidget* parent = nullptr);

    void submitPrompt(const QString& text);
    void focusInput();
    void openSettings();

    void saveChatSessions();
    void loadChatSessions();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

signals:
    void requestRaise();

private:
    void setupUi();
    void onSendClicked();
    void onTokenReceived(const QString& token);
    void onResponseComplete(const AiMessage& message);
    void onToolCallStarted(const QString& toolName, const QString& toolId);
    void onToolCallFinished(const QString& toolName, const QString& toolId,
                            const QString& result, bool isError);
    void onError(const QString& error);
    void onConfirmDestructive(const QString& toolName, const AiToolCall& call);
    void onBusyChanged(bool busy);

    void appendUserBubble(const QString& text);
    void appendAssistantBubble(const QString& text);
    void appendToolBubble(const QString& toolName, const QString& result, bool isError);
    void appendErrorBubble(const QString& text);
    void updateStreamingBubble(const QString& token);
    void finalizeStreamingBubble();
    void scrollToBottom();

    void showSettingsDialog();
    void clearChatUi();

    void startNewChat();
    void switchToSession(const QString& sessionId);
    void rebuildUiFromHistory(const QVector<AiMessage>& messages);
    void refreshSessionList();
    void onSessionTitleGenerated(const QString& title);
    QString chatFilePath() const;

    QTextBrowser* createMessageBrowser(QWidget* parent) const;

    AiService* aiService_;
    AiChatStore* chatStore_;
    EditManager* editMgr_;

    QVBoxLayout* mainLayout_ = nullptr;
    QWidget* headerBar_ = nullptr;
    QScrollArea* scrollArea_ = nullptr;
    QWidget* messagesContainer_ = nullptr;
    QVBoxLayout* messagesLayout_ = nullptr;
    QTextEdit* inputEdit_ = nullptr;
    QPushButton* sendBtn_ = nullptr;
    QLabel* statusLabel_ = nullptr;

    QWidget* historyPanel_ = nullptr;
    QListWidget* sessionList_ = nullptr;
    QPushButton* historyToggleBtn_ = nullptr;
    QPushButton* newChatBtn_ = nullptr;

    QTextBrowser* streamingLabel_ = nullptr;
    QString streamingText_;
    bool isStreaming_ = false;
    QTimer* streamThrottle_ = nullptr;
    bool streamDirty_ = false;

    bool showToolOutput_ = false;
    QPushButton* toolToggleBtn_ = nullptr;
    QVector<QWidget*> toolBubbles_;
    void updateToolBubbleVisibility();
};

} // namespace OpenDaw
