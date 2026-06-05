#include "core/case/CaseManifest.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QSaveFile>

namespace occtdebug
{
namespace
{
QString stringValue(const QJsonObject& object, const char* key, const QString& fallback = QString())
{
    const QJsonValue value = object.value(QLatin1String(key));
    return value.isString() ? value.toString() : fallback;
}

int intValue(const QJsonObject& object, const char* key, int fallback = 0)
{
    const QJsonValue value = object.value(QLatin1String(key));
    return value.isDouble() ? value.toInt() : fallback;
}

double doubleValue(const QJsonObject& object, const char* key, double fallback = 0.0)
{
    const QJsonValue value = object.value(QLatin1String(key));
    return value.isDouble() ? value.toDouble() : fallback;
}

QJsonObject objectValue(const QJsonObject& object, const char* key)
{
    const QJsonValue value = object.value(QLatin1String(key));
    return value.isObject() ? value.toObject() : QJsonObject {};
}

QJsonArray arrayValue(const QJsonObject& object, const char* key)
{
    const QJsonValue value = object.value(QLatin1String(key));
    return value.isArray() ? value.toArray() : QJsonArray {};
}

QVector<CaseSummary> readCaseSummaries(const QJsonArray& array)
{
    QVector<CaseSummary> out;
    out.reserve(array.size());
    for (const QJsonValue& value : array)
    {
        if (!value.isObject())
        {
            continue;
        }
        const QJsonObject object = value.toObject();
        out.push_back({
            stringValue(object, "id"),
            stringValue(object, "status"),
            stringValue(object, "title"),
            stringValue(object, "created_at"),
        });
    }
    return out;
}

QVector<WorkflowStep> readWorkflowSteps(const QJsonArray& array)
{
    QVector<WorkflowStep> out;
    out.reserve(array.size());
    int index = 1;
    for (const QJsonValue& value : array)
    {
        if (!value.isObject())
        {
            continue;
        }
        const QJsonObject object = value.toObject();
        out.push_back({
            stringValue(object, "id", QStringLiteral("step_%1").arg(index)),
            stringValue(object, "marker"),
            stringValue(object, "title"),
            stringValue(object, "state"),
            stringValue(object, "note"),
        });
        ++index;
    }
    return out;
}

WorkflowState readWorkflowState(const QJsonObject& object, const QVector<WorkflowStep>& fallbackSteps)
{
    WorkflowState out;
    out.activeStepId = stringValue(object, "active_step_id");
    out.steps = readWorkflowSteps(arrayValue(object, "steps"));
    if (out.steps.isEmpty())
    {
        out.steps = fallbackSteps;
    }
    if (out.activeStepId.isEmpty())
    {
        for (const WorkflowStep& step : out.steps)
        {
            if (step.marker.contains(QStringLiteral("active"), Qt::CaseInsensitive)
                || step.marker.contains(QStringLiteral("▶"))
                || step.state.contains(QStringLiteral("progress"), Qt::CaseInsensitive)
                || step.state.contains(QString::fromUtf8("进行中")))
            {
                out.activeStepId = step.id;
                break;
            }
        }
    }
    if (out.activeStepId.isEmpty() && !out.steps.isEmpty())
    {
        out.activeStepId = out.steps.first().id;
    }
    return out;
}

WorkspaceLayout readWorkspaceLayout(const QJsonObject& object)
{
    WorkspaceLayout out;
    out.activeCenterTab = stringValue(object, "active_center_tab", QStringLiteral("source"));
    out.activeBottomTab = stringValue(object, "active_bottom_tab", QStringLiteral("draw"));
    out.leftWidth = intValue(object, "left_width", out.leftWidth);
    out.centerWidth = intValue(object, "center_width", out.centerWidth);
    out.rightWidth = intValue(object, "right_width", out.rightWidth);
    out.bottomHeight = intValue(object, "bottom_height", out.bottomHeight);
    return out;
}

QVector<LabelValue> readLabelValues(const QJsonArray& array)
{
    QVector<LabelValue> out;
    out.reserve(array.size());
    for (const QJsonValue& value : array)
    {
        if (!value.isObject())
        {
            continue;
        }
        const QJsonObject object = value.toObject();
        out.push_back({
            stringValue(object, "label"),
            stringValue(object, "value"),
        });
    }
    return out;
}

QVector<GeometryCheck> readGeometryChecks(const QJsonArray& array)
{
    QVector<GeometryCheck> out;
    out.reserve(array.size());
    for (const QJsonValue& value : array)
    {
        if (!value.isObject())
        {
            continue;
        }
        const QJsonObject object = value.toObject();
        out.push_back({
            stringValue(object, "name"),
            stringValue(object, "status"),
            stringValue(object, "note"),
        });
    }
    return out;
}

QVector<InputFileRecord> readInputFiles(const QJsonArray& array)
{
    QVector<InputFileRecord> out;
    out.reserve(array.size());
    for (const QJsonValue& value : array)
    {
        if (!value.isObject())
        {
            continue;
        }
        const QJsonObject object = value.toObject();
        out.push_back({
            stringValue(object, "path"),
            stringValue(object, "original_name"),
            stringValue(object, "sha256"),
            static_cast<qint64>(object.value(QStringLiteral("bytes")).toDouble(0.0)),
            stringValue(object, "imported_at"),
        });
    }
    return out;
}

QVector<SimilarCase> readSimilarCases(const QJsonArray& array)
{
    QVector<SimilarCase> out;
    out.reserve(array.size());
    for (const QJsonValue& value : array)
    {
        if (!value.isObject())
        {
            continue;
        }
        const QJsonObject object = value.toObject();
        out.push_back({
            stringValue(object, "id"),
            stringValue(object, "title"),
            stringValue(object, "score"),
        });
    }
    return out;
}

QVector<EvidenceRecord> readEvidenceRecords(const QJsonArray& array)
{
    QVector<EvidenceRecord> out;
    out.reserve(array.size());
    for (const QJsonValue& value : array)
    {
        if (!value.isObject())
        {
            continue;
        }
        const QJsonObject object = value.toObject();
        out.push_back({
            stringValue(object, "type"),
            stringValue(object, "title"),
            stringValue(object, "summary"),
            stringValue(object, "link"),
            stringValue(object, "source_file"),
            intValue(object, "source_line"),
            stringValue(object, "log_file"),
            intValue(object, "log_line"),
            stringValue(object, "stack_frame"),
            stringValue(object, "geometry_object"),
        });
    }
    return out;
}

QVector<TestgridRow> readTestgridRows(const QJsonArray& array)
{
    QVector<TestgridRow> out;
    out.reserve(array.size());
    for (const QJsonValue& value : array)
    {
        if (!value.isObject())
        {
            continue;
        }
        const QJsonObject object = value.toObject();
        out.push_back({
            stringValue(object, "module"),
            stringValue(object, "run"),
            stringValue(object, "pass"),
            stringValue(object, "fail"),
            stringValue(object, "pass_rate"),
        });
    }
    return out;
}

QVector<QString> readStringArray(const QJsonArray& array)
{
    QVector<QString> out;
    out.reserve(array.size());
    for (const QJsonValue& value : array)
    {
        const QString text = value.toString();
        if (!text.isEmpty())
        {
            out.push_back(text);
        }
    }
    return out;
}

VerificationPlan readVerificationPlan(const QJsonObject& object)
{
    return {
        stringValue(object, "testgrid_root"),
        stringValue(object, "testgrid_executable"),
        stringValue(object, "testgrid_arguments"),
        stringValue(object, "testgrid_group"),
        stringValue(object, "testgrid_grid"),
        stringValue(object, "testgrid_case"),
        stringValue(object, "testdiff_executable"),
        stringValue(object, "testdiff_arguments"),
        stringValue(object, "testdiff_output_root"),
    };
}

TestdiffGenerationConfig readTestdiffGenerationConfig(const QJsonObject& object)
{
    const QJsonObject tolerances = objectValue(object, "tolerances");
    const QJsonObject thresholds = objectValue(object, "thresholds");
    const QJsonObject imageTolerance = objectValue(tolerances, "image_pixel_diff");
    const QJsonObject propertyTolerance = objectValue(tolerances, "property_structural_diff");
    const QJsonObject performanceThreshold = objectValue(thresholds, "performance_trend_diff");
    const QJsonObject failureReport = objectValue(object, "failure_report");
    return {
        readStringArray(arrayValue(object, "enabled_generators")),
        imageTolerance.isEmpty()
            ? doubleValue(tolerances, "image_pixel", 0.0)
            : doubleValue(imageTolerance, "pixel_abs", 0.0),
        propertyTolerance.isEmpty()
            ? doubleValue(tolerances, "property_numeric", 0.000001)
            : doubleValue(propertyTolerance, "numeric_abs", 0.000001),
        performanceThreshold.isEmpty()
            ? doubleValue(thresholds, "performance_regression_percent", 5.0)
            : doubleValue(performanceThreshold, "regression_percent", 5.0),
        stringValue(failureReport, "path", QStringLiteral("artifacts/testdiff/generated/failure_report.json")),
    };
}

ReproStatus readReproStatus(const QJsonObject& object)
{
    return {
        stringValue(object, "overall"),
        stringValue(object, "draw"),
        stringValue(object, "cpp"),
        stringValue(object, "testgrid"),
        stringValue(object, "updated_at"),
        stringValue(object, "summary"),
    };
}

QJsonArray writeCaseSummaries(const QVector<CaseSummary>& values)
{
    QJsonArray array;
    for (const CaseSummary& value : values)
    {
        array.append(QJsonObject {
            {QStringLiteral("id"), value.id},
            {QStringLiteral("status"), value.status},
            {QStringLiteral("title"), value.title},
            {QStringLiteral("created_at"), value.createdAt},
        });
    }
    return array;
}

QJsonArray writeWorkflowSteps(const QVector<WorkflowStep>& values)
{
    QJsonArray array;
    for (const WorkflowStep& value : values)
    {
        array.append(QJsonObject {
            {QStringLiteral("id"), value.id},
            {QStringLiteral("marker"), value.marker},
            {QStringLiteral("title"), value.title},
            {QStringLiteral("state"), value.state},
            {QStringLiteral("note"), value.note},
        });
    }
    return array;
}

QJsonObject writeWorkflowState(const WorkflowState& value)
{
    return {
        {QStringLiteral("active_step_id"), value.activeStepId},
        {QStringLiteral("steps"), writeWorkflowSteps(value.steps)},
    };
}

QJsonObject writeWorkspaceLayout(const WorkspaceLayout& value)
{
    return {
        {QStringLiteral("active_center_tab"), value.activeCenterTab},
        {QStringLiteral("active_bottom_tab"), value.activeBottomTab},
        {QStringLiteral("left_width"), value.leftWidth},
        {QStringLiteral("center_width"), value.centerWidth},
        {QStringLiteral("right_width"), value.rightWidth},
        {QStringLiteral("bottom_height"), value.bottomHeight},
    };
}

QJsonArray writeLabelValues(const QVector<LabelValue>& values)
{
    QJsonArray array;
    for (const LabelValue& value : values)
    {
        array.append(QJsonObject {
            {QStringLiteral("label"), value.label},
            {QStringLiteral("value"), value.value},
        });
    }
    return array;
}

QJsonArray writeGeometryChecks(const QVector<GeometryCheck>& values)
{
    QJsonArray array;
    for (const GeometryCheck& value : values)
    {
        array.append(QJsonObject {
            {QStringLiteral("name"), value.name},
            {QStringLiteral("status"), value.status},
            {QStringLiteral("note"), value.note},
        });
    }
    return array;
}

QJsonArray writeInputFiles(const QVector<InputFileRecord>& values)
{
    QJsonArray array;
    for (const InputFileRecord& value : values)
    {
        array.append(QJsonObject {
            {QStringLiteral("path"), value.path},
            {QStringLiteral("original_name"), value.originalName},
            {QStringLiteral("sha256"), value.sha256},
            {QStringLiteral("bytes"), static_cast<double>(value.bytes)},
            {QStringLiteral("imported_at"), value.importedAt},
        });
    }
    return array;
}

QJsonArray writeSimilarCases(const QVector<SimilarCase>& values)
{
    QJsonArray array;
    for (const SimilarCase& value : values)
    {
        array.append(QJsonObject {
            {QStringLiteral("id"), value.id},
            {QStringLiteral("title"), value.title},
            {QStringLiteral("score"), value.score},
        });
    }
    return array;
}

QJsonArray writeEvidenceRecords(const QVector<EvidenceRecord>& values)
{
    QJsonArray array;
    for (const EvidenceRecord& value : values)
    {
        QJsonObject object {
            {QStringLiteral("type"), value.type},
            {QStringLiteral("title"), value.title},
            {QStringLiteral("summary"), value.summary},
            {QStringLiteral("link"), value.link},
        };
        if (!value.sourceFile.isEmpty())
        {
            object.insert(QStringLiteral("source_file"), value.sourceFile);
        }
        if (value.sourceLine > 0)
        {
            object.insert(QStringLiteral("source_line"), value.sourceLine);
        }
        if (!value.logFile.isEmpty())
        {
            object.insert(QStringLiteral("log_file"), value.logFile);
        }
        if (value.logLine > 0)
        {
            object.insert(QStringLiteral("log_line"), value.logLine);
        }
        if (!value.stackFrame.isEmpty())
        {
            object.insert(QStringLiteral("stack_frame"), value.stackFrame);
        }
        if (!value.geometryObject.isEmpty())
        {
            object.insert(QStringLiteral("geometry_object"), value.geometryObject);
        }
        array.append(object);
    }
    return array;
}

QJsonArray writeTestgridRows(const QVector<TestgridRow>& values)
{
    QJsonArray array;
    for (const TestgridRow& value : values)
    {
        array.append(QJsonObject {
            {QStringLiteral("module"), value.module},
            {QStringLiteral("run"), value.runCount},
            {QStringLiteral("pass"), value.passCount},
            {QStringLiteral("fail"), value.failCount},
            {QStringLiteral("pass_rate"), value.passRate},
        });
    }
    return array;
}

QJsonArray writeStringArray(const QVector<QString>& values)
{
    QJsonArray array;
    for (const QString& value : values)
    {
        if (!value.isEmpty())
        {
            array.append(value);
        }
    }
    return array;
}

QJsonObject writeVerificationPlan(const VerificationPlan& value)
{
    return {
        {QStringLiteral("testgrid_root"), value.testgridRoot},
        {QStringLiteral("testgrid_executable"), value.testgridExecutable},
        {QStringLiteral("testgrid_arguments"), value.testgridArguments},
        {QStringLiteral("testgrid_group"), value.testgridGroup},
        {QStringLiteral("testgrid_grid"), value.testgridGrid},
        {QStringLiteral("testgrid_case"), value.testgridCase},
        {QStringLiteral("testdiff_executable"), value.testdiffExecutable},
        {QStringLiteral("testdiff_arguments"), value.testdiffArguments},
        {QStringLiteral("testdiff_output_root"), value.testdiffOutputRoot},
    };
}

QJsonObject writeTestdiffGenerationConfig(const TestdiffGenerationConfig& value)
{
    const QString failureReportPath = value.failureReportPath.isEmpty()
        ? QStringLiteral("artifacts/testdiff/generated/failure_report.json")
        : value.failureReportPath;
    return {
        {QStringLiteral("enabled_generators"), writeStringArray(value.enabledGenerators)},
        {QStringLiteral("tolerances"), QJsonObject {
             {QStringLiteral("image_pixel_diff"), QJsonObject {
                  {QStringLiteral("pixel_abs"), value.imagePixelTolerance},
                  {QStringLiteral("max_changed_ratio"), 0.0},
              }},
             {QStringLiteral("property_structural_diff"), QJsonObject {
                  {QStringLiteral("numeric_abs"), value.propertyNumericTolerance},
                  {QStringLiteral("numeric_rel"), value.propertyNumericTolerance},
              }},
         }},
        {QStringLiteral("thresholds"), QJsonObject {
             {QStringLiteral("performance_trend_diff"), QJsonObject {
                  {QStringLiteral("regression_percent"), value.performanceRegressionPercent},
                  {QStringLiteral("min_sample_count"), 1},
              }},
         }},
        {QStringLiteral("failure_report"), QJsonObject {
             {QStringLiteral("path"), failureReportPath},
         }},
    };
}

QJsonObject writeReproStatus(const ReproStatus& value)
{
    return {
        {QStringLiteral("overall"), value.overall},
        {QStringLiteral("draw"), value.draw},
        {QStringLiteral("cpp"), value.cpp},
        {QStringLiteral("testgrid"), value.testgrid},
        {QStringLiteral("updated_at"), value.updatedAt},
        {QStringLiteral("summary"), value.summary},
    };
}
} // namespace

std::optional<CaseManifest> CaseManifest::fromJson(const QJsonObject& object, QString* error)
{
    CaseManifest manifest;
    manifest.caseId = stringValue(object, "case_id");
    manifest.title = stringValue(object, "title");
    manifest.status = stringValue(object, "status");
    manifest.createdAt = stringValue(object, "created_at");
    manifest.occtVersion = stringValue(object, "occt_version");
    manifest.toolchain = stringValue(object, "toolchain");
    manifest.platform = stringValue(object, "platform");

    if (manifest.caseId.isEmpty())
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("case_id is required");
        }
        return std::nullopt;
    }

    manifest.caseList = readCaseSummaries(arrayValue(object, "case_list"));
    manifest.workflowSteps = readWorkflowSteps(arrayValue(object, "workflow"));
    manifest.workflowState = readWorkflowState(objectValue(object, "workflow_state"), manifest.workflowSteps);
    if (manifest.workflowSteps.isEmpty())
    {
        manifest.workflowSteps = manifest.workflowState.steps;
    }
    if (manifest.workflowState.steps.isEmpty())
    {
        manifest.workflowState.steps = manifest.workflowSteps;
    }
    manifest.workspaceLayout = readWorkspaceLayout(objectValue(object, "workspace_layout"));
    manifest.keyInputs = readLabelValues(arrayValue(object, "key_inputs"));
    manifest.inputFiles = readInputFiles(arrayValue(objectValue(object, "input"), "files"));

    const QJsonObject source = objectValue(object, "source");
    manifest.sourceText = stringValue(source, "text");

    const QJsonObject repro = objectValue(object, "repro");
    manifest.reproScript = stringValue(repro, "script");
    manifest.reproStatus = readReproStatus(objectValue(repro, "status"));

    const QJsonObject geometry = objectValue(object, "geometry");
    manifest.geometrySummary = stringValue(geometry, "summary");
    manifest.geometryChecks = readGeometryChecks(arrayValue(geometry, "checks"));

    const QJsonObject evidence = objectValue(object, "evidence");
    manifest.evidenceSummary = stringValue(evidence, "summary");
    manifest.evidenceItems = readEvidenceRecords(arrayValue(evidence, "items"));

    const QJsonObject diff = objectValue(object, "diff");
    manifest.diffSummary = stringValue(diff, "summary");

    const QJsonObject environment = objectValue(object, "environment");
    manifest.environmentSummary = stringValue(environment, "summary");

    const QJsonObject diagnosis = objectValue(object, "diagnosis");
    manifest.diagnosis = stringValue(diagnosis, "summary");
    manifest.diagnosisConfidence = intValue(diagnosis, "confidence");

    const QJsonObject patch = objectValue(object, "patch");
    manifest.patchDiff = stringValue(patch, "diff");
    manifest.patchReviewStatus = stringValue(patch, "review_status");
    manifest.patchWorktreeRoot = stringValue(patch, "worktree_root");
    manifest.patchApplyStatus = stringValue(patch, "apply_status");
    manifest.patchApplyLog = stringValue(patch, "apply_log");
    manifest.patchSignoffStatus = stringValue(patch, "signoff_status");
    manifest.patchSignoffNote = stringValue(patch, "signoff_note");
    manifest.patchReviewItems = readLabelValues(arrayValue(patch, "review_items"));

    const QJsonObject verification = objectValue(object, "verification");
    manifest.verificationItems = readLabelValues(arrayValue(verification, "items"));
    manifest.verificationPlan = readVerificationPlan(objectValue(verification, "testgrid_plan"));
    manifest.testdiffGenerationConfig = readTestdiffGenerationConfig(objectValue(verification, "testdiff_generation"));

    manifest.similarCases = readSimilarCases(arrayValue(object, "similar_cases"));

    const QJsonObject consoles = objectValue(object, "consoles");
    manifest.drawConsoleText = stringValue(consoles, "draw");
    manifest.cmakeConsoleText = stringValue(consoles, "cmake");
    manifest.testgridRows = readTestgridRows(arrayValue(consoles, "testgrid"));

    return manifest;
}

