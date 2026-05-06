#include "io/SessionSerializer.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>

namespace
{
QString severityToString(DiagnosticSeverity s)
{
    switch (s)
    {
    case DiagnosticSeverity::Critical:
        return QStringLiteral("Critical");
    case DiagnosticSeverity::Error:
        return QStringLiteral("Error");
    case DiagnosticSeverity::Warning:
        return QStringLiteral("Warning");
    case DiagnosticSeverity::Info:
    default:
        return QStringLiteral("Info");
    }
}

bool severityFromString(const QString& str, DiagnosticSeverity* out)
{
    if (str == QLatin1String("Critical"))
    {
        *out = DiagnosticSeverity::Critical;
        return true;
    }
    if (str == QLatin1String("Error"))
    {
        *out = DiagnosticSeverity::Error;
        return true;
    }
    if (str == QLatin1String("Warning"))
    {
        *out = DiagnosticSeverity::Warning;
        return true;
    }
    if (str == QLatin1String("Info"))
    {
        *out = DiagnosticSeverity::Info;
        return true;
    }
    *out = DiagnosticSeverity::Info;
    return false;
}

QString categoryToString(ProblemCategory c)
{
    switch (c)
    {
    case ProblemCategory::Boolean:
        return QStringLiteral("Boolean");
    case ProblemCategory::Projection:
        return QStringLiteral("Projection");
    case ProblemCategory::Classification:
        return QStringLiteral("Classification");
    case ProblemCategory::Topology:
        return QStringLiteral("Topology");
    case ProblemCategory::Tolerance:
        return QStringLiteral("Tolerance");
    case ProblemCategory::Meshing:
        return QStringLiteral("Meshing");
    case ProblemCategory::HLR:
        return QStringLiteral("HLR");
    case ProblemCategory::Performance:
        return QStringLiteral("Performance");
    case ProblemCategory::Crash:
        return QStringLiteral("Crash");
    case ProblemCategory::Unknown:
    default:
        return QStringLiteral("Unknown");
    }
}

ProblemCategory categoryFromString(const QString& str)
{
    if (str == QLatin1String("Boolean"))
    {
        return ProblemCategory::Boolean;
    }
    if (str == QLatin1String("Projection"))
    {
        return ProblemCategory::Projection;
    }
    if (str == QLatin1String("Classification"))
    {
        return ProblemCategory::Classification;
    }
    if (str == QLatin1String("Topology"))
    {
        return ProblemCategory::Topology;
    }
    if (str == QLatin1String("Tolerance"))
    {
        return ProblemCategory::Tolerance;
    }
    if (str == QLatin1String("Meshing"))
    {
        return ProblemCategory::Meshing;
    }
    if (str == QLatin1String("HLR"))
    {
        return ProblemCategory::HLR;
    }
    if (str == QLatin1String("Performance"))
    {
        return ProblemCategory::Performance;
    }
    if (str == QLatin1String("Crash"))
    {
        return ProblemCategory::Crash;
    }
    return ProblemCategory::Unknown;
}

QJsonObject problemToJson(const ProblemContext& p)
{
    QJsonObject o;
    o[QStringLiteral("title")] = QString::fromStdString(p.title);
    o[QStringLiteral("category")] = categoryToString(p.category);
    o[QStringLiteral("description")] = QString::fromStdString(p.description);
    o[QStringLiteral("occtVersion")] = QString::fromStdString(p.occtVersion);
    o[QStringLiteral("compiler")] = QString::fromStdString(p.compiler);
    o[QStringLiteral("buildType")] = QString::fromStdString(p.buildType);

    QJsonArray files;
    for (const std::string& f : p.inputFiles)
    {
        files.append(QString::fromStdString(f));
    }
    o[QStringLiteral("inputFiles")] = files;

    QJsonObject params;
    for (const auto& kv : p.parameters)
    {
        params[QString::fromStdString(kv.first)] = QString::fromStdString(kv.second);
    }
    o[QStringLiteral("parameters")] = params;
    return o;
}

void problemFromJson(const QJsonObject& o, ProblemContext* p)
{
    p->title = o.value(QStringLiteral("title")).toString().toStdString();
    p->category = categoryFromString(o.value(QStringLiteral("category")).toString());
    p->description = o.value(QStringLiteral("description")).toString().toStdString();
    p->occtVersion = o.value(QStringLiteral("occtVersion")).toString().toStdString();
    p->compiler = o.value(QStringLiteral("compiler")).toString().toStdString();
    p->buildType = o.value(QStringLiteral("buildType")).toString().toStdString();

    p->inputFiles.clear();
    const QJsonValue filesV = o.value(QStringLiteral("inputFiles"));
    if (filesV.isArray())
    {
        for (const QJsonValue& v : filesV.toArray())
        {
            p->inputFiles.push_back(v.toString().toStdString());
        }
    }

    p->parameters.clear();
    const QJsonObject params = o.value(QStringLiteral("parameters")).toObject();
    for (auto it = params.begin(); it != params.end(); ++it)
    {
        p->parameters[it.key().toStdString()] = it.value().toString().toStdString();
    }
}

QJsonObject findingToJson(const DiagnosticFinding& f)
{
    QJsonObject o;
    o[QStringLiteral("ruleId")] = QString::fromStdString(f.ruleId);
    o[QStringLiteral("severity")] = severityToString(f.severity);
    o[QStringLiteral("title")] = QString::fromStdString(f.title);
    o[QStringLiteral("description")] = QString::fromStdString(f.description);

    QJsonArray ids;
    for (int id : f.relatedShapeIds)
    {
        ids.append(id);
    }
    o[QStringLiteral("relatedShapeIds")] = ids;

    auto stringsToArr = [](const std::vector<std::string>& vec) {
        QJsonArray a;
        for (const std::string& s : vec)
        {
            a.append(QString::fromStdString(s));
        }
        return a;
    };
    o[QStringLiteral("evidence")] = stringsToArr(f.evidence);
    o[QStringLiteral("possibleCauses")] = stringsToArr(f.possibleCauses);
    o[QStringLiteral("suggestions")] = stringsToArr(f.suggestions);
    return o;
}

void findingFromJson(const QJsonObject& o, DiagnosticFinding* f)
{
    f->ruleId = o.value(QStringLiteral("ruleId")).toString().toStdString();
    (void)severityFromString(o.value(QStringLiteral("severity")).toString(), &f->severity);
    f->title = o.value(QStringLiteral("title")).toString().toStdString();
    f->description = o.value(QStringLiteral("description")).toString().toStdString();

    f->relatedShapeIds.clear();
    const QJsonValue idsV = o.value(QStringLiteral("relatedShapeIds"));
    if (idsV.isArray())
    {
        for (const QJsonValue& v : idsV.toArray())
        {
            f->relatedShapeIds.push_back(v.toInt());
        }
    }

    auto arrToStrings = [](const QJsonArray& a) {
        std::vector<std::string> out;
        for (const QJsonValue& v : a)
        {
            out.push_back(v.toString().toStdString());
        }
        return out;
    };
    f->evidence = arrToStrings(o.value(QStringLiteral("evidence")).toArray());
    f->possibleCauses = arrToStrings(o.value(QStringLiteral("possibleCauses")).toArray());
    f->suggestions = arrToStrings(o.value(QStringLiteral("suggestions")).toArray());
}
} // namespace

