#include "workbench/SourcePanel.h"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTextCursor>
#include <QVBoxLayout>

namespace
{
QString s(const char* text)
{
    return QString::fromUtf8(text);
}

void setMargins(QLayout* layout, int margin = 10, int spacing = 8)
{
    layout->setContentsMargins(margin, margin, margin, margin);
    layout->setSpacing(spacing);
}
} // namespace

namespace occtdebug
{
SourcePanel::SourcePanel(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    setMargins(layout, 0, 6);

    auto* searchLayout = new QHBoxLayout;
    setMargins(searchLayout, 0, 0);
    m_searchEdit = new QLineEdit;
    m_searchEdit->setPlaceholderText(s("搜索源码关键词 / 文件名"));
    auto* searchButton = new QPushButton(s("搜索"));
    connect(searchButton, &QPushButton::clicked, this, &SourcePanel::searchRequested);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &SourcePanel::searchRequested);
    searchLayout->addWidget(m_searchEdit, 1);
    searchLayout->addWidget(searchButton);
    layout->addLayout(searchLayout);

    m_sourceView = new QPlainTextEdit;
    m_sourceView->setReadOnly(true);
    layout->addWidget(m_sourceView, 2);

    m_results = new QListWidget;
    m_results->setObjectName(QStringLiteral("SourceSearchResults"));
    connect(m_results, &QListWidget::itemActivated, this, &SourcePanel::resultActivated);
    connect(m_results, &QListWidget::itemDoubleClicked, this, &SourcePanel::resultActivated);
    layout->addWidget(m_results, 1);
}

QString SourcePanel::searchQuery() const
{
    return m_searchEdit->text().trimmed();
}

QVector<SourceSearchResult> SourcePanel::searchResults() const
{
    QVector<SourceSearchResult> rows;
    for (int row = 0; row < m_results->count(); ++row)
    {
        QListWidgetItem* item = m_results->item(row);
        SourceSearchResult result;
        result.text = item->text();
        result.filePath = item->data(Qt::UserRole).toString();
        result.lineNumber = item->data(Qt::UserRole + 1).toInt();
        rows.push_back(result);
    }
    return rows;
}

SourceSearchResult SourcePanel::currentSearchResult() const
{
    QListWidgetItem* item = m_results->currentItem();
    if (item == nullptr)
    {
        return {};
    }

    SourceSearchResult result;
    result.text = item->text();
    result.filePath = item->data(Qt::UserRole).toString();
    result.lineNumber = item->data(Qt::UserRole + 1).toInt();
    return result;
}

void SourcePanel::setSourceText(const QString& text)
{
    m_sourceView->setPlainText(text);
}

void SourcePanel::showSourceTextAtLine(const QString& text, int lineNumber)
{
    m_sourceView->setPlainText(text);

    QTextCursor cursor = m_sourceView->textCursor();
    cursor.movePosition(QTextCursor::Start);
    for (int line = 1; line < lineNumber; ++line)
    {
        cursor.movePosition(QTextCursor::Down);
    }
    m_sourceView->setTextCursor(cursor);
    m_sourceView->centerCursor();
}

void SourcePanel::clearSearchResults()
{
    m_results->clear();
}

void SourcePanel::addSearchResult(const SourceSearchResult& result)
{
    auto* item = new QListWidgetItem(result.text);
    item->setData(Qt::UserRole, result.filePath);
    item->setData(Qt::UserRole + 1, result.lineNumber);
    m_results->addItem(item);
}

void SourcePanel::addMessageResult(const QString& text)
{
    m_results->addItem(text);
}
} // namespace occtdebug
