#pragma once

#include "core/case/CaseManifest.h"
#include "core/repro/CppReproTemplateWriter.h"
#include "core/repro/DrawLogParser.h"
#include "core/runner/CommandRunner.h"
#include "core/verify/TestgridResultWriter.h"

#include <QString>

namespace occtdebug
{
class ReproStatusEvaluator
{
public:
    static QString commandStatus(const CommandResult& result);
    static ReproStatus withDrawResult(ReproStatus current,
                                      const CommandResult& result,
                                      const DrawLogAnalysis& analysis,
                                      const QString& updatedAt);
    static ReproStatus withCppScaffold(ReproStatus current,
                                       const CppReproTemplateResult& result,
                                       const QString& updatedAt);
    static ReproStatus withTestgridResult(ReproStatus current,
                                          const TestgridResultWriterResult& result,
                                          const QString& updatedAt);
    static ReproStatus recomputeOverall(ReproStatus status);
};
} // namespace occtdebug
