#pragma once

#include "core/case/CaseManifest.h"

#include <QWidget>

class QLabel;
class QTableWidget;

namespace occtdebug
{
class EvidencePanel final : public QWidget
{
    Q_OBJECT

public:
    explicit EvidencePanel(QWidget* parent = nullptr);

    void setData(const QString& summary, const QVector<EvidenceRecord>& records);
    void appendRecord(const EvidenceRecord& record);

signals:
    void recordActivated(const occtdebug::EvidenceRecord& record);

private:
    void emitRecordActivated(int row);

    QVector<EvidenceRecord> m_records;
    QLabel* m_summaryLabel = nullptr;
    QTableWidget* m_table = nullptr;
};
} // namespace occtdebug
