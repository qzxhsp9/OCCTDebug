#pragma once

#include <QString>

class Logger
{
public:
    static void info(const QString& message);
    static void warning(const QString& message);
    static void error(const QString& message);

    static QStringList recentLines();
    static void clearRecent();

private:
    static void append(const QString& level, const QString& message);
};
