#pragma once

#include "core/case/CaseManifest.h"

namespace occtdebug
{
struct WorkbenchMockData;

class CaseManifestSync
{
public:
    static void syncMutableFields(CaseManifest& manifest, const WorkbenchMockData& data);
};
} // namespace occtdebug
