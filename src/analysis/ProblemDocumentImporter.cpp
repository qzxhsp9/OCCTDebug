#include "analysis/ProblemDocumentImporter.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace
{
std::string trim(const std::string& s)
{
    const auto first = std::find_if_not(s.begin(), s.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
    if (first == s.end())
    {
        return {};
    }

    const auto last = std::find_if_not(s.rbegin(), std::string::const_reverse_iterator(first), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
    return std::string(first, last.base());
}

std::string toLower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return s;
}

std::string categoryToString(ProblemCategory category)
{
    switch (category)
    {
    case ProblemCategory::Boolean:
        return "Boolean";
    case ProblemCategory::Projection:
        return "Projection";
    case ProblemCategory::Classification:
        return "Classification";
    case ProblemCategory::Topology:
        return "Topology";
    case ProblemCategory::Tolerance:
        return "Tolerance";
    case ProblemCategory::Meshing:
        return "Meshing";
    case ProblemCategory::HLR:
        return "HLR";
    case ProblemCategory::Performance:
        return "Performance";
    case ProblemCategory::Crash:
        return "Crash";
    case ProblemCategory::Unknown:
    default:
        return "Unknown";
    }
}

std::string normalizeHeading(std::string s)
{
    s = toLower(trim(s));
    s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char ch) {
                return ch == ':' || ch == '/' || ch == '\\';
            }),
            s.end());

    std::string out;
    bool lastWasSpace = false;
    for (unsigned char ch : s)
    {
        if (std::isspace(ch) != 0)
        {
            if (!lastWasSpace)
            {
                out.push_back(' ');
            }
            lastWasSpace = true;
        }
        else
        {
            out.push_back(static_cast<char>(ch));
            lastWasSpace = false;
        }
    }
    return trim(out);
}

bool isHeading(const std::string& line, std::string* title)
{
    std::string s = trim(line);
    if (s.empty() || s.front() != '#')
    {
        return false;
    }

    std::size_t pos = 0;
    while (pos < s.size() && s[pos] == '#')
    {
        ++pos;
    }
    if (pos >= s.size() || std::isspace(static_cast<unsigned char>(s[pos])) == 0)
    {
        return false;
    }

    *title = trim(s.substr(pos));
    return true;
}

std::string stripListMarker(std::string s)
{
    s = trim(s);
    if (s.size() >= 2 && (s[0] == '-' || s[0] == '*') && std::isspace(static_cast<unsigned char>(s[1])) != 0)
    {
        return trim(s.substr(2));
    }
    if (s.size() >= 3 && std::isdigit(static_cast<unsigned char>(s[0])) != 0 && s[1] == '.'
        && std::isspace(static_cast<unsigned char>(s[2])) != 0)
    {
        return trim(s.substr(3));
    }
    return s;
}

bool splitKeyValue(const std::string& line, std::string* key, std::string* value)
{
    const std::string s = stripListMarker(line);
    const std::size_t pos = s.find(':');
    if (pos == std::string::npos)
    {
        return false;
    }
    *key = normalizeHeading(s.substr(0, pos));
    *value = trim(s.substr(pos + 1));
    return !key->empty();
}

ProblemCategory categoryFromString(const std::string& raw)
{
    const std::string s = normalizeHeading(raw);
    if (s.empty() || s == "unknown")
    {
        return ProblemCategory::Unknown;
    }
    if (s.find("boolean") != std::string::npos)
    {
        return ProblemCategory::Boolean;
    }
    if (s.find("projection") != std::string::npos)
    {
        return ProblemCategory::Projection;
    }
    if (s.find("classification") != std::string::npos)
    {
        return ProblemCategory::Classification;
    }
    if (s.find("topology") != std::string::npos)
    {
        return ProblemCategory::Topology;
    }
    if (s.find("tolerance") != std::string::npos)
    {
        return ProblemCategory::Tolerance;
    }
    if (s.find("meshing") != std::string::npos)
    {
        return ProblemCategory::Meshing;
    }
    if (s == "hlr" || s.find("hidden line") != std::string::npos)
    {
        return ProblemCategory::HLR;
    }
    if (s.find("performance") != std::string::npos)
    {
        return ProblemCategory::Performance;
    }
    if (s.find("crash") != std::string::npos)
    {
        return ProblemCategory::Crash;
    }
    return ProblemCategory::Unknown;
}

void appendLine(std::string* target, const std::string& line)
{
    const std::string s = trim(line);
    if (s.empty())
    {
        return;
    }
    if (!target->empty())
    {
        *target += '\n';
    }
    *target += s;
}

bool applyKeyValue(ProblemDocument* doc, const std::string& key, const std::string& value)
{
    if (key == "title")
    {
        doc->context.title = value;
        return true;
    }
    else if (key == "category" || key == "type" || key == "problem type")
    {
        doc->context.category = categoryFromString(value);
        if (doc->context.category == ProblemCategory::Unknown && !value.empty() && normalizeHeading(value) != "unknown")
        {
            doc->warnings.push_back("Unknown problem category: " + value);
        }
        return true;
    }
    else if (key == "occt version")
    {
        doc->context.occtVersion = value;
        return true;
    }
    else if (key == "compiler")
    {
        doc->context.compiler = value;
        return true;
    }
    else if (key == "build type")
    {
        doc->context.buildType = value;
        return true;
    }
    return false;
}
} // namespace

