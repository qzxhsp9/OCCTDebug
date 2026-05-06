#include "diagnose/DiagnosticEngine.h"

#include <algorithm>

namespace
{
int severityRank(DiagnosticSeverity s)
{
    switch (s)
    {
    case DiagnosticSeverity::Critical:
        return 0;
    case DiagnosticSeverity::Error:
        return 1;
    case DiagnosticSeverity::Warning:
        return 2;
    case DiagnosticSeverity::Info:
    default:
        return 3;
    }
}
} // namespace

DiagnosticEngine::DiagnosticEngine()
{
    m_registry.registerDefaultRules();
}

std::vector<DiagnosticFinding> DiagnosticEngine::diagnose(
    const ProblemContext& context,
    const ShapeDocument& document)
{
    std::vector<DiagnosticFinding> all;
    for (const IDiagnosticRule* rule : m_registry.applicableRules(context, document))
    {
        const auto part = rule->run(context, document);
        all.insert(all.end(), part.begin(), part.end());
    }

    std::stable_sort(
        all.begin(),
        all.end(),
        [](const DiagnosticFinding& a, const DiagnosticFinding& b) {
            const int ra = severityRank(a.severity);
            const int rb = severityRank(b.severity);
            if (ra != rb)
            {
                return ra < rb;
            }
            return a.ruleId < b.ruleId;
        });

    return all;
}
