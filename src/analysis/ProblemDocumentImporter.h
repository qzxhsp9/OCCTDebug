#pragma once

#include "core/ProblemContext.h"

#include <string>
#include <vector>

struct ProblemDocument
{
    ProblemContext context;
    std::string reproductionSteps;
    std::string notes;
    std::vector<std::string> warnings;
};

class ProblemDocumentImporter
{
public:
    static bool loadFile(
        const std::string& filePath,
        ProblemDocument* outDocument,
        std::string* errorMessage);

    static ProblemDocument parseMarkdown(
        const std::string& markdown,
        const std::string& baseDirectory = std::string());

    static std::string toMarkdown(const ProblemDocument& document);
};
