#include "core/patch/PatchReview.h"

namespace occtdebug
{
namespace
{
QString statusText(PatchReviewStatus status)
{
    switch (status)
    {
    case PatchReviewStatus::Draft:
        return QStringLiteral("Draft");
    case PatchReviewStatus::NeedsReview:
        return QStringLiteral("Needs review");
    case PatchReviewStatus::Approved:
        return QStringLiteral("Approved");
    case PatchReviewStatus::Rejected:
        return QStringLiteral("Rejected");
    }

    return QStringLiteral("Unknown");
}
} // namespace

PatchReviewWorkflow PatchReviewWorkflow::createDefault(const QString& caseId, const QString& patchDiff)
{
    PatchReviewWorkflow workflow;
    workflow.m_caseId = caseId;
    workflow.m_patchDiff = patchDiff;
    workflow.m_steps = {
        {QStringLiteral("Candidate patch captured"), QStringLiteral("Done"), QStringLiteral("Patch diff is attached to the case.")},
        {QStringLiteral("Root-cause evidence linked"), QStringLiteral("Pending"), QStringLiteral("Attach DRAW log, shape check, and source location.")},
        {QStringLiteral("Reviewer decision"), QStringLiteral("Pending"), QStringLiteral("Waiting for maintainer review.")},
        {QStringLiteral("Regression gate"), QStringLiteral("Pending"), QStringLiteral("Requires testgrid/testdiff evidence.")},
    };
    return workflow;
}

PatchReviewStatus PatchReviewWorkflow::status() const
{
    return m_status;
}

QString PatchReviewWorkflow::statusText() const
{
    return occtdebug::statusText(m_status);
}

const QVector<PatchReviewStep>& PatchReviewWorkflow::steps() const
{
    return m_steps;
}

void PatchReviewWorkflow::markNeedsReview(const QString& note)
{
    m_status = PatchReviewStatus::NeedsReview;
    if (m_steps.size() > 1)
    {
        m_steps[1].state = QStringLiteral("Done");
        m_steps[1].note = note;
    }
    if (m_steps.size() > 2)
    {
        m_steps[2].state = QStringLiteral("Needs review");
    }
}

void PatchReviewWorkflow::approve(const QString& note)
{
    m_status = PatchReviewStatus::Approved;
    if (m_steps.size() > 2)
    {
        m_steps[2].state = QStringLiteral("Approved");
        m_steps[2].note = note;
    }
    if (m_steps.size() > 3)
    {
        m_steps[3].state = QStringLiteral("Ready");
    }
}

void PatchReviewWorkflow::reject(const QString& note)
{
    m_status = PatchReviewStatus::Rejected;
    if (m_steps.size() > 2)
    {
        m_steps[2].state = QStringLiteral("Rejected");
        m_steps[2].note = note;
    }
    if (m_steps.size() > 3)
    {
        m_steps[3].state = QStringLiteral("Blocked");
    }
}
} // namespace occtdebug
