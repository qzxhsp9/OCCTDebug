#include "io/ReproPackageExporter.h"

#include "core/DebugSession.h"
#include "io/SessionSerializer.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

namespace
{
QString inputTypeFromFilePath(const QString& filePath)
{
    const QString ext = QFileInfo(filePath).suffix().toLower();
    if (ext == QStringLiteral("stp") || ext == QStringLiteral("step"))
    {
        return QStringLiteral("step");
    }
    return QStringLiteral("brep");
}
} // namespace

bool ReproPackageExporter::exportMinimalPackage(
    const QString& packageDir,
    const ProblemContext& problem,
    const QString& primaryBrepAbsolute,
    const std::vector<DiagnosticFinding>& findings,
    QString* errorMessage)
{
    const QFileInfo srcFi(primaryBrepAbsolute);
    if (!srcFi.exists() || !srcFi.isFile())
    {
        if (errorMessage != nullptr)
        {
            *errorMessage = QStringLiteral("Primary model path is missing or not a file.");
        }
        return false;
    }

    const QString cleanRoot = QDir::cleanPath(packageDir);
    if (!QDir().mkpath(cleanRoot))
    {
        if (errorMessage != nullptr)
        {
            *errorMessage = QStringLiteral("Could not create package directory.");
        }
        return false;
    }
    const QDir root(cleanRoot);

    const QString caseDir = root.filePath(QStringLiteral("case"));
    if (!QDir().mkpath(caseDir))
    {
        if (errorMessage != nullptr)
        {
            *errorMessage = QStringLiteral("Could not create case/ subdirectory.");
        }
        return false;
    }

    QString caseFileName = srcFi.fileName();
    if (caseFileName.isEmpty())
    {
        caseFileName = QStringLiteral("input.brep");
    }
    const QString destModel = QDir(caseDir).filePath(caseFileName);
    if (QFile::exists(destModel))
    {
        if (!QFile::remove(destModel))
        {
            if (errorMessage != nullptr)
            {
                *errorMessage =
                    QStringLiteral("Could not overwrite existing case/%1.").arg(caseFileName);
            }
            return false;
        }
    }
    if (!QFile::copy(srcFi.absoluteFilePath(), destModel))
    {
        if (errorMessage != nullptr)
        {
            *errorMessage = QStringLiteral("Could not copy model into package.");
        }
        return false;
    }

    const QString relModel = QStringLiteral("case/") + caseFileName;
    const QString storageType = inputTypeFromFilePath(primaryBrepAbsolute);

    DebugSession session;
    session.version = DebugSession::kCurrentVersion;
    session.createdAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs).toStdString();
    session.problem = problem;
    session.problem.inputFiles = {relModel.toStdString()};
    session.diagnostics = findings;
    session.selectedShapeId = -1;

    SessionInput in;
    in.path = relModel.toStdString();
    in.type = storageType.toStdString();
    in.role = "primary";
    session.inputs.push_back(std::move(in));

    const QString sessionPath = root.filePath(QStringLiteral("debug.occtdbg"));
    QString err;
    if (!SessionSerializer::save(sessionPath, session, &err))
    {
        if (errorMessage != nullptr)
        {
            *errorMessage = err;
        }
        return false;
    }

    QFile readme(root.filePath(QStringLiteral("README.txt")));
    if (readme.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
    {
        QTextStream out(&readme);
        out << QStringLiteral("OCCTDebug — minimal reproduction package\n\n");
        out << QStringLiteral("1. Install OCCTDebug on a machine with matching OCCT/Qt if needed.\n");
        out << QStringLiteral("2. File → Open session…\n");
        out << QStringLiteral("3. Choose debug.occtdbg in this folder.\n\n");
        out << QStringLiteral("The session loads %1 relative to this directory.\n").arg(relModel);
    }

    return true;
}
