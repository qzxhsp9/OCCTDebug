#include "workbench/DiffPanel.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QLineEdit>
#include <QPixmap>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace occtdebug
{
namespace
{
void setupTable(QTableWidget* table)
{
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->verticalHeader()->setVisible(false);
    table->setAlternatingRowColors(true);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
}

QString tableText(const QTableWidget* table, int row, int column)
{
    const auto* item = table == nullptr ? nullptr : table->item(row, column);
    return item == nullptr ? QString() : item->text().trimmed();
}
} // namespace

DiffPanel::DiffPanel(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 6, 0, 6);
    layout->setSpacing(8);

    auto* actions = new QHBoxLayout;
    actions->setContentsMargins(0, 0, 0, 0);
    auto* compareButton = new QPushButton(QStringLiteral("Generate topology compare"));
    auto* openButton = new QPushButton(QStringLiteral("Open artifact"));
    auto* previewButton = new QPushButton(QStringLiteral("Preview image"));
    auto* copyPathButton = new QPushButton(QStringLiteral("Copy path"));
    m_kindFilter = new QComboBox;
    m_kindFilter->addItem(QStringLiteral("All kinds"), QString());
    m_kindFilter->addItem(QStringLiteral("Image"), QStringLiteral("image"));
    m_kindFilter->addItem(QStringLiteral("Property"), QStringLiteral("property"));
    m_kindFilter->addItem(QStringLiteral("Performance"), QStringLiteral("performance"));
    m_statusFilter = new QComboBox;
    m_statusFilter->addItem(QStringLiteral("All statuses"), QString());
    m_statusFilter->addItem(QStringLiteral("Paired + diff"), QStringLiteral("paired_with_diff"));
    m_statusFilter->addItem(QStringLiteral("Paired"), QStringLiteral("paired"));
    m_statusFilter->addItem(QStringLiteral("Diff only"), QStringLiteral("diff_only"));
    m_statusFilter->addItem(QStringLiteral("Incomplete"), QStringLiteral("incomplete"));
    m_searchEdit = new QLineEdit;
    m_searchEdit->setPlaceholderText(QStringLiteral("Search artifacts"));
    actions->addWidget(compareButton);
    actions->addWidget(openButton);
    actions->addWidget(previewButton);
    actions->addWidget(copyPathButton);
    actions->addWidget(m_kindFilter);
    actions->addWidget(m_statusFilter);
    actions->addWidget(m_searchEdit, 1);
    actions->addStretch(1);
    layout->addLayout(actions);

    m_diffLabel = new QLabel;
    m_diffLabel->setWordWrap(true);
    layout->addWidget(m_diffLabel);

    m_previewPathLabel = new QLabel(QStringLiteral("Select an image artifact to preview."));
    m_previewPathLabel->setWordWrap(true);
    layout->addWidget(m_previewPathLabel);

    m_previewLabel = new QLabel(QStringLiteral("No image preview loaded."));
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setMinimumHeight(120);
    m_previewLabel->setStyleSheet(QStringLiteral("QLabel { border: 1px solid rgba(255,255,255,32); background: rgba(255,255,255,10); }"));
    layout->addWidget(m_previewLabel);

    m_indexTable = new QTableWidget(0, 6);
    m_indexTable->setHorizontalHeaderLabels({
        QStringLiteral("Kind"),
        QStringLiteral("Key"),
        QStringLiteral("Status"),
        QStringLiteral("Before"),
        QStringLiteral("After"),
        QStringLiteral("Diff"),
    });
    setupTable(m_indexTable);
    layout->addWidget(m_indexTable, 1);

    m_analysisTable = new QTableWidget(0, 4);
    m_analysisTable->setHorizontalHeaderLabels({
        QStringLiteral("Kind"),
        QStringLiteral("Key"),
        QStringLiteral("Status"),
        QStringLiteral("Analysis"),
    });
    setupTable(m_analysisTable);
    layout->addWidget(m_analysisTable, 1);

    connect(compareButton, &QPushButton::clicked, this, &DiffPanel::generateTopologyCompareRequested);
    connect(openButton, &QPushButton::clicked, this, &DiffPanel::requestSelectedArtifact);
    connect(previewButton, &QPushButton::clicked, this, &DiffPanel::requestSelectedPreview);
    connect(copyPathButton, &QPushButton::clicked, this, &DiffPanel::copySelectedArtifactPath);
    connect(m_kindFilter, &QComboBox::currentIndexChanged, this, [this](int) {
        refreshTables();
    });
    connect(m_statusFilter, &QComboBox::currentIndexChanged, this, [this](int) {
        refreshTables();
    });
    connect(m_searchEdit, &QLineEdit::textChanged, this, [this](const QString&) {
        refreshTables();
    });
    connect(m_indexTable, &QTableWidget::cellDoubleClicked, this, [this](int, int) {
        requestSelectedArtifact();
    });
    connect(m_analysisTable, &QTableWidget::cellDoubleClicked, this, [this](int row, int) {
        emit artifactOpenRequested(preferredArtifactPath(tableText(m_analysisTable, row, 0),
                                      tableText(m_analysisTable, row, 1),
                                      tableText(m_analysisTable, row, 2)),
            QStringLiteral("diff analysis"));
    });
}

