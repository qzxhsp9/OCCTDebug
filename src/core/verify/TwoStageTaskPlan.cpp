#include "core/verify/TwoStageTaskPlan.h"

namespace occtdebug
{
namespace
{
CommandTask task(const QString& id,
                 const QString& title,
                 const QString& phase,
                 const QString& subphase,
                 bool dryRun,
                 const CommandRequest& request)
{
    CommandTask out;
    out.id = id;
    out.title = title;
    out.phase = phase;
    out.subphase = subphase;
    out.dryRun = dryRun;
    out.request = request;
    return out;
}
} // namespace

QVector<CommandTask> TwoStageTaskPlan::build(const TwoStageTaskPlanInput& input)
{
    QVector<CommandTask> tasks;
    tasks.push_back(task(QStringLiteral("two_stage.before.gate"),
                         QStringLiteral("Two-stage before DRAW gate"),
                         QStringLiteral("before"),
                         QStringLiteral("gate"),
                         false,
                         input.beforeGate));
    if (input.beforeCommandEnabled)
    {
        tasks.push_back(task(QStringLiteral("two_stage.before.command"),
                             QStringLiteral("Two-stage before command"),
                             QStringLiteral("before"),
                             QStringLiteral("command"),
                             false,
                             input.beforeCommand));
    }
    if (input.patchApplyEnabled)
    {
        tasks.push_back(task(input.patchApplyDryRun ? QStringLiteral("two_stage.patch.apply_dry_run") : QStringLiteral("two_stage.patch.apply"),
                             input.patchApplyDryRun ? QStringLiteral("Two-stage patch apply dry-run") : QStringLiteral("Two-stage patch apply"),
                             QStringLiteral("patch"),
                             input.patchApplyDryRun ? QStringLiteral("apply_dry_run") : QStringLiteral("apply"),
                             input.patchApplyDryRun,
                             input.patchApply));
    }
    tasks.push_back(task(QStringLiteral("two_stage.after.gate"),
                         QStringLiteral("Two-stage after DRAW gate"),
                         QStringLiteral("after"),
                         QStringLiteral("gate"),
                         false,
                         input.afterGate));
    if (input.afterCommandEnabled)
    {
        tasks.push_back(task(QStringLiteral("two_stage.after.command"),
                             QStringLiteral("Two-stage after command"),
                             QStringLiteral("after"),
                             QStringLiteral("command"),
                             false,
                             input.afterCommand));
    }
    if (input.patchUndoEnabled)
    {
        tasks.push_back(task(input.patchUndoDryRun ? QStringLiteral("two_stage.patch.undo_dry_run") : QStringLiteral("two_stage.patch.undo"),
                             input.patchUndoDryRun ? QStringLiteral("Two-stage patch undo dry-run") : QStringLiteral("Two-stage patch undo"),
                             QStringLiteral("patch"),
                             input.patchUndoDryRun ? QStringLiteral("undo_dry_run") : QStringLiteral("undo"),
                             input.patchUndoDryRun,
                             input.patchUndo));
    }
    return tasks;
}
} // namespace occtdebug
