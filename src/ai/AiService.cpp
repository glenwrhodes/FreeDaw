#include "AiService.h"
#include "engine/EditManager.h"
#include "engine/AudioEngine.h"
#include <QNetworkRequest>
#include <QUrl>

namespace OpenDaw {

static const QString kApiUrl = "https://api.anthropic.com/v1/messages";
static const QString kDefaultModel = "claude-sonnet-4-20250514";
static const int kMaxTokens = 8192;

AiService::AiService(EditManager* editMgr, AudioEngine* audioEngine,
                     PluginScanner* pluginScanner, QObject* parent)
    : QObject(parent), editMgr_(editMgr), audioEngine_(audioEngine)
{
    toolExecutor_ = new AiToolExecutor(editMgr, audioEngine, pluginScanner, this);
    nam_ = new QNetworkAccessManager(this);
}

// ── Settings ────────────────────────────────────────────────────────────────

QString AiService::apiKey() const
{
    QSettings s;
    return s.value("ai/apiKey").toString();
}

void AiService::setApiKey(const QString& key)
{
    QSettings s;
    s.setValue("ai/apiKey", key);
}

QString AiService::model() const
{
    QSettings s;
    return s.value("ai/model", kDefaultModel).toString();
}

void AiService::setModel(const QString& m)
{
    QSettings s;
    s.setValue("ai/model", m);
}

bool AiService::confirmDestructive() const
{
    QSettings s;
    return s.value("ai/confirmDestructive", true).toBool();
}

void AiService::setConfirmDestructive(bool confirm)
{
    QSettings s;
    s.setValue("ai/confirmDestructive", confirm);
}

bool AiService::previewMixPlanMode() const
{
    QSettings s;
    return s.value("ai/previewMixPlanMode", false).toBool();
}

void AiService::setPreviewMixPlanMode(bool enabled)
{
    QSettings s;
    s.setValue("ai/previewMixPlanMode", enabled);
}

void AiService::setBusy(bool b)
{
    if (busy_ != b) {
        busy_ = b;
        emit busyChanged(b);
    }
}

// ── System prompt ───────────────────────────────────────────────────────────

QString AiService::buildSystemPrompt() const
{
    QString prompt =
        "You are an AI assistant integrated into OpenDaw, a digital audio workstation. "
        "You help users manage their music projects by manipulating tracks, effects, "
        "routing, and transport controls.\n\n";

    if (editMgr_ && editMgr_->edit()) {
        prompt += "Current project state (live snapshot — includes any changes you have already made in this conversation):\n";
        prompt += QString("- Tempo: %1 BPM\n").arg(editMgr_->getBpm(), 0, 'f', 1);
        prompt += QString("- Time signature: %1/%2\n")
                      .arg(editMgr_->getTimeSigNumerator())
                      .arg(editMgr_->getTimeSigDenominator());

        auto& transport = editMgr_->transport();
        QString state = transport.isRecording() ? "Recording" :
                        transport.isPlaying() ? "Playing" : "Stopped";
        prompt += QString("- Transport: %1 at %2s\n")
                      .arg(state)
                      .arg(transport.getPosition().inSeconds(), 0, 'f', 2);

        auto tracks = editMgr_->getAudioTracks();
        prompt += QString("- Tracks (%1):\n").arg(tracks.size());
        for (int i = 0; i < tracks.size(); ++i) {
            auto* t = tracks[i];
            QString type = editMgr_->isBusTrack(t) ? "bus" :
                          editMgr_->isMidiTrack(t) ? "midi" : "audio";
            QString name = QString::fromStdString(t->getName().toStdString());
            prompt += QString("  [%1] \"%2\" (%3)%4%5\n")
                          .arg(i).arg(name, type,
                               t->isMuted(false) ? " MUTED" : "",
                               t->isSolo(false) ? " SOLO" : "");
        }
    }

    prompt += "\nRules:\n"
              "- Use the provided tools to manipulate the DAW. Call tools as needed.\n"
              "- For bulk operations, call multiple tools in sequence.\n"
              "- The project state shown above is a LIVE snapshot that updates after each tool call. "
              "If you just created tracks or clips, they will appear in the snapshot — do NOT treat "
              "them as pre-existing or comment on them as if seeing them for the first time. "
              "Only reference things the USER mentioned or that existed BEFORE your first action.\n"
              "- After completing actions, briefly confirm what you did.\n"
              "- You can create MIDI clips and write notes into them. Use create_midi_clip to make a clip, "
              "then add_midi_notes to populate it. Note positions are in beats relative to clip start. "
              "Batch as many notes as possible into a single add_midi_notes call for efficiency. "
              "Middle C is MIDI note 60. A bar in 4/4 time is 4 beats.\n"
              "- When users ask about effects parameters, use get_track_effects to see "
              "current values before modifying them.\n"
              "- Parameter values are normalized 0.0 to 1.0.\n"
              "- For mix/master requests, use staged workflow: analyze -> apply -> verify.\n"
              "- Before major processing, call analysis tools first.\n"
              "- Stop when targets are met or no meaningful improvement remains.\n"
              "\nMIDI & Music Theory (CRITICAL — follow strictly):\n"
              "- ALWAYS establish the key and scale BEFORE writing any notes. If the user specifies "
              "a key, use it. If not, ask or assume C major. State the key in your response.\n"
              "- Every note you write MUST belong to the scale of the stated key unless it is an "
              "intentional chromatic passing tone, approach note, or borrowed chord tone. "
              "Random out-of-key notes like Db in C major are WRONG.\n"
              "- Scale reference (use these and their relative modes):\n"
              "  Major (Ionian): W-W-H-W-W-W-H  (C major = C D E F G A B)\n"
              "  Natural minor (Aeolian): W-H-W-W-H-W-W  (A minor = A B C D E F G)\n"
              "  Harmonic minor: raise 7th  (A harm. minor = A B C D E F G#)\n"
              "  Melodic minor (ascending): raise 6th and 7th\n"
              "- Chromatic notes are ONLY acceptable when:\n"
              "  (a) They are passing tones connecting two diatonic notes stepwise (e.g. F-F#-G in C major).\n"
              "  (b) They are part of a secondary dominant or borrowed chord (e.g. E/G# resolving to Am in C major).\n"
              "  (c) The user explicitly requests chromatic or atonal writing.\n"
              "  (d) They are approach notes (half-step below a chord tone, on a weak beat, resolving immediately).\n"
              "- For chords, use diatonic triads/sevenths of the key unless a chromatic chord is musically justified:\n"
              "  C major diatonic triads: C Dm Em F G Am Bdim\n"
              "  A minor diatonic triads: Am Bdim C Dm Em F G\n"
              "- Double-check every note number against the key before submitting. If in doubt, stay diatonic.\n"
              "- When writing drums (channel 10 / GM percussion), key constraints do not apply.\n";

    if (previewMixPlanMode()) {
        prompt += "- Preview mode is ON: call preview_mix_plan before mutating tools and wait for user confirmation intent.\n";
    }

    return prompt;
}

// ── Conversation ────────────────────────────────────────────────────────────

void AiService::clearConversation()
{
    if (currentReply_) {
        currentReply_->abort();
        currentReply_->deleteLater();
        currentReply_ = nullptr;
    }
    conversationHistory_.clear();
    toolRoundCount_ = 0;
    executedToolIds_.clear();
    setBusy(false);
}

void AiService::loadConversation(const QVector<AiMessage>& messages)
{
    clearConversation();
    conversationHistory_ = messages;
}

void AiService::requestSessionTitle(const QString& userMessage)
{
    QString key = apiKey();
    if (key.isEmpty()) return;

    QJsonObject body;
    body["model"] = model();
    body["max_tokens"] = 30;
    body["system"] = QString("Generate a concise 3-6 word title for this chat. "
                             "Reply with ONLY the title, no quotes or punctuation.");
    QJsonArray messages;
    QJsonObject userMsg;
    userMsg["role"] = "user";
    QJsonArray contentArr;
    QJsonObject textBlock;
    textBlock["type"] = "text";
    textBlock["text"] = userMessage;
    contentArr.append(textBlock);
    userMsg["content"] = contentArr;
    messages.append(userMsg);
    body["messages"] = messages;

    QUrl apiUrl("https://api.anthropic.com/v1/messages");
    QNetworkRequest request{apiUrl};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("x-api-key", key.toUtf8());
    request.setRawHeader("anthropic-version", "2023-06-01");
    request.setTransferTimeout(15000);

    auto* reply = nam_->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) return;

