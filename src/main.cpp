#include <QApplication>
#include <QFile>
#include <QDateTime>
#include <juce_events/juce_events.h>
#include "app/FreeDawApplication.h"

static QFile* logFile = nullptr;

void logMessageHandler(QtMsgType type, const QMessageLogContext&, const QString& msg)
{
    if (!logFile) return;
    const char* prefix = "";
    switch (type) {
        case QtDebugMsg:    prefix = "DBG"; break;
        case QtWarningMsg:  prefix = "WRN"; break;
        case QtCriticalMsg: prefix = "CRT"; break;
        case QtFatalMsg:    prefix = "FTL"; break;
        default: break;
    }
    QTextStream out(logFile);
    out << QDateTime::currentDateTime().toString("hh:mm:ss.zzz")
        << " [" << prefix << "] " << msg << "\n";
    out.flush();
}

int main(int argc, char* argv[])
{
    QApplication qtApp(argc, argv);
    qtApp.setApplicationName("FreeDaw");
    qtApp.setApplicationVersion("0.1.0");
    qtApp.setOrganizationName("FreeDaw");

    QFile lf(QApplication::applicationDirPath() + "/freedaw.log");
    if (lf.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        logFile = &lf;
        qInstallMessageHandler(logMessageHandler);
    }

    juce::ScopedJuceInitialiser_GUI juceInit;

    freedaw::FreeDawApplication app;
    if (!app.initialize())
        return 1;

    app.showMainWindow();
    return qtApp.exec();
}
