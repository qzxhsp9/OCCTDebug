#include "diagnose/RuleRegistry.h"

#include "diagnose/rules/CheckBRepValidityRule.h"
#include "diagnose/rules/CheckNullShapeRule.h"
#include "diagnose/rules/CheckSameParameterRule.h"
#include "diagnose/rules/CheckShellClosedRule.h"
#include "diagnose/rules/CheckToleranceRule.h"
#include "diagnose/rules/CheckWireClosedRule.h"

void RuleRegistry::registerRule(std::unique_ptr<IDiagnosticRule> rule)
{
    m_rules.push_back(std::move(rule));
}

std::vector<const IDiagnosticRule*> RuleRegistry::applicableRules(
    const ProblemContext& context,
    const ShapeDocument& document) const
{
    std::vector<const IDiagnosticRule*> out;
    for (const auto& rule : m_rules)
    {
        if (rule->isApplicable(context, document))
        {
            out.push_back(rule.get());
        }
    }
    return out;
}

void RuleRegistry::registerDefaultRules()
{
    registerRule(std::make_unique<CheckNullShapeRule>());
    registerRule(std::make_unique<CheckWireClosedRule>());
    registerRule(std::make_unique<CheckShellClosedRule>());
    registerRule(std::make_unique<CheckToleranceRule>());
    registerRule(std::make_unique<CheckBRepValidityRule>());
    registerRule(std::make_unique<CheckSameParameterRule>());
}
