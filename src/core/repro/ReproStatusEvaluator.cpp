#include "core/repro/ReproStatusEvaluator.h"

#include <QStringList>

#include <utility>

namespace occtdebug
{
namespace
{
bool isEmptyOrUnknown(const QString& status)
{
    return status.trimmed().isEmpty() || status == QStringLiteral("unknown");
}

QString normalized(const QString& status)
{
    return isEmptyOrUnknown(status) ? QStringLiteral("unknown") : status.trimmed();
}
} // namespace

QString ReproStatusEvaluator::commandStatus(const CommandResult& result)
{
    if (result.timedOut)
    {
        return QStringLiteral("timed_out");
    }
    if (result.canceled)
    {
        return QStringLiteral("canceled");
    }
    return result.exitCode == 0 ? QStringLiteral("passed") : QStringLiteral("failed");
}

ReproStatus ReproStatusEvaluator::withDrawResult(ReproStatus current,
                                                 const CommandResult& result,
                                                 const DrawLogAnalysis& analysis,
                                                 const QString& updatedAt)
{
    current.draw = commandStatus(result);
    current.updatedAt = updatedAt;
    current.summary = QStringLiteral("draw=%1 checkshape=%2 errors=%3")
        .arg(current.draw,
             analysis.checkshapeStatus,
             QString::number(analysis.errorLines.size()));
    return recomputeOverall(std::move(current));
}

ReproStatus ReproStatusEvaluator::withCppScaffold(ReproStatus current,
                                                  const CppReproTemplateResult& result,
                                                  const QString& updatedAt)
{
    current.cpp = result.success ? QStringLiteral("generated") : QStringLiteral("failed");
    current.updatedAt = updatedAt;
    current.summary = result.success
        ? QStringLiteral("cpp=generated files=%1 root=%2").arg(result.writtenFiles.size()).arg(result.rootDirectory)
        : QStringLiteral("cpp=failed %1").arg(result.error.left(180));
    return recomputeOverall(std::move(current));
}

ReproStatus ReproStatusEvaluator::withTestgridResult(ReproStatus current,
                                                     const TestgridResultWriterResult& result,
                                                     const QString& updatedAt)
{
    if (!result.gatePassed)
    {
        current.testgrid = QStringLiteral("blocked");
    }
    else if (result.commandExecuted && !result.failureDetails.isEmpty())
    {
        current.testgrid = QStringLiteral("failed");
    }
    else
    {
        current.testgrid = QStringLiteral("passed");
    }
    current.updatedAt = updatedAt;
    current.summary = QStringLiteral("testgrid=%1 gate=%2 runner=%3 rows=%4 failures=%5")
        .arg(current.testgrid,
             result.gatePassed ? QStringLiteral("passed") : QStringLiteral("failed"),
             result.commandExecuted ? QStringLiteral("executed") : QStringLiteral("skipped"),
             QString::number(result.rows.size()),
             QString::number(result.failureDetails.size()));
    return recomputeOverall(std::move(current));
}

ReproStatus ReproStatusEvaluator::recomputeOverall(ReproStatus status)
{
    status.draw = normalized(status.draw);
    status.cpp = normalized(status.cpp);
    status.testgrid = normalized(status.testgrid);

    const QStringList hardStops {
        QStringLiteral("timed_out"),
        QStringLiteral("canceled"),
        QStringLiteral("failed"),
    };
    for (const QString& terminal : hardStops)
    {
        if (status.draw == terminal || status.testgrid == terminal)
        {
            status.overall = terminal;
            return status;
        }
    }

    if (status.testgrid == QStringLiteral("blocked"))
    {
        status.overall = QStringLiteral("blocked");
        return status;
    }

    if (status.draw == QStringLiteral("passed") && status.testgrid == QStringLiteral("passed"))
    {
        status.overall = QStringLiteral("passed");
        return status;
    }

    if (status.draw == QStringLiteral("passed"))
    {
        status.overall = QStringLiteral("reproduced");
        return status;
    }

    if (status.cpp == QStringLiteral("generated"))
    {
        status.overall = QStringLiteral("prepared");
        return status;
    }

    status.overall = QStringLiteral("incomplete");
    return status;
}
} // namespace occtdebug
