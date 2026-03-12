#include <QApplication>
#include <juce_events/juce_events.h>
#include "app/FreeDawApplication.h"

int main(int argc, char* argv[])
{
    QApplication qtApp(argc, argv);
    qtApp.setApplicationName("FreeDaw");
    qtApp.setApplicationVersion("0.1.0");
    qtApp.setOrganizationName("FreeDaw");

    juce::ScopedJuceInitialiser_GUI juceInit;

    freedaw::FreeDawApplication app;
    if (!app.initialize())
        return 1;

    app.showMainWindow();
    return qtApp.exec();
}
