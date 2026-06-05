#include "core/repro/ReproStatusEvaluator.h"

#include <QCoreApplication>
#include <QTextStream>

namespace
{
bool expect(bool condition, const char* message)
{
    if (!condition)
    {
        QTextStream(stderr) << message << '\n';
        return false;
    }
    return true;
}
} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    Q_UNUSED(app);

    occtdebug::ReproStatus status;

    occtdebug::CommandResult draw;
    draw.exitCode = 0;
    occtdebug::DrawLogAnalysis analysis;
    analysis.checkshapeStatus = QStringLiteral("valid");
    status = occtdebug::ReproStatusEvaluator::withDrawResult(
        status,
        draw,
        analysis,
        QStringLiteral("2026-06-05T00:00:00Z"));
    bool ok = expect(status.draw == QStringLiteral("passed"), "draw status should pass")
        && expect(status.overall == QStringLiteral("reproduced"), "draw pass should mark reproduced");

    occtdebug::CppReproTemplateResult cpp;
    cpp.success = true;
    cpp.rootDirectory = QStringLiteral("repro/cpp_minimal");
    cpp.writtenFiles = {QStringLiteral("repro/cpp_minimal/main.cpp")};
    status = occtdebug::ReproStatusEvaluator::withCppScaffold(
        status,
        cpp,
        QStringLiteral("2026-06-05T00:00:01Z"));
    ok = expect(status.cpp == QStringLiteral("generated"), "cpp scaffold should be generated") && ok;
    ok = expect(status.overall == QStringLiteral("reproduced"), "cpp scaffold should not override draw repro") && ok;

    occtdebug::TestgridResultWriterResult blocked;
    blocked.gatePassed = false;
    status = occtdebug::ReproStatusEvaluator::withTestgridResult(
        status,
        blocked,
        QStringLiteral("2026-06-05T00:00:02Z"));
    ok = expect(status.testgrid == QStringLiteral("blocked"), "failed gate should block testgrid") && ok;
    ok = expect(status.overall == QStringLiteral("blocked"), "blocked testgrid should block overall") && ok;

    occtdebug::TestgridResultWriterResult passed;
    passed.gatePassed = true;
    passed.commandExecuted = true;
    passed.rows = {{QStringLiteral("Modeling"), QStringLiteral("1"), QStringLiteral("1"), QStringLiteral("0"), QStringLiteral("100%")}};
    status = occtdebug::ReproStatusEvaluator::withTestgridResult(
        status,
        passed,
        QStringLiteral("2026-06-05T00:00:03Z"));
    ok = expect(status.testgrid == QStringLiteral("passed"), "testgrid should pass") && ok;
    ok = expect(status.overall == QStringLiteral("passed"), "draw plus testgrid should pass overall") && ok;

    draw.timedOut = true;
    status = occtdebug::ReproStatusEvaluator::withDrawResult(
        status,
        draw,
        analysis,
        QStringLiteral("2026-06-05T00:00:04Z"));
    ok = expect(status.draw == QStringLiteral("timed_out"), "timed out draw should be explicit") && ok;
    ok = expect(status.overall == QStringLiteral("timed_out"), "timed out draw should drive overall") && ok;

    if (ok)
    {
        QTextStream(stdout) << "REPRO_STATUS_EVALUATOR_SMOKE_OK\n";
    }
    return ok ? 0 : 1;
}
