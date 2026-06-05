#include "core/report/MarkdownReportGenerator.h"

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

    occtdebug::CaseManifest manifest;
    manifest.caseId = QStringLiteral("OCC-REPORT-INPUT");
    manifest.title = QStringLiteral("Report input file smoke");
    manifest.status = QStringLiteral("Analyzing");
    manifest.createdAt = QStringLiteral("2026-06-05T00:00:00Z");
    manifest.keyInputs = {{QStringLiteral("Problem"), QStringLiteral("input hash should be reported")}};
    manifest.inputFiles = {{
        QStringLiteral("input/demo.brep"),
        QStringLiteral("demo.brep"),
        QStringLiteral("abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789"),
        42,
        QStringLiteral("2026-06-05T00:00:00Z"),
    }};

    const QString reportPath = tempDir.filePath(QStringLiteral("case/report/repro_report.md"));
    QString error;
    if (!occtdebug::MarkdownReportGenerator::writeReproReport(manifest, reportPath, &error))
    {
        QTextStream(stderr) << "failed to write repro report: " << error << "\n";
        return 2;
    }

    QFile report(reportPath);
    if (!report.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream(stderr) << "failed to read repro report\n";
        return 3;
    }
    const QString text = QString::fromUtf8(report.readAll());
    if (!text.contains(QString::fromUtf8("输入文件摘要"))
        || !text.contains(QStringLiteral("input/demo.brep"))
        || !text.contains(manifest.inputFiles.first().sha256)
        || text.contains(tempDir.path()))
    {
        QTextStream(stderr) << "report input file summary mismatch\n";
        return 4;
    }

    QTextStream(stdout) << "MARKDOWN_REPORT_GENERATOR_SMOKE_OK\n";
    return 0;
}
