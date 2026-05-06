#pragma once

#include "core/DiagnosticFinding.h"
#include "core/ProblemContext.h"
#include "core/ShapeDocument.h"

#include <memory>
#include <string>
#include <vector>

class IDiagnosticRule
{
public:
    virtual ~IDiagnosticRule() = default;

    virtual std::string id() const = 0;
    virtual std::string name() const = 0;
    virtual ProblemCategory category() const = 0;

    virtual bool isApplicable(const ProblemContext& context, const ShapeDocument& document) const = 0;

    virtual std::vector<DiagnosticFinding> run(
        const ProblemContext& context,
        const ShapeDocument& document) const = 0;
};