QByteArray SessionSerializer::toJson(const DebugSession& session)
{
    QJsonObject root;
    root[QStringLiteral("format")] = QStringLiteral("occtdbg");
    root[QStringLiteral("version")] = session.version;
    root[QStringLiteral("createdAt")] = QString::fromStdString(session.createdAt);
    root[QStringLiteral("problem")] = problemToJson(session.problem);

    QJsonArray inputs;
    for (const SessionInput& in : session.inputs)
    {
        QJsonObject io;
        io[QStringLiteral("path")] = QString::fromStdString(in.path);
        io[QStringLiteral("type")] = QString::fromStdString(in.type);
        io[QStringLiteral("role")] = QString::fromStdString(in.role);
        inputs.append(io);
    }
    root[QStringLiteral("inputs")] = inputs;

    root[QStringLiteral("operations")] = QJsonArray();

    QJsonObject ui;
    ui[QStringLiteral("selectedShapeId")] = session.selectedShapeId;
    root[QStringLiteral("ui")] = ui;

    QJsonArray diag;
    for (const DiagnosticFinding& f : session.diagnostics)
    {
        diag.append(findingToJson(f));
    }
    root[QStringLiteral("diagnostics")] = diag;

    QJsonDocument doc(root);
    return doc.toJson(QJsonDocument::Indented);
}

