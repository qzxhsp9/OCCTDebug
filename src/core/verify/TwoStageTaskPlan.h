#pragma once

#include "core/runner/CommandTaskQueue.h"

namespace occtdebug
{
struct TwoStageTaskPlanInput
{
    CommandRequest beforeGate;
    bool beforeCommandEnabled = false;
    CommandRequest beforeCommand;
    bool patchApplyEnabled = false;
    bool patchApplyDryRun = false;
    CommandRequest patchApply;
    CommandRequest afterGate;
    bool afterCommandEnabled = false;
    CommandRequest afterCommand;
    bool patchUndoEnabled = false;
    bool patchUndoDryRun = false;
    CommandRequest patchUndo;
};

class TwoStageTaskPlan
{
public:
    static QVector<CommandTask> build(const TwoStageTaskPlanInput& input);
};
} // namespace occtdebug
