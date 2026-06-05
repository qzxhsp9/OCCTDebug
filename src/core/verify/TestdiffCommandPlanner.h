#pragma once

#include "core/case/CaseManifest.h"
#include "core/runner/CommandRunner.h"

#include <QString>

namespace occtdebug
{
struct TestdiffCommandPlanInput
{
    VerificationPlan verificationPlan;
    QString workspaceRoot;
    QString sourceRoot;
    QString verificationDirectory;
    QString artifactDirectory;
};

struct TestdiffCommandPlan
{
    bool success = false;
    QString error;
    QString outputRoot;
    CommandRequest request;
};

class TestdiffCommandPlanner
{
public:
    static TestdiffCommandPlan build(const TestdiffCommandPlanInput& input);
};
} // namespace occtdebug
