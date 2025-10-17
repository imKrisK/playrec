#include <QtWidgets/QApplication>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtGui/QIcon>
#include <QtCore/QDebug>
#include "../../include/gui/main_window.h"

int main(int argc, char *argv[])
{
    qDebug() << "Starting PlayRec GUI application...";
    QApplication app(argc, argv);
    
    // Set application properties
    app.setApplicationName("PlayRec");
    app.setApplicationVersion("1.0.0");
    app.setApplicationDisplayName("PlayRec - Game Capture");
    app.setOrganizationName("PlayRec");
    app.setOrganizationDomain("playrec.app");
    
    // Set application icon (if available)
    // app.setWindowIcon(QIcon(":/icons/playrec.png"));
    
    qDebug() << "Creating main window...";
    // Create and show main window
    MainWindow window;
    qDebug() << "Showing main window...";
    window.show();
    
    qDebug() << "Starting event loop...";
    return app.exec();
}