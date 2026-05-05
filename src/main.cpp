#include "MainWindow.h"

#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("OCCTDebug"));
    QApplication::setOrganizationName(QStringLiteral("OCCTDebug"));

    MainWindow w;
    w.show();
    return app.exec();
}
