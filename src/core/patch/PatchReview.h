#pragma once

#include <QString>
#include <QVector>

namespace occtdebug
{
enum class PatchReviewStatus
{
    Draft,
    NeedsReview,
    Approved,
    Rejected,
};

struct PatchReviewStep
{
    QString title;
    QString state;
    QString note;
};

class PatchReviewWorkflow
{
public:
    static PatchReviewWorkflow createDefault(const QString& caseId, const QString& patchDiff);

    PatchReviewStatus status() const;
    QString statusText() const;
    const QVector<PatchReviewStep>& steps() const;

    void markNeedsReview(const QString& note);
    void approve(const QString& note);
    void reject(const QString& note);

private:
    QString m_caseId;
    QString m_patchDiff;
    PatchReviewStatus m_status = PatchReviewStatus::Draft;
    QVector<PatchReviewStep> m_steps;
};
} // namespace occtdebug
