#include "workbench/VerificationPanel.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace
{
QString s(const char* text)
{
    return QString::fromUtf8(text);
}

QLabel* label(const QString& text, const QString& objectName = QString())
{
    auto* out = new QLabel(text);
    if (!objectName.isEmpty())
    {
        out->setObjectName(objectName);
    }
    out->setWordWrap(true);
    return out;
}

void setMargins(QLayout* layout, int margin = 10, int spacing = 8)
{
    layout->setContentsMargins(margin, margin, margin, margin);
    layout->setSpacing(spacing);
}

void clearLayout(QLayout* layout)
{
    if (layout == nullptr)
    {
        return;
    }

    while (QLayoutItem* child = layout->takeAt(0))
    {
        if (QWidget* widget = child->widget())
        {
            widget->deleteLater();
        }
        delete child;
    }
}
} // namespace

namespace occtdebug
{
VerificationPanel::VerificationPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    setMargins(layout, 0, 6);

    auto* gridHost = new QWidget;
    m_grid = new QGridLayout(gridHost);
    setMargins(m_grid, 0, 6);
    layout->addWidget(gridHost);

    auto* exportButton = new QPushButton(s("导出 Markdown 报告"));
    auto* packButton = new QPushButton(s("导出 Repro Pack"));
    connect(exportButton, &QPushButton::clicked, this, &VerificationPanel::exportMarkdownRequested);
    connect(packButton, &QPushButton::clicked, this, &VerificationPanel::exportReproPackRequested);

    auto* actions = new QWidget;
    auto* actionLayout = new QHBoxLayout(actions);
    actionLayout->setContentsMargins(0, 0, 0, 0);
    actionLayout->setSpacing(8);
    actionLayout->addWidget(exportButton);
    actionLayout->addWidget(packButton);
    actionLayout->addStretch(1);
    layout->addWidget(actions);
}

void VerificationPanel::setItems(const QVector<LabelValue>& items)
{
    clearLayout(m_grid);
    if (m_grid == nullptr)
    {
        return;
    }

    for (int row = 0; row < items.size(); ++row)
    {
        const LabelValue& metric = items[row];
        m_grid->addWidget(label(metric.label, QStringLiteral("MutedText")), row, 0);
        m_grid->addWidget(label(metric.value), row, 1);
    }
}
} // namespace occtdebug