bool SessionSerializer::save(const QString& filePath, const DebugSession& session, QString* errorMessage)
{
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        if (errorMessage)
        {
            *errorMessage = f.errorString();
        }
        return false;
    }
    const QByteArray data = toJson(session);
    if (f.write(data) != data.size())
    {
        if (errorMessage)
        {
            *errorMessage = f.errorString();
        }
        return false;
    }
    return true;
}

bool SessionSerializer::load(const QString& filePath, DebugSession* outSession, QString* errorMessage)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly))
    {
        if (errorMessage)
        {
            *errorMessage = f.errorString();
        }
        return false;
    }

    QJsonParseError parseErr{};
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &parseErr);
    if (parseErr.error != QJsonParseError::NoError || !doc.isObject())
    {
        if (errorMessage)
        {
            *errorMessage = parseErr.errorString();
        }
        return false;
    }

    const QJsonObject root = doc.object();
    if (root.value(QStringLiteral("format")).toString() != QLatin1String("occtdbg"))
    {
        if (errorMessage)
        {
            *errorMessage = QStringLiteral("Not an OCCTDebug session file (missing format \"occtdbg\").");
        }
        return false;
    }

    const int ver = root.value(QStringLiteral("version")).toInt(0);
    if (ver < 1 || ver > DebugSession::kCurrentVersion)
    {
        if (errorMessage)
        {
            *errorMessage = QStringLiteral("Unsupported session version: %1").arg(ver);
        }
        return false;
    }

    DebugSession s;
    s.version = ver;
    s.createdAt = root.value(QStringLiteral("createdAt")).toString().toStdString();

    problemFromJson(root.value(QStringLiteral("problem")).toObject(), &s.problem);

    s.inputs.clear();
    const QJsonValue inputsV = root.value(QStringLiteral("inputs"));
    if (inputsV.isArray())
    {
        for (const QJsonValue& iv : inputsV.toArray())
        {
            const QJsonObject io = iv.toObject();
            SessionInput si;
            si.path = io.value(QStringLiteral("path")).toString().toStdString();
            si.type = io.value(QStringLiteral("type")).toString(QStringLiteral("brep")).toStdString();
            si.role = io.value(QStringLiteral("role")).toString(QStringLiteral("primary")).toStdString();
            s.inputs.push_back(si);
        }
    }

    s.diagnostics.clear();
    const QJsonValue diagV = root.value(QStringLiteral("diagnostics"));
    if (diagV.isArray())
    {
        for (const QJsonValue& dv : diagV.toArray())
        {
            DiagnosticFinding f;
            findingFromJson(dv.toObject(), &f);
            s.diagnostics.push_back(std::move(f));
        }
    }

    const QJsonObject ui = root.value(QStringLiteral("ui")).toObject();
    s.selectedShapeId = ui.value(QStringLiteral("selectedShapeId")).toInt(-1);

    *outSession = std::move(s);
    return true;
}

QString SessionSerializer::toStoredPath(const QString& absolutePath, const QString& sessionFilePath)
{
    const QString abs = QFileInfo(absolutePath).absoluteFilePath();
    const QDir sessionDir = QFileInfo(sessionFilePath).absoluteDir();
    const QString rel = sessionDir.relativeFilePath(abs);
    if (QFileInfo(rel).isAbsolute())
    {
        return abs;
    }
    if (rel.startsWith(QLatin1String("..")))
    {
        return abs;
    }
    return rel;
}

QString SessionSerializer::resolveInputPath(const QString& storedPath, const QString& sessionFilePath)
{
    const QFileInfo storedFi(storedPath);
    if (storedFi.isAbsolute())
    {
        return storedFi.absoluteFilePath();
    }
    const QDir sessionDir = QFileInfo(sessionFilePath).absoluteDir();
    return QDir::cleanPath(sessionDir.absoluteFilePath(storedPath));
}
