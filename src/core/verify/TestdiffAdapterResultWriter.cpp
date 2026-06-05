#include "core/verify/TestdiffAdapterResultWriter.h"

#include "core/verify/TestgridArtifactService.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>

namespace occtdebug
{
namespace
{
bool commandSucceeded(const CommandResult& result)
{
    return !result.canceled
        && !result.timedOut
        && result.exitCode == 0;
}

QString normalizedPath(QString value)
{
    return value.replace(QLatin1Char('\\'), QLatin1Char('/'));
}

QString caseRelativeOrFileName(const QString& workspaceRoot, const QString& path)
{
    if (path.isEmpty())
    {
        return QString();
    }
    const QFileInfo info(path);
    if (!info.exists())
    {
        return info.fileName().isEmpty() ? normalizedPath(path) : info.fileName();
    }
    const QString relative = QDir(workspaceRoot).relativeFilePath(info.absoluteFilePath());
    if (!relative.startsWith(QStringLiteral("..")))
    {
        return normalizedPath(relative);
    }
    return info.fileName();
}

QJsonArray testdiffEntriesToJson(const TestdiffSummary& summary)
{
    QJsonArray out;
    for (const TestdiffEntry& entry : summary.entries)
    {
        out.append(QJsonObject {
            {QStringLiteral("name"), entry.name},
            {QStringLiteral("status"), entry.status},
            {QStringLiteral("metric"), entry.metric},
            {QStringLiteral("note"), entry.note},
        });
    }
    return out;
}

QJsonArray testgridRowsToJson(const QVector<TestgridRow>& rows)
{
    QJsonArray out;
    for (const TestgridRow& row : rows)
    {
        out.append(QJsonObject {
            {QStringLiteral("module"), row.module},
            {QStringLiteral("run"), row.runCount},
            {QStringLiteral("pass"), row.passCount},
            {QStringLiteral("fail"), row.failCount},
            {QStringLiteral("rate"), row.passRate},
        });
    }
    return out;
}

QJsonObject readJsonFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return {};
    }
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    return document.isObject() ? document.object() : QJsonObject();
}

QJsonObject planToJson(const VerificationPlan& plan, const QString& workspaceRoot, const QString& outputRoot)
{
    return {
        {QStringLiteral("testgrid_root"), plan.testgridRoot},
        {QStringLiteral("testgrid_executable"), plan.testgridExecutable},
        {QStringLiteral("testgrid_arguments"), plan.testgridArguments},
        {QStringLiteral("testgrid_group"), plan.testgridGroup},
        {QStringLiteral("testgrid_grid"), plan.testgridGrid},
        {QStringLiteral("testgrid_case"), plan.testgridCase},
        {QStringLiteral("testdiff_executable"), plan.testdiffExecutable},
        {QStringLiteral("testdiff_arguments"), plan.testdiffArguments},
        {QStringLiteral("testdiff_output_root"), caseRelativeOrFileName(workspaceRoot, outputRoot)},
    };
}
} // namespace

