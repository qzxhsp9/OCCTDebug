#include "core/repro/CppReproTemplateWriter.h"

#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    Q_UNUSED(app);

    QTemporaryDir tempDir;
    if (!tempDir.isValid())
    {
        QTextStream(stderr) << "failed to create temp directory\n";
        return 1;
    }

    const occtdebug::CppReproTemplateResult result =
        occtdebug::CppReproTemplateWriter::write(
            tempDir.path(),
            QStringLiteral("OCC-CPP-REPRO"),
            QStringLiteral("pload MODELING\nbox b 10 20 30\ncheckshape b\n"));

    if (!result.success)
    {
        QTextStream(stderr) << "writer failed: " << result.error << "\n";
        return 2;
    }
    if (result.rootDirectory != QStringLiteral("repro/cpp_minimal")
        || !result.writtenFiles.contains(QStringLiteral("repro/cpp_minimal/CMakeLists.txt"))
        || !result.writtenFiles.contains(QStringLiteral("repro/cpp_minimal/main.cpp"))
        || !result.writtenFiles.contains(QStringLiteral("repro/cpp_minimal/repro_from_draw.tcl")))
    {
        QTextStream(stderr) << "unexpected written file list\n";
        return 3;
    }

    QFile mainFile(tempDir.filePath(QStringLiteral("repro/cpp_minimal/main.cpp")));
    QFile cmakeFile(tempDir.filePath(QStringLiteral("repro/cpp_minimal/CMakeLists.txt")));
    QFile drawFile(tempDir.filePath(QStringLiteral("repro/cpp_minimal/repro_from_draw.tcl")));
    if (!mainFile.open(QIODevice::ReadOnly | QIODevice::Text)
        || !cmakeFile.open(QIODevice::ReadOnly | QIODevice::Text)
        || !drawFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream(stderr) << "failed to read generated files\n";
        return 4;
    }

    const QString mainText = QString::fromUtf8(mainFile.readAll());
    const QString cmakeText = QString::fromUtf8(cmakeFile.readAll());
    const QString drawText = QString::fromUtf8(drawFile.readAll());
    if (!mainText.contains(QStringLiteral("OCC-CPP-REPRO"))
        || !cmakeText.contains(QStringLiteral("find_package(OpenCASCADE CONFIG QUIET)"))
        || !cmakeText.contains(QStringLiteral("${OpenCASCADE_LIBRARIES}"))
        || cmakeText.contains(QStringLiteral("D:/"))
        || cmakeText.contains(QStringLiteral("F:/"))
        || !drawText.contains(QStringLiteral("checkshape b")))
    {
        QTextStream(stderr) << "generated template content mismatch\n";
        return 5;
    }

    QTextStream(stdout) << "CPP_REPRO_TEMPLATE_WRITER_SMOKE_OK\n";
    return 0;
}