        auto doc = QJsonDocument::fromJson(reply->readAll());
        if (doc.isNull()) return;
        auto obj = doc.object();
        auto contentArr = obj["content"].toArray();
        if (contentArr.isEmpty()) return;

        QString title = contentArr[0].toObject()["text"].toString().trimmed();
        if (title.isEmpty()) return;

        if (title.length() > 60)
            title = title.left(57) + "...";

        emit sessionTitleGenerated(title);
    });
}

void AiService::pruneHistory()
{
    if (conversationHistory_.size() <= 50) return;
    while (conversationHistory_.size() > 40)
        conversationHistory_.removeFirst();
}

void AiService::sendMessage(const QString& userText)
{
    if (busy_) return;

    QString key = apiKey();
    if (key.isEmpty()) {
        emit errorOccurred("No API key set. Open AI settings to configure your Anthropic API key.");
        return;
    }

    AiMessage userMsg = AiMessage::userMessage(userText);
    conversationHistory_.append(userMsg);
    toolRoundCount_ = 0;

    sendToApi();
}

// ── API call ────────────────────────────────────────────────────────────────

QJsonObject AiService::messageToJson(const AiMessage& msg) const
{
    QJsonObject obj;
    obj["role"] = (msg.role == AiRole::User) ? "user" : "assistant";

    QJsonArray contentArr;
    for (auto& block : msg.content) {
        QJsonObject blockObj;
        if (block.type == "text") {
            blockObj["type"] = "text";
            blockObj["text"] = block.text;
        } else if (block.type == "tool_use") {
            blockObj["type"] = "tool_use";
            blockObj["id"] = block.toolCall.id;
            blockObj["name"] = block.toolCall.name;
            blockObj["input"] = block.toolCall.input;
        } else if (block.type == "tool_result") {
            blockObj["type"] = "tool_result";
            blockObj["tool_use_id"] = block.toolResult.toolUseId;
            blockObj["content"] = block.toolResult.content;
            if (block.toolResult.isError)
                blockObj["is_error"] = true;
        }
        contentArr.append(blockObj);
    }
    obj["content"] = contentArr;
    return obj;
}