bool ProblemDocumentImporter::loadFile(
    const std::string& filePath,
    ProblemDocument* outDocument,
    std::string* errorMessage)
{
    if (outDocument == nullptr)
    {
        if (errorMessage != nullptr)
        {
            *errorMessage = "Output document is null.";
        }
        return false;
    }

    std::ifstream in(filePath, std::ios::binary);
    if (!in)
    {
        if (errorMessage != nullptr)
        {
            *errorMessage = "Unable to open problem document: " + filePath;
        }
        return false;
    }

    std::ostringstream ss;
    ss << in.rdbuf();
    *outDocument = parseMarkdown(ss.str());
    return true;
}

ProblemDocument ProblemDocumentImporter::parseMarkdown(
    const std::string& markdown,
    const std::string& baseDirectory)
{
    (void)baseDirectory;

    ProblemDocument doc;
    std::istringstream input(markdown);
    std::string line;
    std::string section;
    bool firstHeadingSeen = false;

    while (std::getline(input, line))
    {
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        std::string heading;
        if (isHeading(line, &heading))
        {
            if (!firstHeadingSeen && normalizeHeading(heading) != "problem")
            {
                doc.context.title = heading;
            }
            firstHeadingSeen = true;
            section = normalizeHeading(heading);
            continue;
        }

        const std::string content = trim(line);
        if (content.empty())
        {
            continue;
        }

        if (section == "summary")
        {
            appendLine(&doc.context.description, content);
            if (doc.context.title.empty())
            {
                doc.context.title = content;
            }
            continue;
        }
        if (section == "input files" || section == "inputs")
        {
            doc.context.inputFiles.push_back(stripListMarker(content));
            continue;
        }
        if (section == "reproduction steps" || section == "reproduction")
        {
            appendLine(&doc.reproductionSteps, content);
            doc.context.parameters["reproductionSteps"] = doc.reproductionSteps;
            continue;
        }
        if (section == "expected behavior" || section == "expected")
        {
            appendLine(&doc.context.expectedBehavior, content);
            continue;
        }
        if (section == "actual behavior" || section == "actual")
        {
            appendLine(&doc.context.actualBehavior, content);
            continue;
        }
        if (section == "notes suspected area" || section == "notes" || section == "suspected area")
        {
            appendLine(&doc.notes, content);
            doc.context.parameters["notes"] = doc.notes;
            continue;
        }

        std::string key;
        std::string value;
        if (splitKeyValue(content, &key, &value))
        {
            const bool handled = applyKeyValue(&doc, key, value);
            if (key == "expected behavior" || key == "expected")
            {
                doc.context.expectedBehavior = value;
                continue;
            }
            else if (key == "actual behavior" || key == "actual")
            {
                doc.context.actualBehavior = value;
                continue;
            }
            if (!handled)
            {
                doc.context.parameters[key] = value;
            }
            continue;
        }

        if (section == "environment")
        {
            doc.context.parameters["environment"] += (doc.context.parameters["environment"].empty() ? "" : "\n")
                + stripListMarker(content);
        }
        else if (section == "problem" || section.empty())
        {
            appendLine(&doc.context.description, content);
        }
    }

    if (doc.context.title.empty())
    {
        doc.context.title = "Untitled OCCT problem";
    }
    return doc;
}

std::string ProblemDocumentImporter::toMarkdown(const ProblemDocument& document)
{
    const ProblemContext& context = document.context;

    std::ostringstream out;
    out << "# " << (context.title.empty() ? "Untitled OCCT problem" : context.title) << "\n\n";

    out << "## Summary\n\n";
    if (!context.description.empty())
    {
        out << context.description << "\n";
    }
    out << "\n";

    out << "## Environment\n\n";
    out << "- Category: " << categoryToString(context.category) << "\n";
    out << "- OCCT Version: " << context.occtVersion << "\n";
    out << "- Compiler: " << context.compiler << "\n";
    out << "- Build Type: " << context.buildType << "\n";
    for (const auto& item : context.parameters)
    {
        const std::string key = normalizeHeading(item.first);
        if (key == "reproductionsteps" || key == "notes" || key == "environment")
        {
            continue;
        }
        out << "- " << item.first << ": " << item.second << "\n";
    }
    out << "\n";

    out << "## Input Files\n\n";
    for (const std::string& input : context.inputFiles)
    {
        if (!trim(input).empty())
        {
            out << "- " << input << "\n";
        }
    }
    out << "\n";

    out << "## Reproduction Steps\n\n";
    if (!document.reproductionSteps.empty())
    {
        out << document.reproductionSteps << "\n";
    }
    out << "\n";

    out << "## Expected Behavior\n\n";
    if (!context.expectedBehavior.empty())
    {
        out << context.expectedBehavior << "\n";
    }
    out << "\n";

    out << "## Actual Behavior\n\n";
    if (!context.actualBehavior.empty())
    {
        out << context.actualBehavior << "\n";
    }
    out << "\n";

    out << "## Notes / Suspected Area\n\n";
    if (!document.notes.empty())
    {
        out << document.notes << "\n";
    }

    return out.str();
}
