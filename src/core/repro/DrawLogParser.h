#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace occtdebug
{
struct DrawLogAnalysis
{
    int lineCount = 0;
    QStringList successTokens;
    QStringList errorLines;
    QStringList checkshapeLines;
    QString checkshapeStatus = QStringLiteral("not_detected");

    QString summaryText() const;
    QJsonObject toJson() const;
};

class DrawLogParser
{
public:
    static DrawLogAnalysis analyze(const QString& stdoutText, const QString& stderrText);
};
} // namespace occtdebug
