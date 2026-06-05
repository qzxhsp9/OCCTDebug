#include "workbench/EvidencePanel.h"

#include <QAbstractItemView>
#include <QHeaderView>
#include <QLabel>
#include <QTableWidgetSelectionRange>
#include <QTableWidget>
#include <QTableWidgetItem>
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

QTableWidgetItem* item(const QString& text)
{
    auto* out = new QTableWidgetItem(text);
    out->setFlags(out->flags() & ~Qt::ItemIsEditable);
    return out;
}

void setMargins(QLayout* layout, int margin = 10, int spacing = 8)
{
    layout->setContentsMargins(margin, margin, margin, margin);
    layout->setSpacing(spacing);
}
} // namespace

namespace occtdebug
{
EvidencePanel::EvidencePanel(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    setMargins(layout, 0, 6);

    m_summaryLabel = label(QString(), QStringLiteral("EvidenceCard"));
    layout->addWidget(m_summaryLabel);

    m_table = new QTableWidget(0, 4);
    m_table->setHorizontalHeaderLabels({s("类型"), s("标题"), s("摘要"), s("链接")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->hide();
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_table, &QTableWidget::cellActivated, this, [this](int row, int) {
        emitRecordActivated(row);
    });
    connect(m_table, &QTableWidget::cellDoubleClicked, this, [this](int row, int) {
        emitRecordActivated(row);
    });
    layout->addWidget(m_table, 1);
}

void EvidencePanel::setData(const QString& summary, const QVector<EvidenceRecord>& records)
{
    m_summaryLabel->setText(summary);
    m_records.clear();
    m_table->setRowCount(0);
    for (const EvidenceRecord& record : records)
    {
        appendRecord(record);
    }
}

void EvidencePanel::appendRecord(const EvidenceRecord& record)
{
    m_records.push_back(record);
    const int row = m_table->rowCount();
    m_table->insertRow(row);
    m_table->setItem(row, 0, item(record.type));
    m_table->setItem(row, 1, item(record.title));
    m_table->setItem(row, 2, item(record.summary));
    m_table->setItem(row, 3, item(record.link));
}

void EvidencePanel::emitRecordActivated(int row)
{
    if (row < 0 || row >= m_records.size())
    {
        return;
    }
    m_table->setRangeSelected(QTableWidgetSelectionRange(row, 0, row, m_table->columnCount() - 1), true);
    emit recordActivated(m_records[row]);
}
} // namespace occtdebug
