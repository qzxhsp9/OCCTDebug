#pragma once

#include "diagnose/IDiagnosticRule.h"

#include <memory>
#include <vector>

class RuleRegistry
{
public:
    void registerRule(std::unique_ptr<IDiagnosticRule> rule);

    std::vector<const IDiagnosticRule*> applicableRules(
        const ProblemContext& context,
        const ShapeDocument& document) const;

    void registerDefaultRules();

private:
    std::vector<std::unique_ptr<IDiagnosticRule>> m_rules;
};
