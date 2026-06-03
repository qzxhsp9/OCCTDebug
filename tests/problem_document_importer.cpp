#include "analysis/ProblemDocumentImporter.h"

#include <iostream>

namespace
{
int expect(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        return 1;
    }
    return 0;
}
} // namespace

int main()
{
    const char* markdown = R"md(
# Boolean half-space shell cut

## Summary
Shell cut by half-space produces no cap face.

## Environment
- OCCT Version: 7.9.3
- Compiler: MSVC 19.39
- Build Type: Debug
- Category: Boolean
- FuzzyValue: 0.01

## Input Files
- case/input.brep
- case/tool.step

## Reproduction Steps
1. Load input shell.
2. Run BRepAlgoAPI_Cut with half-space tool.

## Expected Behavior
Result status: has a section cap face.

## Actual Behavior
Observed result: remains an open shell.

## Notes / Suspected Area
Likely BOPAlgo behavior for sheet bodies.
)md";

    const ProblemDocument doc = ProblemDocumentImporter::parseMarkdown(markdown);

    if (expect(doc.context.title == "Boolean half-space shell cut", "title mismatch") != 0)
    {
        return 1;
    }
    if (expect(doc.context.category == ProblemCategory::Boolean, "category mismatch") != 0)
    {
        return 1;
    }
    if (expect(doc.context.occtVersion == "7.9.3", "OCCT version mismatch") != 0)
    {
        return 1;
    }
    if (expect(doc.context.compiler == "MSVC 19.39", "compiler mismatch") != 0)
    {
        return 1;
    }
    if (expect(doc.context.buildType == "Debug", "build type mismatch") != 0)
    {
        return 1;
    }
    const auto fuzzyIt = doc.context.parameters.find("fuzzyvalue");
    if (expect(fuzzyIt != doc.context.parameters.end(), "FuzzyValue parameter missing") != 0)
    {
        return 1;
    }
    if (expect(fuzzyIt->second == "0.01", "FuzzyValue parameter mismatch") != 0)
    {
        return 1;
    }
    if (expect(doc.context.inputFiles.size() == 2, "input count mismatch") != 0)
    {
        return 1;
    }
    if (expect(doc.context.inputFiles[0] == "case/input.brep", "first input mismatch") != 0)
    {
        return 1;
    }
    if (expect(doc.context.expectedBehavior == "Result status: has a section cap face.", "expected behavior mismatch") != 0)
    {
        return 1;
    }
    if (expect(doc.context.actualBehavior == "Observed result: remains an open shell.", "actual behavior mismatch") != 0)
    {
        return 1;
    }
    if (expect(
            doc.reproductionSteps.find("BRepAlgoAPI_Cut") != std::string::npos,
            "reproduction steps mismatch")
        != 0)
    {
        return 1;
    }
    if (expect(doc.notes.find("BOPAlgo") != std::string::npos, "notes mismatch") != 0)
    {
        return 1;
    }

    const std::string generated = ProblemDocumentImporter::toMarkdown(doc);
    const ProblemDocument reparsed = ProblemDocumentImporter::parseMarkdown(generated);
    if (expect(reparsed.context.title == doc.context.title, "generated title mismatch") != 0)
    {
        return 1;
    }
    if (expect(reparsed.context.category == ProblemCategory::Boolean, "generated category mismatch") != 0)
    {
        return 1;
    }
    if (expect(
            reparsed.context.expectedBehavior == doc.context.expectedBehavior,
            "generated expected behavior mismatch")
        != 0)
    {
        return 1;
    }
    if (expect(reparsed.context.inputFiles.size() == 2, "generated input count mismatch") != 0)
    {
        return 1;
    }
    const auto reparsedFuzzyIt = reparsed.context.parameters.find("fuzzyvalue");
    if (expect(reparsedFuzzyIt != reparsed.context.parameters.end(), "generated FuzzyValue missing") != 0)
    {
        return 1;
    }
    if (expect(reparsedFuzzyIt->second == "0.01", "generated FuzzyValue mismatch") != 0)
    {
        return 1;
    }

    return 0;
}
