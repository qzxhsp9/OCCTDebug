#include "core/verify/VerificationWorkflow.h"

#include <QCoreApplication>
#include <QDebug>
#include <QTextStream>

#include <iostream>

namespace
{
occtdebug::CommandResult result(int exitCode)
{
    occtdebug::CommandResult out;
    out.exitCode = exitCode;
    out.exitStatus = QProcess::NormalExit;
    out.stdoutText = exitCode == 0 ? QStringLiteral("ok") : QStringLiteral("failed");
    return out;
}

bool expect(bool condition, const char* message)
{
    if (!condition)
    {
        qWarning().noquote() << message;
    }
    return condition;
}
} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    {
        occtdebug::VerificationWorkflow workflow;
        auto decision = workflow.begin();
        if (!expect(decision.action == occtdebug::VerificationWorkflowAction::StartBeforeGate, "begin should start before gate"))
        {
            return 1;
        }

        decision = workflow.onGateFinished(result(0), false);
        if (!expect(decision.persistPhase && decision.phase == QStringLiteral("before"), "before gate should persist without configured testgrid")
            || !expect(decision.action == occtdebug::VerificationWorkflowAction::StartPatchApply, "before gate should start patch apply"))
        {
            return 2;
        }

        decision = workflow.onPatchFinished(result(0));
        if (!expect(workflow.patchApplied(), "patch should be marked applied")
            || !expect(decision.action == occtdebug::VerificationWorkflowAction::StartAfterGate, "patch apply should start after gate"))
        {
            return 3;
        }

        decision = workflow.onGateFinished(result(0), false);
        if (!expect(decision.persistPhase && decision.phase == QStringLiteral("after"), "after gate should persist")
            || !expect(decision.action == occtdebug::VerificationWorkflowAction::StartPatchUndo, "after gate should start cleanup undo"))
        {
            return 4;
        }

        decision = workflow.onPatchFinished(result(0));
        if (!expect(decision.action == occtdebug::VerificationWorkflowAction::Finalize, "cleanup undo should finalize")
            || !expect(decision.finalStatus == QStringLiteral("completed"), "successful workflow should complete")
            || !expect(!workflow.patchApplied(), "patch should be marked cleaned up"))
        {
            return 5;
        }
    }

    {
        occtdebug::VerificationWorkflow workflow;
        workflow.begin();
        auto decision = workflow.onGateFinished(result(0), true);
        if (!expect(!decision.persistPhase, "configured before gate should wait for command result")
            || !expect(decision.action == occtdebug::VerificationWorkflowAction::StartBeforeCommand, "configured before gate should start command"))
        {
            return 6;
        }
        decision = workflow.onCommandFinished(result(0));
        if (!expect(decision.persistPhase && workflow.beforeCommandExecuted(), "before command should persist and mark executed")
            || !expect(decision.action == occtdebug::VerificationWorkflowAction::StartPatchApply, "before command should start patch apply"))
        {
            return 7;
        }
    }

    {
        occtdebug::VerificationWorkflow workflow;
        workflow.begin();
        const auto decision = workflow.onGateFinished(result(1), false);
        if (!expect(decision.persistPhase, "failed before gate should still persist phase evidence")
            || !expect(decision.action == occtdebug::VerificationWorkflowAction::Finalize, "failed before gate should finalize")
            || !expect(decision.finalStatus == QStringLiteral("failed"), "failed before gate should be failed"))
        {
            return 8;
        }
    }

    {
        occtdebug::VerificationWorkflow workflow;
        workflow.begin();
        auto decision = workflow.onGateFinished(result(0), false);
        if (!expect(decision.action == occtdebug::VerificationWorkflowAction::StartPatchApply, "before gate should reach patch apply"))
        {
            return 9;
        }
        decision = workflow.onPatchFinished(result(1));
        if (!expect(decision.action == occtdebug::VerificationWorkflowAction::Finalize, "failed patch apply should finalize")
            || !expect(decision.finalStatus == QStringLiteral("blocked"), "failed patch apply should block workflow"))
        {
            return 10;
        }
    }

    std::cout << "VERIFICATION_WORKFLOW_SMOKE_OK\n";
    return 0;
}
