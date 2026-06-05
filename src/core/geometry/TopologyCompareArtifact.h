#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace occtdebug
{
class TopologyCompareArtifact
{
public:
    static QString defaultArtifactRelativePath();
    static QStringList compareArtifactCandidates();
    static QJsonObject buildFromSignatureFiles(const QString& beforeSignaturePath,
                                               const QString& afterSignaturePath,
                                               const QString& caseRoot,
                                               QString* error = nullptr);
    static QJsonObject writeForCase(const QString& caseRoot,
                                    const QString& beforeSignaturePath,
                                    const QString& afterSignaturePath,
                                    QString* error = nullptr);
    static QJsonObject loadForCase(const QString& caseRoot);
    static QString summaryText(const QJsonObject& artifact);
};
} // namespace occtdebug
