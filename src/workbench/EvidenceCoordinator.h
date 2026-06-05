#pragma once

#include "core/case/CaseManifest.h"
#include "workbench/WorkbenchMockData.h"

namespace occtdebug
{
class EvidencePanel;

class EvidenceCoordinator
{
public:
    static void appendRecord(WorkbenchMockData& data, const EvidenceRecord& evidence);
    static void appendRecord(WorkbenchMockData& data, EvidencePanel* panel, const EvidenceRecord& evidence);
};
} // namespace occtdebug
