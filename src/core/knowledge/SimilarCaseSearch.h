#pragma once

#include "core/case/CaseManifest.h"

#include <QString>
#include <QVector>

namespace occtdebug
{
struct SimilarCaseMatch
{
    SimilarCase similarCase;
    int score = 0;
};

class SimilarCaseSearch
{
public:
    static QVector<SimilarCaseMatch> search(const QVector<SimilarCase>& cases, const QString& query, int limit = 10);
};
} // namespace occtdebug
