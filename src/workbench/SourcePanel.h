#pragma once

#include <QString>
#include <QVector>
#include <QWidget>

class QLineEdit;
class QListWidget;
class QPlainTextEdit;

namespace occtdebug
{
struct SourceSearchResult
{
    QString text;
    QString filePath;
    int lineNumber = 0;
};

class SourcePanel final : public QWidget
{
    Q_OBJECT

public:
    explicit SourcePanel(QWidget* parent = nullptr);

    QString searchQuery() const;
    QVector<SourceSearchResult> searchResults() const;
    SourceSearchResult currentSearchResult() const;

    void setSourceText(const QString& text);
    void showSourceTextAtLine(const QString& text, int lineNumber);
    void clearSearchResults();
    void addSearchResult(const SourceSearchResult& result);
    void addMessageResult(const QString& text);

signals:
    void searchRequested();
    void resultActivated();

private:
    QLineEdit* m_searchEdit = nullptr;
    QPlainTextEdit* m_sourceView = nullptr;
    QListWidget* m_results = nullptr;
};
} // namespace occtdebug
