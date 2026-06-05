#pragma once

#include <QString>
#include <QStringList>

namespace occtdebug
{
struct CppReproTemplateResult
{
    bool success = false;
    QString rootDirectory;
    QStringList writtenFiles;
    QString error;
};

class CppReproTemplateWriter
{
public:
    static CppReproTemplateResult write(const QString& caseWorkspaceRoot,
                                        const QString& caseId,
                                        const QString& reproScript,
                                        bool overwrite = true);
};
} // namespace occtdebug
