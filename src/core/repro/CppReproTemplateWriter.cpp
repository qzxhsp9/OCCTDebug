#include "core/repro/CppReproTemplateWriter.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStringConverter>
#include <QTextStream>

namespace occtdebug
{
namespace
{
QString normalizedRelativePath(const QDir& root, const QString& absolutePath)
{
    QString relative = root.relativeFilePath(absolutePath);
    relative.replace(QLatin1Char('\\'), QLatin1Char('/'));
    return relative;
}

bool writeUtf8File(const QString& path, const QString& text, QString* error)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("failed to open %1: %2").arg(path, file.errorString());
        }
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << text;
    if (!file.commit())
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("failed to commit %1: %2").arg(path, file.errorString());
        }
        return false;
    }
    return true;
}

QString cmakeTemplate()
{
    return QStringLiteral(R"(cmake_minimum_required(VERSION 3.20)
project(OCCTDebugMinimalRepro LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCASCADE CONFIG QUIET)
if(NOT OpenCASCADE_FOUND)
    message(FATAL_ERROR
        "OpenCASCADE was not found. Configure with -DOpenCASCADE_DIR=<occt>/lib/cmake/opencascade "
        "or provide a toolchain/preset that exposes OCCT.")
endif()

add_executable(occt_minimal_repro main.cpp)
target_include_directories(occt_minimal_repro PRIVATE ${OpenCASCADE_INCLUDE_DIR})
target_link_libraries(occt_minimal_repro PRIVATE ${OpenCASCADE_LIBRARIES})
)");
}

QString mainTemplate(const QString& caseId)
{
    return QStringLiteral(R"(#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <TopoDS_Shape.hxx>

#include <iostream>

int main()
{
    // Minimal OCCT repro scaffold for case %1.
    // Replace this demo shape with the smallest shape and operation
    // that reproduces the issue.
    const TopoDS_Shape shape = BRepPrimAPI_MakeBox(10.0, 20.0, 30.0).Shape();
    const BRepCheck_Analyzer analyzer(shape);

    std::cout << "case=%1\n";
    std::cout << "shape_valid=" << (analyzer.IsValid() ? "true" : "false") << "\n";
    return analyzer.IsValid() ? 0 : 1;
}
)").arg(caseId.isEmpty() ? QStringLiteral("unknown") : caseId);
}

QString readmeTemplate(const QString& caseId)
{
    return QStringLiteral(R"(# C++ Minimal Repro

Case: `%1`

This directory is a local scaffold for translating a DRAW/Tcl reproduction into
a minimal C++ OCCT reproduction.

## Configure

```powershell
cmake -S . -B build -DOpenCASCADE_DIR=<occt>/lib/cmake/opencascade
cmake --build build --config Debug
```

Keep private model paths and large CAD data outside the source file. Prefer
copying the smallest allowed input into the parent Case `input/` directory and
referencing it with a Case-relative path.
)").arg(caseId.isEmpty() ? QStringLiteral("unknown") : caseId);
}
} // namespace

CppReproTemplateResult CppReproTemplateWriter::write(const QString& caseWorkspaceRoot,
                                                     const QString& caseId,
                                                     const QString& reproScript,
                                                     bool overwrite)
{
    CppReproTemplateResult result;
    if (caseWorkspaceRoot.trimmed().isEmpty())
    {
        result.error = QStringLiteral("case workspace root is empty");
        return result;
    }

    const QDir workspace(caseWorkspaceRoot);
    const QString rootPath = workspace.filePath(QStringLiteral("repro/cpp_minimal"));
    QDir dir;
    if (!dir.mkpath(rootPath))
    {
        result.error = QStringLiteral("failed to create C++ repro directory: %1").arg(rootPath);
        return result;
    }

    const QDir reproRoot(rootPath);
    const QVector<QPair<QString, QString>> files {
        {QStringLiteral("CMakeLists.txt"), cmakeTemplate()},
        {QStringLiteral("main.cpp"), mainTemplate(caseId)},
        {QStringLiteral("README.md"), readmeTemplate(caseId)},
        {QStringLiteral("repro_from_draw.tcl"), reproScript},
    };

    for (const auto& file : files)
    {
        const QString path = reproRoot.filePath(file.first);
        if (!overwrite && QFileInfo::exists(path))
        {
            result.writtenFiles.push_back(normalizedRelativePath(workspace, path));
            continue;
        }
        QString error;
        if (!writeUtf8File(path, file.second, &error))
        {
            result.error = error;
            return result;
        }
        result.writtenFiles.push_back(normalizedRelativePath(workspace, path));
    }

    result.success = true;
    result.rootDirectory = normalizedRelativePath(workspace, rootPath);
    return result;
}
} // namespace occtdebug
