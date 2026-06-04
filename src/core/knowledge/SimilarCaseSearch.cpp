#include "core/knowledge/SimilarCaseSearch.h"

#include <algorithm>

#include <QStringList>

namespace occtdebug
{
namespace
{
int scoreCase(const SimilarCase& similarCase, const QString& query)
{
    if (query.trimmed().isEmpty())
    {
        return similarCase.score.toDouble() > 0.0 ? static_cast<int>(similarCase.score.toDouble() * 100.0) : 1;
    }

    int score = 0;
    const QString lowerQuery = query.toLower();
    const QStringList tokens = lowerQuery.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    const QString haystack = QStringLiteral("%1 %2").arg(similarCase.id, similarCase.title).toLower();
    for (const QString& token : tokens)
    {
        if (similarCase.id.toLower().contains(token))
        {
            score += 40;
        }
        if (haystack.contains(token))
        {
            score += 20;
        }
    }

    bool ok = false;
    const double numericScore = similarCase.score.toDouble(&ok);
    if (ok)
    {
        score += static_cast<int>(numericScore * 30.0);
    }
    return score;
}
} // namespace

QVector<SimilarCaseMatch> SimilarCaseSearch::search(const QVector<SimilarCase>& cases, const QString& query, int limit)
{
    QVector<SimilarCaseMatch> matches;
    if (limit <= 0)
    {
        return matches;
    }

    matches.reserve(cases.size());
    for (const SimilarCase& similarCase : cases)
    {
        const int score = scoreCase(similarCase, query);
        if (score > 0)
        {
            matches.push_back({similarCase, score});
        }
    }

    std::sort(matches.begin(), matches.end(), [](const SimilarCaseMatch& lhs, const SimilarCaseMatch& rhs) {
        return lhs.score > rhs.score;
    });
    if (matches.size() > limit)
    {
        matches.resize(limit);
    }
    return matches;
}
} // namespace occtdebug
