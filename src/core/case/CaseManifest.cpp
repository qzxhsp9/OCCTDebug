#include "core/case/CaseManifest.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>

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
    for (const QJsonValue& value : array)
    {
        if (!value.isObject())
        {
            continue;
        }
        const QJsonObject object = value.toObject();
        out.push_back({
            stringValue(object, "marker"),
            stringValue(object, "title"),
            stringValue(object, "state"),
        });
    }
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
            {QStringLiteral("marker"), value.marker},
            {QStringLiteral("title"), value.title},
            {QStringLiteral("state"), value.state},
        });
    }
    return array;
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
        array.append(QJsonObject {
            {QStringLiteral("type"), value.type},
            {QStringLiteral("title"), value.title},
            {QStringLiteral("summary"), value.summary},
            {QStringLiteral("link"), value.link},
        });
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
    manifest.keyInputs = readLabelValues(arrayValue(object, "key_inputs"));

    const QJsonObject source = objectValue(object, "source");
    manifest.sourceText = stringValue(source, "text");

    const QJsonObject repro = objectValue(object, "repro");
    manifest.reproScript = stringValue(repro, "script");

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

    const QJsonObject verification = objectValue(object, "verification");
    manifest.verificationItems = readLabelValues(arrayValue(verification, "items"));

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
        {QStringLiteral("key_inputs"), writeLabelValues(keyInputs)},
        {QStringLiteral("source"), QJsonObject {{QStringLiteral("text"), sourceText}}},
        {QStringLiteral("repro"), QJsonObject {{QStringLiteral("script"), reproScript}}},
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
        {QStringLiteral("patch"), QJsonObject {{QStringLiteral("diff"), patchDiff}}},
        {QStringLiteral("verification"), QJsonObject {{QStringLiteral("items"), writeLabelValues(verificationItems)}}},
        {QStringLiteral("similar_cases"), writeSimilarCases(similarCases)},
        {QStringLiteral("consoles"), QJsonObject {
             {QStringLiteral("draw"), drawConsoleText},
             {QStringLiteral("cmake"), cmakeConsoleText},
             {QStringLiteral("testgrid"), writeTestgridRows(testgridRows)},
         }},
    };
}
} // namespace occtdebug
