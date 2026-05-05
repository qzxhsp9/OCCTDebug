#include "MainWindow.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

#include <Standard_Version.hxx>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);

    auto* title = new QLabel(tr("OCCTDebug — Qt6 + OCCT 骨架"), central);
    title->setStyleSheet(QStringLiteral("font-size: 16px; font-weight: bold;"));

    const QString occtVer = QString::fromLatin1(OCC_VERSION_STRING);
    auto* ver = new QLabel(tr("OCCT 版本: %1").arg(occtVer), central);

    layout->addWidget(title);
    layout->addWidget(ver);
    layout->addStretch();

    central->setLayout(layout);
    setCentralWidget(central);
    resize(520, 320);
    setWindowTitle(tr("OCCTDebug"));
}
