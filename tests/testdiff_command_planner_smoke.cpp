#include "core/verify/TestdiffCommandPlanner.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QTemporaryDir>

#include <iostream>

namespace
{
bool expect(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        return false;
    }
    return true;
}
} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir temp;
    if (!expect(temp.isValid(), "temporary workspace creation failed"))
    {
        return 1;
    }

    const QString workspace = temp.path();
    const QString sourceRoot = QDir(workspace).filePath(QStringLiteral("src"));
    const QString verificationDir = QDir(workspace).filePath(QStringLiteral("verification"));
    const QString artifactDir = QDir(workspace).filePath(QStringLiteral("artifacts"));
    QDir().mkpath(sourceRoot);
    QDir().mkpath(verificationDir);
    QDir().mkpath(artifactDir);

    occtdebug::VerificationPlan plan;
    plan.testdiffExecutable = QStringLiteral("testdiff-runner.exe");
    plan.testgridGroup = QStringLiteral("bugs");
    plan.testgridGrid = QStringLiteral("moddata");
    plan.testgridCase = QStringLiteral("bug123");
    plan.testdiffArguments = QStringLiteral("--group {group} --grid {grid} --case {case} --workspace \"{workspace}\" --verification \"{verification}\" --artifacts \"{artifacts}\" --output \"{output}\"");

    const occtdebug::TestdiffCommandPlan built = occtdebug::TestdiffCommandPlanner::build({
        plan,
        workspace,
        sourceRoot,
        verificationDir,
        artifactDir,
    });

    const QString expectedOutput = QDir::cleanPath(QDir(artifactDir).filePath(QStringLiteral("testdiff_runner_output")));
    bool ok = true;
    ok = expect(built.success, "default command plan should succeed") && ok;
    ok = expect(built.outputRoot == expectedOutput, "default output root mismatch") && ok;
    ok = expect(QFileInfo::exists(expectedOutput), "default output directory should be created") && ok;
    ok = expect(built.request.program == plan.testdiffExecutable, "program mismatch") && ok;
    ok = expect(built.request.workingDirectory == sourceRoot, "source root fallback mismatch") && ok;
    ok = expect(built.request.arguments.contains(QStringLiteral("bugs")), "group placeholder mismatch") && ok;
    ok = expect(built.request.arguments.contains(QStringLiteral("moddata")), "grid placeholder mismatch") && ok;
    ok = expect(built.request.arguments.contains(QStringLiteral("bug123")), "case placeholder mismatch") && ok;
    ok = expect(built.request.arguments.contains(workspace), "workspace placeholder mismatch") && ok;
    ok = expect(built.request.arguments.contains(verificationDir), "verification placeholder mismatch") && ok;
    ok = expect(built.request.arguments.contains(artifactDir), "artifact placeholder mismatch") && ok;
    ok = expect(built.request.arguments.contains(expectedOutput), "output placeholder mismatch") && ok;

    plan.testgridRoot = QDir(workspace).filePath(QStringLiteral("testgrid-root"));
    plan.testdiffOutputRoot = QStringLiteral("verification/custom_testdiff_output");
    const occtdebug::TestdiffCommandPlan relativeOutput = occtdebug::TestdiffCommandPlanner::build({
        plan,
        workspace,
        sourceRoot,
        verificationDir,
        artifactDir,
    });
    const QString expectedRelativeOutput = QDir::cleanPath(QDir(workspace).filePath(plan.testdiffOutputRoot));
    ok = expect(relativeOutput.success, "relative output command plan should succeed") && ok;
    ok = expect(relativeOutput.outputRoot == expectedRelativeOutput, "relative output root mismatch") && ok;
    ok = expect(relativeOutput.request.workingDirectory == plan.testgridRoot, "configured working directory mismatch") && ok;
    ok = expect(QFileInfo::exists(expectedRelativeOutput), "relative output directory should be created") && ok;

    plan.testdiffExecutable.clear();
    const occtdebug::TestdiffCommandPlan missingExecutable = occtdebug::TestdiffCommandPlanner::build({
        plan,
        workspace,
        sourceRoot,
        verificationDir,
        artifactDir,
    });
    ok = expect(!missingExecutable.success, "missing executable should fail") && ok;
    ok = expect(missingExecutable.error.contains(QStringLiteral("not configured")), "missing executable error mismatch") && ok;

    if (ok)
    {
        std::cout << "TESTDIFF_COMMAND_PLANNER_SMOKE_OK\n";
    }
    return ok ? 0 : 1;
}
