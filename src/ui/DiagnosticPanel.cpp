#include "ui/DiagnosticPanel.h"

#include <QListWidget>
#include <QListWidgetItem>
#include <QVBoxLayout>

namespace
{
QString severityPrefix(DiagnosticSeverity s)
{
    switch (s)
    {
    case DiagnosticSeverity::Critical:
        return QStringLiteral("[Critical]");
    case DiagnosticSeverity::Error:
        return QStringLiteral("[Error]");
    case DiagnosticSeverity::Warning:
        return QStringLiteral("[Warning]");
    case DiagnosticSeverity::Info:
    default:
        return QStringLiteral("[Info]");
    }
}
} // namespace

DiagnosticPanel::DiagnosticPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    m_list = new QListWidget(this);
    layout->addWidget(m_list);

    connect(m_list, &QListWidget::itemActivated, this, [this](QListWidgetItem*) { onItemActivated(); });
    connect(m_list, &QListWidget::itemClicked, this, [this](QListWidgetItem*) { onItemActivated(); });
}

void DiagnosticPanel::setFindings(const std::vector<DiagnosticFinding>& findings)
{
    m_findings = findings;
    m_list->clear();
    int row = 0;
    for (const DiagnosticFinding& f : findings)
    {
        const QString line = QStringLiteral("%1 %2 — %3")
                                   .arg(severityPrefix(f.severity))
                                   .arg(QString::fromStdString(f.ruleId))
                                   .arg(QString::fromStdString(f.title));
        auto* item = new QListWidgetItem(line);
        item->setData(Qt::UserRole, row);
        m_list->addItem(item);
        ++row;
    }
}

void DiagnosticPanel::onItemActivated()
{
    const QList<QListWidgetItem*> items = m_list->selectedItems();
    if (items.isEmpty())
    {
        return;
    }
    const int idx = items.front()->data(Qt::UserRole).toInt();
    if (idx < 0 || idx >= static_cast<int>(m_findings.size()))
    {
        return;
    }
    const DiagnosticFinding& f = m_findings[static_cast<size_t>(idx)];
    if (!f.relatedShapeIds.empty())
    {
        emit findingActivated(f.relatedShapeIds.front());
    }
}
