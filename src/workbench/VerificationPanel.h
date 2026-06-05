#pragma once

#include "core/case/CaseManifest.h"

#include <QWidget>

class QGridLayout;

namespace occtdebug
{
class VerificationPanel final : public QWidget
{
    Q_OBJECT

public:
    explicit VerificationPanel(QWidget* parent = nullptr);

    void setItems(const QVector<LabelValue>& items);

signals:
    void exportMarkdownRequested();
    void exportReproPackRequested();

private:
    QGridLayout* m_grid = nullptr;
};
} // namespace occtdebug