void DiffPanel::setDiffSummary(const QString& summary)
{
    if (m_diffLabel != nullptr)
    {
        m_diffLabel->setText(summary);
    }
}

void DiffPanel::setTestdiffArtifacts(const QJsonObject& testdiffArtifacts)
{
    m_testdiffArtifacts = testdiffArtifacts;
    refreshTables();
}

DiffArtifactsPresenter::Filter DiffPanel::filter() const
{
    DiffArtifactsPresenter::Filter currentFilter;
    if (m_kindFilter != nullptr)
    {
        currentFilter.kind = m_kindFilter->currentData().toString();
    }
    if (m_statusFilter != nullptr)
    {
        currentFilter.status = m_statusFilter->currentData().toString();
    }
    if (m_searchEdit != nullptr)
    {
        currentFilter.searchText = m_searchEdit->text();
    }
    return currentFilter;
}

QString DiffPanel::selectedArtifactPath() const
{
    if (m_analysisTable != nullptr && m_analysisTable->hasFocus())
    {
        const int row = m_analysisTable->currentRow();
        if (row >= 0)
        {
            return preferredArtifactPath(tableText(m_analysisTable, row, 0),
                                         tableText(m_analysisTable, row, 1),
                                         tableText(m_analysisTable, row, 2));
        }
    }

    if (m_indexTable == nullptr)
    {
        return {};
    }

    const int row = m_indexTable->currentRow();
    if (row < 0)
    {
        return {};
    }

    const int column = m_indexTable->currentColumn();
    if (column >= 3 && column <= 5)
    {
        const QString path = tableText(m_indexTable, row, column);
        if (!path.isEmpty() && path != QStringLiteral("-"))
        {
            return path;
        }
    }

    for (const int candidateColumn : {5, 4, 3})
    {
        const QString path = tableText(m_indexTable, row, candidateColumn);
        if (!path.isEmpty() && path != QStringLiteral("-"))
        {
            return path;
        }
    }
    return {};
}

QString DiffPanel::preferredArtifactPath(const QString& kind, const QString& key, const QString& status) const
{
    const QJsonArray groups =
        m_testdiffArtifacts.value(QStringLiteral("artifact_index")).toObject().value(QStringLiteral("groups")).toArray();
    for (const QJsonValue& value : groups)
    {
        const QJsonObject group = value.toObject();
        if (group.value(QStringLiteral("kind")).toString() != kind
            || group.value(QStringLiteral("key")).toString() != key
            || (!status.isEmpty() && group.value(QStringLiteral("status")).toString() != status))
        {
            continue;
        }

        for (const QString& role : {QStringLiteral("diff"), QStringLiteral("after"), QStringLiteral("before")})
        {
            const QString path = group.value(role).toString().trimmed();
            if (!path.isEmpty())
            {
                return path;
            }
        }
    }
    return {};
}

void DiffPanel::setPreviewImage(const QString& path, const QPixmap& pixmap, const QString& message)
{
    if (m_previewPathLabel != nullptr)
    {
        m_previewPathLabel->setText(path.trimmed().isEmpty() ? message : path);
    }
    if (m_previewLabel == nullptr)
    {
        return;
    }
    if (pixmap.isNull())
    {
        m_previewLabel->clear();
        m_previewLabel->setText(message.isEmpty() ? QStringLiteral("Image preview unavailable.") : message);
        return;
    }
    m_previewLabel->setPixmap(pixmap.scaled(520, 220, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void DiffPanel::setPreviewMessage(const QString& message)
{
    if (m_previewPathLabel != nullptr)
    {
        m_previewPathLabel->setText(message);
    }
    if (m_previewLabel != nullptr)
    {
        m_previewLabel->clear();
        m_previewLabel->setText(message);
    }
}

QLabel* DiffPanel::summaryLabel() const
{
    return m_diffLabel;
}

int DiffPanel::indexRowCount() const
{
    return m_indexTable == nullptr ? 0 : m_indexTable->rowCount();
}

int DiffPanel::analysisRowCount() const
{
    return m_analysisTable == nullptr ? 0 : m_analysisTable->rowCount();
}

QString DiffPanel::previewText() const
{
    return m_previewPathLabel == nullptr ? QString() : m_previewPathLabel->text();
}

void DiffPanel::refreshTables()
{
    const DiffArtifactsPresenter::Filter currentFilter = filter();
    DiffArtifactsPresenter::applyIndexToTable(m_indexTable, m_testdiffArtifacts, currentFilter);
    DiffArtifactsPresenter::applyAnalysisToTable(m_analysisTable, m_testdiffArtifacts, currentFilter);
}

void DiffPanel::requestSelectedArtifact()
{
    emit artifactOpenRequested(selectedArtifactPath(), QStringLiteral("diff artifact"));
}

void DiffPanel::requestSelectedPreview()
{
    emit artifactPreviewRequested(selectedArtifactPath(), QStringLiteral("diff preview"));
}

void DiffPanel::copySelectedArtifactPath()
{
    const QString path = selectedArtifactPath();
    if (path.isEmpty())
    {
        setPreviewMessage(QStringLiteral("No artifact selected."));
        return;
    }
    if (QApplication::clipboard() != nullptr)
    {
        QApplication::clipboard()->setText(path);
    }
    setPreviewMessage(QStringLiteral("Copied artifact path: %1").arg(path));
}
} // namespace occtdebug