void AiService::sendToApi()
{
    setBusy(true);
    streamBuffer_.clear();
    currentAssistantText_.clear();
    currentContentBlocks_.clear();
    currentToolInputStr_.clear();
    inToolInput_ = false;
    currentBlockIndex_ = -1;

    QJsonObject body;
    body["model"] = model();
    body["max_tokens"] = kMaxTokens;
    body["stream"] = true;
    body["system"] = buildSystemPrompt();
    body["tools"] = AiToolDefs::allTools();

    QJsonArray messages;
    for (auto& msg : conversationHistory_)
        messages.append(messageToJson(msg));
    body["messages"] = messages;

    QUrl apiUrl(kApiUrl);
    QNetworkRequest request{apiUrl};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("x-api-key", apiKey().toUtf8());
    request.setRawHeader("anthropic-version", "2023-06-01");

    request.setTransferTimeout(60000);

    pruneHistory();

    QByteArray bodyData = QJsonDocument(body).toJson(QJsonDocument::Compact);
    currentReply_ = nam_->post(request, bodyData);

    connect(currentReply_, &QNetworkReply::readyRead,
            this, &AiService::handleStreamChunk);
    connect(currentReply_, &QNetworkReply::finished,
            this, &AiService::handleStreamFinished);
}

// ── SSE stream parsing ──────────────────────────────────────────────────────

