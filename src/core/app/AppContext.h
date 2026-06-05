#pragma once

#include "core/config/ConfigService.h"

#include <QString>

#include <optional>

namespace occtdebug
{
class AppContext
{
public:
    static std::optional<AppContext> createDefault(QString* error = nullptr);
    static std::optional<AppContext> create(const QString& repoRoot, QString* error = nullptr);

    const WorkbenchConfig& config() const;

private:
    explicit AppContext(const WorkbenchConfig& config);

    WorkbenchConfig m_config;
};
} // namespace occtdebug
