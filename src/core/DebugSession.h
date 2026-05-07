#pragma once

#include "core/DiagnosticFinding.h"
#include "core/ProblemContext.h"

#include <string>
#include <vector>

/// Single input file reference stored in a `.occtdbg` session (paths may be relative to the session file).
struct SessionInput
{
    std::string path;
    std::string type; // e.g. "brep"
    std::string role; // e.g. "primary"
};

/// Serializable debugging session (see doc/session_format.md).
struct DebugSession
{
    static constexpr int kCurrentVersion = 1;

    int version = kCurrentVersion;
    std::string createdAt;
    ProblemContext problem;
    std::vector<SessionInput> inputs;
    std::vector<DiagnosticFinding> diagnostics;
    int selectedShapeId = -1;
};
