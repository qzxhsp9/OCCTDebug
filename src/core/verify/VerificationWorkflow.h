#pragma once

#include "core/runner/CommandRunner.h"

#include <QString>

namespace occtdebug
{
enum class VerificationWorkflowPhase
{
    Idle,
    BeforeGate,
    BeforeCommand,
    PatchApply,
    AfterGate,
    AfterCommand,
    PatchUndo,
};

enum class VerificationWorkflowAction
{
    None,
    StartBeforeGate,
    StartBeforeCommand,
    StartPatchApply,
    StartAfterGate,
    StartAfterCommand,
    StartPatchUndo,
    Finalize,
};

struct VerificationWorkflowDecision
{
    VerificationWorkflowAction action = VerificationWorkflowAction::None;
    bool persistPhase = false;
    QString phase;
    QString note;
    QString finalStatus;
    QString finalNote;
};

class VerificationWorkflow
{
public:
    void reset();

    VerificationWorkflowDecision begin();
    VerificationWorkflowDecision onGateFinished(const CommandResult& result, bool hasConfiguredTestgrid);
    VerificationWorkflowDecision onCommandFinished(const CommandResult& result);
    VerificationWorkflowDecision onPatchFinished(const CommandResult& result);
    VerificationWorkflowDecision onStartFailure(VerificationWorkflowAction action, const QString& error);

    VerificationWorkflowPhase phase() const;
    bool patchApplied() const;
    bool beforeCommandExecuted() const;
    bool afterCommandExecuted() const;
    QString finalStatus() const;
    QString finalNote() const;
    CommandResult beforeGateResult() const;
    CommandResult beforeCommandResult() const;
    CommandResult afterGateResult() const;
    CommandResult afterCommandResult() const;
    CommandResult gateResult(const QString& phase) const;
    CommandResult commandResult(const QString& phase) const;
    bool commandExecuted(const QString& phase) const;

    static bool commandSucceeded(const CommandResult& result);
    static QString phaseLabel(VerificationWorkflowPhase phase);

private:
    VerificationWorkflowDecision finalize(QString status, QString note);
    VerificationWorkflowDecision persistOnly(const QString& phase, const QString& note) const;

    VerificationWorkflowPhase m_phase = VerificationWorkflowPhase::Idle;
    CommandResult m_beforeGateResult;
    CommandResult m_beforeCommandResult;
    CommandResult m_afterGateResult;
    CommandResult m_afterCommandResult;
    bool m_beforeCommandExecuted = false;
    bool m_afterCommandExecuted = false;
    bool m_patchApplied = false;
    QString m_finalStatus;
    QString m_finalNote;
};
} // namespace occtdebug