void AiService::handleStreamChunk()
{
    if (!currentReply_) return;
    streamBuffer_.append(currentReply_->readAll());

    while (true) {
        int nlPos = streamBuffer_.indexOf('\n');
        if (nlPos < 0) break;

        QByteArray line = streamBuffer_.left(nlPos).trimmed();
        streamBuffer_.remove(0, nlPos + 1);

        if (line.isEmpty() || line.startsWith("event:")) continue;
        if (!line.startsWith("data: ")) continue;

        QByteArray jsonData = line.mid(6);
        if (jsonData == "[DONE]") continue;

        QJsonParseError parseErr;
        auto doc = QJsonDocument::fromJson(jsonData, &parseErr);
        if (parseErr.error != QJsonParseError::NoError) continue;

        auto obj = doc.object();
        QString type = obj["type"].toString();

        if (type == "content_block_start") {
            auto blockObj = obj["content_block"].toObject();
            QString blockType = blockObj["type"].toString();
            currentBlockIndex_ = obj["index"].toInt();

            AiContentBlock block;
            block.type = blockType;

            if (blockType == "tool_use") {
                block.toolCall.id = blockObj["id"].toString();
                block.toolCall.name = blockObj["name"].toString();
                inToolInput_ = true;
                currentToolInputStr_.clear();
                emit toolCallStarted(block.toolCall.name, block.toolCall.id);
            }

            currentContentBlocks_.append(block);
        }
        else if (type == "content_block_delta") {
            auto delta = obj["delta"].toObject();
            QString deltaType = delta["type"].toString();

            if (deltaType == "text_delta") {
                QString text = delta["text"].toString();
                currentAssistantText_ += text;
                if (currentBlockIndex_ >= 0 && currentBlockIndex_ < currentContentBlocks_.size())
                    currentContentBlocks_[currentBlockIndex_].text += text;
                emit tokenReceived(text);
            }
            else if (deltaType == "input_json_delta") {
                currentToolInputStr_ += delta["partial_json"].toString();
            }
        }
        else if (type == "content_block_stop") {
            if (inToolInput_ && currentBlockIndex_ >= 0
                && currentBlockIndex_ < currentContentBlocks_.size()) {
                auto& block = currentContentBlocks_[currentBlockIndex_];
                if (block.type == "tool_use") {
                    auto inputDoc = QJsonDocument::fromJson(currentToolInputStr_.toUtf8());
                    block.toolCall.input = inputDoc.object();
                }
                inToolInput_ = false;
                currentToolInputStr_.clear();
            }
            currentBlockIndex_ = -1;
        }
        else if (type == "message_stop") {
            // handled in handleStreamFinished
        }
        else if (type == "error") {
            auto errObj = obj["error"].toObject();
            emit errorOccurred(errObj["message"].toString());
            setBusy(false);
        }
    }
}

void AiService::handleStreamFinished()
{
    if (!currentReply_) return;

    if (currentReply_->error() != QNetworkReply::NoError
        && currentReply_->error() != QNetworkReply::OperationCanceledError) {
        QByteArray responseBody = currentReply_->readAll();
        QString errorMsg = currentReply_->errorString();

        auto doc = QJsonDocument::fromJson(responseBody);
        if (doc.isObject()) {
            auto errObj = doc.object()["error"].toObject();
            if (!errObj.isEmpty())
                errorMsg = errObj["message"].toString();
        }

        emit errorOccurred(errorMsg);
        setBusy(false);
        currentReply_->deleteLater();
        currentReply_ = nullptr;
        return;
    }

    currentReply_->deleteLater();
    currentReply_ = nullptr;

    processAssistantResponse();
}

// ── Process response & agentic loop ─────────────────────────────────────────

