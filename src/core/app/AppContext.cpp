#include "core/app/AppContext.h"

#include <QDir>

#ifndef OCCTDEBUG_SOURCE_DIR
#define OCCTDEBUG_SOURCE_DIR "."
#endif

namespace occtdebug
{
std::optional<AppContext> AppContext::createDefault(QString* error)
{
    return create(QString::fromUtf8(OCCTDEBUG_SOURCE_DIR), error);
}

std::optional<AppContext> AppContext::create(const QString& repoRoot, QString* error)
{
    ConfigService configService(QDir::cleanPath(repoRoot));
    if (!configService.load(error))
    {
        return std::nullopt;
    }
    return AppContext(configService.config());
}

const WorkbenchConfig& AppContext::config() const
{
    return m_config;
}

AppContext::AppContext(const WorkbenchConfig& config)
    : m_config(config)
{
}
} // namespace occtdebug
