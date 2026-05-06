#pragma once

#include <map>
#include <string>
#include <vector>

enum class ProblemCategory
{
    Unknown,
    Boolean,
    Projection,
    Classification,
    Topology,
    Tolerance,
    Meshing,
    HLR,
    Performance,
    Crash
};

struct ProblemContext
{
    std::string title;
    ProblemCategory category = ProblemCategory::Unknown;
    std::string description;

    std::string occtVersion;
    std::string compiler;
    std::string buildType;

    std::vector<std::string> inputFiles;
    std::map<std::string, std::string> parameters;
};