void AiService::processAssistantResponse()
{
    AiMessage assistantMsg;
    assistantMsg.role = AiRole::Assistant;
    assistantMsg.timestamp = QDateTime::currentDateTime();
    assistantMsg.content = currentContentBlocks_;

    conversationHistory_.append(assistantMsg);

    if (assistantMsg.hasToolUse() && toolRoundCount_ < MAX_TOOL_ROUNDS) {
        toolRoundCount_++;
        QVector<AiToolCall> calls;
        for (auto& block : assistantMsg.content) {
            if (block.type == "tool_use")
                calls.append(block.toolCall);
        }
        executeToolCalls(calls);
    } else {
        emit responseComplete(assistantMsg);
        setBusy(false);
    }
}

void AiService::executeToolCalls(const QVector<AiToolCall>& calls)
{
    executedToolIds_.clear();
    QVector<AiToolResult> results;

    toolExecutor_->beginBatch();

    for (auto& call : calls) {
        if (confirmDestructive() && toolExecutor_->isDestructive(call.name)) {
            toolExecutor_->endBatch();
            emit confirmDestructiveAction(call.name, call);
            return;
        }

        auto result = toolExecutor_->execute(call);
        results.append(result);
        executedToolIds_.insert(call.id);

        emit toolCallFinished(call.name, call.id, result.content, result.isError);
    }

    toolExecutor_->endBatch();

    AiMessage assistantMsg = conversationHistory_.last();
    continueAfterToolResults(results, assistantMsg);
}

void AiService::executeConfirmedTool(const AiToolCall& call)
{
    auto result = toolExecutor_->execute(call);
    emit toolCallFinished(call.name, call.id, result.content, result.isError);
    executedToolIds_.insert(call.id);

    AiMessage assistantMsg = conversationHistory_.last();
    QVector<AiToolResult> allResults;

    for (auto& block : assistantMsg.content) {
        if (block.type == "tool_use") {
            if (block.toolCall.id == call.id) {
                allResults.append(result);
            } else if (executedToolIds_.contains(block.toolCall.id)) {
                continue;
            } else if (confirmDestructive() && toolExecutor_->isDestructive(block.toolCall.name)) {
                emit confirmDestructiveAction(block.toolCall.name, block.toolCall);
                return;
            } else {
                auto existingResult = toolExecutor_->execute(block.toolCall);
                allResults.append(existingResult);
                executedToolIds_.insert(block.toolCall.id);
                emit toolCallFinished(block.toolCall.name, block.toolCall.id,
                                     existingResult.content, existingResult.isError);
            }
        }
    }

    continueAfterToolResults(allResults, assistantMsg);
}

void AiService::cancelPendingTool(const AiToolCall& call)
{
    AiToolResult cancelResult;
    cancelResult.toolUseId = call.id;
    cancelResult.content = "User cancelled this action.";
    cancelResult.isError = true;

    AiMessage assistantMsg = conversationHistory_.last();
    QVector<AiToolResult> results;
    for (auto& block : assistantMsg.content) {
        if (block.type == "tool_use") {
            if (block.toolCall.id == call.id)
                results.append(cancelResult);
            else {
                AiToolResult skipResult;
                skipResult.toolUseId = block.toolCall.id;
                skipResult.content = "Skipped because another tool was cancelled.";
                skipResult.isError = true;
                results.append(skipResult);
            }
        }
    }
    continueAfterToolResults(results, assistantMsg);
}

void AiService::continueAfterToolResults(const QVector<AiToolResult>& results,
                                          const AiMessage& /*assistantMsg*/)
{
    AiMessage toolResultMsg;
    toolResultMsg.role = AiRole::User;
    toolResultMsg.timestamp = QDateTime::currentDateTime();

    for (auto& result : results) {
        AiContentBlock block;
        block.type = "tool_result";
        block.toolResult = result;
        toolResultMsg.content.append(block);
    }

    conversationHistory_.append(toolResultMsg);

    sendToApi();
}

} // namespace OpenDaw
