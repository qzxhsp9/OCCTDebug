#pragma once

#include <string>
#include <vector>

enum class DiagnosticSeverity
{
    Info,
    Warning,
    Error,
    Critical
};

struct DiagnosticFinding
{
    std::string ruleId;
    DiagnosticSeverity severity = DiagnosticSeverity::Info;
    std::string title;
    std::string description;

    std::vector<int> relatedShapeIds;
    std::vector<std::string> evidence;
    std::vector<std::string> possibleCauses;
    std::vector<std::string> suggestions;
};
