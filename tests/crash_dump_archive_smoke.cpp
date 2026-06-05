#include "core/case/CrashDumpArchive.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QTextStream>

namespace
{
bool expect(bool condition, const char* message)
{
    if (!condition)
    {
        QTextStream(stderr) << message << "\n";
    }
    return condition;
}

bool writeFile(const QString& path, const QByteArray& bytes)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
    {
        return false;
    }
    file.write(bytes);
    return true;
}
} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir temp;
    if (!expect(temp.isValid(), "temporary directory is invalid"))
    {
        return 1;
    }

    const QString sourcePath = temp.filePath(QStringLiteral("source crash.dmp"));
    if (!expect(writeFile(sourcePath, QByteArray("minidump bytes")), "failed to write source dump"))
    {
        return 2;
    }

    occtdebug::CrashDumpArchiveResult result;
    QString error;
    if (!expect(occtdebug::CrashDumpArchive::archiveFile(sourcePath, temp.path(), QStringLiteral("CASE-DUMP"), &result, &error),
                "archive failed"))
    {
        QTextStream(stderr) << error << "\n";
        return 3;
    }

    const QString serialized = QString::fromUtf8(QJsonDocument(result.manifest).toJson(QJsonDocument::Compact));
    bool ok = true;
    ok = expect(result.artifactRelativePath.startsWith(QStringLiteral("artifacts/crash/")), "artifact path should be case-relative") && ok;
    ok = expect(result.manifestRelativePath.startsWith(QStringLiteral("artifacts/crash/")), "manifest path should be case-relative") && ok;
    ok = expect(QFile::exists(temp.filePath(result.artifactRelativePath)), "archived dump missing") && ok;
    ok = expect(QFile::exists(temp.filePath(result.manifestRelativePath)), "manifest missing") && ok;
    ok = expect(result.bytes == QByteArray("minidump bytes").size(), "archived bytes mismatch") && ok;
    ok = expect(result.sha256.size() == 64, "sha256 length mismatch") && ok;
    ok = expect(result.originalName == QStringLiteral("source crash.dmp"), "original name mismatch") && ok;
    ok = expect(!serialized.contains(temp.path()), "manifest leaked workspace absolute path") && ok;
    ok = expect(!serialized.contains(sourcePath), "manifest leaked source absolute path") && ok;

    if (ok)
    {
        QTextStream(stdout) << "CRASH_DUMP_ARCHIVE_SMOKE_OK\n";
    }
    return ok ? 0 : 1;
}
