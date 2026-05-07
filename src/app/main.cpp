#include "app/MainWindow.h"

#include "core/Logger.h"

#include <QApplication>

#include <Standard_Version.hxx>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("OCCTDebug"));
    QApplication::setOrganizationName(QStringLiteral("OCCTDebug"));
    //QApplication::setApplicationDisplayName(
    //    QStringLiteral("OCCTDebug ¡ª Qt6 + OCCT %1").arg(QString::fromLatin1(OCC_VERSION_STRING)));

    Logger::info(QStringLiteral("OCCT %1").arg(QString::fromLatin1(OCC_VERSION_STRING)));

    MainWindow w;
    w.show();
    return app.exec();
}