bool TestdiffAdapterResultWriter::writeResult(const TestdiffAdapterResultWriterInput& input,
                                              TestdiffAdapterResultWriterResult* result,
                                              QString* error)
{
    TestdiffAdapterResultWriterResult local;
    if (!TestgridArtifactService::ensureWorkspaceDirectories(input.workspaceRoot, error)
        || !TestgridArtifactService::writeCommandLogs(
            input.workspaceRoot,
            QStringLiteral("testdiff_runner.stdout.log"),
            QStringLiteral("testdiff_runner.stderr.log"),
            input.commandResult,
            error))
    {
        return false;
    }

    const QString commandText = input.commandResult.stdoutText + QLatin1Char('\n') + input.commandResult.stderrText;
    const QString summaryPath = TestgridArtifactService::verificationPath(input.workspaceRoot, QStringLiteral("testdiff_summary.txt"));
    if (!TestgridArtifactService::writeTextArtifact(summaryPath, commandText, error))
    {
        return false;
    }

    local.testdiff = VerificationResultParser::parseTestdiffText(commandText);
    local.importResult = TestdiffRunnerAdapter::importOutput(
        input.workspaceRoot,
        input.outputRoot,
        summaryPath,
        QStringLiteral("logs/testdiff_runner.stdout.log"),
        QStringLiteral("logs/testdiff_runner.stderr.log"),
        local.testdiff.entries.size(),
        local.testdiff.changedCount,
        local.testdiff.failedCount);

    const bool commandOk = commandSucceeded(input.commandResult);
    local.adapterStatus = local.importResult.success
        ? QStringLiteral("imported %1 files").arg(local.importResult.copiedFiles)
        : QStringLiteral("import failed: %1").arg(local.importResult.error);
    local.status = commandOk && local.importResult.success ? QStringLiteral("passed") : QStringLiteral("failed");
    local.diffSummary = local.importResult.success
        ? QStringLiteral("%1\n%2").arg(local.testdiff.entries.isEmpty() ? QStringLiteral("testdiff entries not parsed") : local.testdiff.summaryText(),
                                       local.adapterStatus)
        : local.adapterStatus;

    local.adapterResult = {
        {QStringLiteral("schema_version"), 1},
        {QStringLiteral("source"), QStringLiteral("testdiff_runner_adapter")},
        {QStringLiteral("status"), local.status},
        {QStringLiteral("note"), input.note},
        {QStringLiteral("command_exit_code"), input.commandResult.exitCode},
        {QStringLiteral("command_elapsed_ms"), static_cast<double>(input.commandResult.elapsedMs)},
        {QStringLiteral("command_canceled"), input.commandResult.canceled},
        {QStringLiteral("command_timed_out"), input.commandResult.timedOut},
        {QStringLiteral("command_timeout_ms"), input.commandResult.timeoutMs},
        {QStringLiteral("command_stdout"), QStringLiteral("logs/testdiff_runner.stdout.log")},
        {QStringLiteral("command_stderr"), QStringLiteral("logs/testdiff_runner.stderr.log")},
        {QStringLiteral("summary"), QStringLiteral("verification/testdiff_summary.txt")},
        {QStringLiteral("output_root"), caseRelativeOrFileName(input.workspaceRoot, input.outputRoot)},
        {QStringLiteral("adapter_status"), local.adapterStatus},
    };
    local.adapterResult.insert(QStringLiteral("testdiff_entries"), testdiffEntriesToJson(local.testdiff));
    local.adapterResult.insert(QStringLiteral("adapter_manifest"), local.importResult.manifest);

    local.adapterResultPath = TestgridArtifactService::artifactPath(input.workspaceRoot, QStringLiteral("testdiff_adapter_result.json"));
    if (!TestgridArtifactService::writeJsonArtifact(local.adapterResultPath, local.adapterResult, error))
    {
        return false;
    }

    local.adapterManifestPath = TestgridArtifactService::artifactPath(input.workspaceRoot, QStringLiteral("testdiff_adapter_manifest.json"));
    if (!TestgridArtifactService::writeJsonArtifact(local.adapterManifestPath, local.importResult.manifest, error))
    {
        return false;
    }

    local.testgridResultPath = TestgridArtifactService::artifactPath(input.workspaceRoot, QStringLiteral("testgrid_result.json"));
    local.testgridResult = readJsonFile(local.testgridResultPath);
    if (local.testgridResult.isEmpty())
    {
        local.testgridResult = QJsonObject {
            {QStringLiteral("schema_version"), 1},
            {QStringLiteral("mode"), QStringLiteral("testdiff_adapter")},
            {QStringLiteral("testgrid_rows"), testgridRowsToJson(input.existingRows)},
        };
    }
    local.testgridResult.insert(QStringLiteral("testdiff_summary"), QStringLiteral("verification/testdiff_summary.txt"));
    local.testgridResult.insert(QStringLiteral("testdiff_entries"), testdiffEntriesToJson(local.testdiff));
    local.testgridResult.insert(QStringLiteral("testdiff_artifacts"), local.importResult.manifest);
    local.testgridResult.insert(QStringLiteral("testdiff_adapter_result"), QStringLiteral("artifacts/testdiff_adapter_result.json"));
    local.testgridResult.insert(QStringLiteral("testdiff_adapter_status"), local.status);
    local.testgridResult.insert(QStringLiteral("testgrid_plan"), planToJson(input.plan, input.workspaceRoot, input.outputRoot));
    if (!TestgridArtifactService::writeJsonArtifact(local.testgridResultPath, local.testgridResult, error))
    {
        return false;
    }

    if (result != nullptr)
    {
        *result = local;
    }
    return true;
}
} // namespace occtdebug
