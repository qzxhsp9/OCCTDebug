#include "core/Logger.h"
#include "workbench/WorkbenchWindow.h"

#include <QApplication>

#include <Standard_Version.hxx>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("OCCT Kernel Expert Workbench"));
    QApplication::setOrganizationName(QStringLiteral("OCCTDebug"));
    QApplication::setApplicationDisplayName(QString::fromUtf8("OCCT 内核专家工作台"));

    Logger::info(QStringLiteral("OCCT %1").arg(QString::fromLatin1(OCC_VERSION_STRING)));

    WorkbenchWindow w;
    w.show();
    return app.exec();
}
