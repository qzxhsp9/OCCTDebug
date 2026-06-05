#pragma once

#include "workbench/TwoStageFinalResultCoordinator.h"
#include "workbench/WorkbenchMockData.h"

class QLabel;
class QTableWidget;

namespace occtdebug
{
class EvidencePanel;
class VerificationPanel;

struct TwoStageFinalResultUiTargets
{
    QLabel* diffLabel = nullptr;
    QTableWidget* testgridTable = nullptr;
    VerificationPanel* verificationPanel = nullptr;
    EvidencePanel* evidencePanel = nullptr;
};

struct TwoStageFinalResultUiActions
{
    bool refreshDiffArtifacts = true;
    bool saveCaseManifest = false;
    bool writeEvidenceBundle = false;
    bool writeVerificationReport = false;
};

class TwoStageFinalResultUiAdapter
{
public:
    static TwoStageFinalResultUiActions apply(const TwoStageFinalResultUiTargets& targets,
                                             const WorkbenchMockData& data,
                                             const TwoStageFinalResultSyncResult& syncResult);
};
} // namespace occtdebug
