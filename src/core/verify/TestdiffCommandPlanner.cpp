#include "core/verify/TestdiffCommandPlanner.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>

namespace occtdebug
{
TestdiffCommandPlan TestdiffCommandPlanner::build(const TestdiffCommandPlanInput& input)
{
    TestdiffCommandPlan out;
    const VerificationPlan& plan = input.verificationPlan;
    const QString executable = plan.testdiffExecutable.trimmed();
    if (executable.isEmpty())
    {
        out.error = QStringLiteral("testdiff executable is not configured");
        return out;
    }

    QString outputRoot = plan.testdiffOutputRoot.trimmed();
    if (outputRoot.isEmpty())
    {
        outputRoot = QDir(input.artifactDirectory).filePath(QStringLiteral("testdiff_runner_output"));
    }
    else if (!QFileInfo(outputRoot).isAbsolute())
    {
        outputRoot = QDir(input.workspaceRoot).filePath(outputRoot);
    }
    outputRoot = QDir::cleanPath(outputRoot);

    QDir outputDir;
    if (!outputDir.mkpath(outputRoot))
    {
        out.error = QStringLiteral("failed to create testdiff output directory: %1").arg(outputRoot);
        return out;
    }

    QString workingDirectory = plan.testgridRoot.trimmed();
    if (workingDirectory.isEmpty())
    {
        workingDirectory = input.sourceRoot;
    }

    QString arguments = plan.testdiffArguments;
    arguments.replace(QStringLiteral("{group}"), plan.testgridGroup);
    arguments.replace(QStringLiteral("{grid}"), plan.testgridGrid);
    arguments.replace(QStringLiteral("{case}"), plan.testgridCase);
    arguments.replace(QStringLiteral("{workspace}"), input.workspaceRoot);
    arguments.replace(QStringLiteral("{verification}"), input.verificationDirectory);
    arguments.replace(QStringLiteral("{artifacts}"), input.artifactDirectory);
    arguments.replace(QStringLiteral("{output}"), outputRoot);

    out.outputRoot = outputRoot;
    out.request.program = executable;
    out.request.workingDirectory = workingDirectory;
    out.request.environment = QProcessEnvironment::systemEnvironment();
    out.request.arguments = arguments.trimmed().isEmpty()
        ? QStringList {}
        : QProcess::splitCommand(arguments);
    out.success = true;
    return out;
}
} // namespace occtdebug
