#pragma once

#include "core/DebugSession.h"

#include <QByteArray>
#include <QString>

/// Reads/writes `.occtdbg` JSON sessions (see doc/session_format.md).
class SessionSerializer
{
public:
    /// Encode session as UTF-8 JSON (pretty-printed).
    static QByteArray toJson(const DebugSession& session);

    static bool save(const QString& filePath, const DebugSession& session, QString* errorMessage);

    static bool load(const QString& filePath, DebugSession* outSession, QString* errorMessage);

    /// Store `absolutePath` relative to the session file directory when possible.
    static QString toStoredPath(const QString& absolutePath, const QString& sessionFilePath);

    /// Resolve a path from session file (relative paths are against the session directory).
    static QString resolveInputPath(const QString& storedPath, const QString& sessionFilePath);
};