std::optional<CaseManifest> CaseManifest::loadFromFile(const QString& filePath, QString* error)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("cannot open %1: %2").arg(filePath, file.errorString());
        }
        return std::nullopt;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("invalid case JSON %1: %2").arg(filePath, parseError.errorString());
        }
        return std::nullopt;
    }

    return fromJson(document.object(), error);
}

bool CaseManifest::saveToFile(const QString& filePath, QString* error) const
{
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("cannot write %1: %2").arg(filePath, file.errorString());
        }
        return false;
    }

    const QJsonDocument document(toJson());
    file.write(document.toJson(QJsonDocument::Indented));
    if (!file.commit())
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("cannot commit %1: %2").arg(filePath, file.errorString());
        }
        return false;
    }
    return true;
}

QJsonObject CaseManifest::toJson() const
{
    return QJsonObject {
        {QStringLiteral("case_id"), caseId},
        {QStringLiteral("title"), title},
        {QStringLiteral("status"), status},
        {QStringLiteral("created_at"), createdAt},
        {QStringLiteral("occt_version"), occtVersion},
        {QStringLiteral("toolchain"), toolchain},
        {QStringLiteral("platform"), platform},
        {QStringLiteral("case_list"), writeCaseSummaries(caseList)},
        {QStringLiteral("workflow"), writeWorkflowSteps(workflowSteps)},
        {QStringLiteral("workflow_state"), writeWorkflowState(workflowState)},
        {QStringLiteral("workspace_layout"), writeWorkspaceLayout(workspaceLayout)},
        {QStringLiteral("key_inputs"), writeLabelValues(keyInputs)},
        {QStringLiteral("input"), QJsonObject {{QStringLiteral("files"), writeInputFiles(inputFiles)}}},
        {QStringLiteral("source"), QJsonObject {{QStringLiteral("text"), sourceText}}},
        {QStringLiteral("repro"), QJsonObject {
             {QStringLiteral("script"), reproScript},
             {QStringLiteral("status"), writeReproStatus(reproStatus)},
         }},
        {QStringLiteral("geometry"), QJsonObject {
             {QStringLiteral("summary"), geometrySummary},
             {QStringLiteral("checks"), writeGeometryChecks(geometryChecks)},
         }},
        {QStringLiteral("evidence"), QJsonObject {
             {QStringLiteral("summary"), evidenceSummary},
             {QStringLiteral("items"), writeEvidenceRecords(evidenceItems)},
         }},
        {QStringLiteral("diff"), QJsonObject {{QStringLiteral("summary"), diffSummary}}},
        {QStringLiteral("environment"), QJsonObject {{QStringLiteral("summary"), environmentSummary}}},
        {QStringLiteral("diagnosis"), QJsonObject {
             {QStringLiteral("summary"), diagnosis},
             {QStringLiteral("confidence"), diagnosisConfidence},
         }},
        {QStringLiteral("patch"), QJsonObject {
             {QStringLiteral("diff"), patchDiff},
             {QStringLiteral("review_status"), patchReviewStatus},
             {QStringLiteral("worktree_root"), patchWorktreeRoot},
             {QStringLiteral("apply_status"), patchApplyStatus},
             {QStringLiteral("apply_log"), patchApplyLog},
             {QStringLiteral("signoff_status"), patchSignoffStatus},
             {QStringLiteral("signoff_note"), patchSignoffNote},
             {QStringLiteral("review_items"), writeLabelValues(patchReviewItems)},
         }},
        {QStringLiteral("verification"), QJsonObject {
             {QStringLiteral("items"), writeLabelValues(verificationItems)},
             {QStringLiteral("testgrid_plan"), writeVerificationPlan(verificationPlan)},
             {QStringLiteral("testdiff_generation"), writeTestdiffGenerationConfig(testdiffGenerationConfig)},
         }},
        {QStringLiteral("similar_cases"), writeSimilarCases(similarCases)},
        {QStringLiteral("consoles"), QJsonObject {
             {QStringLiteral("draw"), drawConsoleText},
             {QStringLiteral("cmake"), cmakeConsoleText},
             {QStringLiteral("testgrid"), writeTestgridRows(testgridRows)},
         }},
    };
}
} // namespace occtdebug
