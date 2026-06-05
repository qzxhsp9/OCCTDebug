#include "core/repro/DrawLogParser.h"

#include <QJsonArray>
#include <QRegularExpression>

namespace occtdebug
{
namespace
{
QJsonArray toJsonArray(const QStringList& values)
{
    QJsonArray array;
    for (const QString& value : values)
    {
        array.push_back(value);
    }
    return array;
}
} // namespace

QString DrawLogAnalysis::summaryText() const
{
    const QString tokenSummary = successTokens.isEmpty()
        ? QStringLiteral("none")
        : successTokens.join(QStringLiteral(", "));
    const QString firstError = errorLines.isEmpty()
        ? QStringLiteral("none")
        : errorLines.first();

    return QStringLiteral("tokens=%1 errors=%2 first_error=%3 checkshape=%4")
        .arg(tokenSummary)
        .arg(errorLines.size())
        .arg(firstError)
        .arg(checkshapeStatus);
}

QJsonObject DrawLogAnalysis::toJson() const
{
    return {
        {QStringLiteral("schema_version"), 1},
        {QStringLiteral("line_count"), lineCount},
        {QStringLiteral("success_tokens"), toJsonArray(successTokens)},
        {QStringLiteral("error_count"), errorLines.size()},
        {QStringLiteral("error_lines"), toJsonArray(errorLines)},
        {QStringLiteral("checkshape"), QJsonObject {
            {QStringLiteral("status"), checkshapeStatus},
            {QStringLiteral("lines"), toJsonArray(checkshapeLines)},
        }},
    };
}

DrawLogAnalysis DrawLogParser::analyze(const QString& stdoutText, const QString& stderrText)
{
    DrawLogAnalysis analysis;

    const QString text = stdoutText + QLatin1Char('\n') + stderrText;
    const QStringList lines = text.split(QRegularExpression(QStringLiteral("\\r?\\n")));
    analysis.lineCount = lines.size();

    const QRegularExpression errorPattern(
        QStringLiteral("Exception|Faulty|invalid command|DRAW_SMOKE_FAILED|DRAW_SMOKE_ERROR|(^|\\s)Error(:|\\s|$)"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression tokenPattern(QStringLiteral("DRAW_[A-Z0-9_]+_OK|DRAW_SMOKE_OK"));
    const QRegularExpression checkshapePattern(
        QStringLiteral("checkshape|This shape seems to be valid|Faulty|valid"),
        QRegularExpression::CaseInsensitiveOption);

    for (const QString& line : lines)
    {
        if (tokenPattern.match(line).hasMatch())
        {
            analysis.successTokens.push_back(line.trimmed());
        }
        if (errorPattern.match(line).hasMatch())
        {
            analysis.errorLines.push_back(line.trimmed());
        }
        if (checkshapePattern.match(line).hasMatch())
        {
            analysis.checkshapeLines.push_back(line.trimmed());
        }
    }

    if (!analysis.checkshapeLines.isEmpty())
    {
        analysis.checkshapeStatus = QStringLiteral("unknown");
    }
    for (const QString& line : analysis.checkshapeLines)
    {
        if (line.contains(QStringLiteral("This shape seems to be valid"), Qt::CaseInsensitive))
        {
            analysis.checkshapeStatus = QStringLiteral("valid");
        }
        if (line.contains(QStringLiteral("Faulty"), Qt::CaseInsensitive))
        {
            analysis.checkshapeStatus = QStringLiteral("faulty");
            break;
        }
    }

    return analysis;
}
} // namespace occtdebug
