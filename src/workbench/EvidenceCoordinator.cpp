#include "workbench/EvidenceCoordinator.h"

#include "workbench/EvidencePanel.h"

namespace occtdebug
{
void EvidenceCoordinator::appendRecord(WorkbenchMockData& data, const EvidenceRecord& evidence)
{
    data.evidenceItems.push_back(evidence);
    data.manifest.evidenceItems.push_back(evidence);
}

void EvidenceCoordinator::appendRecord(WorkbenchMockData& data, EvidencePanel* panel, const EvidenceRecord& evidence)
{
    appendRecord(data, evidence);
    if (panel != nullptr)
    {
        panel->appendRecord(evidence);
    }
}
} // namespace occtdebug
