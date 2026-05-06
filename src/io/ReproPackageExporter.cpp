#include "io/ReproPackageExporter.h"

#include "core/DebugSession.h"
#include "io/SessionSerializer.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

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
            *errorMessage = QStringLiteral("Primary BREP path is missing or not a file.");
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

    const QString destBrep = QDir(caseDir).filePath(QStringLiteral("input.brep"));
    if (QFile::exists(destBrep))
    {
        if (!QFile::remove(destBrep))
        {
            if (errorMessage != nullptr)
            {
                *errorMessage = QStringLiteral("Could not overwrite existing case/input.brep.");
            }
            return false;
        }
    }
    if (!QFile::copy(srcFi.absoluteFilePath(), destBrep))
    {
        if (errorMessage != nullptr)
        {
            *errorMessage = QStringLiteral("Could not copy BREP into package.");
        }
        return false;
    }

    const QString relBrep = QStringLiteral("case/input.brep");

    DebugSession session;
    session.version = DebugSession::kCurrentVersion;
    session.createdAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs).toStdString();
    session.problem = problem;
    session.problem.inputFiles = {relBrep.toStdString()};
    session.diagnostics = findings;
    session.selectedShapeId = -1;

    SessionInput in;
    in.path = relBrep.toStdString();
    in.type = "brep";
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
        out << QStringLiteral("The session loads case/input.brep relative to this directory.\n");
    }

    return true;
}
