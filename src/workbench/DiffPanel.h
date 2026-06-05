#pragma once

#include "workbench/DiffArtifactsPresenter.h"

#include <QJsonObject>
#include <QWidget>

class QLabel;
class QComboBox;
class QLineEdit;
class QPixmap;
class QTableWidget;

namespace occtdebug
{
class DiffPanel : public QWidget
{
    Q_OBJECT

public:
    explicit DiffPanel(QWidget* parent = nullptr);

    void setDiffSummary(const QString& summary);
    void setTestdiffArtifacts(const QJsonObject& testdiffArtifacts);
    DiffArtifactsPresenter::Filter filter() const;
    QString selectedArtifactPath() const;
    QString preferredArtifactPath(const QString& kind, const QString& key, const QString& status = QString()) const;
    void setPreviewImage(const QString& path, const QPixmap& pixmap, const QString& message = QString());
    void setPreviewMessage(const QString& message);

    QLabel* summaryLabel() const;
    int indexRowCount() const;
    int analysisRowCount() const;
    QString previewText() const;

signals:
    void generateTopologyCompareRequested();
    void artifactOpenRequested(const QString& path, const QString& origin);
    void artifactPreviewRequested(const QString& path, const QString& origin);

private:
    void refreshTables();
    void requestSelectedArtifact();
    void requestSelectedPreview();
    void copySelectedArtifactPath();

    QLabel* m_diffLabel = nullptr;
    QLabel* m_previewLabel = nullptr;
    QLabel* m_previewPathLabel = nullptr;
    QTableWidget* m_indexTable = nullptr;
    QTableWidget* m_analysisTable = nullptr;
    QComboBox* m_kindFilter = nullptr;
    QComboBox* m_statusFilter = nullptr;
    QLineEdit* m_searchEdit = nullptr;
    QJsonObject m_testdiffArtifacts;
};
} // namespace occtdebug
