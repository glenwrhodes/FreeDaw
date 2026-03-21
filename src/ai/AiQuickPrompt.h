#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QKeyEvent>

namespace OpenDaw {

class AiQuickPrompt : public QWidget {
    Q_OBJECT
public:
    explicit AiQuickPrompt(QWidget* parent = nullptr);

    void showCentered();

signals:
    void promptSubmitted(const QString& text);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void changeEvent(QEvent* event) override;

private:
    QLineEdit* lineEdit_ = nullptr;
};

} // namespace OpenDaw
