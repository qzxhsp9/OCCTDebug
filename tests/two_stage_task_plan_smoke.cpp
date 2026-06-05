#include "core/verify/TwoStageTaskPlan.h"

#include <QTextStream>

namespace
{
occtdebug::CommandRequest request(const QString& name)
{
    occtdebug::CommandRequest out;
    out.program = name;
    out.arguments = {QStringLiteral("--run")};
    out.workingDirectory = QStringLiteral("workspace");
    return out;
}

bool expect(bool condition, const char* message)
{
    if (!condition)
    {
        QTextStream(stderr) << message << "\n";
    }
    return condition;
}
} // namespace

int main()
{
    const QVector<occtdebug::CommandTask> tasks = occtdebug::TwoStageTaskPlan::build({
        request(QStringLiteral("before_gate")),
        true,
        request(QStringLiteral("before_command")),
        true,
        true,
        request(QStringLiteral("patch_apply_check")),
        request(QStringLiteral("after_gate")),
        true,
        request(QStringLiteral("after_command")),
        true,
        true,
        request(QStringLiteral("patch_undo_check")),
    });

    if (!expect(tasks.size() == 6, "two-stage task plan should include six tasks")
        || !expect(tasks[0].id == QStringLiteral("two_stage.before.gate"), "before gate id mismatch")
        || !expect(tasks[0].phase == QStringLiteral("before") && tasks[0].subphase == QStringLiteral("gate"), "before gate metadata mismatch")
        || !expect(tasks[1].request.program == QStringLiteral("before_command"), "before command request mismatch")
        || !expect(tasks[2].dryRun && tasks[2].subphase == QStringLiteral("apply_dry_run"), "patch apply dry-run metadata mismatch")
        || !expect(tasks[3].phase == QStringLiteral("after") && tasks[3].subphase == QStringLiteral("gate"), "after gate metadata mismatch")
        || !expect(tasks[4].id == QStringLiteral("two_stage.after.command"), "after command id mismatch")
        || !expect(tasks[5].dryRun && tasks[5].subphase == QStringLiteral("undo_dry_run"), "patch undo dry-run metadata mismatch"))
    {
        return 1;
    }

    const QVector<occtdebug::CommandTask> minimal = occtdebug::TwoStageTaskPlan::build({
        request(QStringLiteral("before_gate")),
        false,
        {},
        false,
        false,
        {},
        request(QStringLiteral("after_gate")),
        false,
        {},
        false,
        false,
        {},
    });
    if (!expect(minimal.size() == 2, "minimal two-stage task plan should only include gates")
        || !expect(minimal[1].id == QStringLiteral("two_stage.after.gate"), "minimal after gate id mismatch"))
    {
        return 2;
    }

    QTextStream(stdout) << "TWO_STAGE_TASK_PLAN_SMOKE_OK\n";
    return 0;
}
