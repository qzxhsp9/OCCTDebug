#include "core/verify/VerificationWorkflow.h"

#include <utility>

namespace occtdebug
{
namespace
{
bool isBeforePhase(VerificationWorkflowPhase phase)
{
    return phase == VerificationWorkflowPhase::BeforeGate
        || phase == VerificationWorkflowPhase::BeforeCommand;
}
} // namespace

void VerificationWorkflow::reset()
{
    m_phase = VerificationWorkflowPhase::Idle;
    m_beforeGateResult = {};
    m_beforeCommandResult = {};
    m_afterGateResult = {};
    m_afterCommandResult = {};
    m_beforeCommandExecuted = false;
    m_afterCommandExecuted = false;
    m_patchApplied = false;
    m_finalStatus.clear();
    m_finalNote.clear();
}

VerificationWorkflowDecision VerificationWorkflow::begin()
{
    reset();
    m_phase = VerificationWorkflowPhase::BeforeGate;
    return {VerificationWorkflowAction::StartBeforeGate, false, QStringLiteral("before")};
}

VerificationWorkflowDecision VerificationWorkflow::onGateFinished(const CommandResult& result, bool hasConfiguredTestgrid)
{
    const bool before = m_phase == VerificationWorkflowPhase::BeforeGate;
    const QString phaseName = before ? QStringLiteral("before") : QStringLiteral("after");
    if (before)
    {
        m_beforeGateResult = result;
    }
    else
    {
        m_afterGateResult = result;
    }

    if (!commandSucceeded(result))
    {
        m_finalStatus = QStringLiteral("failed");
        m_finalNote = QStringLiteral("%1 draw_smoke gate failed").arg(phaseName);
        VerificationWorkflowDecision decision = persistOnly(
            phaseName,
            QStringLiteral("%1 draw_smoke gate failed; configured testgrid command was not started").arg(phaseName));
        if (m_patchApplied)
        {
            m_phase = VerificationWorkflowPhase::PatchUndo;
            decision.action = VerificationWorkflowAction::StartPatchUndo;
            return decision;
        }
        m_phase = VerificationWorkflowPhase::Idle;
        decision.action = VerificationWorkflowAction::Finalize;
        decision.finalStatus = m_finalStatus;
        decision.finalNote = m_finalNote;
        return decision;
    }

    if (hasConfiguredTestgrid)
    {
        m_phase = before ? VerificationWorkflowPhase::BeforeCommand : VerificationWorkflowPhase::AfterCommand;
        return {
            before ? VerificationWorkflowAction::StartBeforeCommand : VerificationWorkflowAction::StartAfterCommand,
            false,
            phaseName,
        };
    }

    VerificationWorkflowDecision decision = persistOnly(
        phaseName,
        QStringLiteral("%1 draw_smoke gate passed; testgrid executable is not configured, parsed local summary files only").arg(phaseName));
    if (before)
    {
        m_phase = VerificationWorkflowPhase::PatchApply;
        decision.action = VerificationWorkflowAction::StartPatchApply;
        return decision;
    }

    m_finalStatus = QStringLiteral("completed");
    m_finalNote = QStringLiteral("after phase completed");
    if (m_patchApplied)
    {
        m_phase = VerificationWorkflowPhase::PatchUndo;
        decision.action = VerificationWorkflowAction::StartPatchUndo;
        return decision;
    }
    m_phase = VerificationWorkflowPhase::Idle;
    decision.action = VerificationWorkflowAction::Finalize;
    decision.finalStatus = m_finalStatus;
    decision.finalNote = m_finalNote;
    return decision;
}

VerificationWorkflowDecision VerificationWorkflow::onCommandFinished(const CommandResult& result)
{
    const bool before = m_phase == VerificationWorkflowPhase::BeforeCommand;
    const QString phaseName = before ? QStringLiteral("before") : QStringLiteral("after");
    if (before)
    {
        m_beforeCommandResult = result;
        m_beforeCommandExecuted = true;
    }
    else
    {
        m_afterCommandResult = result;
        m_afterCommandExecuted = true;
    }

    VerificationWorkflowDecision decision =
        persistOnly(phaseName, QStringLiteral("%1 configured testgrid command finished").arg(phaseName));
    if (before)
    {
        m_phase = VerificationWorkflowPhase::PatchApply;
        decision.action = VerificationWorkflowAction::StartPatchApply;
        return decision;
    }

    m_finalStatus = QStringLiteral("completed");
    m_finalNote = QStringLiteral("after configured testgrid command finished");
    if (m_patchApplied)
    {
        m_phase = VerificationWorkflowPhase::PatchUndo;
        decision.action = VerificationWorkflowAction::StartPatchUndo;
        return decision;
    }
    m_phase = VerificationWorkflowPhase::Idle;
    decision.action = VerificationWorkflowAction::Finalize;
    decision.finalStatus = m_finalStatus;
    decision.finalNote = m_finalNote;
    return decision;
}

VerificationWorkflowDecision VerificationWorkflow::onPatchFinished(const CommandResult& result)
{
    const bool apply = m_phase == VerificationWorkflowPhase::PatchApply;
    const bool success = commandSucceeded(result);
    if (apply)
    {
        if (!success)
        {
            m_patchApplied = false;
            return finalize(QStringLiteral("blocked"), QStringLiteral("patch apply failed; after phase was not started"));
        }

        m_patchApplied = true;
        m_phase = VerificationWorkflowPhase::AfterGate;
        return {VerificationWorkflowAction::StartAfterGate, false, QStringLiteral("after")};
    }

    m_patchApplied = !success;
    const QString status = success ? m_finalStatus : QStringLiteral("undo_failed");
    const QString note = success
        ? QStringLiteral("%1; patch cleanup undo completed").arg(m_finalNote)
        : QStringLiteral("%1; patch cleanup undo failed").arg(m_finalNote);
    return finalize(status.isEmpty() ? QStringLiteral("completed") : status, note);
}

VerificationWorkflowDecision VerificationWorkflow::onStartFailure(VerificationWorkflowAction action, const QString& error)
{
    switch (action)
    {
    case VerificationWorkflowAction::StartBeforeCommand:
    case VerificationWorkflowAction::StartAfterCommand:
    {
        const bool before = isBeforePhase(m_phase);
        const QString phaseName = before ? QStringLiteral("before") : QStringLiteral("after");
        m_finalStatus = QStringLiteral("failed");
        m_finalNote = QStringLiteral("%1 testgrid command failed to start").arg(phaseName);
        VerificationWorkflowDecision decision = persistOnly(
            phaseName,
            QStringLiteral("%1 draw_smoke gate passed; configured testgrid command failed to start: %2").arg(phaseName, error));
        if (!before && m_patchApplied)
        {
            m_phase = VerificationWorkflowPhase::PatchUndo;
            decision.action = VerificationWorkflowAction::StartPatchUndo;
            return decision;
        }
        m_phase = VerificationWorkflowPhase::Idle;
        decision.action = VerificationWorkflowAction::Finalize;
        decision.finalStatus = m_finalStatus;
        decision.finalNote = m_finalNote;
        return decision;
    }
    case VerificationWorkflowAction::StartPatchApply:
        return finalize(QStringLiteral("blocked"),
            QStringLiteral("before phase completed, but patch apply could not start: %1").arg(error));
    case VerificationWorkflowAction::StartAfterGate:
        m_finalStatus = QStringLiteral("failed");
        m_finalNote = QStringLiteral("patch applied, but after gate failed to start");
        m_phase = VerificationWorkflowPhase::PatchUndo;
        return {
            VerificationWorkflowAction::StartPatchUndo,
            false,
            QStringLiteral("after"),
            QString(),
            QString(),
            QStringLiteral("%1: %2").arg(m_finalNote, error),
        };
    case VerificationWorkflowAction::StartPatchUndo:
        return finalize(QStringLiteral("undo_failed"),
            QStringLiteral("%1; failed to start cleanup undo: %2").arg(m_finalNote, error));
    default:
        return finalize(QStringLiteral("failed"), QStringLiteral("workflow start failed: %1").arg(error));
    }
}

VerificationWorkflowPhase VerificationWorkflow::phase() const
{
    return m_phase;
}

bool VerificationWorkflow::patchApplied() const
{
    return m_patchApplied;
}

bool VerificationWorkflow::beforeCommandExecuted() const
{
    return m_beforeCommandExecuted;
}

bool VerificationWorkflow::afterCommandExecuted() const
{
    return m_afterCommandExecuted;
}

QString VerificationWorkflow::finalStatus() const
{
    return m_finalStatus;
}

QString VerificationWorkflow::finalNote() const
{
    return m_finalNote;
}

CommandResult VerificationWorkflow::beforeGateResult() const
{
    return m_beforeGateResult;
}

CommandResult VerificationWorkflow::beforeCommandResult() const
{
    return m_beforeCommandResult;
}

CommandResult VerificationWorkflow::afterGateResult() const
{
    return m_afterGateResult;
}

CommandResult VerificationWorkflow::afterCommandResult() const
{
    return m_afterCommandResult;
}

CommandResult VerificationWorkflow::gateResult(const QString& phase) const
{
    return phase == QStringLiteral("after") ? m_afterGateResult : m_beforeGateResult;
}

CommandResult VerificationWorkflow::commandResult(const QString& phase) const
{
    return phase == QStringLiteral("after") ? m_afterCommandResult : m_beforeCommandResult;
}

bool VerificationWorkflow::commandExecuted(const QString& phase) const
{
    return phase == QStringLiteral("after") ? m_afterCommandExecuted : m_beforeCommandExecuted;
}

bool VerificationWorkflow::commandSucceeded(const CommandResult& result)
{
    return !result.canceled
        && !result.timedOut
        && result.exitCode == 0;
}

QString VerificationWorkflow::phaseLabel(VerificationWorkflowPhase phase)
{
    switch (phase)
    {
    case VerificationWorkflowPhase::BeforeGate:
    case VerificationWorkflowPhase::BeforeCommand:
        return QStringLiteral("before");
    case VerificationWorkflowPhase::AfterGate:
    case VerificationWorkflowPhase::AfterCommand:
        return QStringLiteral("after");
    case VerificationWorkflowPhase::PatchApply:
        return QStringLiteral("patch_apply");
    case VerificationWorkflowPhase::PatchUndo:
        return QStringLiteral("patch_undo");
    case VerificationWorkflowPhase::Idle:
        break;
    }
    return QStringLiteral("idle");
}

VerificationWorkflowDecision VerificationWorkflow::finalize(QString status, QString note)
{
    m_finalStatus = std::move(status);
    m_finalNote = std::move(note);
    m_phase = VerificationWorkflowPhase::Idle;
    return {
        VerificationWorkflowAction::Finalize,
        false,
        QString(),
        QString(),
        m_finalStatus,
        m_finalNote,
    };
}

VerificationWorkflowDecision VerificationWorkflow::persistOnly(const QString& phase, const QString& note) const
{
    return {
        VerificationWorkflowAction::None,
        true,
        phase,
        note,
    };
}
} // namespace occtdebug
