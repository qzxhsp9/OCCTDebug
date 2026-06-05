#include "workbench/WorkbenchWindow.h"

#include "core/geometry/OcctViewerWidget.h"
#include "core/case/CrashDumpArchive.h"
#include "core/geometry/TopologyCompareArtifact.h"
#include "core/knowledge/SimilarCaseSearch.h"
#include "core/report/MarkdownReportGenerator.h"
#include "core/repro/CppReproTemplateWriter.h"
#include "core/repro/DrawLogParser.h"
#include "core/repro/ReproStatusEvaluator.h"
#include "core/source/SourceIndex.h"
#include "core/verify/VerificationResultParser.h"
#include "core/verify/TestdiffAdapterResultWriter.h"
#include "core/verify/TestdiffCommandPlanner.h"
#include "core/verify/TestgridArtifactService.h"
#include "core/verify/TestgridResultWriter.h"
#include "core/verify/TwoStageFinalResultBuilder.h"
#include "core/verify/TwoStageFinalResultWriter.h"
#include "core/verify/TwoStagePhaseResultWriter.h"
#include "core/verify/TwoStageVerificationResultWriter.h"
#include "workbench/CaseManifestSync.h"
#include "workbench/CasePanel.h"
#include "workbench/DiffPanel.h"
#include "workbench/EvidenceCoordinator.h"
#include "workbench/EvidencePanel.h"
#include "workbench/ReportRefreshCoordinator.h"
#include "workbench/SourcePanel.h"
#include "workbench/TaskHistoryPanel.h"
#include "workbench/TestdiffAdapterResultCoordinator.h"
#include "workbench/TestgridTablePresenter.h"
#include "workbench/TwoStageFinalResultCoordinator.h"
#include "workbench/TwoStageFinalResultUiAdapter.h"
#include "workbench/VerificationPanel.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QComboBox>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QMessageBox>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QSplitter>
#include <QStringList>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextEdit>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>

#include <algorithm>
#include <utility>

namespace
{
QString s(const char* text)
{
    return QString::fromUtf8(text);
}

QLabel* label(const QString& text, const QString& objectName = QString())
{
    auto* out = new QLabel(text);
    if (!objectName.isEmpty())
    {
        out->setObjectName(objectName);
    }
    out->setWordWrap(true);
    return out;
}

QTableWidgetItem* item(const QString& text)
{
    auto* out = new QTableWidgetItem(text);
    out->setFlags(out->flags() & ~Qt::ItemIsEditable);
    return out;
}

void setMargins(QLayout* layout, int margin = 10, int spacing = 8)
{
    layout->setContentsMargins(margin, margin, margin, margin);
    layout->setSpacing(spacing);
}

bool writeTextFile(const QString& path, const QString& text, QString* error = nullptr)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("failed to write %1: %2").arg(path, file.errorString());
        }
        return false;
    }
    file.write(text.toUtf8());
    return true;
}

QString fileSha256Hex(const QString& path, QString* error = nullptr)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("cannot hash %1: %2").arg(path, file.errorString());
        }
        return QString();
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&file))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("cannot read %1 for hashing").arg(path);
        }
        return QString();
    }
    return QString::fromLatin1(hash.result().toHex());
}

QString readTextFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return QString();
    }
    return QString::fromUtf8(file.readAll());
}

QString boolText(bool value)
{
    return value ? QStringLiteral("yes") : QStringLiteral("no");
}

constexpr int kDrawCommandTimeoutMs = 120000;
constexpr int kEnvironmentCommandTimeoutMs = 180000;
constexpr int kReproPackCommandTimeoutMs = 120000;
constexpr int kVerificationCommandTimeoutMs = 600000;
constexpr int kPatchCommandTimeoutMs = 120000;

QString commandOutcomeText(const occtdebug::CommandResult& result)
{
    if (result.timedOut)
    {
        return QStringLiteral("timed_out");
    }
    if (result.canceled)
    {
        return QStringLiteral("canceled");
    }
    return !result.canceled && !result.timedOut && result.exitCode == 0
        ? QStringLiteral("passed")
        : QStringLiteral("failed");
}

QString currentUtcIsoTimestamp()
{
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
}

QString timeoutSuffix(int timeoutMs)
{
    return timeoutMs > 0 ? QStringLiteral(" timeout=%1ms").arg(timeoutMs) : QString();
}

QStringList standardCaseDirectories()
{
    return {
        QStringLiteral("input"),
        QStringLiteral("repro"),
        QStringLiteral("env"),
        QStringLiteral("logs"),
        QStringLiteral("artifacts"),
        QStringLiteral("report"),
        QStringLiteral("verification"),
    };
}

QString sanitizedCaseId(QString value)
{
    value = value.trimmed();
    value.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9_.-]")), QStringLiteral("_"));
    return value.isEmpty() ? QStringLiteral("OCC-LOCAL-UNNAMED") : value;
}

void clearLayout(QLayout* layout)
{
    if (layout == nullptr)
    {
        return;
    }

    while (QLayoutItem* child = layout->takeAt(0))
    {
        if (QWidget* widget = child->widget())
        {
            widget->deleteLater();
        }
        delete child;
    }
}

QString centerTabIdForIndex(int index)
{
    static const QStringList ids {
        QStringLiteral("source"),
        QStringLiteral("repro"),
        QStringLiteral("geometry"),
        QStringLiteral("evidence"),
        QStringLiteral("diff"),
        QStringLiteral("environment"),
    };
    return (index >= 0 && index < ids.size()) ? ids[index] : QStringLiteral("source");
}

int centerTabIndexForId(const QString& id)
{
    const QString value = id.trimmed().toLower();
    if (value == QStringLiteral("repro") || value == QString::fromUtf8("婢跺秶骞囬懘姘拱"))
    {
        return 1;
    }
    if (value == QStringLiteral("geometry") || value == QString::fromUtf8("閸戠姳缍嶇憴鍡楁禈"))
    {
        return 2;
    }
    if (value == QStringLiteral("evidence") || value == QStringLiteral("evidence_chain"))
    {
        return 3;
    }
    if (value == QStringLiteral("diff") || value == QStringLiteral("diff_compare"))
    {
        return 4;
    }
    if (value == QStringLiteral("environment") || value == QString::fromUtf8("閻滎垰顣ㄦ穱鈩冧紖"))
    {
        return 5;
    }
    return 0;
}

QString bottomTabIdForIndex(int index)
{
    static const QStringList ids {
        QStringLiteral("draw"),
        QStringLiteral("cmake"),
        QStringLiteral("testgrid"),
        QStringLiteral("tasks"),
    };
    return (index >= 0 && index < ids.size()) ? ids[index] : QStringLiteral("draw");
}

int bottomTabIndexForId(const QString& id)
{
    const QString value = id.trimmed().toLower();
    if (value == QStringLiteral("cmake") || value.contains(QStringLiteral("cmake")))
    {
        return 1;
    }
    if (value == QStringLiteral("testgrid") || value.contains(QStringLiteral("testgrid")))
    {
        return 2;
    }
    if (value == QStringLiteral("tasks") || value.contains(QStringLiteral("task")))
    {
        return 3;
    }
    return 0;
}

struct EvidenceLinkTarget
{
    QString path;
    int line = 0;
};

EvidenceLinkTarget splitTrailingLineReference(const QString& raw)
{
    const QString value = raw.trimmed();
    const QRegularExpression withLine(QStringLiteral("^(.+):(\\d+)$"));
    const QRegularExpressionMatch match = withLine.match(value);
    if (!match.hasMatch())
    {
        return {value, 0};
    }
    return {match.captured(1).trimmed(), match.captured(2).toInt()};
}

bool isRelativeLocalReference(const QString& value)
{
    return !value.trimmed().isEmpty()
        && !QFileInfo(value).isAbsolute()
        && !value.contains(QStringLiteral("://"));
}

bool isCaseArtifactReference(const QString& value)
{
    const QString normalized = value.trimmed().replace(QLatin1Char('\\'), QLatin1Char('/')).toLower();
    return normalized.startsWith(QStringLiteral("logs/"))
        || normalized.startsWith(QStringLiteral("artifacts/"))
        || normalized.startsWith(QStringLiteral("report/"))
        || normalized.startsWith(QStringLiteral("verification/"))
        || normalized.startsWith(QStringLiteral("env/"))
        || normalized.startsWith(QStringLiteral("input/"))
        || normalized.startsWith(QStringLiteral("repro/"))
        || normalized.startsWith(QStringLiteral("evidence/"));
}

QString caseRelativeOrFileName(const QString& workspaceRoot, const QString& path)
{
    const QFileInfo info(path);
    if (path.trimmed().isEmpty())
    {
        return {};
    }
    if (workspaceRoot.trimmed().isEmpty())
    {
        return info.fileName();
    }

    const QDir workspace(workspaceRoot);
    QString relative = workspace.relativeFilePath(info.absoluteFilePath()).replace(QLatin1Char('\\'), QLatin1Char('/'));
    if (!relative.startsWith(QStringLiteral("../")) && relative != QStringLiteral("..") && !QFileInfo(relative).isAbsolute())
    {
        return relative;
    }
    return info.fileName();
}

void scrollTextEditToLine(QTextEdit* edit, int lineNumber)
{
    if (edit == nullptr || lineNumber <= 0 || edit->document() == nullptr)
    {
        return;
    }

    QTextCursor cursor(edit->document());
    cursor.movePosition(QTextCursor::Start);
    for (int line = 1; line < lineNumber && cursor.movePosition(QTextCursor::Down); ++line)
    {
    }
    cursor.select(QTextCursor::LineUnderCursor);
    edit->setTextCursor(cursor);
    edit->ensureCursorVisible();
}

int firstDrawErrorLine(const QString& text)
{
    const QStringList lines = text.split(QRegularExpression(QStringLiteral("\\r?\\n")));
    const QRegularExpression errorPattern(
        QStringLiteral("Exception|Faulty|invalid command|DRAW_SMOKE_FAILED|DRAW_SMOKE_ERROR|(^|\\s)Error(:|\\s|$)"),
        QRegularExpression::CaseInsensitiveOption);
    for (int index = 0; index < lines.size(); ++index)
    {
        if (errorPattern.match(lines[index]).hasMatch())
        {
            return index + 1;
        }
    }
    return 0;
}

QJsonArray testgridComparisonRowsToJson(const occtdebug::TestgridComparison& comparison)
{
    QJsonArray rows;
    for (const occtdebug::TestgridComparisonRow& row : comparison.rows)
    {
        rows.append(QJsonObject {
            {QStringLiteral("module"), row.module},
            {QStringLiteral("before_run_count"), row.beforeRunCount},
            {QStringLiteral("before_pass_count"), row.beforePassCount},
            {QStringLiteral("before_fail_count"), row.beforeFailCount},
            {QStringLiteral("after_run_count"), row.afterRunCount},
            {QStringLiteral("after_pass_count"), row.afterPassCount},
            {QStringLiteral("after_fail_count"), row.afterFailCount},
            {QStringLiteral("pass_delta"), row.passDelta},
            {QStringLiteral("fail_delta"), row.failDelta},
            {QStringLiteral("status"), row.status},
        });
    }
    return rows;
}

QJsonObject testgridComparisonToJson(const occtdebug::TestgridComparison& comparison,
                                     const QString& beforePath,
                                     const QString& afterPath,
                                     const QString& workspaceRoot)
{
    const bool available = comparison.isAvailable();
    return QJsonObject {
        {QStringLiteral("available"), available},
        {QStringLiteral("status"), !available ? QStringLiteral("unavailable") : (comparison.hasRegression() ? QStringLiteral("regressed") : QStringLiteral("not_regressed"))},
        {QStringLiteral("summary"), comparison.summaryText()},
        {QStringLiteral("before_run_total"), comparison.beforeRunTotal},
        {QStringLiteral("before_pass_total"), comparison.beforePassTotal},
        {QStringLiteral("before_fail_total"), comparison.beforeFailTotal},
        {QStringLiteral("after_run_total"), comparison.afterRunTotal},
        {QStringLiteral("after_pass_total"), comparison.afterPassTotal},
        {QStringLiteral("after_fail_total"), comparison.afterFailTotal},
        {QStringLiteral("pass_delta"), comparison.passDelta},
        {QStringLiteral("fail_delta"), comparison.failDelta},
        {QStringLiteral("before_summary"), beforePath.isEmpty() ? QString() : QDir(workspaceRoot).relativeFilePath(beforePath)},
        {QStringLiteral("after_summary"), afterPath.isEmpty() ? QString() : QDir(workspaceRoot).relativeFilePath(afterPath)},
        {QStringLiteral("rows"), testgridComparisonRowsToJson(comparison)},
    };
}

QJsonArray patchConflictHints(const QString& text)
{
    QJsonArray conflicts;
    const QStringList lines = text.split(QRegularExpression(QStringLiteral("\\r?\\n")));
    const QRegularExpression failedAt(QStringLiteral("patch failed:\\s*([^:]+):(\\d+)"), QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression interesting(QStringLiteral("error:|failed|does not apply|cannot apply|No such file"), QRegularExpression::CaseInsensitiveOption);
    for (const QString& rawLine : lines)
    {
        const QString line = rawLine.trimmed();
        if (line.isEmpty() || !interesting.match(line).hasMatch())
        {
            continue;
        }

        QJsonObject conflict {
            {QStringLiteral("message"), line},
        };
        const QRegularExpressionMatch match = failedAt.match(line);
        if (match.hasMatch())
        {
            conflict.insert(QStringLiteral("source_file"), match.captured(1).trimmed());
            conflict.insert(QStringLiteral("source_line"), match.captured(2).toInt());
        }
        conflicts.append(conflict);
    }
    return conflicts;
}

bool commandSucceeded(const occtdebug::CommandResult& result)
{
    return !result.canceled
        && !result.timedOut
        && result.exitCode == 0;
}

QJsonObject readJsonObject(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return {};
    }
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    return document.isObject() ? document.object() : QJsonObject {};
}
} // namespace

WorkbenchWindow::WorkbenchWindow(QWidget* parent)
    : WorkbenchWindow(occtdebug::createMockWorkbenchData(), parent)
{
}

WorkbenchWindow::WorkbenchWindow(const occtdebug::WorkbenchMockData& initialData, QWidget* parent)
    : QMainWindow(parent)
    , m_data(initialData)
    , m_patchReview(occtdebug::PatchReviewWorkflow::createDefault(m_data.caseId, m_data.patchDiff))
{
    m_drawRunner = new occtdebug::CommandRunner(this);
    m_envRunner = new occtdebug::CommandRunner(this);
    m_packRunner = new occtdebug::CommandRunner(this);
    m_testgridRunner = new occtdebug::CommandRunner(this);
    m_patchRunner = new occtdebug::CommandRunner(this);
    if (m_data.patchReviewStatus == QStringLiteral("Needs review"))
    {
        m_patchReview.markNeedsReview(QStringLiteral("Restored from case manifest."));
    }
    else if (m_data.patchReviewStatus == QStringLiteral("Approved"))
    {
        m_patchReview.approve(QStringLiteral("Restored from case manifest."));
    }
    else if (m_data.patchReviewStatus == QStringLiteral("Rejected"))
    {
        m_patchReview.reject(QStringLiteral("Restored from case manifest."));
    }
    connect(m_drawRunner, &occtdebug::CommandRunner::outputReceived, this, [this](const QString& text) {
        if (m_drawConsole != nullptr)
        {
            m_drawConsole->moveCursor(QTextCursor::End);
            m_drawConsole->insertPlainText(text);
        }
    });
    connect(m_drawRunner, &occtdebug::CommandRunner::errorOutputReceived, this, [this](const QString& text) {
        if (m_drawConsole != nullptr)
        {
            m_drawConsole->moveCursor(QTextCursor::End);
            m_drawConsole->insertPlainText(text);
        }
    });
    connect(m_drawRunner, &occtdebug::CommandRunner::finished, this, [this](const occtdebug::CommandResult& result) {
        if (m_drawConsole != nullptr)
        {
            m_drawConsole->append(QStringLiteral("\n[DRAW finished] status=%1 exit=%2 elapsed=%3ms")
                    .arg(commandOutcomeText(result))
                    .arg(result.exitCode)
                    .arg(result.elapsedMs));
        }
        recordTaskFinished(QStringLiteral("draw"),
                           result,
                           QStringLiteral("artifacts/draw_result.json"),
                           QStringLiteral("logs/draw.stdout.log"),
                           QStringLiteral("logs/draw.stderr.log"));
        persistDrawRunResult(result);
    });
    connect(m_envRunner, &occtdebug::CommandRunner::outputReceived, this, [this](const QString& text) {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->moveCursor(QTextCursor::End);
            m_cmakeConsole->insertPlainText(text);
        }
    });
    connect(m_envRunner, &occtdebug::CommandRunner::errorOutputReceived, this, [this](const QString& text) {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->moveCursor(QTextCursor::End);
            m_cmakeConsole->insertPlainText(text);
        }
    });
    connect(m_envRunner, &occtdebug::CommandRunner::finished, this, [this](const occtdebug::CommandResult& result) {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("\n[env] finished status=%1 exit=%2 elapsed=%3ms")
                    .arg(commandOutcomeText(result))
                    .arg(result.exitCode)
                    .arg(result.elapsedMs));
        }
        recordTaskFinished(QStringLiteral("env"),
                           result,
                           QStringLiteral("artifacts/env_capture_result.json"),
                           QStringLiteral("logs/env_capture.stdout.log"),
                           QStringLiteral("logs/env_capture.stderr.log"),
                           QStringLiteral("snapshot=env/env_snapshot.json"));
        persistEnvironmentCaptureResult(result);
    });
    connect(m_packRunner, &occtdebug::CommandRunner::outputReceived, this, [this](const QString& text) {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->moveCursor(QTextCursor::End);
            m_cmakeConsole->insertPlainText(text);
        }
    });
    connect(m_packRunner, &occtdebug::CommandRunner::errorOutputReceived, this, [this](const QString& text) {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->moveCursor(QTextCursor::End);
            m_cmakeConsole->insertPlainText(text);
        }
    });
    connect(m_packRunner, &occtdebug::CommandRunner::finished, this, [this](const occtdebug::CommandResult& result) {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("\n[pack] finished status=%1 exit=%2 elapsed=%3ms")
                    .arg(commandOutcomeText(result))
                    .arg(result.exitCode)
                    .arg(result.elapsedMs));
        }
        recordTaskFinished(QStringLiteral("repro_pack"),
                           result,
                           QStringLiteral("artifacts/repro_pack_result.json"),
                           QStringLiteral("logs/repro_pack.stdout.log"),
                           QStringLiteral("logs/repro_pack.stderr.log"));
        persistReproPackResult(result);
    });
    connect(m_testgridRunner, &occtdebug::CommandRunner::outputReceived, this, [this](const QString& text) {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->moveCursor(QTextCursor::End);
            m_cmakeConsole->insertPlainText(text);
        }
    });
    connect(m_testgridRunner, &occtdebug::CommandRunner::errorOutputReceived, this, [this](const QString& text) {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->moveCursor(QTextCursor::End);
            m_cmakeConsole->insertPlainText(text);
        }
    });
    connect(m_testgridRunner, &occtdebug::CommandRunner::finished, this, [this](const occtdebug::CommandResult& result) {
        const auto finishCurrentTask = [this, &result]() {
            QString taskId = QStringLiteral("testgrid.draw_gate");
            QString artifact = QStringLiteral("artifacts/testgrid_result.json");
            QString stdoutLog = QStringLiteral("logs/testgrid_gate.stdout.log");
            QString stderrLog = QStringLiteral("logs/testgrid_gate.stderr.log");
            switch (m_testgridRunPhase)
            {
            case TestgridRunPhase::DrawGate:
                break;
            case TestgridRunPhase::TestgridCommand:
                taskId = QStringLiteral("testgrid.command");
                stdoutLog = QStringLiteral("logs/testgrid.stdout.log");
                stderrLog = QStringLiteral("logs/testgrid.stderr.log");
                break;
            case TestgridRunPhase::TestdiffGate:
                taskId = QStringLiteral("testdiff.draw_gate");
                artifact = QStringLiteral("artifacts/testdiff_adapter_result.json");
                break;
            case TestgridRunPhase::TestdiffCommand:
                taskId = QStringLiteral("testdiff.command");
                artifact = QStringLiteral("artifacts/testdiff_adapter_result.json");
                stdoutLog = QStringLiteral("logs/testdiff_runner.stdout.log");
                stderrLog = QStringLiteral("logs/testdiff_runner.stderr.log");
                break;
            case TestgridRunPhase::TwoStageBeforeGate:
            case TestgridRunPhase::TwoStageAfterGate:
            {
                const QString phase = m_testgridRunPhase == TestgridRunPhase::TwoStageBeforeGate
                    ? QStringLiteral("before")
                    : QStringLiteral("after");
                taskId = QStringLiteral("two_stage.%1.draw_gate").arg(phase);
                artifact = QStringLiteral("artifacts/testgrid_%1_result.json").arg(phase);
                stdoutLog = QStringLiteral("logs/testgrid_%1_gate.stdout.log").arg(phase);
                stderrLog = QStringLiteral("logs/testgrid_%1_gate.stderr.log").arg(phase);
                break;
            }
            case TestgridRunPhase::TwoStageBeforeCommand:
            case TestgridRunPhase::TwoStageAfterCommand:
            {
                const QString phase = m_testgridRunPhase == TestgridRunPhase::TwoStageBeforeCommand
                    ? QStringLiteral("before")
                    : QStringLiteral("after");
                taskId = QStringLiteral("two_stage.%1.command").arg(phase);
                artifact = QStringLiteral("artifacts/testgrid_%1_result.json").arg(phase);
                stdoutLog = QStringLiteral("logs/testgrid_%1.stdout.log").arg(phase);
                stderrLog = QStringLiteral("logs/testgrid_%1.stderr.log").arg(phase);
                break;
            }
            case TestgridRunPhase::TwoStagePatchApply:
            case TestgridRunPhase::TwoStagePatchUndo:
            case TestgridRunPhase::Idle:
                break;
            }
            recordTaskFinished(taskId, result, artifact, stdoutLog, stderrLog);
        };

        if (result.canceled)
        {
            if (m_cmakeConsole != nullptr)
            {
                m_cmakeConsole->append(QStringLiteral("[testgrid] canceled elapsed=%1ms").arg(result.elapsedMs));
            }
            finishCurrentTask();
            m_testgridRunPhase = TestgridRunPhase::Idle;
            saveCurrentCaseManifest();
            return;
        }

        if (m_testgridRunPhase == TestgridRunPhase::DrawGate)
        {
            if (m_cmakeConsole != nullptr)
            {
                m_cmakeConsole->append(QStringLiteral("\n[testgrid] draw_smoke gate finished status=%1 exit=%2 elapsed=%3ms")
                        .arg(commandOutcomeText(result))
                        .arg(result.exitCode)
                        .arg(result.elapsedMs));
            }
            finishCurrentTask();
            handleTestgridGateFinished(result);
            return;
        }

        if (m_testgridRunPhase == TestgridRunPhase::TestgridCommand)
        {
            if (m_cmakeConsole != nullptr)
            {
                m_cmakeConsole->append(QStringLiteral("\n[testgrid] configured command finished status=%1 exit=%2 elapsed=%3ms")
                        .arg(commandOutcomeText(result))
                        .arg(result.exitCode)
                        .arg(result.elapsedMs));
            }
            finishCurrentTask();
            handleTestgridCommandFinished(result);
            return;
        }

        if (m_testgridRunPhase == TestgridRunPhase::TestdiffGate)
        {
            if (m_cmakeConsole != nullptr)
            {
                m_cmakeConsole->append(QStringLiteral("\n[testdiff] draw_smoke gate finished status=%1 exit=%2 elapsed=%3ms")
                        .arg(commandOutcomeText(result))
                        .arg(result.exitCode)
                        .arg(result.elapsedMs));
            }
            finishCurrentTask();
            handleTestdiffGateFinished(result);
            return;
        }

        if (m_testgridRunPhase == TestgridRunPhase::TestdiffCommand)
        {
            if (m_cmakeConsole != nullptr)
            {
                m_cmakeConsole->append(QStringLiteral("\n[testdiff] configured command finished status=%1 exit=%2 elapsed=%3ms")
                        .arg(commandOutcomeText(result))
                        .arg(result.exitCode)
                        .arg(result.elapsedMs));
            }
            finishCurrentTask();
            handleTestdiffCommandFinished(result);
            return;
        }

        if (m_testgridRunPhase == TestgridRunPhase::TwoStageBeforeGate
            || m_testgridRunPhase == TestgridRunPhase::TwoStageAfterGate)
        {
            if (m_cmakeConsole != nullptr)
            {
                m_cmakeConsole->append(QStringLiteral("\n[testgrid] two-stage gate finished status=%1 exit=%2 elapsed=%3ms")
                        .arg(commandOutcomeText(result))
                        .arg(result.exitCode)
                        .arg(result.elapsedMs));
            }
            finishCurrentTask();
            handleTwoStageGateFinished(result);
            return;
        }

        if (m_testgridRunPhase == TestgridRunPhase::TwoStageBeforeCommand
            || m_testgridRunPhase == TestgridRunPhase::TwoStageAfterCommand)
        {
            if (m_cmakeConsole != nullptr)
            {
                m_cmakeConsole->append(QStringLiteral("\n[testgrid] two-stage command finished status=%1 exit=%2 elapsed=%3ms")
                        .arg(commandOutcomeText(result))
                        .arg(result.exitCode)
                        .arg(result.elapsedMs));
            }
            finishCurrentTask();
            handleTwoStageCommandFinished(result);
            return;
        }

        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[testgrid] finished while no run phase was active"));
        }
    });
    connect(m_patchRunner, &occtdebug::CommandRunner::outputReceived, this, [this](const QString& text) {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->moveCursor(QTextCursor::End);
            m_cmakeConsole->insertPlainText(text);
        }
    });
    connect(m_patchRunner, &occtdebug::CommandRunner::errorOutputReceived, this, [this](const QString& text) {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->moveCursor(QTextCursor::End);
            m_cmakeConsole->insertPlainText(text);
        }
    });
    connect(m_patchRunner, &occtdebug::CommandRunner::finished, this, [this](const occtdebug::CommandResult& result) {
        const auto finishPatchTask = [this, &result]() {
            QString id = QStringLiteral("patch.apply");
            QString artifact = QStringLiteral("artifacts/patch_apply_result.json");
            QString stdoutLog = QStringLiteral("logs/patch_apply.stdout.log");
            QString stderrLog = QStringLiteral("logs/patch_apply.stderr.log");
            if (m_patchRunMode == PatchRunMode::Generate)
            {
                id = QStringLiteral("patch.generate");
                artifact = QStringLiteral("artifacts/patch_generate_result.json");
                stdoutLog = QStringLiteral("logs/patch_generate.stdout.log");
                stderrLog = QStringLiteral("logs/patch_generate.stderr.log");
            }
            else if (m_patchRunMode == PatchRunMode::Undo)
            {
                id = QStringLiteral("patch.undo");
                artifact = QStringLiteral("artifacts/patch_undo_result.json");
                stdoutLog = QStringLiteral("logs/patch_undo.stdout.log");
                stderrLog = QStringLiteral("logs/patch_undo.stderr.log");
            }
            recordTaskFinished(id, result, artifact, stdoutLog, stderrLog);
        };

        if (result.canceled)
        {
            if (m_cmakeConsole != nullptr)
            {
                m_cmakeConsole->append(QStringLiteral("[patch] canceled elapsed=%1ms").arg(result.elapsedMs));
            }
            finishPatchTask();
            m_patchRunMode = PatchRunMode::None;
            if (m_testgridRunPhase == TestgridRunPhase::TwoStagePatchApply
                || m_testgridRunPhase == TestgridRunPhase::TwoStagePatchUndo)
            {
                m_testgridRunPhase = TestgridRunPhase::Idle;
            }
            saveCurrentCaseManifest();
            return;
        }

        if (m_cmakeConsole != nullptr)
        {
            QString action = QStringLiteral("apply");
            if (m_patchRunMode == PatchRunMode::Undo)
            {
                action = QStringLiteral("undo");
            }
            else if (m_patchRunMode == PatchRunMode::Generate)
            {
                action = QStringLiteral("generate");
            }
            m_cmakeConsole->append(QStringLiteral("\n[patch] %1 finished status=%2 exit=%3 elapsed=%4ms")
                    .arg(action)
                    .arg(commandOutcomeText(result))
                    .arg(result.exitCode)
                    .arg(result.elapsedMs));
        }

        if (m_testgridRunPhase == TestgridRunPhase::TwoStagePatchApply
            || m_testgridRunPhase == TestgridRunPhase::TwoStagePatchUndo)
        {
            finishPatchTask();
            handleTwoStagePatchFinished(result);
            return;
        }

        if (m_patchRunMode == PatchRunMode::Generate)
        {
            finishPatchTask();
            persistPatchCandidateGenerationResult(result);
            m_patchRunMode = PatchRunMode::None;
            return;
        }

        finishPatchTask();
        persistPatchCommandResult(result);
        m_patchRunMode = PatchRunMode::None;
    });

    setWindowTitle(s("OCCT Kernel Expert Workbench"));
    setMinimumSize(1440, 900);
    resize(1680, 960);

    auto* root = new QWidget(this);
    auto* rootLayout = new QVBoxLayout(root);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    rootLayout->addWidget(createTitleBar());
    rootLayout->addWidget(createWorkflowToolbar());

    m_mainSplitter = new QSplitter(Qt::Horizontal);
    m_mainSplitter->setObjectName(QStringLiteral("MainSplitter"));
    m_mainSplitter->addWidget(createLeftColumn());
    m_mainSplitter->addWidget(createCenterWorkspace());
    m_mainSplitter->addWidget(createRightColumn());
    m_mainSplitter->setStretchFactor(0, 0);
    m_mainSplitter->setStretchFactor(1, 1);
    m_mainSplitter->setStretchFactor(2, 0);
    m_mainSplitter->setSizes({
        m_data.manifest.workspaceLayout.leftWidth,
        m_data.manifest.workspaceLayout.centerWidth,
        m_data.manifest.workspaceLayout.rightWidth,
    });
    rootLayout->addWidget(m_mainSplitter, 1);
    rootLayout->addWidget(createBottomConsole());

    setCentralWidget(root);
    applyWorkbenchTheme();
    populateMockCaseData();
}

QWidget* WorkbenchWindow::createTitleBar()
{
    auto* bar = new QFrame;
    bar->setObjectName(QStringLiteral("TitleBar"));
    auto* layout = new QHBoxLayout(bar);
    layout->setContentsMargins(14, 6, 14, 6);
    layout->setSpacing(12);

    auto* icon = label(QStringLiteral("OC"), QStringLiteral("AppIcon"));
    auto* title = label(s("OCCT Kernel Expert Workbench"), QStringLiteral("AppTitle"));
    m_caseBadge = createBadge(QStringLiteral("濡楀牅绶ラ敍?1").arg(m_data.caseId), QStringLiteral("Badge"));
    layout->addWidget(icon);
    layout->addWidget(title);
    layout->addSpacing(12);
    layout->addWidget(m_caseBadge);
    layout->addWidget(createBadge(m_data.occtVersion, QStringLiteral("Badge")));
    layout->addWidget(createBadge(m_data.toolchain, QStringLiteral("Badge")));
    layout->addWidget(createBadge(m_data.platform, QStringLiteral("Badge")));
    layout->addStretch(1);
    m_statusBadge = createBadge(QString::fromUtf8("閳?閻樿埖鈧緤绱?1").arg(m_data.caseStatus), QStringLiteral("SuccessBadge"));
    layout->addWidget(m_statusBadge);
    return bar;
}

QWidget* WorkbenchWindow::createWorkflowToolbar()
{
    auto* bar = new QFrame;
    bar->setObjectName(QStringLiteral("WorkflowToolbar"));
    auto* layout = new QHBoxLayout(bar);
    layout->setContentsMargins(14, 8, 14, 8);
    layout->setSpacing(8);
    layout->addWidget(createToolbarButton(s("閳?闂傤噣顣借ぐ鏇炲弳"), QStringLiteral("ToolButton")));
    layout->addWidget(createToolbarButton(s("閳?婢跺秶骞囬悽鐔稿灇"), QStringLiteral("ToolButton")));
    layout->addWidget(createToolbarButton(s("Problem Intake"), QStringLiteral("PrimaryToolButton")));
    layout->addWidget(createToolbarButton(s("閳?鐞涖儰绔甸弬瑙勵攳"), QStringLiteral("ToolButton")));
    layout->addWidget(createToolbarButton(s("閴?閸ョ偛缍婃宀冪槈"), QStringLiteral("ToolButton")));
    layout->addWidget(createToolbarButton(s("Repro Generate"), QStringLiteral("ToolButton")));
    layout->addStretch(1);
    return bar;
}

QWidget* WorkbenchWindow::createLeftColumn()
{
    m_casePanel = new occtdebug::CasePanel;
    m_casePanel->setData(m_data);
    connect(m_casePanel, &occtdebug::CasePanel::newCaseRequested, this, [this]() {
        createNewCase();
    });
    connect(m_casePanel, &occtdebug::CasePanel::openCaseRequested, this, [this]() {
        openCaseDirectory();
    });
    connect(m_casePanel, &occtdebug::CasePanel::saveCaseRequested, this, [this]() {
        if (saveCurrentCaseManifest() && m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[case] saved %1").arg(m_data.caseId));
        }
    });
    connect(m_casePanel, &occtdebug::CasePanel::refreshRequested, this, [this]() {
        refreshCaseList();
    });
    connect(m_casePanel, &occtdebug::CasePanel::caseActivated, this, [this](const QString& caseId) {
        loadCaseById(caseId);
    });
    return m_casePanel;
}

QWidget* WorkbenchWindow::createCenterWorkspace()
{
    auto* container = new QWidget;
    container->setObjectName(QStringLiteral("CenterWorkspace"));
    auto* layout = new QVBoxLayout(container);
    setMargins(layout, 0, 0);

    m_workspaceTabs = new QTabWidget;
    m_workspaceTabs->setObjectName(QStringLiteral("WorkspaceTabs"));
    m_workspaceTabs->addTab(createSourceTab(), s("Source"));
    m_workspaceTabs->addTab(createReproScriptTab(), s("婢跺秶骞囬懘姘拱"));
    m_workspaceTabs->addTab(createGeometryTab(), s("閸戠姳缍嶇憴鍡楁禈"));
    m_workspaceTabs->addTab(createEvidenceTab(), s("Evidence"));
    m_workspaceTabs->addTab(createDiffTab(), s("Diff"));
    m_workspaceTabs->addTab(createEnvironmentTab(), s("閻滎垰顣ㄦ穱鈩冧紖"));
    m_workspaceTabs->setCurrentIndex(centerTabIndexForId(m_data.manifest.workspaceLayout.activeCenterTab));
    layout->addWidget(m_workspaceTabs);
    return container;
}

QWidget* WorkbenchWindow::createRightColumn()
{
    auto* column = new QWidget;
    column->setObjectName(QStringLiteral("Column"));
    auto* layout = new QVBoxLayout(column);
    setMargins(layout, 10, 8);
    layout->addWidget(createDiagnosisPanel(), 2);
    layout->addWidget(createPatchPanel(), 3);
    layout->addWidget(createPatchReviewPanel(), 1);
    layout->addWidget(createVerificationPanel(), 2);
    layout->addWidget(createSimilarCasesPanel(), 2);
    return column;
}

QWidget* WorkbenchWindow::createBottomConsole()
{
    auto* tabs = new QTabWidget;
    m_bottomTabs = tabs;
    tabs->setObjectName(QStringLiteral("BottomConsole"));
    tabs->setFixedHeight(m_data.manifest.workspaceLayout.bottomHeight > 0
            ? m_data.manifest.workspaceLayout.bottomHeight
            : 235);

    m_drawConsole = new QTextEdit;
    m_drawConsole->setReadOnly(true);
    m_drawConsole->setPlainText(m_data.drawConsoleText);

    m_cmakeConsole = new QTextEdit;
    m_cmakeConsole->setReadOnly(true);
    m_cmakeConsole->setPlainText(m_data.cmakeConsoleText);

    auto* testgridTab = new QWidget;
    auto* testgridLayout = new QVBoxLayout(testgridTab);
    setMargins(testgridLayout, 6, 6);
    auto* testgridActions = new QHBoxLayout;
    setMargins(testgridActions, 0, 0);
    m_testgridGroupEdit = new QLineEdit(m_data.verificationPlan.testgridGroup);
    m_testgridGroupEdit->setPlaceholderText(QStringLiteral("group"));
    m_testgridGridEdit = new QLineEdit(m_data.verificationPlan.testgridGrid);
    m_testgridGridEdit->setPlaceholderText(QStringLiteral("grid"));
    m_testgridCaseEdit = new QLineEdit(m_data.verificationPlan.testgridCase);
    m_testgridCaseEdit->setPlaceholderText(QStringLiteral("case"));
    m_testgridGroupEdit->setClearButtonEnabled(true);
    m_testgridGridEdit->setClearButtonEnabled(true);
    m_testgridCaseEdit->setClearButtonEnabled(true);

    auto* runTestgridButton = new QPushButton(s("鏉╂劘顢戞宀冪槈"));
    connect(runTestgridButton, &QPushButton::clicked, this, [this]() {
        runTestgridVerification();
    });
    auto* runTestdiffButton = new QPushButton(QStringLiteral("Run testdiff"));
    connect(runTestdiffButton, &QPushButton::clicked, this, [this]() {
        runTestdiffAdapter();
    });
    auto* runTwoStageButton = new QPushButton(s("Two-stage verification"));
    connect(runTwoStageButton, &QPushButton::clicked, this, [this]() {
        runTwoStageVerification();
    });
    auto* cancelTestgridButton = new QPushButton(QStringLiteral("Cancel"));
    connect(cancelTestgridButton, &QPushButton::clicked, this, [this]() {
        cancelRunner(m_testgridRunner, QStringLiteral("testgrid"), m_cmakeConsole);
    });
    testgridActions->addWidget(label(QStringLiteral("group"), QStringLiteral("MutedText")));
    testgridActions->addWidget(m_testgridGroupEdit);
    testgridActions->addWidget(label(QStringLiteral("grid"), QStringLiteral("MutedText")));
    testgridActions->addWidget(m_testgridGridEdit);
    testgridActions->addWidget(label(QStringLiteral("case"), QStringLiteral("MutedText")));
    testgridActions->addWidget(m_testgridCaseEdit);
    testgridActions->addWidget(runTestgridButton);
    testgridActions->addWidget(runTestdiffButton);
    testgridActions->addWidget(runTwoStageButton);
    testgridActions->addWidget(cancelTestgridButton);
    testgridActions->addStretch();
    testgridLayout->addLayout(testgridActions);

    m_testgridTable = new QTableWidget(m_data.testgridRows.size(), 5);
    m_testgridTable->setHorizontalHeaderLabels({s("Module"), s("Run"), s("Pass"), s("Fail"), s("Pass Rate")});
    occtdebug::TestgridTablePresenter::applyToTable(m_testgridTable, m_data.testgridRows);
    m_testgridTable->horizontalHeader()->setStretchLastSection(true);
    m_testgridTable->verticalHeader()->hide();
    testgridLayout->addWidget(m_testgridTable, 1);

    tabs->addTab(m_drawConsole, s("DRAW Console"));
    tabs->addTab(m_cmakeConsole, s("PowerShell / CMake"));
    tabs->addTab(testgridTab, s("testgrid Results"));
    m_taskHistoryPanel = new occtdebug::TaskHistoryPanel;
    m_taskHistoryPanel->setTasks(m_data.taskHistory);
    tabs->addTab(m_taskHistoryPanel, QStringLiteral("Tasks"));
    tabs->setCurrentIndex(bottomTabIndexForId(m_data.manifest.workspaceLayout.activeBottomTab));
    return tabs;
}

QWidget* WorkbenchWindow::createPanel(const QString& title, QWidget* body)
{
    auto* frame = new QFrame;
    frame->setObjectName(QStringLiteral("Panel"));
    auto* layout = new QVBoxLayout(frame);
    setMargins(layout, 10, 8);
    auto* heading = label(title, QStringLiteral("PanelTitle"));
    layout->addWidget(heading);
    layout->addWidget(body, 1);
    return frame;
}

QWidget* WorkbenchWindow::createSourceTab()
{
    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->setObjectName(QStringLiteral("SourceSplitter"));

    m_sourcePanel = new occtdebug::SourcePanel;
    m_sourcePanel->setSourceText(m_data.sourceText);
    connect(m_sourcePanel, &occtdebug::SourcePanel::searchRequested, this, [this]() {
        searchSourceText();
    });
    connect(m_sourcePanel, &occtdebug::SourcePanel::resultActivated, this, [this]() {
        openSelectedSourceResult();
    });

    auto* side = new QWidget;
    auto* sideLayout = new QVBoxLayout(side);
    setMargins(sideLayout, 0, 0);
    sideLayout->addWidget(label(m_data.geometrySummary, QStringLiteral("EvidenceCard")), 1);
    sideLayout->addWidget(label(m_data.evidenceSummary, QStringLiteral("EvidenceCard")), 2);

    splitter->addWidget(createPanel(s("Source"), m_sourcePanel));
    splitter->addWidget(side);
    splitter->setSizes({560, 480});
    return splitter;
}

QWidget* WorkbenchWindow::createReproScriptTab()
{
    auto* body = new QWidget;
    auto* layout = new QVBoxLayout(body);
    setMargins(layout, 0, 6);

    m_reproScriptEdit = new QPlainTextEdit;
    m_reproScriptEdit->setPlainText(m_data.reproScript);
    layout->addWidget(m_reproScriptEdit, 3);

    auto* actions = new QWidget;
    auto* actionLayout = new QHBoxLayout(actions);
    actionLayout->setContentsMargins(0, 0, 0, 0);
    actionLayout->setSpacing(8);
    auto* saveButton = new QPushButton(s("娣囨繂鐡?repro.tcl"));
    auto* runButton = new QPushButton(s("鏉╂劘顢?DRAW"));
    auto* cppButton = new QPushButton(QStringLiteral("C++ Repro"));
    auto* cancelButton = new QPushButton(QStringLiteral("Cancel"));
    connect(saveButton, &QPushButton::clicked, this, [this]() {
        saveCurrentReproScript();
    });
    connect(runButton, &QPushButton::clicked, this, [this]() {
        runCurrentDrawScript();
    });
    connect(cppButton, &QPushButton::clicked, this, [this]() {
        generateCppReproTemplate();
    });
    connect(cancelButton, &QPushButton::clicked, this, [this]() {
        cancelRunner(m_drawRunner, QStringLiteral("DRAW"), m_drawConsole);
    });
    actionLayout->addWidget(saveButton);
    actionLayout->addWidget(runButton);
    actionLayout->addWidget(cppButton);
    actionLayout->addWidget(cancelButton);
    actionLayout->addStretch(1);
    layout->addWidget(actions);
    layout->addWidget(label(m_data.evidenceSummary, QStringLiteral("EvidenceCard")), 1);
    return createPanel(s("婢跺秶骞囬懘姘拱"), body);
}

QWidget* WorkbenchWindow::createGeometryTab()
{
    auto* body = new QWidget;
    auto* layout = new QVBoxLayout(body);
    setMargins(layout, 0, 6);

    auto* actions = new QHBoxLayout;
    setMargins(actions, 0, 0);
    auto* loadButton = new QPushButton(s("Load Model"));
    auto* demoButton = new QPushButton(QStringLiteral("Demo"));
    auto* fitButton = new QPushButton(QStringLiteral("Fit"));
    auto* screenshotButton = new QPushButton(s("娣囨繂鐡ㄩ幋顏勬禈"));
    auto* topologyButton = new QPushButton(s("娣囨繂鐡ㄩ弰鐘茬殸"));
    actions->addWidget(loadButton);
    actions->addWidget(demoButton);
    actions->addWidget(fitButton);
    actions->addWidget(screenshotButton);
    actions->addWidget(topologyButton);
    actions->addStretch();
    layout->addLayout(actions);

    m_geometryViewerHost = new QWidget;
    auto* viewerHostLayout = new QVBoxLayout(m_geometryViewerHost);
    setMargins(viewerHostLayout, 0, 0);
    viewerHostLayout->addWidget(label(QStringLiteral("OCCT Viewer is ready. Load a model or run Demo to initialize the native viewport."), QStringLiteral("EvidenceCard")));
    layout->addWidget(m_geometryViewerHost, 1);
    m_geometrySummaryLabel = label(m_data.geometrySummary, QStringLiteral("EvidenceCard"));
    layout->addWidget(m_geometrySummaryLabel);

    connect(loadButton, &QPushButton::clicked, this, [this]() {
        importAndLoadGeometryModel();
    });
    connect(demoButton, &QPushButton::clicked, this, [this]() {
        if (ensureGeometryViewer())
        {
            m_geometryViewer->displayDemoShape();
            m_data.geometrySummary = QStringLiteral("Demo OCCT box loaded in viewer.");
            m_data.geometryChecks = {
                {QStringLiteral("Viewer demo"), QStringLiteral("ok"), QStringLiteral("BRepPrimAPI_MakeBox display path verified")},
            };
            syncGeometryTopologyStats();
            if (m_geometrySummaryLabel != nullptr)
            {
                m_geometrySummaryLabel->setText(m_data.geometrySummary);
            }
            refreshGeometryChecks();
        }
    });
    connect(fitButton, &QPushButton::clicked, this, [this]() {
        if (ensureGeometryViewer())
        {
            m_geometryViewer->fitAll();
        }
    });
    connect(screenshotButton, &QPushButton::clicked, this, [this]() {
        captureGeometryScreenshot();
    });
    connect(topologyButton, &QPushButton::clicked, this, [this]() {
        exportTopologySignature();
    });

    m_geometryTable = new QTableWidget(m_data.geometryChecks.size(), 3);
    m_geometryTable->setHorizontalHeaderLabels({s("Check"), s("Status"), s("Note")});
    m_geometryTable->horizontalHeader()->setStretchLastSection(true);
    m_geometryTable->verticalHeader()->hide();
    refreshGeometryChecks();
    layout->addWidget(m_geometryTable);
    return createPanel(s("Geometry Checks / Viewer"), body);
}

QWidget* WorkbenchWindow::createDiffTab()
{
    m_diffPanel = new occtdebug::DiffPanel;
    m_diffPanel->setDiffSummary(m_data.diffSummary);
    refreshDiffArtifactTables();
    connect(m_diffPanel, &occtdebug::DiffPanel::generateTopologyCompareRequested, this, [this]() {
        generateTopologyCompareArtifact();
    });
    connect(m_diffPanel, &occtdebug::DiffPanel::artifactOpenRequested, this, [this](const QString& path, const QString& origin) {
        openDiffArtifactPath(path, origin);
    });
    connect(m_diffPanel, &occtdebug::DiffPanel::artifactPreviewRequested, this, [this](const QString& path, const QString& origin) {
        previewDiffArtifactPath(path, origin);
    });
    return createPanel(s("Diff Compare"), m_diffPanel);
}

QWidget* WorkbenchWindow::createEvidenceTab()
{
    auto* body = new QWidget;
    auto* layout = new QVBoxLayout(body);
    setMargins(layout, 0, 6);

    auto* actions = new QHBoxLayout;
    setMargins(actions, 0, 0);
    auto* dumpButton = new QPushButton(QStringLiteral("Archive dump"));
    actions->addWidget(dumpButton);
    actions->addStretch();
    layout->addLayout(actions);

    m_evidencePanel = new occtdebug::EvidencePanel;
    m_evidencePanel->setData(m_data.evidenceSummary, m_data.evidenceItems);
    layout->addWidget(m_evidencePanel, 1);
    connect(m_evidencePanel, &occtdebug::EvidencePanel::recordActivated, this, [this](const occtdebug::EvidenceRecord& evidence) {
        activateEvidenceRecord(evidence);
    });
    connect(dumpButton, &QPushButton::clicked, this, [this]() {
        archiveCrashDump();
    });
    return createPanel(s("Evidence"), body);
}

QWidget* WorkbenchWindow::createEnvironmentTab()
{
    auto* body = new QWidget;
    auto* layout = new QVBoxLayout(body);
    setMargins(layout, 0, 6);

    m_environmentText = new QTextEdit;
    m_environmentText->setReadOnly(true);
    m_environmentText->setPlainText(m_data.environmentSummary);
    layout->addWidget(m_environmentText, 1);

    auto* actions = new QWidget;
    auto* actionLayout = new QHBoxLayout(actions);
    actionLayout->setContentsMargins(0, 0, 0, 0);
    actionLayout->setSpacing(8);
    auto* collectButton = new QPushButton(s("闁插洭娉﹂悳顖氼暔"));
    auto* cancelButton = new QPushButton(QStringLiteral("Cancel"));
    connect(collectButton, &QPushButton::clicked, this, [this]() {
        runEnvironmentCapture();
    });
    connect(cancelButton, &QPushButton::clicked, this, [this]() {
        cancelRunner(m_envRunner, QStringLiteral("env"), m_cmakeConsole);
    });
    actionLayout->addWidget(collectButton);
    actionLayout->addWidget(cancelButton);
    actionLayout->addStretch(1);
    layout->addWidget(actions);
    return createPanel(s("閻滎垰顣ㄦ穱鈩冧紖"), body);
}

QWidget* WorkbenchWindow::createDiagnosisPanel()
{
    auto* body = new QWidget;
    auto* layout = new QVBoxLayout(body);
    setMargins(layout, 0, 6);
    m_diagnosisLabel = label(m_data.diagnosis);
    layout->addWidget(m_diagnosisLabel);
    m_confidenceBar = new QProgressBar;
    m_confidenceBar->setRange(0, 100);
    m_confidenceBar->setValue(m_data.diagnosisConfidence);
    m_confidenceBar->setFormat(s("缂冾喕淇婃惔锔肩窗妤?(%p%)"));
    layout->addWidget(m_confidenceBar);

    auto* exportButton = new QPushButton(s("Export Patch Review"));
    connect(exportButton, &QPushButton::clicked, this, [this]() {
        exportDiagnosisReport();
    });
    layout->addWidget(exportButton);
    return createPanel(s("鐠囧﹥鏌囩紒鎾诡啈"), body);
}

QWidget* WorkbenchWindow::createPatchPanel()
{
    auto* body = new QWidget;
    auto* layout = new QVBoxLayout(body);
    setMargins(layout, 0, 6);

    m_patchDiffEdit = new QPlainTextEdit;
    m_patchDiffEdit->setReadOnly(false);
    m_patchDiffEdit->setPlainText(m_data.patchDiff);
    layout->addWidget(m_patchDiffEdit, 1);

    auto* actions = new QHBoxLayout;
    setMargins(actions, 0, 0);
    auto* generateButton = new QPushButton(s("Generate from worktree"));
    auto* importButton = new QPushButton(s("鐎电厧鍙?diff"));
    auto* saveButton = new QPushButton(s("Save Candidate"));
    auto* exportButton = new QPushButton(s("鐎电厧鍤?patch"));
    auto* cancelButton = new QPushButton(QStringLiteral("Cancel"));
    connect(generateButton, &QPushButton::clicked, this, [this]() {
        generatePatchCandidateFromWorktree();
    });
    connect(importButton, &QPushButton::clicked, this, [this]() {
        importPatchCandidateDiff();
    });
    connect(saveButton, &QPushButton::clicked, this, [this]() {
        savePatchCandidateDiff();
    });
    connect(exportButton, &QPushButton::clicked, this, [this]() {
        exportPatchCandidateDiff();
    });
    connect(cancelButton, &QPushButton::clicked, this, [this]() {
        cancelRunner(m_patchRunner, QStringLiteral("patch"), m_cmakeConsole);
    });
    actions->addWidget(generateButton);
    actions->addWidget(importButton);
    actions->addWidget(saveButton);
    actions->addWidget(exportButton);
    actions->addWidget(cancelButton);
    actions->addStretch();
    layout->addLayout(actions);
    return createPanel(s("Candidate Patch"), body);
}

QWidget* WorkbenchWindow::createVerificationPanel()
{
    m_verificationPanel = new occtdebug::VerificationPanel;
    m_verificationPanel->setItems(m_data.verificationItems);
    connect(m_verificationPanel, &occtdebug::VerificationPanel::exportMarkdownRequested, this, [this]() {
        exportMarkdownReport();
    });
    connect(m_verificationPanel, &occtdebug::VerificationPanel::exportReproPackRequested, this, [this]() {
        exportReproPack();
    });
    return createPanel(s("妤犲矁鐦夌紒鎾寸亯"), m_verificationPanel);
}

QWidget* WorkbenchWindow::createPatchReviewPanel()
{
    auto* body = new QWidget;
    auto* layout = new QVBoxLayout(body);
    setMargins(layout, 0, 6);

    m_patchReviewStatus = label(QString(), QStringLiteral("EvidenceCard"));
    updatePatchReviewStatus();
    layout->addWidget(m_patchReviewStatus);

    auto* actions = new QWidget;
    auto* actionLayout = new QHBoxLayout(actions);
    actionLayout->setContentsMargins(0, 0, 0, 0);
    actionLayout->setSpacing(8);

    auto* reviewButton = new QPushButton(QStringLiteral("Review"));
    auto* approveButton = new QPushButton(QStringLiteral("Approve"));
    auto* rejectButton = new QPushButton(QStringLiteral("Reject"));
    auto* applyButton = new QPushButton(QStringLiteral("Apply"));
    auto* undoButton = new QPushButton(QStringLiteral("Undo"));
    auto* signoffButton = new QPushButton(QStringLiteral("Signoff"));
    auto* exportButton = new QPushButton(QStringLiteral("Export"));
    connect(reviewButton, &QPushButton::clicked, this, [this]() {
        m_patchReview.markNeedsReview(QStringLiteral("Evidence is linked; ready for maintainer review."));
        recordPatchReviewState(QStringLiteral("Patch candidate submitted for maintainer review."));
        updatePatchReviewStatus();
    });
    connect(approveButton, &QPushButton::clicked, this, [this]() {
        m_patchReview.approve(QStringLiteral("Reviewer accepted the candidate direction."));
        recordPatchReviewState(QStringLiteral("Patch candidate approved for verification planning."));
        updatePatchReviewStatus();
    });
    connect(rejectButton, &QPushButton::clicked, this, [this]() {
        m_patchReview.reject(QStringLiteral("Reviewer rejected the current patch candidate."));
        recordPatchReviewState(QStringLiteral("Patch candidate rejected by reviewer."));
        updatePatchReviewStatus();
    });
    connect(applyButton, &QPushButton::clicked, this, [this]() {
        applyPatchCandidate();
    });
    connect(undoButton, &QPushButton::clicked, this, [this]() {
        undoPatchCandidate();
    });
    connect(signoffButton, &QPushButton::clicked, this, [this]() {
        signOffPatchCandidate();
    });
    connect(exportButton, &QPushButton::clicked, this, [this]() {
        exportPatchReviewReport();
    });

    actionLayout->addWidget(reviewButton);
    actionLayout->addWidget(approveButton);
    actionLayout->addWidget(rejectButton);
    actionLayout->addWidget(applyButton);
    actionLayout->addWidget(undoButton);
    actionLayout->addWidget(signoffButton);
    actionLayout->addWidget(exportButton);
    layout->addWidget(actions);

    return createPanel(QStringLiteral("Patch Review"), body);
}

QWidget* WorkbenchWindow::createSimilarCasesPanel()
{
    auto* body = new QWidget;
    auto* layout = new QVBoxLayout(body);
    setMargins(layout, 0, 6);

    auto* searchLayout = new QHBoxLayout;
    setMargins(searchLayout, 0, 0);
    m_similarCaseSearchEdit = new QLineEdit;
    m_similarCaseSearchEdit->setPlaceholderText(s("Search similar cases"));
    auto* searchButton = new QPushButton(s("Search"));
    auto* diagnosisButton = new QPushButton(s("Use Diagnosis"));
    connect(searchButton, &QPushButton::clicked, this, [this]() {
        refreshSimilarCases(m_similarCaseSearchEdit != nullptr ? m_similarCaseSearchEdit->text() : QString());
    });
    connect(diagnosisButton, &QPushButton::clicked, this, [this]() {
        refreshSimilarCases();
    });
    connect(m_similarCaseSearchEdit, &QLineEdit::returnPressed, this, [this]() {
        refreshSimilarCases(m_similarCaseSearchEdit != nullptr ? m_similarCaseSearchEdit->text() : QString());
    });
    searchLayout->addWidget(m_similarCaseSearchEdit, 1);
    searchLayout->addWidget(searchButton);
    searchLayout->addWidget(diagnosisButton);
    layout->addLayout(searchLayout);

    m_similarCasesList = new QListWidget;
    layout->addWidget(m_similarCasesList, 1);
    refreshSimilarCases(QStringLiteral(" "));
    return createPanel(s("閻╅晲鎶€濡楀牅绶?/ Issues"), body);
}

QLabel* WorkbenchWindow::createBadge(const QString& text, const QString& objectName)
{
    auto* out = label(text, objectName);
    out->setAlignment(Qt::AlignCenter);
    out->setMinimumHeight(26);
    return out;
}

QToolButton* WorkbenchWindow::createToolbarButton(const QString& text, const QString& objectName)
{
    auto* button = new QToolButton;
    button->setText(text);
    button->setObjectName(objectName);
    button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    button->setMinimumHeight(34);
    button->setCursor(Qt::PointingHandCursor);
    return button;
}

void WorkbenchWindow::populateMockCaseData()
{
    refreshCaseList();
}

void WorkbenchWindow::refreshCaseList()
{
    if (m_casePanel == nullptr)
    {
        return;
    }

    QVector<occtdebug::CaseSummary> summaries;
    const QDir root(caseRootDirectory());
    if (root.exists())
    {
        const QFileInfoList entries = root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        for (const QFileInfo& entry : entries)
        {
            const QString manifestPath = QDir(entry.absoluteFilePath()).filePath(QStringLiteral("case.json"));
            QString error;
            const std::optional<occtdebug::CaseManifest> manifest = occtdebug::CaseManifest::loadFromFile(manifestPath, &error);
            if (!manifest.has_value())
            {
                continue;
            }
            summaries.push_back({manifest->caseId, manifest->status, manifest->title, manifest->createdAt});
        }
    }

    if (summaries.isEmpty())
    {
        summaries = m_data.cases;
    }
    if (summaries.isEmpty())
    {
        summaries.push_back({m_data.caseId, m_data.caseStatus, m_data.manifest.title, m_data.manifest.createdAt});
    }

    m_casePanel->setCaseSummaries(summaries, m_data.caseId);
}

void WorkbenchWindow::refreshGeometryChecks()
{
    if (m_geometryTable == nullptr)
    {
        return;
    }

    m_geometryTable->setRowCount(m_data.geometryChecks.size());
    for (int row = 0; row < m_data.geometryChecks.size(); ++row)
    {
        const auto& check = m_data.geometryChecks[row];
        m_geometryTable->setItem(row, 0, item(check.name));
        m_geometryTable->setItem(row, 1, item(check.status));
        m_geometryTable->setItem(row, 2, item(check.note));
    }
}

void WorkbenchWindow::refreshDiffArtifactTables()
{
    if (m_diffPanel == nullptr)
    {
        return;
    }
    const QJsonObject result =
        readJsonObject(QDir(runtimeArtifactDirectory()).filePath(QStringLiteral("testgrid_result.json")));
    m_diffPanel->setDiffSummary(m_data.diffSummary);
    m_diffPanel->setTestdiffArtifacts(result.value(QStringLiteral("testdiff_artifacts")).toObject());
}

bool WorkbenchWindow::resolveDiffArtifactPath(const QString& path, const QString& origin, QString* targetPath) const
{
    const QString link = path.trimmed();
    if (link.isEmpty())
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[%1] no artifact selected").arg(origin));
        }
        return false;
    }
    if (!isRelativeLocalReference(link) || m_data.workspaceRoot.isEmpty())
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[%1] blocked non-local artifact: %2").arg(origin, link));
        }
        return false;
    }

    const QString resolvedPath = QDir::cleanPath(QDir(m_data.workspaceRoot).filePath(link));
    QString workspaceRoot = QDir::cleanPath(QFileInfo(m_data.workspaceRoot).absoluteFilePath());
    QString absoluteTarget = QDir::cleanPath(QFileInfo(resolvedPath).absoluteFilePath());
#ifdef Q_OS_WIN
    workspaceRoot = workspaceRoot.toLower();
    absoluteTarget = absoluteTarget.toLower();
#endif
    if (absoluteTarget != workspaceRoot && !absoluteTarget.startsWith(workspaceRoot + QLatin1Char('/')))
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[%1] blocked artifact outside case: %2").arg(origin, link));
        }
        return false;
    }

    const QFileInfo info(resolvedPath);
    if (!info.exists() || !info.isFile())
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[%1] artifact missing: %2").arg(origin, QDir::toNativeSeparators(resolvedPath)));
        }
        return false;
    }
    if (targetPath != nullptr)
    {
        *targetPath = resolvedPath;
    }
    return true;
}

void WorkbenchWindow::openDiffArtifactPath(const QString& path, const QString& origin)
{
    const QString link = path.trimmed();
    QString targetPath;
    if (!resolveDiffArtifactPath(path, origin, &targetPath))
    {
        return;
    }

    const QFileInfo info(targetPath);
    const QString suffix = info.suffix().toLower();
    const bool previewAsText = QStringList {
        QStringLiteral("txt"),
        QStringLiteral("log"),
        QStringLiteral("json"),
        QStringLiteral("md"),
        QStringLiteral("tcl"),
        QStringLiteral("diff"),
        QStringLiteral("patch"),
        QStringLiteral("xml"),
        QStringLiteral("yaml"),
        QStringLiteral("yml"),
        QStringLiteral("csv"),
    }.contains(suffix);
    if (previewAsText)
    {
        if (m_bottomTabs != nullptr)
        {
            m_bottomTabs->setCurrentIndex(link.contains(QStringLiteral("draw"), Qt::CaseInsensitive) ? 0 : 1);
        }
        QTextEdit* console = link.contains(QStringLiteral("draw"), Qt::CaseInsensitive) ? m_drawConsole : m_cmakeConsole;
        if (console != nullptr)
        {
            const QString text = readTextFile(targetPath);
            console->setPlainText(text.isEmpty()
                    ? QStringLiteral("[%1] artifact is empty: %2").arg(origin, QDir::toNativeSeparators(targetPath))
                    : text.left(200000));
        }
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[%1] opened text artifact %2").arg(origin, QDir::toNativeSeparators(targetPath)));
        }
        return;
    }

    const bool opened = QDesktopServices::openUrl(QUrl::fromLocalFile(targetPath));
    if (m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(opened
                ? QStringLiteral("[%1] opened artifact %2").arg(origin, QDir::toNativeSeparators(targetPath))
                : QStringLiteral("[%1] failed to open artifact %2").arg(origin, QDir::toNativeSeparators(targetPath)));
    }
}

void WorkbenchWindow::previewDiffArtifactPath(const QString& path, const QString& origin)
{
    if (m_diffPanel == nullptr)
    {
        return;
    }

    QString targetPath;
    if (!resolveDiffArtifactPath(path, origin, &targetPath))
    {
        m_diffPanel->setPreviewMessage(QStringLiteral("Image preview unavailable: invalid or missing artifact."));
        return;
    }

    const QFileInfo info(targetPath);
    const QString suffix = info.suffix().toLower();
    const bool isImage = QStringList {
        QStringLiteral("png"),
        QStringLiteral("jpg"),
        QStringLiteral("jpeg"),
        QStringLiteral("bmp"),
        QStringLiteral("gif"),
        QStringLiteral("webp"),
    }.contains(suffix);
    if (!isImage)
    {
        m_diffPanel->setPreviewMessage(QStringLiteral("Selected artifact is not a supported image: %1").arg(path));
        return;
    }

    QPixmap pixmap(targetPath);
    if (pixmap.isNull())
    {
        m_diffPanel->setPreviewImage(path, pixmap, QStringLiteral("Failed to load image preview: %1").arg(path));
        return;
    }

    m_diffPanel->setPreviewImage(path, pixmap);
    if (m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[%1] previewed image artifact %2").arg(origin, QDir::toNativeSeparators(targetPath)));
    }
}

bool WorkbenchWindow::ensureGeometryViewer()
{
    if (m_geometryViewer != nullptr)
    {
        return true;
    }
    if (m_geometryViewerHost == nullptr || m_geometryViewerHost->layout() == nullptr)
    {
        return false;
    }

    auto* viewer = new occtdebug::OcctViewerWidget(m_geometryViewerHost);
    viewer->setObjectName(QStringLiteral("GeometryViewport"));
    connect(viewer, &occtdebug::OcctViewerWidget::geometryObjectPicked, this, [this](const QString& objectId, const QString& summary) {
        handleViewerGeometryObjectPicked(objectId, summary);
    });
    clearLayout(m_geometryViewerHost->layout());
    m_geometryViewerHost->layout()->addWidget(viewer);
    m_geometryViewer = viewer;
    return true;
}

void WorkbenchWindow::handleViewerGeometryObjectPicked(const QString& objectId, const QString& summary)
{
    if (objectId.trimmed().isEmpty())
    {
        return;
    }

    const QString timestamp = QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMddTHHmmsszzzZ"));
    QDir dir;
    dir.mkpath(runtimeArtifactDirectory());
    const QString artifactName = QStringLiteral("geometry_selection_%1.json").arg(timestamp);
    const QString artifactPath = QDir(runtimeArtifactDirectory()).filePath(artifactName);
    const QString topology = m_geometryViewer != nullptr ? m_geometryViewer->topologySummary() : QStringLiteral("viewer unavailable");
    QString signatureError;
    const QString stableId = m_geometryViewer != nullptr
        ? m_geometryViewer->topologySignatureForObject(objectId, &signatureError)
        : QString();
    const QString topologyMap = writeTopologySignatureArtifact(QStringLiteral("viewer_selection"), objectId, true);

    const QJsonObject json {
        {QStringLiteral("schema_version"), 1},
        {QStringLiteral("case_id"), m_data.caseId},
        {QStringLiteral("created_at"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
        {QStringLiteral("geometry_object"), objectId},
        {QStringLiteral("stable_geometry_object"), stableId},
        {QStringLiteral("selection_basis"), QStringLiteral("TopExp traversal object_id plus SHA-256 of BRepTools::Write(subshape)")},
        {QStringLiteral("summary"), summary},
        {QStringLiteral("topology_summary"), topology},
        {QStringLiteral("topology_signature"), topologyMap},
        {QStringLiteral("signature_error"), stableId.isEmpty() ? signatureError : QString()},
    };

    QString error;
    if (!writeTextFile(artifactPath, QString::fromUtf8(QJsonDocument(json).toJson(QJsonDocument::Indented)), &error)
        && m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[geometry] %1").arg(error));
    }

    occtdebug::EvidenceRecord evidence;
    evidence.type = QStringLiteral("Geometry");
    evidence.title = QStringLiteral("Viewer selection %1").arg(objectId);
    evidence.summary = stableId.isEmpty()
        ? summary
        : QStringLiteral("%1; stable_id=%2").arg(summary, stableId);
    evidence.link = QStringLiteral("artifacts/%1").arg(artifactName);
    evidence.geometryObject = objectId;
    appendEvidenceRecord(evidence);

    m_data.geometrySummary = QStringLiteral("Viewer selected %1\nstable_id=%2\n%3")
        .arg(objectId, stableId.isEmpty() ? QStringLiteral("unavailable") : stableId, topology);
    m_data.manifest.geometrySummary = m_data.geometrySummary;
    if (m_geometrySummaryLabel != nullptr)
    {
        m_geometrySummaryLabel->setText(m_data.geometrySummary);
    }

    bool existingCheck = false;
    for (const occtdebug::GeometryCheck& check : m_data.geometryChecks)
    {
        if (check.name == QStringLiteral("Viewer selection") && check.note.contains(objectId))
        {
            existingCheck = true;
            break;
        }
    }
    if (!existingCheck)
    {
        m_data.geometryChecks.push_back({
            QStringLiteral("Viewer selection"),
            QStringLiteral("picked"),
            stableId.isEmpty()
                ? QStringLiteral("%1; basis=TopExp traversal index; signature unavailable").arg(objectId)
                : QStringLiteral("%1; stable_id=%2").arg(objectId, stableId),
        });
        m_data.manifest.geometryChecks = m_data.geometryChecks;
        refreshGeometryChecks();
    }
    if (m_geometryTable != nullptr)
    {
        for (int row = 0; row < m_data.geometryChecks.size(); ++row)
        {
            if (m_data.geometryChecks[row].note.contains(objectId))
            {
                m_geometryTable->selectRow(row);
                break;
            }
        }
    }

    saveCurrentCaseManifest();
    refreshEvidenceBundle();
    if (m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[geometry] picked %1; evidence written to %2")
                .arg(objectId, QDir::toNativeSeparators(artifactPath)));
    }
}

void WorkbenchWindow::syncGeometryTopologyStats(const QString& status)
{
    if (m_geometryViewer == nullptr)
    {
        return;
    }

    const QString topology = m_geometryViewer->topologySummary().trimmed();
    if (topology.isEmpty())
    {
        return;
    }

    const QString checkName = QStringLiteral("Topology stats");
    bool updated = false;
    for (occtdebug::GeometryCheck& check : m_data.geometryChecks)
    {
        if (check.name == checkName)
        {
            check.status = status;
            check.note = topology;
            updated = true;
            break;
        }
    }
    if (!updated)
    {
        m_data.geometryChecks.push_back({checkName, status, topology});
    }
    m_data.manifest.geometryChecks = m_data.geometryChecks;
}

void WorkbenchWindow::recordImportedInputFile(const QString& targetPath, const QString& originalName)
{
    const QFileInfo info(targetPath);
    if (!info.exists() || !info.isFile())
    {
        return;
    }

    const QString workspaceRoot = m_data.workspaceRoot.isEmpty()
        ? QCoreApplication::applicationDirPath()
        : m_data.workspaceRoot;
    QString relativePath = QDir(workspaceRoot).relativeFilePath(info.absoluteFilePath());
    relativePath.replace(QLatin1Char('\\'), QLatin1Char('/'));

    QString hashError;
    const QString sha256 = fileSha256Hex(info.absoluteFilePath(), &hashError);
    if (sha256.isEmpty())
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[input] %1").arg(hashError));
        }
        return;
    }

    occtdebug::InputFileRecord record;
    record.path = relativePath;
    record.originalName = originalName;
    record.sha256 = sha256;
    record.bytes = info.size();
    record.importedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    bool updated = false;
    for (occtdebug::InputFileRecord& existing : m_data.manifest.inputFiles)
    {
        if (existing.path == record.path)
        {
            existing = record;
            updated = true;
            break;
        }
    }
    if (!updated)
    {
        m_data.manifest.inputFiles.push_back(record);
    }

    if (m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[input] recorded %1 bytes=%2 sha256=%3")
                .arg(record.path)
                .arg(record.bytes)
                .arg(record.sha256.left(12)));
    }
}

void WorkbenchWindow::captureGeometryScreenshot()
{
    if (!ensureGeometryViewer())
    {
        QMessageBox::warning(this, s("Geometry Viewer"), QStringLiteral("failed to create OCCT Viewer."));
        return;
    }

    QDir dir;
    if (!dir.mkpath(runtimeArtifactDirectory()))
    {
        QMessageBox::warning(this, s("Geometry Viewer"), QStringLiteral("failed to create artifacts directory."));
        return;
    }

    const QString timestamp = QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMddTHHmmsszzzZ"));
    const QString fileName = QStringLiteral("geometry_screenshot_%1.png").arg(timestamp);
    const QString path = QDir(runtimeArtifactDirectory()).filePath(fileName);
    QString error;
    if (!m_geometryViewer->saveScreenshot(path, &error))
    {
        QMessageBox::warning(this, s("娣囨繂鐡ㄩ幋顏勬禈"), error);
        return;
    }

    const QString objectId = m_geometryViewer->highlightedObjectId();
    const QString topology = m_geometryViewer->topologySummary();
    const QString stableId = objectId.isEmpty()
        ? QString()
        : m_geometryViewer->topologySignatureForObject(objectId);
    const QString topologyMap = writeTopologySignatureArtifact(QStringLiteral("viewer_screenshot"), objectId, true);
    occtdebug::EvidenceRecord evidence;
    evidence.type = QStringLiteral("Geometry");
    evidence.title = objectId.isEmpty()
        ? QStringLiteral("Viewer screenshot")
        : QStringLiteral("Viewer screenshot %1").arg(objectId);
    evidence.summary = objectId.isEmpty()
        ? QStringLiteral("OCCT Viewer screenshot; topology_signature=%1; %2").arg(topologyMap, topology)
        : QStringLiteral("OCCT Viewer screenshot with highlighted object %1; stable_id=%2; topology_signature=%3; %4")
            .arg(objectId,
                 stableId.isEmpty() ? QStringLiteral("unavailable") : stableId,
                 topologyMap,
                 topology);
    evidence.link = QStringLiteral("artifacts/%1").arg(fileName);
    evidence.geometryObject = objectId;
    appendEvidenceRecord(evidence);

    m_data.geometrySummary = QStringLiteral("Viewer screenshot saved: artifacts/%1\n%2").arg(fileName, topology);
    m_data.manifest.geometrySummary = m_data.geometrySummary;
    if (m_geometrySummaryLabel != nullptr)
    {
        m_geometrySummaryLabel->setText(m_data.geometrySummary);
    }

    saveCurrentCaseManifest();
    refreshEvidenceBundle();
    if (m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[geometry] screenshot saved to %1").arg(QDir::toNativeSeparators(path)));
    }
}

QString WorkbenchWindow::writeTopologySignatureArtifact(const QString& reason,
                                                        const QString& selectedObjectId,
                                                        bool registerEvidence)
{
    if (m_geometryViewer == nullptr)
    {
        return QString();
    }

    QDir dir;
    if (!dir.mkpath(runtimeArtifactDirectory()))
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[geometry] failed to create artifact directory for topology signature"));
        }
        return QString();
    }

    QString sourceLabel = QStringLiteral("current viewer shape");
    for (const occtdebug::GeometryCheck& check : m_data.geometryChecks)
    {
        if (check.name == QStringLiteral("Model load") && !check.note.trimmed().isEmpty())
        {
            sourceLabel = check.note;
            break;
        }
    }

    QJsonObject json = m_geometryViewer->topologySignatureJson(sourceLabel, selectedObjectId);
    json.insert(QStringLiteral("case_id"), m_data.caseId);
    json.insert(QStringLiteral("created_at"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    json.insert(QStringLiteral("reason"), reason);
    json.insert(QStringLiteral("selected_geometry_object"), selectedObjectId);

    const QString timestamp = QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMddTHHmmsszzzZ"));
    const QString fileName = QStringLiteral("topology_signature_%1.json").arg(timestamp);
    const QString path = QDir(runtimeArtifactDirectory()).filePath(fileName);
    QString error;
    if (!writeTextFile(path, QString::fromUtf8(QJsonDocument(json).toJson(QJsonDocument::Indented)), &error))
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[geometry] %1").arg(error));
        }
        return QString();
    }

    const QString relativePath = QStringLiteral("artifacts/%1").arg(fileName);
    if (registerEvidence)
    {
        const QJsonObject counts = json.value(QStringLiteral("counts")).toObject();
        occtdebug::EvidenceRecord evidence;
        evidence.type = QStringLiteral("Geometry");
        evidence.title = selectedObjectId.trimmed().isEmpty()
            ? QStringLiteral("Topology signature map")
            : QStringLiteral("Topology signature map %1").arg(selectedObjectId);
        evidence.summary = QStringLiteral("reason=%1 V=%2 E=%3 F=%4 SOLID=%5 basis=BRepTools::Write sha256")
            .arg(reason)
            .arg(counts.value(QStringLiteral("vertex")).toInt())
            .arg(counts.value(QStringLiteral("edge")).toInt())
            .arg(counts.value(QStringLiteral("face")).toInt())
            .arg(counts.value(QStringLiteral("solid")).toInt());
        evidence.link = relativePath;
        evidence.geometryObject = selectedObjectId;
        appendEvidenceRecord(evidence);
    }

    return relativePath;
}

void WorkbenchWindow::exportTopologySignature()
{
    if (!ensureGeometryViewer())
    {
        QMessageBox::warning(this, s("Capture Geometry"), QStringLiteral("failed to create OCCT Viewer."));
        return;
    }

    const QString selectedObject = m_geometryViewer->highlightedObjectId();
    const QString artifact = writeTopologySignatureArtifact(QStringLiteral("manual_export"), selectedObject, true);
    if (artifact.isEmpty())
    {
        QMessageBox::warning(this, s("Capture Geometry"), QStringLiteral("failed to write geometry screenshot artifact."));
        return;
    }

    m_data.geometrySummary = QStringLiteral("Topology signature saved: %1\n%2")
        .arg(artifact, m_geometryViewer->topologySummary());
    m_data.manifest.geometrySummary = m_data.geometrySummary;
    if (m_geometrySummaryLabel != nullptr)
    {
        m_geometrySummaryLabel->setText(m_data.geometrySummary);
    }
    saveCurrentCaseManifest();
    refreshEvidenceBundle();
    if (m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[geometry] topology signature saved to %1").arg(artifact));
    }
}

QString WorkbenchWindow::focusGeometryObjectFromTopologyCompare(const QJsonObject& compare) const
{
    const QString highlighted = m_geometryViewer != nullptr ? m_geometryViewer->highlightedObjectId().trimmed() : QString();
    if (!highlighted.isEmpty())
    {
        return highlighted;
    }

    const auto firstObjectId = [](const QJsonArray& array, const QString& nestedKey = QString()) {
        for (const QJsonValue& value : array)
        {
            const QJsonObject object = value.toObject();
            const QJsonObject record = nestedKey.isEmpty() ? object : object.value(nestedKey).toObject();
            const QString id = record.value(QStringLiteral("object_id")).toString().trimmed();
            if (!id.isEmpty())
            {
                return id;
            }
        }
        return QString();
    };

    QString id = firstObjectId(compare.value(QStringLiteral("unmatched_before")).toArray());
    if (!id.isEmpty())
    {
        return id;
    }
    id = firstObjectId(compare.value(QStringLiteral("unmatched_after")).toArray());
    if (!id.isEmpty())
    {
        return id;
    }
    return firstObjectId(compare.value(QStringLiteral("matches")).toArray(), QStringLiteral("before"));
}

void WorkbenchWindow::generateTopologyCompareArtifact()
{
    const QString beforePath = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Select before topology signature"),
        runtimeArtifactDirectory(),
        QStringLiteral("Topology signature (*.json);;JSON (*.json);;All files (*.*)"));
    if (beforePath.isEmpty())
    {
        return;
    }

    const QString afterPath = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Select after topology signature"),
        runtimeArtifactDirectory(),
        QStringLiteral("Topology signature (*.json);;JSON (*.json);;All files (*.*)"));
    if (afterPath.isEmpty())
    {
        return;
    }

    QString error;
    const QJsonObject compare = occtdebug::TopologyCompareArtifact::writeForCase(
        m_data.workspaceRoot,
        beforePath,
        afterPath,
        &error);
    if (compare.isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("Generate topology compare"), error);
        return;
    }

    const QString summary = occtdebug::TopologyCompareArtifact::summaryText(compare);
    const QString artifact = compare.value(QStringLiteral("artifact")).toString(
        occtdebug::TopologyCompareArtifact::defaultArtifactRelativePath());
    const QString focusObject = focusGeometryObjectFromTopologyCompare(compare);

    m_data.diffSummary = focusObject.isEmpty()
        ? QStringLiteral("%1\nArtifact: %2").arg(summary, artifact)
        : QStringLiteral("%1\nArtifact: %2\nGeometry object: %3").arg(summary, artifact, focusObject);
    m_data.manifest.diffSummary = m_data.diffSummary;
    setVerificationMetric(QStringLiteral("topology compare"),
                          QStringLiteral("%1; %2").arg(compare.value(QStringLiteral("status")).toString(QStringLiteral("unknown")),
                                                        summary));
    refreshDiffArtifactTables();

    occtdebug::EvidenceRecord evidence;
    evidence.type = QStringLiteral("Geometry");
    evidence.title = QStringLiteral("Topology before/after compare");
    evidence.summary = focusObject.isEmpty()
        ? summary
        : QStringLiteral("%1; focus_object=%2").arg(summary, focusObject);
    evidence.link = artifact;
    evidence.geometryObject = focusObject;
    appendEvidenceRecord(evidence);

    saveCurrentCaseManifest();
    refreshReports({true, true, QStringLiteral("save_case")});
    if (m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[geometry] topology compare written to %1").arg(artifact));
    }
}

void WorkbenchWindow::archiveCrashDump()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Archive Crash Dump"),
        QString(),
        QStringLiteral("Crash dumps (*.dmp *.mdmp *.dump);;All files (*.*)"));
    if (filePath.isEmpty())
    {
        return;
    }

    const QString workspaceRoot = m_data.workspaceRoot.isEmpty()
        ? caseDirectory(m_data.caseId)
        : m_data.workspaceRoot;
    QDir().mkpath(workspaceRoot);

    occtdebug::CrashDumpArchiveResult result;
    QString error;
    if (!occtdebug::CrashDumpArchive::archiveFile(filePath, workspaceRoot, m_data.caseId, &result, &error))
    {
        QMessageBox::warning(this, QStringLiteral("Archive Crash Dump"), error);
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[crash] %1").arg(error));
        }
        return;
    }

    if (m_data.workspaceRoot.isEmpty())
    {
        m_data.workspaceRoot = workspaceRoot;
    }
    occtdebug::EvidenceRecord evidence;
    evidence.type = QStringLiteral("Crash Dump");
    evidence.title = QStringLiteral("Crash dump archived");
    evidence.summary = QStringLiteral("file=%1 bytes=%2 sha256=%3")
        .arg(result.originalName)
        .arg(result.bytes)
        .arg(result.sha256.left(12));
    evidence.link = result.manifestRelativePath;
    appendEvidenceRecord(evidence);

    m_data.evidenceSummary = QStringLiteral("Crash dump archived: %1").arg(result.artifactRelativePath);
    m_data.manifest.evidenceSummary = m_data.evidenceSummary;
    saveCurrentCaseManifest();
    refreshEvidenceBundle();
    refreshVerificationReport();

    if (m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[crash] archived %1 -> %2")
                .arg(QDir::toNativeSeparators(filePath), result.artifactRelativePath));
    }
}

void WorkbenchWindow::importAndLoadGeometryModel()
{
    if (!ensureGeometryViewer())
    {
        QMessageBox::warning(this, s("Load geometry model"), QStringLiteral("failed to create OCCT Viewer."));
        return;
    }

    const QString sourcePath = QFileDialog::getOpenFileName(
        this,
        s("Load geometry model"),
        runtimeInputDirectory(),
        QStringLiteral("OCCT Models (*.brep *.brp *.rle *.step *.stp *.iges *.igs);;BREP (*.brep *.brp *.rle);;STEP (*.step *.stp);;IGES (*.iges *.igs);;All files (*.*)"));
    if (sourcePath.isEmpty())
    {
        return;
    }

    const QFileInfo sourceInfo(sourcePath);
    QDir inputDir;
    if (!inputDir.mkpath(runtimeInputDirectory()))
    {
        QMessageBox::warning(this, s("Load geometry model"), QStringLiteral("failed to create input directory: %1").arg(QDir::toNativeSeparators(runtimeInputDirectory())));
        return;
    }

    const QString targetPath = QDir(runtimeInputDirectory()).filePath(sourceInfo.fileName());
    if (QFileInfo::exists(targetPath) && QFileInfo(targetPath).absoluteFilePath() != sourceInfo.absoluteFilePath())
    {
        if (!QFile::remove(targetPath))
        {
            QMessageBox::warning(this, s("Load geometry model"), QStringLiteral("failed to replace existing input model: %1").arg(QDir::toNativeSeparators(targetPath)));
            return;
        }
    }
    if (QFileInfo(targetPath).absoluteFilePath() != sourceInfo.absoluteFilePath() && !QFile::copy(sourcePath, targetPath))
    {
        QMessageBox::warning(this, s("Load geometry model"), QStringLiteral("failed to copy model into Case input: %1").arg(QDir::toNativeSeparators(targetPath)));
        return;
    }

    QString error;
    if (!m_geometryViewer->loadModelFile(targetPath, &error))
    {
        m_data.geometrySummary = QStringLiteral("Model load failed: %1").arg(error);
        m_data.geometryChecks = {
            {QStringLiteral("Model load"), QStringLiteral("failed"), error},
        };
        if (m_geometrySummaryLabel != nullptr)
        {
            m_geometrySummaryLabel->setText(m_data.geometrySummary);
        }
        refreshGeometryChecks();
        QMessageBox::warning(this, s("Load geometry model"), error);
        return;
    }

    const QString relativePath = QDir(m_data.workspaceRoot.isEmpty() ? QCoreApplication::applicationDirPath() : m_data.workspaceRoot).relativeFilePath(targetPath);
    recordImportedInputFile(targetPath, sourceInfo.fileName());
    m_data.geometrySummary = QStringLiteral("Loaded geometry model: %1").arg(sourceInfo.fileName());
    m_data.geometryChecks = {
        {QStringLiteral("Model load"), QStringLiteral("ok"), relativePath},
        {QStringLiteral("Viewer display"), QStringLiteral("ok"), QStringLiteral("shape displayed and fit all completed")},
    };
    syncGeometryTopologyStats();
    const QString topologyMap = writeTopologySignatureArtifact(QStringLiteral("model_loaded"), QString(), true);
    if (!topologyMap.isEmpty())
    {
        m_data.geometryChecks.push_back({
            QStringLiteral("Topology signature"),
            QStringLiteral("ok"),
            topologyMap,
        });
    }
    if (m_geometrySummaryLabel != nullptr)
    {
        m_geometrySummaryLabel->setText(m_data.geometrySummary);
    }
    refreshGeometryChecks();
    saveCurrentCaseManifest();
}

void WorkbenchWindow::searchSourceText()
{
    if (m_sourcePanel == nullptr)
    {
        return;
    }

    const QString query = m_sourcePanel->searchQuery();
    m_sourcePanel->clearSearchResults();
    if (query.isEmpty())
    {
        return;
    }

    QStringList searchRoots;
    const QDir repo(sourceRootDirectory());
    searchRoots << repo.filePath(QStringLiteral("src"));
    searchRoots << repo.filePath(QStringLiteral("tests"));
    if (!m_data.workspaceRoot.isEmpty())
    {
        searchRoots << QDir(m_data.workspaceRoot).filePath(QStringLiteral("repro"));
    }
    const QString occtInclude = repo.filePath(QStringLiteral("depends/occt/include"));
    if (QFileInfo::exists(occtInclude))
    {
        searchRoots << occtInclude;
    }

    int added = 0;
    for (const QString& root : searchRoots)
    {
        if (!QFileInfo::exists(root))
        {
            continue;
        }
        const occtdebug::SourceIndex index = occtdebug::SourceIndex::build(root);
        const QVector<occtdebug::SourceIndexEntry> hits = index.search(query, 20);
        for (const auto& hit : hits)
        {
            m_sourcePanel->addSearchResult({
                QStringLiteral("%1:%2  %3")
                    .arg(QDir(sourceRootDirectory()).relativeFilePath(hit.filePath))
                    .arg(hit.lineNumber)
                    .arg(hit.text.trimmed()),
                hit.filePath,
                hit.lineNumber,
            });
            ++added;
            if (added >= 50)
            {
                break;
            }
        }
        if (added >= 50)
        {
            break;
        }
    }

    if (added == 0)
    {
        m_sourcePanel->addMessageResult(QStringLiteral("No source match for '%1'").arg(query));
    }
    else if (m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[source] %1 matches for '%2'").arg(added).arg(query));
    }
}

void WorkbenchWindow::openSelectedSourceResult()
{
    if (m_sourcePanel == nullptr)
    {
        return;
    }

    const occtdebug::SourceSearchResult selected = m_sourcePanel->currentSearchResult();
    const QString filePath = selected.filePath;
    const int lineNumber = selected.lineNumber;
    if (filePath.isEmpty() || lineNumber <= 0)
    {
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        m_sourcePanel->setSourceText(QStringLiteral("Failed to open source file: %1").arg(QDir::toNativeSeparators(filePath)));
        return;
    }

    const QString text = QString::fromUtf8(file.readAll());
    m_sourcePanel->showSourceTextAtLine(text, lineNumber);

    if (m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[source] opened %1:%2")
                .arg(QDir::toNativeSeparators(filePath))
                .arg(lineNumber));
    }
}

void WorkbenchWindow::activateEvidenceRecord(const occtdebug::EvidenceRecord& evidence)
{
    if (openEvidenceSourceReference(evidence))
    {
        return;
    }
    if (selectGeometryEvidence(evidence))
    {
        return;
    }
    if (openEvidenceArtifact(evidence))
    {
        return;
    }

    if (m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[evidence] no jump target for '%1' (%2)")
                .arg(evidence.title, evidence.link));
    }
}

bool WorkbenchWindow::openEvidenceSourceReference(const occtdebug::EvidenceRecord& evidence)
{
    if (m_sourcePanel == nullptr)
    {
        return false;
    }

    QString sourceName = evidence.sourceFile.trimmed();
    int lineNumber = evidence.sourceLine;
    if (sourceName.isEmpty() || lineNumber <= 0)
    {
        const EvidenceLinkTarget target = splitTrailingLineReference(evidence.link);
        if (!target.path.isEmpty()
            && !isCaseArtifactReference(target.path)
            && isRelativeLocalReference(target.path)
            && target.line > 0)
        {
            sourceName = target.path;
            lineNumber = target.line;
        }
    }

    if ((sourceName.isEmpty() || lineNumber <= 0) && !evidence.stackFrame.trimmed().isEmpty())
    {
        const QRegularExpression sourceInFrame(QStringLiteral("([A-Za-z0-9_./\\\\-]+\\.(?:cxx|cpp|hxx|hpp|h|c)):(\\d+)"));
        const QRegularExpressionMatch frameMatch = sourceInFrame.match(evidence.stackFrame);
        if (frameMatch.hasMatch())
        {
            sourceName = frameMatch.captured(1).trimmed();
            lineNumber = frameMatch.captured(2).toInt();
        }
    }

    if (sourceName.isEmpty() || lineNumber <= 0 || !isRelativeLocalReference(sourceName))
    {
        return false;
    }

    if (m_workspaceTabs != nullptr)
    {
        m_workspaceTabs->setCurrentIndex(centerTabIndexForId(QStringLiteral("source")));
    }

    const QString resolvedPath = resolveSourceReferencePath(sourceName);
    if (!resolvedPath.isEmpty())
    {
        QFile file(resolvedPath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            m_sourcePanel->showSourceTextAtLine(QString::fromUtf8(file.readAll()), lineNumber);
            if (m_cmakeConsole != nullptr)
            {
                m_cmakeConsole->append(QStringLiteral("[evidence] source %1:%2")
                        .arg(QDir::toNativeSeparators(resolvedPath))
                        .arg(lineNumber));
            }
            return true;
        }
    }

    QString fallback = QStringLiteral("Source reference was not resolved on disk: %1:%2\n\n").arg(sourceName).arg(lineNumber);
    if (!evidence.stackFrame.isEmpty())
    {
        fallback += QStringLiteral("Stack frame: %1\n\n").arg(evidence.stackFrame);
    }
    fallback += m_data.sourceText.isEmpty() ? evidence.summary : m_data.sourceText;
    m_sourcePanel->setSourceText(fallback);
    if (m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[evidence] source reference not found on disk: %1:%2")
                .arg(sourceName)
                .arg(lineNumber));
    }
    return true;
}

bool WorkbenchWindow::openEvidenceArtifact(const occtdebug::EvidenceRecord& evidence)
{
    EvidenceLinkTarget target;
    if (!evidence.logFile.trimmed().isEmpty())
    {
        target.path = evidence.logFile.trimmed();
        target.line = evidence.logLine;
    }
    else
    {
        target = splitTrailingLineReference(evidence.link);
    }

    const QString link = target.path.trimmed();
    if (link.isEmpty() || !isRelativeLocalReference(link) || m_data.workspaceRoot.isEmpty())
    {
        return false;
    }

    const QString targetPath = QDir::cleanPath(QDir(m_data.workspaceRoot).filePath(link));
    QString workspaceRoot = QDir::cleanPath(QFileInfo(m_data.workspaceRoot).absoluteFilePath());
    QString absoluteTarget = QDir::cleanPath(QFileInfo(targetPath).absoluteFilePath());
#ifdef Q_OS_WIN
    workspaceRoot = workspaceRoot.toLower();
    absoluteTarget = absoluteTarget.toLower();
#endif
    if (absoluteTarget != workspaceRoot && !absoluteTarget.startsWith(workspaceRoot + QLatin1Char('/')))
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[evidence] blocked artifact outside case: %1").arg(link));
        }
        return true;
    }

    const QString text = readTextFile(targetPath);
    const bool isLog = link.startsWith(QStringLiteral("logs/"), Qt::CaseInsensitive) || link.endsWith(QStringLiteral(".log"), Qt::CaseInsensitive);
    if (isLog)
    {
        QTextEdit* console = link.contains(QStringLiteral("draw"), Qt::CaseInsensitive) ? m_drawConsole : m_cmakeConsole;
        if (m_bottomTabs != nullptr)
        {
            m_bottomTabs->setCurrentIndex(link.contains(QStringLiteral("draw"), Qt::CaseInsensitive) ? 0 : 1);
        }
        if (console != nullptr)
        {
            console->setPlainText(text.isEmpty()
                    ? QStringLiteral("[evidence] log missing or empty: %1").arg(QDir::toNativeSeparators(targetPath))
                    : text);
            scrollTextEditToLine(console, target.line);
        }
    }
    else if (m_cmakeConsole != nullptr)
    {
        if (m_workspaceTabs != nullptr)
        {
            m_workspaceTabs->setCurrentIndex(centerTabIndexForId(QStringLiteral("evidence")));
        }
        m_cmakeConsole->append(text.isEmpty()
                ? QStringLiteral("[evidence] artifact missing or empty: %1").arg(QDir::toNativeSeparators(targetPath))
                : QStringLiteral("[evidence] artifact %1\n%2").arg(QDir::toNativeSeparators(targetPath), text.left(4000)));
        scrollTextEditToLine(m_cmakeConsole, target.line);
    }
    return true;
}

bool WorkbenchWindow::selectGeometryEvidence(const occtdebug::EvidenceRecord& evidence)
{
    const QString haystack = QStringLiteral("%1 %2 %3 %4")
        .arg(evidence.type, evidence.title, evidence.summary, evidence.link)
        .toLower();
    const bool isGeometryEvidence = haystack.contains(QStringLiteral("shape"))
        || haystack.contains(QStringLiteral("geometry"))
        || haystack.contains(QStringLiteral("edge"))
        || haystack.contains(QStringLiteral("curve"))
        || haystack.contains(QStringLiteral("brep"));
    if (!isGeometryEvidence)
    {
        return false;
    }

    if (m_workspaceTabs != nullptr)
    {
        m_workspaceTabs->setCurrentIndex(centerTabIndexForId(QStringLiteral("geometry")));
    }
    QString viewerStatus = QStringLiteral("Viewer highlight: not requested");
    const QString objectText = evidence.geometryObject.isEmpty()
        ? QStringLiteral("unspecified object")
        : evidence.geometryObject;
    if (!evidence.geometryObject.trimmed().isEmpty())
    {
        if (ensureGeometryViewer())
        {
            QString highlightError;
            if (m_geometryViewer->highlightGeometryObject(evidence.geometryObject, &highlightError))
            {
                viewerStatus = QStringLiteral("Viewer highlight: %1").arg(m_geometryViewer->highlightedObjectId());
            }
            else
            {
                viewerStatus = QStringLiteral("Viewer highlight failed: %1").arg(highlightError);
            }
        }
        else
        {
            viewerStatus = QStringLiteral("Viewer highlight failed: viewer is not available");
        }
    }
    if (m_geometrySummaryLabel != nullptr)
    {
        m_geometrySummaryLabel->setText(QStringLiteral("Evidence selected: %1 - %2\nGeometry object: %3\n%4")
                .arg(evidence.title, evidence.summary, objectText, viewerStatus));
    }

    if (m_geometryTable != nullptr && m_geometryTable->rowCount() > 0)
    {
        int selectedRow = 0;
        const QString objectNeedle = evidence.geometryObject.trimmed().toLower();
        bool matchedObject = false;
        if (!objectNeedle.isEmpty())
        {
            for (int row = 0; row < m_data.geometryChecks.size(); ++row)
            {
                const QString rowText = QStringLiteral("%1 %2")
                    .arg(m_data.geometryChecks[row].name, m_data.geometryChecks[row].note)
                    .toLower();
                if (rowText.contains(objectNeedle))
                {
                    selectedRow = row;
                    matchedObject = true;
                    break;
                }
            }
        }
        for (int row = 0; !matchedObject && row < m_data.geometryChecks.size(); ++row)
        {
            const QString status = m_data.geometryChecks[row].status.toLower();
            if (status.contains(QStringLiteral("fail")) || status.contains(QStringLiteral("warn")))
            {
                selectedRow = row;
                break;
            }
        }
        m_geometryTable->selectRow(selectedRow);
        m_geometryTable->scrollToItem(m_geometryTable->item(selectedRow, 0));
    }

    if (m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[evidence] geometry context: %1 (%2) %3")
                .arg(evidence.title, evidence.link, viewerStatus));
    }
    return true;
}

QString WorkbenchWindow::resolveSourceReferencePath(const QString& fileName) const
{
    const QString normalizedFile = QDir::cleanPath(fileName.trimmed());
    if (normalizedFile.isEmpty() || QFileInfo(normalizedFile).isAbsolute())
    {
        return QString();
    }

    const QDir repo(sourceRootDirectory());
    QStringList candidates {
        repo.filePath(normalizedFile),
        repo.filePath(QStringLiteral("src/%1").arg(normalizedFile)),
        repo.filePath(QStringLiteral("tests/%1").arg(normalizedFile)),
        repo.filePath(QStringLiteral("depends/occt/include/%1").arg(normalizedFile)),
    };
    if (!m_data.workspaceRoot.isEmpty())
    {
        candidates << QDir(m_data.workspaceRoot).filePath(normalizedFile);
        candidates << QDir(m_data.workspaceRoot).filePath(QStringLiteral("repro/%1").arg(normalizedFile));
    }

    for (const QString& candidate : candidates)
    {
        if (QFileInfo::exists(candidate) && QFileInfo(candidate).isFile())
        {
            return QFileInfo(candidate).absoluteFilePath();
        }
    }

    const QString baseName = QFileInfo(normalizedFile).fileName();
    if (baseName.isEmpty())
    {
        return QString();
    }

    const QStringList roots {
        repo.filePath(QStringLiteral("src")),
        repo.filePath(QStringLiteral("tests")),
        repo.filePath(QStringLiteral("depends/occt/include")),
    };
    for (const QString& root : roots)
    {
        if (!QFileInfo::exists(root))
        {
            continue;
        }
        QDirIterator it(root, QStringList {baseName}, QDir::Files, QDirIterator::Subdirectories);
        if (it.hasNext())
        {
            return QFileInfo(it.next()).absoluteFilePath();
        }
    }
    return QString();
}

void WorkbenchWindow::refreshSimilarCases(const QString& query)
{
    if (m_similarCasesList == nullptr)
    {
        return;
    }

    const QString effectiveQuery = query.trimmed().isEmpty()
        ? QStringLiteral("%1 %2").arg(m_data.diagnosis, m_data.evidenceSummary)
        : query;
    const QVector<occtdebug::SimilarCaseMatch> matches =
        occtdebug::SimilarCaseSearch::search(m_data.manifest.similarCases.isEmpty() ? m_data.similarCases : m_data.manifest.similarCases,
                                             effectiveQuery,
                                             20);

    m_similarCasesList->clear();
    if (matches.isEmpty())
    {
        m_similarCasesList->addItem(QStringLiteral("No similar case match."));
        return;
    }

    m_data.similarCases.clear();
    for (const auto& match : matches)
    {
        m_data.similarCases.push_back(match.similarCase);
        m_similarCasesList->addItem(QStringLiteral("%1  %2   score %3")
                .arg(match.similarCase.id, match.similarCase.title)
                .arg(match.score));
    }
}

QString WorkbenchWindow::caseRootDirectory() const
{
    if (!m_data.workspaceRoot.isEmpty())
    {
        return QFileInfo(m_data.workspaceRoot).absoluteDir().absolutePath();
    }
    return QDir(sourceRootDirectory()).filePath(QStringLiteral("cases"));
}

QString WorkbenchWindow::caseDirectory(const QString& caseId) const
{
    return QDir(caseRootDirectory()).filePath(sanitizedCaseId(caseId));
}

QString WorkbenchWindow::generateCaseId() const
{
    const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"));
    return QStringLiteral("OCC-LOCAL-%1").arg(stamp);
}

void WorkbenchWindow::applyWorkbenchData(occtdebug::WorkbenchMockData nextData)
{
    m_data = std::move(nextData);
    m_patchReview = occtdebug::PatchReviewWorkflow::createDefault(m_data.caseId, m_data.patchDiff);
    if (m_data.patchReviewStatus == QStringLiteral("Needs review"))
    {
        m_patchReview.markNeedsReview(QStringLiteral("Restored from case manifest."));
    }
    else if (m_data.patchReviewStatus == QStringLiteral("Approved"))
    {
        m_patchReview.approve(QStringLiteral("Restored from case manifest."));
    }
    else if (m_data.patchReviewStatus == QStringLiteral("Rejected"))
    {
        m_patchReview.reject(QStringLiteral("Restored from case manifest."));
    }

    if (m_caseBadge != nullptr)
    {
        m_caseBadge->setText(QStringLiteral("濡楀牅绶ラ敍?1").arg(m_data.caseId));
    }
    if (m_statusBadge != nullptr)
    {
        m_statusBadge->setText(QString::fromUtf8("閳?閻樿埖鈧緤绱?1").arg(m_data.caseStatus));
    }
    if (m_sourcePanel != nullptr)
    {
        m_sourcePanel->setSourceText(m_data.sourceText);
        m_sourcePanel->clearSearchResults();
    }
    if (m_reproScriptEdit != nullptr)
    {
        m_reproScriptEdit->setPlainText(m_data.reproScript);
    }
    if (m_geometrySummaryLabel != nullptr)
    {
        m_geometrySummaryLabel->setText(m_data.geometrySummary);
    }
    refreshGeometryChecks();
    refreshDiffArtifactTables();
    if (m_environmentText != nullptr)
    {
        m_environmentText->setPlainText(m_data.environmentSummary);
    }
    if (m_diagnosisLabel != nullptr)
    {
        m_diagnosisLabel->setText(m_data.diagnosis);
    }
    if (m_confidenceBar != nullptr)
    {
        m_confidenceBar->setValue(m_data.diagnosisConfidence);
    }
    if (m_patchDiffEdit != nullptr)
    {
        m_patchDiffEdit->setPlainText(m_data.patchDiff);
    }
    if (m_drawConsole != nullptr)
    {
        m_drawConsole->setPlainText(m_data.drawConsoleText);
    }
    if (m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->setPlainText(m_data.cmakeConsoleText);
    }
    if (m_bottomTabs != nullptr && m_data.manifest.workspaceLayout.bottomHeight > 0)
    {
        m_bottomTabs->setFixedHeight(m_data.manifest.workspaceLayout.bottomHeight);
    }
    refreshTaskHistoryPanel();
    if (m_mainSplitter != nullptr)
    {
        m_mainSplitter->setSizes({
            m_data.manifest.workspaceLayout.leftWidth,
            m_data.manifest.workspaceLayout.centerWidth,
            m_data.manifest.workspaceLayout.rightWidth,
        });
    }
    if (m_workspaceTabs != nullptr)
    {
        m_workspaceTabs->setCurrentIndex(centerTabIndexForId(m_data.manifest.workspaceLayout.activeCenterTab));
    }
    if (m_bottomTabs != nullptr)
    {
        m_bottomTabs->setCurrentIndex(bottomTabIndexForId(m_data.manifest.workspaceLayout.activeBottomTab));
    }

    if (m_casePanel != nullptr)
    {
        m_casePanel->setData(m_data);
    }

    if (m_verificationPanel != nullptr)
    {
        m_verificationPanel->setItems(m_data.verificationItems);
    }

    if (m_evidencePanel != nullptr)
    {
        m_evidencePanel->setData(m_data.evidenceSummary, m_data.evidenceItems);
    }

    occtdebug::TestgridTablePresenter::applyToTable(m_testgridTable, m_data.testgridRows);
    if (m_testgridGroupEdit != nullptr)
    {
        m_testgridGroupEdit->setText(m_data.verificationPlan.testgridGroup);
    }
    if (m_testgridGridEdit != nullptr)
    {
        m_testgridGridEdit->setText(m_data.verificationPlan.testgridGrid);
    }
    if (m_testgridCaseEdit != nullptr)
    {
        m_testgridCaseEdit->setText(m_data.verificationPlan.testgridCase);
    }

    if (m_similarCasesList != nullptr)
    {
        refreshSimilarCases(QStringLiteral(" "));
    }

    updatePatchReviewStatus();
    refreshCaseList();
}

void WorkbenchWindow::createNewCase()
{
    bool accepted = false;
    const QString title = QInputDialog::getText(
        this,
        s("閺傛澘缂?Case"),
        s("闂傤噣顣介弽鍥暯"),
        QLineEdit::Normal,
        QString(),
        &accepted);
    if (!accepted)
    {
        return;
    }

    const QString caseId = generateCaseId();
    const QString workspace = caseDirectory(caseId);
    QDir root;
    if (!root.mkpath(workspace))
    {
        QMessageBox::warning(this, s("閺傛澘缂?Case"), QStringLiteral("閺冪姵纭堕崚娑樼紦閻╊喖缍嶉敍?1").arg(QDir::toNativeSeparators(workspace)));
        return;
    }

    const QDir workspaceDir(workspace);
    for (const QString& relativeDirectory : standardCaseDirectories())
    {
        if (!workspaceDir.mkpath(relativeDirectory))
        {
            QMessageBox::warning(this, s("閺傛澘缂?Case"), QStringLiteral("閺冪姵纭堕崚娑樼紦鐎涙劗娲拌ぐ鏇窗%1").arg(relativeDirectory));
            return;
        }
    }

    occtdebug::CaseManifest manifest;
    manifest.caseId = caseId;
    manifest.title = title.trimmed().isEmpty() ? QStringLiteral("Untitled OCCT Case") : title.trimmed();
    manifest.status = QStringLiteral("Draft / Intake");
    manifest.createdAt = QDateTime::currentDateTime().toString(Qt::ISODate);
    manifest.occtVersion = m_data.occtVersion;
    manifest.toolchain = m_data.toolchain;
    manifest.platform = m_data.platform;
    manifest.caseList = {{caseId, manifest.status, manifest.title, manifest.createdAt}};
    manifest.workflowSteps = {
        {QStringLiteral("problem_intake"), QStringLiteral("[active]"), QStringLiteral("1 Problem intake"), QStringLiteral("In progress"), QString()},
        {QStringLiteral("environment_capture"), QStringLiteral("[todo]"), QStringLiteral("2 Environment capture"), QStringLiteral("Not started"), QString()},
        {QStringLiteral("draw_reproduction"), QStringLiteral("[todo]"), QStringLiteral("3 DRAW reproduction"), QStringLiteral("Not started"), QString()},
        {QStringLiteral("evidence_collection"), QStringLiteral("[todo]"), QStringLiteral("4 Evidence collection"), QStringLiteral("Not started"), QString()},
        {QStringLiteral("report_archive"), QStringLiteral("[todo]"), QStringLiteral("5 Report / archive"), QStringLiteral("Not started"), QString()},
    };
    manifest.workflowState.activeStepId = QStringLiteral("problem_intake");
    manifest.workflowState.steps = manifest.workflowSteps;
    manifest.workspaceLayout.activeCenterTab = QStringLiteral("repro");
    manifest.workspaceLayout.activeBottomTab = QStringLiteral("draw");
    manifest.keyInputs = {
        {QStringLiteral("Input model"), QStringLiteral("not selected")},
        {QStringLiteral("Reproduction type"), QStringLiteral("DRAW Tcl script")},
        {QStringLiteral("Failure mode"), QStringLiteral("not classified")},
        {QStringLiteral("Confidentiality"), QStringLiteral("internal")},
    };
    manifest.sourceText = QStringLiteral("No source location selected yet.");
    manifest.reproScript = QStringLiteral("# Minimal DRAW repro script\nputs {OCCTDEBUG_REPRO_PLACEHOLDER_OK}\nquit\n");
    manifest.geometrySummary = QStringLiteral("No model loaded yet.");
    manifest.evidenceSummary = QStringLiteral("No evidence collected yet.");
    manifest.diffSummary = QStringLiteral("No diff or testdiff result yet.");
    manifest.environmentSummary = QStringLiteral("Environment has not been captured yet.");
    manifest.diagnosis = QStringLiteral("No diagnosis yet.");
    manifest.diagnosisConfidence = 0;
    manifest.patchDiff = QStringLiteral("# No patch candidate yet.");
    manifest.patchReviewStatus = QStringLiteral("Draft");
    manifest.patchWorktreeRoot = m_data.patchWorktreeRoot;
    manifest.patchApplyStatus = QStringLiteral("not configured");
    manifest.patchApplyLog = QString();
    manifest.patchSignoffStatus = QStringLiteral("not requested");
    manifest.patchSignoffNote = QString();
    manifest.patchReviewItems = {
        {QStringLiteral("Candidate patch captured"), QStringLiteral("Pending")},
        {QStringLiteral("Root-cause evidence linked"), QStringLiteral("Pending")},
        {QStringLiteral("Reviewer decision"), QStringLiteral("Pending")},
        {QStringLiteral("Regression gate"), QStringLiteral("Pending")},
    };
    manifest.drawConsoleText = QStringLiteral("DRAW console is ready.");
    manifest.cmakeConsoleText = QStringLiteral("Case created. Next: capture environment and run DRAW.");
    manifest.verificationItems = {
        {QStringLiteral("Original issue"), QStringLiteral("not verified")},
        {QStringLiteral("DRAW smoke"), QStringLiteral("not run")},
        {QStringLiteral("testgrid"), QStringLiteral("not connected")},
    };
    manifest.verificationPlan = m_data.verificationPlan;

    QString error;
    if (!manifest.saveToFile(workspaceDir.filePath(QStringLiteral("case.json")), &error))
    {
        QMessageBox::warning(this, s("閺傛澘缂?Case"), error);
        return;
    }

    occtdebug::WorkbenchMockData newCaseData = occtdebug::createWorkbenchDataFromCase(manifest);
    newCaseData.workspaceRoot = workspaceDir.absolutePath();
    applyWorkbenchData(std::move(newCaseData));
    if (m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[case] created %1").arg(caseId));
    }
}

void WorkbenchWindow::openCaseDirectory()
{
    const QString directory = QFileDialog::getExistingDirectory(
        this,
        s("Open Case Directory"),
        caseRootDirectory());
    if (directory.isEmpty())
    {
        return;
    }

    const QString manifestPath = QDir(directory).filePath(QStringLiteral("case.json"));
    if (!QFileInfo::exists(manifestPath))
    {
        QMessageBox::warning(this, s("閹垫挸绱?Case"), QStringLiteral("閹碘偓闁娲拌ぐ鏇氱瑝閸栧懎鎯?case.json閿?1").arg(QDir::toNativeSeparators(directory)));
        return;
    }

    QString error;
    occtdebug::WorkbenchMockData openedCaseData = occtdebug::createWorkbenchDataFromCaseDirectory(directory, &error);
    if (!error.isEmpty())
    {
        QMessageBox::warning(this, s("閹垫挸绱?Case"), error);
        return;
    }
    applyWorkbenchData(std::move(openedCaseData));
    if (m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[case] opened %1").arg(QDir::toNativeSeparators(directory)));
    }
}

void WorkbenchWindow::loadCaseById(const QString& caseId)
{
    if (caseId.isEmpty() || caseId == m_data.caseId)
    {
        return;
    }

    saveCurrentCaseManifest();

    const QString directory = caseDirectory(caseId);
    QString error;
    occtdebug::WorkbenchMockData selectedCaseData = occtdebug::createWorkbenchDataFromCaseDirectory(directory, &error);
    if (!error.isEmpty())
    {
        QMessageBox::warning(this, s("閸掑洦宕?Case"), error);
        return;
    }
    applyWorkbenchData(std::move(selectedCaseData));
    if (m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[case] switched to %1").arg(caseId));
    }
}

QString WorkbenchWindow::runtimeReproDirectory() const
{
    if (!m_data.workspaceRoot.isEmpty())
    {
        return QDir(m_data.workspaceRoot).filePath(QStringLiteral("repro"));
    }
    return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("runtime/%1/repro").arg(m_data.caseId));
}

QString WorkbenchWindow::runtimeReportDirectory() const
{
    if (!m_data.workspaceRoot.isEmpty())
    {
        return QDir(m_data.workspaceRoot).filePath(QStringLiteral("report"));
    }
    return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("runtime/%1/report").arg(m_data.caseId));
}

QString WorkbenchWindow::runtimeLogDirectory() const
{
    if (!m_data.workspaceRoot.isEmpty())
    {
        return QDir(m_data.workspaceRoot).filePath(QStringLiteral("logs"));
    }
    return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("runtime/%1/logs").arg(m_data.caseId));
}

QString WorkbenchWindow::runtimeArtifactDirectory() const
{
    if (!m_data.workspaceRoot.isEmpty())
    {
        return QDir(m_data.workspaceRoot).filePath(QStringLiteral("artifacts"));
    }
    return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("runtime/%1/artifacts").arg(m_data.caseId));
}

QString WorkbenchWindow::runtimeEnvDirectory() const
{
    if (!m_data.workspaceRoot.isEmpty())
    {
        return QDir(m_data.workspaceRoot).filePath(QStringLiteral("env"));
    }
    return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("runtime/%1/env").arg(m_data.caseId));
}

QString WorkbenchWindow::runtimeInputDirectory() const
{
    if (!m_data.workspaceRoot.isEmpty())
    {
        return QDir(m_data.workspaceRoot).filePath(QStringLiteral("input"));
    }
    return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("runtime/%1/input").arg(m_data.caseId));
}

QString WorkbenchWindow::runtimeVerificationDirectory() const
{
    if (!m_data.workspaceRoot.isEmpty())
    {
        return QDir(m_data.workspaceRoot).filePath(QStringLiteral("verification"));
    }
    return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("runtime/%1/verification").arg(m_data.caseId));
}

QString WorkbenchWindow::currentReproScriptPath() const
{
    return QDir(runtimeReproDirectory()).filePath(QStringLiteral("repro.tcl"));
}

QString WorkbenchWindow::environmentSnapshotPath() const
{
    return QDir(runtimeEnvDirectory()).filePath(QStringLiteral("env_snapshot.json"));
}

QString WorkbenchWindow::sourceRootDirectory() const
{
#ifdef OCCTDEBUG_SOURCE_DIR
    return QStringLiteral(OCCTDEBUG_SOURCE_DIR);
#else
    return QCoreApplication::applicationDirPath();
#endif
}

QString WorkbenchWindow::buildRootDirectory() const
{
#ifdef OCCTDEBUG_BINARY_DIR
    return QStringLiteral(OCCTDEBUG_BINARY_DIR);
#else
    return QCoreApplication::applicationDirPath();
#endif
}

QString WorkbenchWindow::findDrawExecutable() const
{
    QStringList candidates;
#ifdef OCCTDEBUG_SOURCE_DIR
    const QString sourceRoot = sourceRootDirectory();
    candidates << QDir(sourceRoot).filePath(QStringLiteral("depends/occt/lib/Debug/bind/DRAWEXE.exe"));
    candidates << QDir(sourceRoot).filePath(QStringLiteral("depends/occt/lib/Release/bin/DRAWEXE.exe"));
    candidates << QDir(sourceRoot).filePath(QStringLiteral("depends/occt/lib/RelWithDebInfo/bini/DRAWEXE.exe"));
#endif
    for (const QString& candidate : candidates)
    {
        if (QFileInfo::exists(candidate))
        {
            return QFileInfo(candidate).absoluteFilePath();
        }
    }
    return QString();
}

bool WorkbenchWindow::saveCurrentReproScript()
{
    if (m_reproScriptEdit == nullptr)
    {
        return false;
    }

    QDir dir;
    if (!dir.mkpath(runtimeReproDirectory()))
    {
        if (m_drawConsole != nullptr)
        {
            m_drawConsole->append(QStringLiteral("[repro] failed to create runtime directory"));
        }
        return false;
    }

    QFile file(currentReproScriptPath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        if (m_drawConsole != nullptr)
        {
            m_drawConsole->append(QStringLiteral("[repro] failed to write %1: %2").arg(file.fileName(), file.errorString()));
        }
        return false;
    }

    file.write(m_reproScriptEdit->toPlainText().toUtf8());
    m_data.reproScript = m_reproScriptEdit->toPlainText();
    m_data.manifest.reproScript = m_data.reproScript;
    saveCurrentCaseManifest();
    if (m_drawConsole != nullptr)
    {
        m_drawConsole->append(QStringLiteral("[repro] saved %1").arg(QDir::toNativeSeparators(file.fileName())));
    }
    return true;
}

bool WorkbenchWindow::saveCurrentCaseManifest()
{
    if (m_data.workspaceRoot.isEmpty())
    {
        return true;
    }

    if (m_reproScriptEdit != nullptr)
    {
        m_data.reproScript = m_reproScriptEdit->toPlainText();
    }
    occtdebug::CaseManifestSync::syncMutableFields(m_data.manifest, m_data);
    if (m_bottomTabs != nullptr)
    {
        m_data.manifest.workspaceLayout.bottomHeight = m_bottomTabs->height();
        m_data.manifest.workspaceLayout.activeBottomTab = bottomTabIdForIndex(m_bottomTabs->currentIndex());
    }
    if (m_workspaceTabs != nullptr)
    {
        m_data.manifest.workspaceLayout.activeCenterTab = centerTabIdForIndex(m_workspaceTabs->currentIndex());
    }
    if (m_mainSplitter != nullptr)
    {
        const QList<int> sizes = m_mainSplitter->sizes();
        if (sizes.size() >= 3)
        {
            m_data.manifest.workspaceLayout.leftWidth = sizes[0];
            m_data.manifest.workspaceLayout.centerWidth = sizes[1];
            m_data.manifest.workspaceLayout.rightWidth = sizes[2];
        }
    }

    QString error;
    const QString manifestPath = QDir(m_data.workspaceRoot).filePath(QStringLiteral("case.json"));
    refreshEvidenceBundle();
    refreshVerificationReport();
    if (!m_data.manifest.saveToFile(manifestPath, &error))
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[case] failed to save manifest: %1").arg(error));
        }
        return false;
    }
    return true;
}

bool WorkbenchWindow::refreshReports(const occtdebug::ReportRefreshRequest& request)
{
    if (m_data.workspaceRoot.isEmpty()
        || (!request.writeEvidenceBundle && !request.writeVerificationReport))
    {
        return true;
    }

    const occtdebug::ReportRefreshResult result = occtdebug::ReportRefreshCoordinator::refresh(
        m_data.manifest,
        {
            m_data.workspaceRoot,
            runtimeArtifactDirectory(),
            runtimeReportDirectory(),
            runtimeVerificationDirectory(),
        },
        request);
    if (!result.success && m_cmakeConsole != nullptr)
    {
        for (const QString& error : result.errors)
        {
            m_cmakeConsole->append(error);
        }
    }
    return result.success;
}

bool WorkbenchWindow::refreshEvidenceBundle()
{
    return refreshReports({true, false, QStringLiteral("evidence")});
}

bool WorkbenchWindow::refreshVerificationReport()
{
    return refreshReports({false, true, QStringLiteral("verification")});
}

void WorkbenchWindow::runCurrentDrawScript()
{
    if (m_drawRunner == nullptr || m_drawRunner->isRunning())
    {
        if (m_drawConsole != nullptr)
        {
            m_drawConsole->append(QStringLiteral("[DRAW] runner is busy"));
        }
        return;
    }

    if (!saveCurrentReproScript())
    {
        return;
    }

    const QString drawExe = findDrawExecutable();
    if (drawExe.isEmpty())
    {
        if (m_drawConsole != nullptr)
        {
            m_drawConsole->append(QStringLiteral("[DRAW] DRAWEXE.exe was not found under depends/occt"));
        }
        return;
    }

    const QString scriptPath = currentReproScriptPath();
    const QString command = QStringLiteral("\"%1\" < \"%2\"")
            .arg(QDir::toNativeSeparators(drawExe), QDir::toNativeSeparators(scriptPath));

    occtdebug::CommandRequest request;
    request.program = QStringLiteral("cmd.exe");
    request.arguments = {QStringLiteral("/c"), command};
    request.workingDirectory = runtimeReproDirectory();
    request.environment = QProcessEnvironment::systemEnvironment();
    request.timeoutMs = kDrawCommandTimeoutMs;

    const QString drawBinDir = QFileInfo(drawExe).absolutePath();
    QString casRoot = drawBinDir;
    QStringList pathEntries;
    pathEntries << drawBinDir;
#ifdef OCCTDEBUG_SOURCE_DIR
    const QString sourceRoot = QStringLiteral(OCCTDEBUG_SOURCE_DIR);
    casRoot = QDir(sourceRoot).filePath(QStringLiteral("depends/occt"));
    const QDir thirdPartyRoot(QDir(sourceRoot).filePath(QStringLiteral("depends/occt_3rdparty")));
    const QString freetypeBinDir = thirdPartyRoot.filePath(QStringLiteral("freetype-2.13.3-x64/bin"));
    const QString tcltkBinDir = thirdPartyRoot.filePath(QStringLiteral("tcltk-8.6.15-x64/bin"));
    const QString tcltkLibDir = thirdPartyRoot.filePath(QStringLiteral("tcltk-8.6.15-x64/lib"));
    pathEntries << freetypeBinDir << tcltkBinDir << tcltkLibDir;
    request.environment.insert(QStringLiteral("TCL_LIBRARY"), QDir(tcltkLibDir).filePath(QStringLiteral("tcl8.6")));
    request.environment.insert(QStringLiteral("TK_LIBRARY"), QDir(tcltkLibDir).filePath(QStringLiteral("tk8.6")));
#endif

    request.environment.insert(QStringLiteral("CASROOT"), casRoot);
    request.environment.insert(QStringLiteral("PATH"),
        pathEntries.join(QLatin1Char(';')) + QStringLiteral(";") + request.environment.value(QStringLiteral("PATH")));

    if (m_drawConsole != nullptr)
    {
        m_drawConsole->append(QStringLiteral("[DRAW] %1%2").arg(command, timeoutSuffix(request.timeoutMs)));
    }

    QString error;
    if (!m_drawRunner->start(request, &error))
    {
        if (m_drawConsole != nullptr)
        {
            m_drawConsole->append(QStringLiteral("[DRAW] failed to start: %1").arg(error));
        }
        return;
    }
    recordTaskStarted(QStringLiteral("draw"),
                      QStringLiteral("DRAW repro"),
                      request,
                      QStringLiteral("artifacts/draw_result.json"),
                      QStringLiteral("logs/draw.stdout.log"),
                      QStringLiteral("logs/draw.stderr.log"));
}

void WorkbenchWindow::generateCppReproTemplate()
{
    if (m_data.workspaceRoot.trimmed().isEmpty())
    {
        if (m_drawConsole != nullptr)
        {
            m_drawConsole->append(QStringLiteral("[repro] current Case workspace is empty"));
        }
        return;
    }

    if (!saveCurrentReproScript())
    {
        return;
    }

    const occtdebug::CppReproTemplateResult result =
        occtdebug::CppReproTemplateWriter::write(
            m_data.workspaceRoot,
            m_data.caseId,
            m_data.reproScript);
    if (!result.success)
    {
        if (m_drawConsole != nullptr)
        {
            m_drawConsole->append(QStringLiteral("[repro] failed to generate C++ repro template: %1").arg(result.error));
        }
        return;
    }

    occtdebug::EvidenceRecord evidence;
    evidence.type = QStringLiteral("Repro");
    evidence.title = QStringLiteral("C++ minimal repro scaffold");
    evidence.summary = QStringLiteral("Generated %1 files under %2")
        .arg(result.writtenFiles.size())
        .arg(result.rootDirectory);
    evidence.link = QStringLiteral("%1/README.md").arg(result.rootDirectory);
    appendEvidenceRecord(evidence);
    m_data.reproStatus = occtdebug::ReproStatusEvaluator::withCppScaffold(
        m_data.reproStatus,
        result,
        currentUtcIsoTimestamp());
    updateReproStatusMetric();
    saveCurrentCaseManifest();
    refreshEvidenceBundle();
    refreshVerificationReport();

    if (m_drawConsole != nullptr)
    {
        m_drawConsole->append(QStringLiteral("[repro] C++ repro template generated under %1").arg(result.rootDirectory));
        for (const QString& file : result.writtenFiles)
        {
            m_drawConsole->append(QStringLiteral("[repro] wrote %1").arg(file));
        }
    }
}

void WorkbenchWindow::cancelRunner(occtdebug::CommandRunner* runner, const QString& labelText, QTextEdit* console)
{
    if (runner == nullptr || !runner->isRunning())
    {
        if (console != nullptr)
        {
            console->append(QStringLiteral("[%1] no running command to cancel").arg(labelText));
        }
        return;
    }

    if (console != nullptr)
    {
        console->append(QStringLiteral("[%1] cancel requested").arg(labelText));
    }
    runner->cancel();
}

void WorkbenchWindow::recordTaskStarted(const QString& id,
                                        const QString& title,
                                        const occtdebug::CommandRequest& request,
                                        const QString& artifact,
                                        const QString& stdoutLog,
                                        const QString& stderrLog)
{
    occtdebug::TaskRecord record;
    record.id = id;
    record.title = title;
    record.status = QStringLiteral("running");
    record.program = QFileInfo(request.program).fileName();
    record.arguments = request.arguments.join(QLatin1Char(' '));
    record.workingDirectory = caseRelativeOrFileName(m_data.workspaceRoot, request.workingDirectory);
    record.startedAt = currentUtcIsoTimestamp();
    record.artifact = artifact;
    record.stdoutLog = stdoutLog;
    record.stderrLog = stderrLog;

    m_data.taskHistory.push_back(record);
    if (m_data.taskHistory.size() > 200)
    {
        m_data.taskHistory.erase(m_data.taskHistory.begin(), m_data.taskHistory.begin() + (m_data.taskHistory.size() - 200));
    }
    m_data.manifest.taskHistory = m_data.taskHistory;
    refreshTaskHistoryPanel();
}

void WorkbenchWindow::recordTaskFinished(const QString& id,
                                         const occtdebug::CommandResult& result,
                                         const QString& artifact,
                                         const QString& stdoutLog,
                                         const QString& stderrLog,
                                         const QString& note)
{
    auto match = std::find_if(m_data.taskHistory.rbegin(), m_data.taskHistory.rend(), [&](const occtdebug::TaskRecord& task) {
        return task.id == id && task.status == QStringLiteral("running");
    });

    if (match == m_data.taskHistory.rend())
    {
        occtdebug::TaskRecord record;
        record.id = id;
        record.title = id;
        record.startedAt = currentUtcIsoTimestamp();
        m_data.taskHistory.push_back(record);
        match = m_data.taskHistory.rbegin();
    }

    match->status = commandOutcomeText(result);
    match->program = QFileInfo(result.program).fileName();
    match->arguments = result.arguments.join(QLatin1Char(' '));
    match->workingDirectory = caseRelativeOrFileName(m_data.workspaceRoot, result.workingDirectory);
    match->finishedAt = currentUtcIsoTimestamp();
    match->elapsedMs = result.elapsedMs;
    match->exitCode = result.exitCode;
    if (!artifact.isEmpty())
    {
        match->artifact = artifact;
    }
    if (!stdoutLog.isEmpty())
    {
        match->stdoutLog = stdoutLog;
    }
    if (!stderrLog.isEmpty())
    {
        match->stderrLog = stderrLog;
    }
    match->note = note;

    m_data.manifest.taskHistory = m_data.taskHistory;
    refreshTaskHistoryPanel();
}

void WorkbenchWindow::refreshTaskHistoryPanel()
{
    if (m_taskHistoryPanel != nullptr)
    {
        m_taskHistoryPanel->setTasks(m_data.taskHistory);
    }
}

void WorkbenchWindow::runEnvironmentCapture()
{
    if (m_envRunner == nullptr || m_envRunner->isRunning())
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[env] runner is busy"));
        }
        return;
    }

    const QString repoRoot = sourceRootDirectory();
    const QString scriptPath = QDir(repoRoot).filePath(QStringLiteral("scripts/verify_env.ps1"));
    if (!QFileInfo::exists(scriptPath))
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[env] missing script: %1").arg(QDir::toNativeSeparators(scriptPath)));
        }
        return;
    }

    QDir dir;
    if (!dir.mkpath(runtimeEnvDirectory()) || !dir.mkpath(runtimeLogDirectory()) || !dir.mkpath(runtimeArtifactDirectory()))
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[env] failed to create case env/log/artifact directories"));
        }
        return;
    }

    occtdebug::CommandRequest request;
    request.program = QStringLiteral("powershell.exe");
    request.arguments = {
        QStringLiteral("-NoProfile"),
        QStringLiteral("-ExecutionPolicy"),
        QStringLiteral("Bypass"),
        QStringLiteral("-File"),
        scriptPath,
        QStringLiteral("-RepoRoot"),
        repoRoot,
        QStringLiteral("-BuildDir"),
        buildRootDirectory(),
        QStringLiteral("-OutputPath"),
        environmentSnapshotPath(),
    };
    request.workingDirectory = repoRoot;
    request.environment = QProcessEnvironment::systemEnvironment();
    request.timeoutMs = kEnvironmentCommandTimeoutMs;

    if (m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[env] powershell -File %1 -OutputPath %2%3")
                .arg(QDir::toNativeSeparators(scriptPath),
                     QDir::toNativeSeparators(environmentSnapshotPath()),
                     timeoutSuffix(request.timeoutMs)));
    }

    QString error;
    if (!m_envRunner->start(request, &error))
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[env] failed to start: %1").arg(error));
        }
        return;
    }
    recordTaskStarted(QStringLiteral("env"),
                      QStringLiteral("Environment capture"),
                      request,
                      QStringLiteral("artifacts/env_capture_result.json"),
                      QStringLiteral("logs/env_capture.stdout.log"),
                      QStringLiteral("logs/env_capture.stderr.log"));
}

void WorkbenchWindow::runTestgridVerification()
{
    if (m_testgridRunner == nullptr || m_testgridRunner->isRunning())
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[testgrid] runner is busy"));
        }
        return;
    }

    QDir dir;
    if (!dir.mkpath(runtimeVerificationDirectory()) || !dir.mkpath(runtimeLogDirectory()) || !dir.mkpath(runtimeArtifactDirectory()))
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[testgrid] failed to create case verification/log/artifact directories"));
        }
        return;
    }

    syncTestgridPlanFromUi();
    saveCurrentCaseManifest();

    QString error;
    if (!startTestgridDrawGate(TestgridRunPhase::DrawGate, QStringLiteral("testgrid"), &error) && m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[testgrid] failed to start gate: %1").arg(error));
    }
}

void WorkbenchWindow::runTestdiffAdapter()
{
    if (m_testgridRunner == nullptr || m_testgridRunner->isRunning()
        || m_testgridRunPhase != TestgridRunPhase::Idle)
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[testdiff] runner is busy"));
        }
        return;
    }

    QDir dir;
    if (!dir.mkpath(runtimeVerificationDirectory()) || !dir.mkpath(runtimeLogDirectory()) || !dir.mkpath(runtimeArtifactDirectory()))
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[testdiff] failed to create case verification/log/artifact directories"));
        }
        return;
    }

    syncTestgridPlanFromUi();
    saveCurrentCaseManifest();

    QString error;
    if (!startTestgridDrawGate(TestgridRunPhase::TestdiffGate, QStringLiteral("testdiff"), &error)
        && m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[testdiff] failed to start gate: %1").arg(error));
    }
}

void WorkbenchWindow::runTwoStageVerification()
{
    if (m_testgridRunner == nullptr || m_testgridRunner->isRunning()
        || m_patchRunner == nullptr || m_patchRunner->isRunning()
        || m_testgridRunPhase != TestgridRunPhase::Idle)
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[testgrid] another verification or patch command is already running"));
        }
        return;
    }

    QDir dir;
    if (!dir.mkpath(runtimeVerificationDirectory()) || !dir.mkpath(runtimeLogDirectory()) || !dir.mkpath(runtimeArtifactDirectory()))
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[testgrid] failed to create case verification/log/artifact directories"));
        }
        return;
    }

    syncTestgridPlanFromUi();
    saveCurrentCaseManifest();

    const occtdebug::VerificationWorkflowDecision decision = m_twoStageWorkflow.begin();

    if (m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[testgrid] two-stage verification started: before -> patch apply -> after -> patch undo"));
    }

    QString error;
    if (!startTestgridDrawGate(TestgridRunPhase::TwoStageBeforeGate, decision.phase, &error))
    {
        m_testgridRunPhase = TestgridRunPhase::Idle;
        m_twoStageWorkflow.reset();
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[testgrid] failed to start before gate: %1").arg(error));
        }
    }
}

void WorkbenchWindow::syncTestgridPlanFromUi()
{
    if (m_testgridGroupEdit != nullptr)
    {
        m_data.verificationPlan.testgridGroup = m_testgridGroupEdit->text().trimmed();
    }
    if (m_testgridGridEdit != nullptr)
    {
        m_data.verificationPlan.testgridGrid = m_testgridGridEdit->text().trimmed();
    }
    if (m_testgridCaseEdit != nullptr)
    {
        m_data.verificationPlan.testgridCase = m_testgridCaseEdit->text().trimmed();
    }
    m_data.manifest.verificationPlan = m_data.verificationPlan;
}

bool WorkbenchWindow::startConfiguredTestgridCommand(QString* error)
{
    return startConfiguredTestgridCommand(TestgridRunPhase::TestgridCommand, QStringLiteral("testgrid"), error);
}

bool WorkbenchWindow::startTestgridDrawGate(TestgridRunPhase phase, const QString& label, QString* error)
{
    if (m_testgridRunner == nullptr || m_testgridRunner->isRunning())
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("testgrid runner is busy");
        }
        return false;
    }

    occtdebug::CommandRequest request;
    request.program = QStringLiteral("ctest");
    request.arguments = {
        QStringLiteral("--test-dir"),
        buildRootDirectory(),
        QStringLiteral("-R"),
        QStringLiteral("draw_smoke"),
        QStringLiteral("--output-on-failure"),
    };
    request.workingDirectory = sourceRootDirectory();
    request.environment = QProcessEnvironment::systemEnvironment();
    request.timeoutMs = kDrawCommandTimeoutMs;

    if (m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[testgrid:%1] gate: ctest --test-dir %2 -R draw_smoke --output-on-failure%3")
                .arg(label, QDir::toNativeSeparators(buildRootDirectory()), timeoutSuffix(request.timeoutMs)));
    }

    m_testgridRunPhase = phase;
    if (!m_testgridRunner->start(request, error))
    {
        m_testgridRunPhase = TestgridRunPhase::Idle;
        return false;
    }
    QString taskId = QStringLiteral("testgrid.draw_gate");
    QString taskTitle = QStringLiteral("testgrid draw_smoke gate");
    QString artifact = QStringLiteral("artifacts/testgrid_result.json");
    QString stdoutLog = QStringLiteral("logs/testgrid_gate.stdout.log");
    QString stderrLog = QStringLiteral("logs/testgrid_gate.stderr.log");
    if (phase == TestgridRunPhase::TestdiffGate)
    {
        taskId = QStringLiteral("testdiff.draw_gate");
        taskTitle = QStringLiteral("testdiff draw_smoke gate");
        artifact = QStringLiteral("artifacts/testdiff_adapter_result.json");
    }
    else if (phase == TestgridRunPhase::TwoStageBeforeGate || phase == TestgridRunPhase::TwoStageAfterGate)
    {
        taskId = QStringLiteral("two_stage.%1.draw_gate").arg(label);
        taskTitle = QStringLiteral("two-stage %1 draw_smoke gate").arg(label);
        artifact = QStringLiteral("artifacts/testgrid_%1_result.json").arg(label);
        stdoutLog = QStringLiteral("logs/testgrid_%1_gate.stdout.log").arg(label);
        stderrLog = QStringLiteral("logs/testgrid_%1_gate.stderr.log").arg(label);
    }
    recordTaskStarted(taskId, taskTitle, request, artifact, stdoutLog, stderrLog);
    return true;
}

bool WorkbenchWindow::startConfiguredTestgridCommand(TestgridRunPhase phase, const QString& label, QString* error)
{
    const occtdebug::VerificationPlan& plan = m_data.verificationPlan;
    const QString executable = plan.testgridExecutable.trimmed();
    if (executable.isEmpty())
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("testgrid executable is not configured");
        }
        return false;
    }

    QString workingDirectory = plan.testgridRoot.trimmed();
    if (workingDirectory.isEmpty())
    {
        workingDirectory = sourceRootDirectory();
    }

    QString arguments = plan.testgridArguments;
    arguments.replace(QStringLiteral("{group}"), plan.testgridGroup);
    arguments.replace(QStringLiteral("{grid}"), plan.testgridGrid);
    arguments.replace(QStringLiteral("{case}"), plan.testgridCase);
    arguments.replace(QStringLiteral("{workspace}"), m_data.workspaceRoot);
    arguments.replace(QStringLiteral("{verification}"), runtimeVerificationDirectory());

    occtdebug::CommandRequest request;
    request.program = executable;
    request.workingDirectory = workingDirectory;
    request.environment = QProcessEnvironment::systemEnvironment();
    request.timeoutMs = kVerificationCommandTimeoutMs;
    request.arguments = arguments.trimmed().isEmpty()
        ? QStringList {}
        : QProcess::splitCommand(arguments);
    if (request.arguments.isEmpty())
    {
        if (!plan.testgridGroup.isEmpty())
        {
            request.arguments << plan.testgridGroup;
        }
        if (!plan.testgridGrid.isEmpty())
        {
            request.arguments << plan.testgridGrid;
        }
        if (!plan.testgridCase.isEmpty())
        {
            request.arguments << plan.testgridCase;
        }
    }

    if (m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[testgrid:%1] command: %2 %3%4")
                .arg(label,
                     QDir::toNativeSeparators(executable),
                     request.arguments.join(QLatin1Char(' ')),
                     timeoutSuffix(request.timeoutMs)));
    }

    m_testgridRunPhase = phase;
    if (!m_testgridRunner->start(request, error))
    {
        m_testgridRunPhase = TestgridRunPhase::Idle;
        return false;
    }
    QString taskId = QStringLiteral("testgrid.command");
    QString taskTitle = QStringLiteral("configured testgrid");
    QString artifact = QStringLiteral("artifacts/testgrid_result.json");
    QString stdoutLog = QStringLiteral("logs/testgrid.stdout.log");
    QString stderrLog = QStringLiteral("logs/testgrid.stderr.log");
    if (phase == TestgridRunPhase::TwoStageBeforeCommand || phase == TestgridRunPhase::TwoStageAfterCommand)
    {
        taskId = QStringLiteral("two_stage.%1.command").arg(label);
        taskTitle = QStringLiteral("two-stage %1 testgrid").arg(label);
        artifact = QStringLiteral("artifacts/testgrid_%1_result.json").arg(label);
        stdoutLog = QStringLiteral("logs/testgrid_%1.stdout.log").arg(label);
        stderrLog = QStringLiteral("logs/testgrid_%1.stderr.log").arg(label);
    }
    recordTaskStarted(taskId, taskTitle, request, artifact, stdoutLog, stderrLog);
    return true;
}

bool WorkbenchWindow::startConfiguredTestdiffCommand(QString* error)
{
    const occtdebug::TestdiffCommandPlan plan = occtdebug::TestdiffCommandPlanner::build({
        m_data.verificationPlan,
        m_data.workspaceRoot,
        sourceRootDirectory(),
        runtimeVerificationDirectory(),
        runtimeArtifactDirectory(),
    });
    if (!plan.success)
    {
        if (error != nullptr)
        {
            *error = plan.error;
        }
        return false;
    }
    m_lastTestdiffOutputRoot = plan.outputRoot;

    occtdebug::CommandRequest request = plan.request;
    request.timeoutMs = kVerificationCommandTimeoutMs;

    if (m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[testdiff] command: %1 %2%3")
                .arg(QDir::toNativeSeparators(request.program),
                     request.arguments.join(QLatin1Char(' ')),
                     timeoutSuffix(request.timeoutMs)));
    }

    m_testgridRunPhase = TestgridRunPhase::TestdiffCommand;
    if (!m_testgridRunner->start(request, error))
    {
        m_testgridRunPhase = TestgridRunPhase::Idle;
        return false;
    }
    recordTaskStarted(QStringLiteral("testdiff.command"),
                      QStringLiteral("configured testdiff"),
                      request,
                      QStringLiteral("artifacts/testdiff_adapter_result.json"),
                      QStringLiteral("logs/testdiff_runner.stdout.log"),
                      QStringLiteral("logs/testdiff_runner.stderr.log"));
    return true;
}

void WorkbenchWindow::handleTestgridGateFinished(const occtdebug::CommandResult& result)
{
    m_lastTestgridGateResult = result;
    const bool gatePassed = commandSucceeded(result);

    if (gatePassed && !m_data.verificationPlan.testgridExecutable.trimmed().isEmpty())
    {
        QString error;
        if (startConfiguredTestgridCommand(&error))
        {
            return;
        }

        m_testgridRunPhase = TestgridRunPhase::Idle;
        persistTestgridResult(result, nullptr, QStringLiteral("draw_smoke gate passed; configured testgrid command failed to start: %1").arg(error));
        return;
    }

    m_testgridRunPhase = TestgridRunPhase::Idle;
    const QString note = gatePassed
        ? QStringLiteral("draw_smoke gate passed; testgrid executable is not configured, parsed local summary files only")
        : QStringLiteral("draw_smoke gate failed; testgrid/testdiff command was not started");
    persistTestgridResult(result, nullptr, note);
}

void WorkbenchWindow::handleTestgridCommandFinished(const occtdebug::CommandResult& result)
{
    m_testgridRunPhase = TestgridRunPhase::Idle;
    persistTestgridResult(m_lastTestgridGateResult, &result, QStringLiteral("configured testgrid command finished"));
}

void WorkbenchWindow::handleTestdiffGateFinished(const occtdebug::CommandResult& result)
{
    const bool gatePassed = commandSucceeded(result);
    if (gatePassed)
    {
        QString error;
        if (startConfiguredTestdiffCommand(&error))
        {
            return;
        }

        m_testgridRunPhase = TestgridRunPhase::Idle;
        setVerificationMetric(QStringLiteral("testdiff adapter"), QStringLiteral("blocked: %1").arg(error));
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[testdiff] failed to start configured command: %1").arg(error));
        }
        return;
    }

    m_testgridRunPhase = TestgridRunPhase::Idle;
    setVerificationMetric(QStringLiteral("testdiff adapter"), QStringLiteral("blocked: draw_smoke gate failed"));
    refreshEvidenceBundle();
    refreshVerificationReport();
}

void WorkbenchWindow::handleTestdiffCommandFinished(const occtdebug::CommandResult& result)
{
    m_testgridRunPhase = TestgridRunPhase::Idle;
    persistTestdiffAdapterResult(result, QStringLiteral("configured testdiff command finished"));
}

void WorkbenchWindow::handleTwoStageGateFinished(const occtdebug::CommandResult& result)
{
    handleTwoStageDecision(m_twoStageWorkflow.onGateFinished(
        result,
        !m_data.verificationPlan.testgridExecutable.trimmed().isEmpty()));
}

void WorkbenchWindow::handleTwoStageCommandFinished(const occtdebug::CommandResult& result)
{
    handleTwoStageDecision(m_twoStageWorkflow.onCommandFinished(result));
}

void WorkbenchWindow::handleTwoStagePatchFinished(const occtdebug::CommandResult& result)
{
    persistPatchCommandResult(result);
    m_patchRunMode = PatchRunMode::None;
    handleTwoStageDecision(m_twoStageWorkflow.onPatchFinished(result));
}

void WorkbenchWindow::handleTwoStageDecision(const occtdebug::VerificationWorkflowDecision& decision)
{
    if (decision.persistPhase)
    {
        const occtdebug::CommandResult gateResult = m_twoStageWorkflow.gateResult(decision.phase);
        const occtdebug::CommandResult commandResult = m_twoStageWorkflow.commandResult(decision.phase);
        const occtdebug::CommandResult* commandResultPtr =
            m_twoStageWorkflow.commandExecuted(decision.phase) ? &commandResult : nullptr;
        persistTwoStagePhase(decision.phase, gateResult, commandResultPtr, decision.note);
    }

    switch (decision.action)
    {
    case occtdebug::VerificationWorkflowAction::StartBeforeCommand:
    case occtdebug::VerificationWorkflowAction::StartAfterCommand:
    {
        QString error;
        const TestgridRunPhase nextPhase =
            decision.action == occtdebug::VerificationWorkflowAction::StartBeforeCommand
            ? TestgridRunPhase::TwoStageBeforeCommand
            : TestgridRunPhase::TwoStageAfterCommand;
        if (!startConfiguredTestgridCommand(nextPhase, decision.phase, &error))
        {
            handleTwoStageDecision(m_twoStageWorkflow.onStartFailure(decision.action, error));
        }
        break;
    }
    case occtdebug::VerificationWorkflowAction::StartPatchApply:
    {
        QString error;
        if (!startTwoStagePatchCommand(PatchRunMode::Apply, &error))
        {
            handleTwoStageDecision(m_twoStageWorkflow.onStartFailure(decision.action, error));
        }
        break;
    }
    case occtdebug::VerificationWorkflowAction::StartAfterGate:
    {
        QString error;
        if (!startTestgridDrawGate(TestgridRunPhase::TwoStageAfterGate, decision.phase, &error))
        {
            handleTwoStageDecision(m_twoStageWorkflow.onStartFailure(decision.action, error));
        }
        break;
    }
    case occtdebug::VerificationWorkflowAction::StartPatchUndo:
    {
        QString error;
        if (!startTwoStagePatchCommand(PatchRunMode::Undo, &error))
        {
            handleTwoStageDecision(m_twoStageWorkflow.onStartFailure(decision.action, error));
        }
        break;
    }
    case occtdebug::VerificationWorkflowAction::Finalize:
        persistTwoStageWorkflowResult(decision.finalStatus, decision.finalNote);
        break;
    case occtdebug::VerificationWorkflowAction::StartBeforeGate:
    case occtdebug::VerificationWorkflowAction::None:
        break;
    }
}

QString WorkbenchWindow::environmentSummaryFromSnapshot(const QString& snapshotPath) const
{
    QFile file(snapshotPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return QStringLiteral("閻滎垰顣ㄨ箛顐ゅ弾娑撳秴鐡ㄩ崷顭掔窗%1").arg(QDir::toNativeSeparators(snapshotPath));
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject())
    {
        return QStringLiteral("閻滎垰顣ㄨ箛顐ゅ弾閺嶇厧绱￠弮鐘虫櫏閿?1").arg(QDir::toNativeSeparators(snapshotPath));
    }

    const QJsonObject root = doc.object();
    const QJsonObject windows = root.value(QStringLiteral("windows")).toObject();
    const QJsonObject vs = root.value(QStringLiteral("visual_studio")).toObject();
    const QJsonObject cmake = root.value(QStringLiteral("cmake")).toObject();
    const QJsonObject qt = root.value(QStringLiteral("qt")).toObject();
    const QJsonObject occt = root.value(QStringLiteral("occt")).toObject();
    const QJsonObject tcltk = root.value(QStringLiteral("tcltk")).toObject();
    const QJsonObject drawTests = root.value(QStringLiteral("draw_tests")).toObject();
    const QJsonObject drawSmoke = drawTests.value(QStringLiteral("draw_smoke")).toObject();
    const QJsonObject checkshapeSmoke = drawTests.value(QStringLiteral("draw_checkshape_smoke")).toObject();

    const auto drawStatus = [](const QJsonObject& test) {
        if (!test.value(QStringLiteral("result_exists")).toBool())
        {
            return QStringLiteral("missing_result");
        }
        return test.value(QStringLiteral("success_token_found")).toBool()
            ? QStringLiteral("passed")
            : QStringLiteral("failed_or_unknown");
    };

    QStringList lines;
    lines << QStringLiteral("閻滎垰顣ㄨ箛顐ゅ弾閿涙瓱nv/env_snapshot.json");
    lines << QStringLiteral("閻㈢喐鍨氶弮鍫曟？閿?1").arg(root.value(QStringLiteral("generated_at")).toString());
    lines << QStringLiteral("Windows閿?1 %2 %3")
            .arg(windows.value(QStringLiteral("caption")).toString(),
                 windows.value(QStringLiteral("version")).toString(),
                 windows.value(QStringLiteral("architecture")).toString());
    lines << QStringLiteral("Visual Studio閿涙瓲ound=%1 %2")
            .arg(boolText(vs.value(QStringLiteral("found")).toBool()),
                 vs.value(QStringLiteral("display_name")).toString());
    lines << QStringLiteral("CMake閿涙瓲ound=%1 %2")
            .arg(boolText(cmake.value(QStringLiteral("found")).toBool()),
                 cmake.value(QStringLiteral("version")).toString());
    lines << QStringLiteral("Qt閿涙瓱xists=%1 root=%2")
            .arg(boolText(qt.value(QStringLiteral("exists")).toBool()),
                 qt.value(QStringLiteral("root")).toString());
    lines << QStringLiteral("OCCT閿涙瓱xists=%1 root=%2")
            .arg(boolText(occt.value(QStringLiteral("exists")).toBool()),
                 occt.value(QStringLiteral("root")).toString());
    lines << QStringLiteral("DRAWEXE閿涙瓱xists=%1 path=%2")
            .arg(boolText(occt.value(QStringLiteral("drawexe_exists")).toBool()),
                 occt.value(QStringLiteral("drawexe")).toString());
    lines << QStringLiteral("Tcl/Tk閿涙瓱xists=%1 tcl86.dll=%2 tk86.dll=%3")
            .arg(boolText(tcltk.value(QStringLiteral("exists")).toBool()),
                 boolText(tcltk.value(QStringLiteral("tcl86_dll_exists")).toBool()),
                 boolText(tcltk.value(QStringLiteral("tk86_dll_exists")).toBool()));
    lines << QStringLiteral("DRAW smoke閿?1 exit=%2 token=%3")
            .arg(drawStatus(drawSmoke),
                 QString::number(drawSmoke.value(QStringLiteral("exit_code")).toInt(-1)),
                 drawSmoke.value(QStringLiteral("success_token")).toString());
    lines << QStringLiteral("checkshape smoke閿?1 exit=%2 token=%3")
            .arg(drawStatus(checkshapeSmoke),
                 QString::number(checkshapeSmoke.value(QStringLiteral("exit_code")).toInt(-1)),
                 checkshapeSmoke.value(QStringLiteral("success_token")).toString());
    lines << QStringLiteral("testgrid 闂傘劎顩﹂敍?1")
            .arg(drawTests.value(QStringLiteral("testgrid_gate")).toString());
    return lines.join(QLatin1Char('\n'));
}

void WorkbenchWindow::persistEnvironmentCaptureResult(const occtdebug::CommandResult& result)
{
    QDir dir;
    if (!dir.mkpath(runtimeLogDirectory()) || !dir.mkpath(runtimeArtifactDirectory()))
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[env] failed to create case log/artifact directories"));
        }
        return;
    }

    QString error;
    const QString stdoutPath = QDir(runtimeLogDirectory()).filePath(QStringLiteral("env_capture.stdout.log"));
    const QString stderrPath = QDir(runtimeLogDirectory()).filePath(QStringLiteral("env_capture.stderr.log"));
    if (!writeTextFile(stdoutPath, result.stdoutText, &error) || !writeTextFile(stderrPath, result.stderrText, &error))
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[env] %1").arg(error));
        }
        return;
    }

    const QJsonObject json {
        {QStringLiteral("schema_version"), 1},
        {QStringLiteral("case_id"), m_data.caseId},
        {QStringLiteral("program"), result.program},
        {QStringLiteral("arguments"), result.arguments.join(QLatin1Char(' '))},
        {QStringLiteral("working_directory"), result.workingDirectory},
        {QStringLiteral("exit_code"), result.exitCode},
        {QStringLiteral("elapsed_ms"), static_cast<double>(result.elapsedMs)},
        {QStringLiteral("canceled"), result.canceled},
        {QStringLiteral("timed_out"), result.timedOut},
        {QStringLiteral("timeout_ms"), result.timeoutMs},
        {QStringLiteral("env_snapshot"), QStringLiteral("env/env_snapshot.json")},
        {QStringLiteral("stdout"), QStringLiteral("logs/env_capture.stdout.log")},
        {QStringLiteral("stderr"), QStringLiteral("logs/env_capture.stderr.log")},
    };

    const QString resultPath = QDir(runtimeArtifactDirectory()).filePath(QStringLiteral("env_capture_result.json"));
    QFile resultFile(resultPath);
    if (!resultFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[env] failed to write %1: %2").arg(resultPath, resultFile.errorString()));
        }
        return;
    }
    resultFile.write(QJsonDocument(json).toJson(QJsonDocument::Indented));

    const QString summary = environmentSummaryFromSnapshot(environmentSnapshotPath());
    m_data.environmentSummary = summary;
    m_data.manifest.environmentSummary = summary;
    if (m_environmentText != nullptr)
    {
        m_environmentText->setPlainText(summary);
    }

    appendEvidenceRecord({
        QStringLiteral("Env"),
        QStringLiteral("Environment snapshot"),
        QStringLiteral("verify_env.ps1 exit=%1 elapsed=%2ms").arg(result.exitCode).arg(result.elapsedMs),
        QStringLiteral("env/env_snapshot.json"),
    });

    saveCurrentCaseManifest();

    if (m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[env] artifacts written to %1").arg(QDir::toNativeSeparators(runtimeArtifactDirectory())));
    }
}

void WorkbenchWindow::persistTestgridResult(const occtdebug::CommandResult& gateResult,
                                            const occtdebug::CommandResult* testgridResult,
                                            const QString& note)
{
    QString error;
    if (!occtdebug::TestgridArtifactService::ensureWorkspaceDirectories(m_data.workspaceRoot, &error)
        || !occtdebug::TestgridArtifactService::writeCommandLogs(
            m_data.workspaceRoot,
            QStringLiteral("testgrid_gate.stdout.log"),
            QStringLiteral("testgrid_gate.stderr.log"),
            gateResult,
            &error))
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[testgrid] %1").arg(error));
        }
        return;
    }

    if (testgridResult != nullptr)
    {
        if (!occtdebug::TestgridArtifactService::writeCommandLogs(
                m_data.workspaceRoot,
                QStringLiteral("testgrid.stdout.log"),
                QStringLiteral("testgrid.stderr.log"),
                *testgridResult,
                &error))
        {
            if (m_cmakeConsole != nullptr)
            {
                m_cmakeConsole->append(QStringLiteral("[testgrid] %1").arg(error));
            }
            return;
        }
    }

    occtdebug::TestgridResultWriterResult result;
    const occtdebug::TestgridResultWriterInput input {
        m_data.caseId,
        m_data.workspaceRoot,
        note,
        m_data.verificationPlan,
        gateResult,
        testgridResult != nullptr,
        testgridResult != nullptr ? *testgridResult : occtdebug::CommandResult(),
    };
    if (!occtdebug::TestgridResultWriter::writeSingleStageResult(input, &result, &error))
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[testgrid] %1").arg(error));
        }
        return;
    }

    m_data.testgridRows = result.rows;
    m_data.manifest.testgridRows = result.rows;
    m_data.verificationItems = result.verificationItems;
    m_data.manifest.verificationItems = result.verificationItems;
    m_data.manifest.verificationPlan = m_data.verificationPlan;
    m_data.diffSummary = result.diffSummary;
    m_data.manifest.diffSummary = m_data.diffSummary;
    m_data.reproStatus = occtdebug::ReproStatusEvaluator::withTestgridResult(
        m_data.reproStatus,
        result,
        currentUtcIsoTimestamp());
    m_data.manifest.reproStatus = m_data.reproStatus;
    updateReproStatusMetric();
    refreshDiffArtifactTables();

    occtdebug::TestgridTablePresenter::applyToTable(m_testgridTable, m_data.testgridRows);
    if (m_verificationPanel != nullptr)
    {
        m_verificationPanel->setItems(m_data.verificationItems);
    }

    appendEvidenceRecord({
        QStringLiteral("Verification"),
        QStringLiteral("testgrid verification"),
        QStringLiteral("gate=%1 runner=%2 rows=%3 failures=%4 elapsed=%5ms")
            .arg(result.gatePassed ? QStringLiteral("passed") : QStringLiteral("failed"),
                 result.commandExecuted ? QStringLiteral("executed") : QStringLiteral("skipped"))
            .arg(result.rows.size())
            .arg(result.failureDetails.size())
            .arg(result.timing.totalElapsedMs),
        QStringLiteral("artifacts/testgrid_result.json"),
    });

    saveCurrentCaseManifest();
    if (m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[testgrid] result written to %1").arg(QDir::toNativeSeparators(result.artifactPath)));
    }
}

void WorkbenchWindow::persistTestdiffAdapterResult(const occtdebug::CommandResult& result, const QString& note)
{
    occtdebug::TestdiffAdapterResultWriterResult writerResult;
    QString error;
    if (!occtdebug::TestdiffAdapterResultWriter::writeResult({
            m_data.workspaceRoot,
            m_data.verificationPlan,
            m_lastTestdiffOutputRoot,
            m_data.testgridRows,
            result,
            note,
        },
        &writerResult,
        &error))
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[testdiff] %1").arg(error));
        }
        return;
    }

    const occtdebug::TestdiffAdapterResultSyncResult syncResult =
        occtdebug::TestdiffAdapterResultCoordinator::sync(m_data, writerResult);
    refreshDiffArtifactTables();
    if (m_verificationPanel != nullptr)
    {
        m_verificationPanel->setItems(m_data.verificationItems);
    }
    if (m_evidencePanel != nullptr)
    {
        m_evidencePanel->appendRecord(syncResult.evidence);
    }

    if (syncResult.saveCaseManifest)
    {
        saveCurrentCaseManifest();
    }
    if (syncResult.writeEvidenceBundle)
    {
        refreshEvidenceBundle();
    }
    if (syncResult.writeVerificationReport)
    {
        refreshVerificationReport();
    }

    if (m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[testdiff] %1").arg(writerResult.adapterStatus));
    }
}

bool WorkbenchWindow::startTwoStagePatchCommand(PatchRunMode mode, QString* error)
{
    if (mode == PatchRunMode::None)
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("patch mode is empty");
        }
        return false;
    }
    if (m_patchRunner == nullptr || m_patchRunner->isRunning())
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("patch runner is busy");
        }
        return false;
    }

    const QString worktree = QDir::cleanPath(m_data.patchWorktreeRoot.trimmed());
    if (worktree.isEmpty() || !QFileInfo::exists(worktree))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("patch.worktree_root is not configured or does not exist");
        }
        return false;
    }
    if (m_data.patchDiff.trimmed().isEmpty() || m_data.patchDiff.trimmed().startsWith(QStringLiteral("# No patch candidate")))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("no patch candidate diff is available");
        }
        return false;
    }

    QDir dir;
    dir.mkpath(runtimeArtifactDirectory());
    dir.mkpath(runtimeLogDirectory());
    const QString patchPath = QDir(runtimeArtifactDirectory()).filePath(QStringLiteral("candidate_patch.diff"));
    QString writeError;
    if (!writeTextFile(patchPath, m_data.patchDiff, &writeError))
    {
        if (error != nullptr)
        {
            *error = writeError;
        }
        return false;
    }

    if (!runPatchDryRun(mode, worktree, patchPath))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("patch %1 dry-run failed").arg(mode == PatchRunMode::Undo ? QStringLiteral("undo") : QStringLiteral("apply"));
        }
        return false;
    }

    QStringList arguments {QStringLiteral("apply")};
    if (mode == PatchRunMode::Undo)
    {
        arguments << QStringLiteral("-R");
    }
    arguments << patchPath;

    const QString action = mode == PatchRunMode::Undo ? QStringLiteral("undo") : QStringLiteral("apply");
    m_patchRunMode = mode;
    m_testgridRunPhase = mode == PatchRunMode::Undo ? TestgridRunPhase::TwoStagePatchUndo : TestgridRunPhase::TwoStagePatchApply;
    m_data.patchApplyStatus = QStringLiteral("two-stage %1 running").arg(action);
    m_data.manifest.patchApplyStatus = m_data.patchApplyStatus;
    updatePatchReviewStatus();

    if (m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[testgrid] two-stage patch %1: git %2 in %3%4")
                .arg(action,
                     arguments.join(QLatin1Char(' ')),
                     QDir::toNativeSeparators(worktree),
                     timeoutSuffix(kPatchCommandTimeoutMs)));
    }

    occtdebug::CommandRequest request;
    request.program = QStringLiteral("git");
    request.arguments = arguments;
    request.workingDirectory = worktree;
    request.environment = QProcessEnvironment::systemEnvironment();
    request.timeoutMs = kPatchCommandTimeoutMs;

    if (!m_patchRunner->start(request, error))
    {
        m_patchRunMode = PatchRunMode::None;
        m_testgridRunPhase = TestgridRunPhase::Idle;
        return false;
    }
    const QString taskId = mode == PatchRunMode::Undo ? QStringLiteral("patch.undo") : QStringLiteral("patch.apply");
    recordTaskStarted(taskId,
                      QStringLiteral("Two-stage patch %1").arg(action),
                      request,
                      QStringLiteral("artifacts/patch_%1_result.json").arg(action),
                      QStringLiteral("logs/patch_%1.stdout.log").arg(action),
                      QStringLiteral("logs/patch_%1.stderr.log").arg(action));
    return true;
}

void WorkbenchWindow::persistTwoStagePhase(const QString& phase,
                                           const occtdebug::CommandResult& gateResult,
                                           const occtdebug::CommandResult* testgridResult,
                                           const QString& note)
{
    occtdebug::TwoStagePhaseResultWriterResult result;
    QString error;
    if (!occtdebug::TwoStagePhaseResultWriter::writePhaseResult({
            m_data.caseId,
            m_data.workspaceRoot,
            phase,
            note,
            gateResult,
            testgridResult != nullptr,
            testgridResult != nullptr ? *testgridResult : occtdebug::CommandResult(),
        },
        &result,
        &error))
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[testgrid] %1").arg(error));
        }
        return;
    }

    if (phase == QStringLiteral("after"))
    {
        m_data.testgridRows = result.rows;
        m_data.manifest.testgridRows = result.rows;
    }

    if (m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[testgrid:%1] phase result written to %2")
                .arg(phase, QDir::toNativeSeparators(result.artifactPath)));
    }
}

void WorkbenchWindow::persistTwoStageWorkflowResult(const QString& finalStatus, const QString& note)
{
    m_testgridRunPhase = TestgridRunPhase::Idle;

    const occtdebug::TwoStageFinalResultBuilderResult builderResult =
        occtdebug::TwoStageFinalResultBuilder::build({
            m_data.caseId,
            m_data.workspaceRoot,
            finalStatus,
            note,
            m_data.verificationPlan,
            m_twoStageWorkflow.patchApplied(),
            m_twoStageWorkflow.beforeCommandExecuted(),
            m_twoStageWorkflow.afterCommandExecuted(),
            m_twoStageWorkflow.beforeGateResult(),
            m_twoStageWorkflow.beforeCommandResult(),
            m_twoStageWorkflow.afterGateResult(),
            m_twoStageWorkflow.afterCommandResult(),
        });

    occtdebug::TwoStageFinalResultWriterResult writerResult;
    QString error;
    if (!occtdebug::TwoStageFinalResultWriter::writeFinalResult(builderResult.writerInput,
        &writerResult,
        &error))
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[testgrid] %1").arg(error));
        }
    }

    const occtdebug::TwoStageFinalResultSyncResult syncResult =
        occtdebug::TwoStageFinalResultCoordinator::sync(m_data, {
            finalStatus,
            note,
            builderResult.beforeCommandExecuted,
            builderResult.afterCommandExecuted,
            builderResult.afterRows,
            builderResult.comparison,
            builderResult.testdiff,
            builderResult.failureDetails,
            builderResult.timing,
        });
    const occtdebug::TwoStageFinalResultUiActions uiActions =
        occtdebug::TwoStageFinalResultUiAdapter::apply({
                m_diffPanel == nullptr ? nullptr : m_diffPanel->summaryLabel(),
                m_testgridTable,
                m_verificationPanel,
                m_evidencePanel,
            },
            m_data,
            syncResult);
    if (uiActions.refreshDiffArtifacts)
    {
        refreshDiffArtifactTables();
    }

    if (uiActions.saveCaseManifest)
    {
        saveCurrentCaseManifest();
    }
    if (uiActions.writeEvidenceBundle)
    {
        refreshEvidenceBundle();
    }
    if (uiActions.writeVerificationReport)
    {
        refreshVerificationReport();
    }

    if (m_cmakeConsole != nullptr && !writerResult.twoStageResultPath.isEmpty())
    {
        m_cmakeConsole->append(QStringLiteral("[testgrid] two-stage result written to %1")
                .arg(QDir::toNativeSeparators(writerResult.twoStageResultPath)));
    }
    m_twoStageWorkflow.reset();
}

void WorkbenchWindow::exportReproPack()
{
    if (m_packRunner == nullptr || m_packRunner->isRunning())
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[pack] runner is busy"));
        }
        return;
    }

    if (!saveCurrentReproScript())
    {
        return;
    }

    const QString repoRoot = sourceRootDirectory();
    const QString scriptPath = QDir(repoRoot).filePath(QStringLiteral("scripts/export_repro_pack.ps1"));
    if (!QFileInfo::exists(scriptPath))
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[pack] missing script: %1").arg(QDir::toNativeSeparators(scriptPath)));
        }
        return;
    }

    QDir dir;
    if (!dir.mkpath(runtimeLogDirectory()) || !dir.mkpath(runtimeArtifactDirectory()))
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[pack] failed to create case log/artifact directories"));
        }
        return;
    }

    const QString outputDir = QDir(runtimeArtifactDirectory()).filePath(QStringLiteral("repro_pack"));
    occtdebug::CommandRequest request;
    request.program = QStringLiteral("powershell.exe");
    request.arguments = {
        QStringLiteral("-NoProfile"),
        QStringLiteral("-ExecutionPolicy"),
        QStringLiteral("Bypass"),
        QStringLiteral("-File"),
        scriptPath,
        QStringLiteral("-TclScript"),
        currentReproScriptPath(),
        QStringLiteral("-DrawLogDir"),
        runtimeLogDirectory(),
        QStringLiteral("-RepoRoot"),
        repoRoot,
        QStringLiteral("-CaseId"),
        m_data.caseId,
        QStringLiteral("-OutputDir"),
        outputDir,
    };
    request.workingDirectory = repoRoot;
    request.environment = QProcessEnvironment::systemEnvironment();
    request.timeoutMs = kReproPackCommandTimeoutMs;

    if (m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[pack] powershell -File %1 -OutputDir %2%3")
                .arg(QDir::toNativeSeparators(scriptPath),
                     QDir::toNativeSeparators(outputDir),
                     timeoutSuffix(request.timeoutMs)));
    }

    QString error;
    if (!m_packRunner->start(request, &error))
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[pack] failed to start: %1").arg(error));
        }
        return;
    }
    recordTaskStarted(QStringLiteral("repro_pack"),
                      QStringLiteral("Export Repro Pack"),
                      request,
                      QStringLiteral("artifacts/repro_pack_result.json"),
                      QStringLiteral("logs/repro_pack.stdout.log"),
                      QStringLiteral("logs/repro_pack.stderr.log"));
}

void WorkbenchWindow::persistReproPackResult(const occtdebug::CommandResult& result)
{
    QDir dir;
    if (!dir.mkpath(runtimeLogDirectory()) || !dir.mkpath(runtimeArtifactDirectory()))
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[pack] failed to create case log/artifact directories"));
        }
        return;
    }

    QString error;
    const QString stdoutPath = QDir(runtimeLogDirectory()).filePath(QStringLiteral("repro_pack.stdout.log"));
    const QString stderrPath = QDir(runtimeLogDirectory()).filePath(QStringLiteral("repro_pack.stderr.log"));
    if (!writeTextFile(stdoutPath, result.stdoutText, &error) || !writeTextFile(stderrPath, result.stderrText, &error))
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[pack] %1").arg(error));
        }
        return;
    }

    const QJsonObject json {
        {QStringLiteral("schema_version"), 1},
        {QStringLiteral("case_id"), m_data.caseId},
        {QStringLiteral("program"), result.program},
        {QStringLiteral("arguments"), result.arguments.join(QLatin1Char(' '))},
        {QStringLiteral("working_directory"), result.workingDirectory},
        {QStringLiteral("exit_code"), result.exitCode},
        {QStringLiteral("elapsed_ms"), static_cast<double>(result.elapsedMs)},
        {QStringLiteral("canceled"), result.canceled},
        {QStringLiteral("timed_out"), result.timedOut},
        {QStringLiteral("timeout_ms"), result.timeoutMs},
        {QStringLiteral("repro_pack"), QStringLiteral("artifacts/repro_pack")},
        {QStringLiteral("stdout"), QStringLiteral("logs/repro_pack.stdout.log")},
        {QStringLiteral("stderr"), QStringLiteral("logs/repro_pack.stderr.log")},
    };

    const QString resultPath = QDir(runtimeArtifactDirectory()).filePath(QStringLiteral("repro_pack_result.json"));
    QFile resultFile(resultPath);
    if (!resultFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[pack] failed to write %1: %2").arg(resultPath, resultFile.errorString()));
        }
        return;
    }
    resultFile.write(QJsonDocument(json).toJson(QJsonDocument::Indented));

    appendEvidenceRecord({
        QStringLiteral("ReproPack"),
        QStringLiteral("Repro Pack export"),
        QStringLiteral("exit=%1 elapsed=%2ms output=artifacts/repro_pack").arg(result.exitCode).arg(result.elapsedMs),
        QStringLiteral("artifacts/repro_pack/manifest.json"),
    });
    saveCurrentCaseManifest();

    if (m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[pack] result written to %1").arg(QDir::toNativeSeparators(resultPath)));
    }
}

void WorkbenchWindow::persistDrawRunResult(const occtdebug::CommandResult& result)
{
    QDir dir;
    if (!dir.mkpath(runtimeLogDirectory()) || !dir.mkpath(runtimeArtifactDirectory()))
    {
        if (m_drawConsole != nullptr)
        {
            m_drawConsole->append(QStringLiteral("[DRAW] failed to create case log/artifact directories"));
        }
        return;
    }

    const QString stdoutPath = QDir(runtimeLogDirectory()).filePath(QStringLiteral("draw.stdout.log"));
    const QString stderrPath = QDir(runtimeLogDirectory()).filePath(QStringLiteral("draw.stderr.log"));
    const QString resultPath = QDir(runtimeArtifactDirectory()).filePath(QStringLiteral("draw_result.json"));

    auto writeText = [this](const QString& path, const QString& text) -> bool {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            if (m_drawConsole != nullptr)
            {
                m_drawConsole->append(QStringLiteral("[DRAW] failed to write %1: %2").arg(path, file.errorString()));
            }
            return false;
        }
        file.write(text.toUtf8());
        return true;
    };

    if (!writeText(stdoutPath, result.stdoutText) || !writeText(stderrPath, result.stderrText))
    {
        return;
    }

    const QJsonObject json {
        {QStringLiteral("schema_version"), 1},
        {QStringLiteral("case_id"), m_data.caseId},
        {QStringLiteral("program"), result.program},
        {QStringLiteral("arguments"), result.arguments.join(QLatin1Char(' '))},
        {QStringLiteral("working_directory"), result.workingDirectory},
        {QStringLiteral("exit_code"), result.exitCode},
        {QStringLiteral("elapsed_ms"), static_cast<double>(result.elapsedMs)},
        {QStringLiteral("canceled"), result.canceled},
        {QStringLiteral("timed_out"), result.timedOut},
        {QStringLiteral("timeout_ms"), result.timeoutMs},
        {QStringLiteral("stdout"), QStringLiteral("logs/draw.stdout.log")},
        {QStringLiteral("stderr"), QStringLiteral("logs/draw.stderr.log")},
        {QStringLiteral("repro_script"), QStringLiteral("repro/repro.tcl")},
    };

    QFile resultFile(resultPath);
    if (!resultFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        if (m_drawConsole != nullptr)
        {
            m_drawConsole->append(QStringLiteral("[DRAW] failed to write %1: %2").arg(resultPath, resultFile.errorString()));
        }
        return;
    }
    resultFile.write(QJsonDocument(json).toJson(QJsonDocument::Indented));

    if (m_drawConsole != nullptr)
    {
        m_drawConsole->append(QStringLiteral("[DRAW] artifacts written to %1").arg(QDir::toNativeSeparators(runtimeArtifactDirectory())));
    }
    appendDrawEvidenceFromResult(result);
}

void WorkbenchWindow::appendEvidenceRecord(const occtdebug::EvidenceRecord& evidence)
{
    occtdebug::EvidenceCoordinator::appendRecord(m_data, m_evidencePanel, evidence);
}

void WorkbenchWindow::appendDrawEvidenceFromResult(const occtdebug::CommandResult& result)
{
    const occtdebug::DrawLogAnalysis analysis = occtdebug::DrawLogParser::analyze(result.stdoutText, result.stderrText);
    m_data.reproStatus = occtdebug::ReproStatusEvaluator::withDrawResult(
        m_data.reproStatus,
        result,
        analysis,
        currentUtcIsoTimestamp());
    m_data.manifest.reproStatus = m_data.reproStatus;
    updateReproStatusMetric();

    QString logFile = QStringLiteral("logs/draw.stdout.log");
    int logLine = firstDrawErrorLine(result.stdoutText);
    if (logLine <= 0)
    {
        const int stderrLine = firstDrawErrorLine(result.stderrText);
        if (stderrLine > 0)
        {
            logFile = QStringLiteral("logs/draw.stderr.log");
            logLine = stderrLine;
        }
    }

    occtdebug::EvidenceRecord evidence;
    evidence.type = QStringLiteral("DRAW");
    evidence.title = QStringLiteral("DRAW run result");
    evidence.summary = QStringLiteral("exit=%1 elapsed=%2ms %3").arg(result.exitCode).arg(result.elapsedMs).arg(analysis.summaryText());
    evidence.link = logLine > 0
        ? QStringLiteral("%1:%2").arg(logFile).arg(logLine)
        : QStringLiteral("artifacts/draw_result.json");
    evidence.logFile = logLine > 0 ? logFile : QString();
    evidence.logLine = logLine;
    appendEvidenceRecord(evidence);

    const QString evidencePath = QDir(runtimeArtifactDirectory()).filePath(QStringLiteral("draw_evidence.json"));
    QFile evidenceFile(evidencePath);
    if (!evidenceFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        if (m_drawConsole != nullptr)
        {
            m_drawConsole->append(QStringLiteral("[DRAW] failed to write %1: %2").arg(evidencePath, evidenceFile.errorString()));
        }
        return;
    }

    const QJsonObject json {
        {QStringLiteral("schema_version"), 1},
        {QStringLiteral("case_id"), m_data.caseId},
        {QStringLiteral("type"), evidence.type},
        {QStringLiteral("title"), evidence.title},
        {QStringLiteral("summary"), evidence.summary},
        {QStringLiteral("link"), evidence.link},
        {QStringLiteral("log_file"), evidence.logFile},
        {QStringLiteral("log_line"), evidence.logLine},
        {QStringLiteral("analysis"), analysis.toJson()},
        {QStringLiteral("stdout"), QStringLiteral("logs/draw.stdout.log")},
        {QStringLiteral("stderr"), QStringLiteral("logs/draw.stderr.log")},
    };
    evidenceFile.write(QJsonDocument(json).toJson(QJsonDocument::Indented));

    const QString analysisPath = QDir(runtimeArtifactDirectory()).filePath(QStringLiteral("draw_log_analysis.json"));
    QFile analysisFile(analysisPath);
    if (!analysisFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        if (m_drawConsole != nullptr)
        {
            m_drawConsole->append(QStringLiteral("[DRAW] failed to write %1: %2").arg(analysisPath, analysisFile.errorString()));
        }
        return;
    }
    QJsonObject analysisJson = analysis.toJson();
    analysisJson.insert(QStringLiteral("case_id"), m_data.caseId);
    analysisJson.insert(QStringLiteral("stdout"), QStringLiteral("logs/draw.stdout.log"));
    analysisJson.insert(QStringLiteral("stderr"), QStringLiteral("logs/draw.stderr.log"));
    analysisFile.write(QJsonDocument(analysisJson).toJson(QJsonDocument::Indented));
    saveCurrentCaseManifest();
}

void WorkbenchWindow::exportMarkdownReport()
{
    if (m_reproScriptEdit != nullptr)
    {
        m_data.reproScript = m_reproScriptEdit->toPlainText();
        m_data.manifest.reproScript = m_data.reproScript;
    }
    m_data.manifest.environmentSummary = m_data.environmentSummary;
    m_data.manifest.evidenceItems = m_data.evidenceItems;
    m_data.manifest.testgridRows = m_data.testgridRows;

    const QString reportPath = QDir(runtimeReportDirectory()).filePath(QStringLiteral("repro_report.md"));
    QString error;
    refreshEvidenceBundle();
    refreshVerificationReport();
    if (!occtdebug::MarkdownReportGenerator::writeReproReport(m_data.manifest, reportPath, &error))
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[report] failed: %1").arg(error));
        }
        return;
    }

    if (m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[report] generated %1").arg(QDir::toNativeSeparators(reportPath)));
    }
}

void WorkbenchWindow::exportDiagnosisReport()
{
    if (m_reproScriptEdit != nullptr)
    {
        m_data.reproScript = m_reproScriptEdit->toPlainText();
        m_data.manifest.reproScript = m_data.reproScript;
    }
    m_data.manifest.environmentSummary = m_data.environmentSummary;
    m_data.manifest.geometrySummary = m_data.geometrySummary;
    m_data.manifest.geometryChecks = m_data.geometryChecks;
    m_data.manifest.evidenceItems = m_data.evidenceItems;
    m_data.manifest.verificationItems = m_data.verificationItems;
    m_data.manifest.testgridRows = m_data.testgridRows;

    QDir dir;
    if (!dir.mkpath(runtimeReportDirectory()))
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[diagnosis] failed to create report directory"));
        }
        return;
    }

    const auto cell = [](QString value) {
        value.replace(QLatin1Char('|'), QStringLiteral("\\|"));
        value.replace(QLatin1Char('\n'), QStringLiteral("<br>"));
        return value;
    };

    QStringList lines;
    lines << QStringLiteral("# Diagnosis Report");
    lines << QString();
    lines << QStringLiteral("- Case ID: `%1`").arg(m_data.caseId);
    lines << QStringLiteral("- Generated at: %1").arg(QDateTime::currentDateTime().toString(Qt::ISODate));
    lines << QStringLiteral("- Status: %1").arg(m_data.caseStatus);
    lines << QStringLiteral("- Confidence: %1%").arg(m_data.diagnosisConfidence);
    lines << QString();
    lines << QStringLiteral("## Diagnosis");
    lines << QString();
    lines << (m_data.diagnosis.isEmpty() ? QStringLiteral("No diagnosis text yet.") : m_data.diagnosis);
    lines << QString();
    lines << QStringLiteral("## Evidence");
    lines << QString();
    lines << QStringLiteral("| Type | Title | Summary | Link |");
    lines << QStringLiteral("|---|---|---|---|");
    if (m_data.evidenceItems.isEmpty())
    {
        lines << QStringLiteral("| _none_ | _none_ | No evidence items yet. | |");
    }
    for (const auto& evidence : m_data.evidenceItems)
    {
        lines << QStringLiteral("| %1 | %2 | %3 | `%4` |")
                .arg(cell(evidence.type), cell(evidence.title), cell(evidence.summary), cell(evidence.link));
    }
    lines << QString();
    lines << QStringLiteral("## Related Source Hits");
    lines << QString();
    lines << QStringLiteral("| File | Line | Preview |");
    lines << QStringLiteral("|---|---:|---|");
    int sourceHitCount = 0;
    if (m_sourcePanel != nullptr)
    {
        for (const occtdebug::SourceSearchResult& result : m_sourcePanel->searchResults())
        {
            const QString filePath = result.filePath;
            const int lineNumber = result.lineNumber;
            if (filePath.isEmpty() || lineNumber <= 0)
            {
                continue;
            }
            lines << QStringLiteral("| `%1` | %2 | %3 |")
                    .arg(cell(QDir(sourceRootDirectory()).relativeFilePath(filePath)))
                    .arg(lineNumber)
                    .arg(cell(result.text));
            ++sourceHitCount;
            if (sourceHitCount >= 20)
            {
                break;
            }
        }
    }
    if (sourceHitCount == 0)
    {
        lines << QStringLiteral("| _none_ | 0 | Run source search to attach local hits. |");
    }
    lines << QString();
    lines << QStringLiteral("## Similar Cases");
    lines << QString();
    lines << QStringLiteral("| Case | Title | Score |");
    lines << QStringLiteral("|---|---|---:|");
    if (m_data.similarCases.isEmpty())
    {
        lines << QStringLiteral("| _none_ | No similar cases selected. | 0 |");
    }
    for (const auto& similarCase : m_data.similarCases)
    {
        lines << QStringLiteral("| `%1` | %2 | %3 |")
                .arg(cell(similarCase.id), cell(similarCase.title), cell(similarCase.score));
    }
    lines << QString();
    lines << QStringLiteral("## Verification");
    lines << QString();
    lines << QStringLiteral("| Item | Value |");
    lines << QStringLiteral("|---|---|");
    if (m_data.verificationItems.isEmpty())
    {
        lines << QStringLiteral("| _none_ | No verification result yet. |");
    }
    for (const auto& itemValue : m_data.verificationItems)
    {
        lines << QStringLiteral("| %1 | %2 |").arg(cell(itemValue.label), cell(itemValue.value));
    }
    lines << QString();
    lines << QStringLiteral("## Limitations");
    lines << QString();
    lines << QStringLiteral("- This report is generated from local Case evidence and manual/source-search context.");
    lines << QStringLiteral("- It does not apply patches or run full testgrid automatically.");
    lines << QStringLiteral("- Source hits are bounded UI search results, not a full semantic code analysis.");
    lines << QString();

    const QString reportPath = QDir(runtimeReportDirectory()).filePath(QStringLiteral("diagnosis_report.md"));
    QString error;
    if (!writeTextFile(reportPath, lines.join(QLatin1Char('\n')), &error))
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[diagnosis] %1").arg(error));
        }
        return;
    }

    appendEvidenceRecord({
        QStringLiteral("diagnosis_report"),
        QStringLiteral("Diagnosis report"),
        QStringLiteral("diagnosis=%1 confidence=%2 source_hits=%3")
            .arg(m_data.diagnosis.left(80))
            .arg(m_data.diagnosisConfidence)
            .arg(sourceHitCount),
        QStringLiteral("report/diagnosis_report.md"),
    });
    saveCurrentCaseManifest();
    if (m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[diagnosis] generated %1").arg(QDir::toNativeSeparators(reportPath)));
    }
}

void WorkbenchWindow::recordPatchReviewState(const QString& note)
{
    m_data.patchReviewStatus = m_patchReview.statusText();
    m_data.patchReviewItems.clear();
    for (const occtdebug::PatchReviewStep& step : m_patchReview.steps())
    {
        const QString value = step.note.isEmpty()
            ? step.state
            : QStringLiteral("%1 - %2").arg(step.state, step.note);
        m_data.patchReviewItems.push_back({step.title, value});
    }

    auto setMetric = [this](const QString& labelText, const QString& valueText) {
        for (auto& itemValue : m_data.verificationItems)
        {
            if (itemValue.label == labelText)
            {
                itemValue.value = valueText;
                return;
            }
        }
        m_data.verificationItems.push_back({labelText, valueText});
    };
    setMetric(QStringLiteral("patch review"), QStringLiteral("%1: %2").arg(m_patchReview.statusText(), note));

    m_data.manifest.patchDiff = m_data.patchDiff;
    m_data.manifest.patchReviewStatus = m_data.patchReviewStatus;
    m_data.manifest.patchWorktreeRoot = m_data.patchWorktreeRoot;
    m_data.manifest.patchApplyStatus = m_data.patchApplyStatus;
    m_data.manifest.patchApplyLog = m_data.patchApplyLog;
    m_data.manifest.patchSignoffStatus = m_data.patchSignoffStatus;
    m_data.manifest.patchSignoffNote = m_data.patchSignoffNote;
    m_data.manifest.patchReviewItems = m_data.patchReviewItems;
    m_data.manifest.verificationItems = m_data.verificationItems;
    saveCurrentCaseManifest();
}

void WorkbenchWindow::savePatchCandidateDiff()
{
    const QString nextDiff = m_patchDiffEdit != nullptr ? m_patchDiffEdit->toPlainText() : m_data.patchDiff;
    const QString trimmed = nextDiff.trimmed();
    if (trimmed.isEmpty())
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[patch] candidate diff is empty"));
        }
        return;
    }

    QDir dir;
    if (!dir.mkpath(runtimeArtifactDirectory()))
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[patch] failed to create artifact directory"));
        }
        return;
    }

    const bool changed = nextDiff != m_data.patchDiff;
    m_data.patchDiff = nextDiff;
    m_data.manifest.patchDiff = m_data.patchDiff;
    if (changed)
    {
        m_patchReview = occtdebug::PatchReviewWorkflow::createDefault(m_data.caseId, m_data.patchDiff);
        m_data.patchReviewStatus = m_patchReview.statusText();
        m_data.patchSignoffStatus = QStringLiteral("not requested");
        m_data.patchSignoffNote = QStringLiteral("Patch candidate changed; previous signoff was reset.");
        m_data.patchApplyStatus = QStringLiteral("not applied");
        m_data.patchApplyLog.clear();
    }

    QString error;
    const QString patchPath = QDir(runtimeArtifactDirectory()).filePath(QStringLiteral("candidate_patch.diff"));
    if (!writeTextFile(patchPath, m_data.patchDiff, &error))
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[patch] %1").arg(error));
        }
        return;
    }

    const QByteArray diffBytes = m_data.patchDiff.toUtf8();
    const QString sha256 = QString::fromLatin1(QCryptographicHash::hash(diffBytes, QCryptographicHash::Sha256).toHex());
    const QJsonObject json {
        {QStringLiteral("schema_version"), 1},
        {QStringLiteral("case_id"), m_data.caseId},
        {QStringLiteral("created_at"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
        {QStringLiteral("patch"), QStringLiteral("artifacts/candidate_patch.diff")},
        {QStringLiteral("bytes"), diffBytes.size()},
        {QStringLiteral("sha256"), sha256},
        {QStringLiteral("review_status"), m_data.patchReviewStatus},
        {QStringLiteral("signoff_status"), m_data.patchSignoffStatus},
        {QStringLiteral("worktree_root_configured"), !m_data.patchWorktreeRoot.trimmed().isEmpty()},
    };
    const QString manifestPath = QDir(runtimeArtifactDirectory()).filePath(QStringLiteral("candidate_patch_manifest.json"));
    if (!writeTextFile(manifestPath, QString::fromUtf8(QJsonDocument(json).toJson(QJsonDocument::Indented)), &error))
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[patch] %1").arg(error));
        }
        return;
    }

    m_data.manifest.patchReviewStatus = m_data.patchReviewStatus;
    m_data.manifest.patchApplyStatus = m_data.patchApplyStatus;
    m_data.manifest.patchApplyLog = m_data.patchApplyLog;
    m_data.manifest.patchSignoffStatus = m_data.patchSignoffStatus;
    m_data.manifest.patchSignoffNote = m_data.patchSignoffNote;
    updatePatchReviewStatus();
    appendEvidenceRecord({
        QStringLiteral("patch"),
        QStringLiteral("Candidate patch captured"),
        QStringLiteral("bytes=%1 sha256=%2").arg(diffBytes.size()).arg(sha256.left(12)),
        QStringLiteral("artifacts/candidate_patch_manifest.json"),
    });
    saveCurrentCaseManifest();
    refreshEvidenceBundle();
    refreshVerificationReport();
    if (m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[patch] candidate saved to %1").arg(QDir::toNativeSeparators(patchPath)));
    }
}

void WorkbenchWindow::importPatchCandidateDiff()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this,
        s("Import patch candidate"),
        runtimeArtifactDirectory(),
        QStringLiteral("Patch files (*.patch *.diff);;All files (*.*)"));
    if (filePath.isEmpty())
    {
        return;
    }

    const QString diff = readTextFile(filePath);
    if (diff.trimmed().isEmpty())
    {
        QMessageBox::warning(this, s("Import patch candidate"), QStringLiteral("patch file is empty or unreadable: %1").arg(QDir::toNativeSeparators(filePath)));
        return;
    }
    if (m_patchDiffEdit != nullptr)
    {
        m_patchDiffEdit->setPlainText(diff);
    }
    savePatchCandidateDiff();
    if (m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[patch] imported candidate diff from %1").arg(QDir::toNativeSeparators(filePath)));
    }
}

void WorkbenchWindow::generatePatchCandidateFromWorktree()
{
    if (m_patchRunner == nullptr || m_patchRunner->isRunning())
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[patch] another patch command is already running"));
        }
        return;
    }

    const QString worktree = QDir::cleanPath(m_data.patchWorktreeRoot.trimmed());
    if (worktree.isEmpty() || !QFileInfo::exists(worktree))
    {
        QMessageBox::warning(this, QStringLiteral("Generate patch candidate"), QStringLiteral("patch.worktree_root is not configured or does not exist."));
        return;
    }

    QDir dir;
    if (!dir.mkpath(runtimeArtifactDirectory()) || !dir.mkpath(runtimeLogDirectory()))
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[patch] failed to create case log/artifact directories"));
        }
        return;
    }

    occtdebug::CommandRequest request;
    request.program = QStringLiteral("git");
    request.arguments = {QStringLiteral("diff"), QStringLiteral("--binary"), QStringLiteral("HEAD")};
    request.workingDirectory = worktree;
    request.environment = QProcessEnvironment::systemEnvironment();
    request.timeoutMs = kPatchCommandTimeoutMs;

    m_patchRunMode = PatchRunMode::Generate;
    QString error;
    if (!m_patchRunner->start(request, &error))
    {
        m_patchRunMode = PatchRunMode::None;
        QMessageBox::warning(this, QStringLiteral("Generate patch candidate"), QStringLiteral("failed to start git diff: %1").arg(error));
        return;
    }
    recordTaskStarted(QStringLiteral("patch.generate"),
                      QStringLiteral("Generate candidate patch"),
                      request,
                      QStringLiteral("artifacts/patch_generate_result.json"),
                      QStringLiteral("logs/patch_generate.stdout.log"),
                      QStringLiteral("logs/patch_generate.stderr.log"));

    if (m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[patch] started: git diff --binary HEAD in %1%2")
                .arg(QDir::toNativeSeparators(worktree), timeoutSuffix(request.timeoutMs)));
    }
    return;

}

void WorkbenchWindow::exportPatchCandidateDiff()
{
    const QString diff = m_patchDiffEdit != nullptr ? m_patchDiffEdit->toPlainText() : m_data.patchDiff;
    if (diff.trimmed().isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("Export patch candidate"), QStringLiteral("current patch candidate diff is empty."));
        return;
    }

    const QString defaultName = QStringLiteral("%1_candidate.patch").arg(m_data.caseId.isEmpty() ? QStringLiteral("OCCTDebug") : m_data.caseId);
    const QString outputPath = QFileDialog::getSaveFileName(
        this,
        s("Export patch candidate"),
        QDir(runtimeArtifactDirectory()).filePath(defaultName),
        QStringLiteral("Patch files (*.patch *.diff);;All files (*.*)"));
    if (outputPath.isEmpty())
    {
        return;
    }

    QString error;
    if (!writeTextFile(outputPath, diff, &error))
    {
        QMessageBox::warning(this, s("Export patch candidate"), error);
        return;
    }
    if (m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[patch] exported candidate patch to %1").arg(QDir::toNativeSeparators(outputPath)));
    }
}

void WorkbenchWindow::signOffPatchCandidate()
{
    const QString diff = m_patchDiffEdit != nullptr ? m_patchDiffEdit->toPlainText() : m_data.patchDiff;
    if (diff.trimmed().isEmpty())
    {
        QMessageBox::warning(this, s("Patch signoff"), QStringLiteral("current patch candidate diff is empty; cannot sign off."));
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[patch] signoff blocked: candidate diff is empty"));
        }
        return;
    }

    savePatchCandidateDiff();
    refreshVerificationReport();

    const QString reportJsonPath = QDir(runtimeVerificationDirectory()).filePath(QStringLiteral("verification_report.json"));
    QFile file(reportJsonPath);
    QJsonObject report;
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        report = QJsonDocument::fromJson(file.readAll()).object();
    }

    const QString overall = report.value(QStringLiteral("overall_status")).toString();
    const QString patchReviewGate = report.value(QStringLiteral("gate")).toObject()
        .value(QStringLiteral("patch_review")).toObject()
        .value(QStringLiteral("state")).toString();
    const bool canSignOff = overall == QStringLiteral("passed")
        && (patchReviewGate == QStringLiteral("passed") || patchReviewGate == QStringLiteral("not_failed"));

    m_data.patchSignoffStatus = canSignOff ? QStringLiteral("signed off") : QStringLiteral("blocked");
    m_data.patchSignoffNote = canSignOff
        ? QStringLiteral("VerificationReport overall_status=passed and patch review gate is acceptable.")
        : QStringLiteral("VerificationReport overall_status=%1 patch_review=%2; signoff requires passed verification.")
            .arg(overall.isEmpty() ? QStringLiteral("unknown") : overall,
                 patchReviewGate.isEmpty() ? QStringLiteral("unknown") : patchReviewGate);
    m_data.manifest.patchSignoffStatus = m_data.patchSignoffStatus;
    m_data.manifest.patchSignoffNote = m_data.patchSignoffNote;

    setVerificationMetric(QStringLiteral("patch signoff"), QStringLiteral("%1: %2").arg(m_data.patchSignoffStatus, m_data.patchSignoffNote));
    appendEvidenceRecord({
        QStringLiteral("patch"),
        QStringLiteral("Patch signoff"),
        QStringLiteral("%1: %2").arg(m_data.patchSignoffStatus, m_data.patchSignoffNote.left(180)),
        QStringLiteral("verification/verification_report.json"),
    });
    updatePatchReviewStatus();
    saveCurrentCaseManifest();
    refreshEvidenceBundle();
    refreshVerificationReport();
    if (m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[patch] signoff %1: %2")
                .arg(m_data.patchSignoffStatus, m_data.patchSignoffNote));
    }
}

void WorkbenchWindow::applyPatchCandidate()
{
    runPatchCommand(PatchRunMode::Apply);
}

void WorkbenchWindow::undoPatchCandidate()
{
    runPatchCommand(PatchRunMode::Undo);
}

void WorkbenchWindow::runPatchCommand(PatchRunMode mode)
{
    if (mode == PatchRunMode::None)
    {
        return;
    }
    if (m_patchRunner == nullptr || m_patchRunner->isRunning())
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[patch] another patch command is already running"));
        }
        return;
    }

    const QString worktree = QDir::cleanPath(m_data.patchWorktreeRoot.trimmed());
    if (worktree.isEmpty() || !QFileInfo::exists(worktree))
    {
        m_data.patchApplyStatus = QStringLiteral("not configured");
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[patch] patch.worktree_root is not configured or does not exist"));
        }
        updatePatchReviewStatus();
        saveCurrentCaseManifest();
        return;
    }

    if (m_data.patchDiff.trimmed().isEmpty() || m_data.patchDiff.trimmed().startsWith(QStringLiteral("# No patch candidate")))
    {
        m_data.patchApplyStatus = QStringLiteral("no candidate");
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[patch] no patch candidate diff is available"));
        }
        updatePatchReviewStatus();
        saveCurrentCaseManifest();
        return;
    }

    QDir dir;
    dir.mkpath(runtimeArtifactDirectory());
    const QString patchPath = QDir(runtimeArtifactDirectory()).filePath(QStringLiteral("candidate_patch.diff"));
    QString writeError;
    if (!writeTextFile(patchPath, m_data.patchDiff, &writeError))
    {
        m_data.patchApplyStatus = QStringLiteral("write failed");
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[patch] %1").arg(writeError));
        }
        updatePatchReviewStatus();
        saveCurrentCaseManifest();
        return;
    }

    if (!runPatchDryRun(mode, worktree, patchPath))
    {
        m_patchRunMode = PatchRunMode::None;
        updatePatchReviewStatus();
        saveCurrentCaseManifest();
        return;
    }

    QStringList arguments {QStringLiteral("apply")};
    if (mode == PatchRunMode::Undo)
    {
        arguments << QStringLiteral("-R");
    }
    arguments << patchPath;

    const QString action = mode == PatchRunMode::Undo ? QStringLiteral("undo") : QStringLiteral("apply");
    m_patchRunMode = mode;
    m_data.patchApplyStatus = QStringLiteral("%1 running").arg(action);
    m_data.manifest.patchApplyStatus = m_data.patchApplyStatus;
    updatePatchReviewStatus();

    QString startError;
    occtdebug::CommandRequest request;
    request.program = QStringLiteral("git");
    request.arguments = arguments;
    request.workingDirectory = worktree;
    request.environment = QProcessEnvironment::systemEnvironment();
    request.timeoutMs = kPatchCommandTimeoutMs;

    if (!m_patchRunner->start(request, &startError))
    {
        m_patchRunMode = PatchRunMode::None;
        m_data.patchApplyStatus = QStringLiteral("%1 start failed").arg(action);
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[patch] failed to start git apply: %1").arg(startError));
        }
        updatePatchReviewStatus();
        saveCurrentCaseManifest();
        return;
    }
    const QString taskId = mode == PatchRunMode::Undo ? QStringLiteral("patch.undo") : QStringLiteral("patch.apply");
    recordTaskStarted(taskId,
                      QStringLiteral("Patch %1").arg(action),
                      request,
                      QStringLiteral("artifacts/patch_%1_result.json").arg(action),
                      QStringLiteral("logs/patch_%1.stdout.log").arg(action),
                      QStringLiteral("logs/patch_%1.stderr.log").arg(action));

    if (m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[patch] git %1 in %2 using %3%4")
                .arg(arguments.join(QLatin1Char(' ')),
                     QDir::toNativeSeparators(worktree),
                     QDir::toNativeSeparators(patchPath),
                     timeoutSuffix(request.timeoutMs)));
    }
}

bool WorkbenchWindow::runPatchDryRun(PatchRunMode mode, const QString& worktree, const QString& patchPath)
{
    const bool isUndo = mode == PatchRunMode::Undo;
    const QString action = isUndo ? QStringLiteral("undo") : QStringLiteral("apply");
    const QString artifactName = QStringLiteral("patch_%1_dry_run_result.json").arg(action);
    const QString stdoutName = QStringLiteral("patch_%1_dry_run.stdout.log").arg(action);
    const QString stderrName = QStringLiteral("patch_%1_dry_run.stderr.log").arg(action);
    const QString artifactPath = QDir(runtimeArtifactDirectory()).filePath(artifactName);
    const QString stdoutPath = QDir(runtimeLogDirectory()).filePath(stdoutName);
    const QString stderrPath = QDir(runtimeLogDirectory()).filePath(stderrName);

    QDir dir;
    dir.mkpath(runtimeArtifactDirectory());
    dir.mkpath(runtimeLogDirectory());

    QStringList arguments {QStringLiteral("apply")};
    if (isUndo)
    {
        arguments << QStringLiteral("-R");
    }
    arguments << QStringLiteral("--check") << patchPath;

    QProcess process;
    process.setProgram(QStringLiteral("git"));
    process.setArguments(arguments);
    process.setWorkingDirectory(worktree);
    process.setProcessEnvironment(QProcessEnvironment::systemEnvironment());

    if (m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[patch] dry-run: git %1 in %2")
                .arg(arguments.join(QLatin1Char(' ')), QDir::toNativeSeparators(worktree)));
    }

    const QElapsedTimer timer = [] {
        QElapsedTimer out;
        out.start();
        return out;
    }();
    process.start();
    const bool started = process.waitForStarted(10000);
    bool finished = false;
    if (started)
    {
        finished = process.waitForFinished(60000);
    }

    if (started && !finished)
    {
        process.kill();
        process.waitForFinished(3000);
    }

    const QString stdoutText = QString::fromUtf8(process.readAllStandardOutput());
    QString stderrText = QString::fromUtf8(process.readAllStandardError());
    if (!started)
    {
        stderrText = QStringLiteral("failed to start git apply --check");
    }
    else if (!finished)
    {
        stderrText += QStringLiteral("\ngit apply --check timed out");
    }

    const bool success = started
        && finished
        && process.exitStatus() == QProcess::NormalExit
        && process.exitCode() == 0;

    QString writeError;
    writeTextFile(stdoutPath, stdoutText, &writeError);
    writeTextFile(stderrPath, stderrText, &writeError);

    QJsonObject json {
        {QStringLiteral("schema_version"), 1},
        {QStringLiteral("case_id"), m_data.caseId},
        {QStringLiteral("action"), action},
        {QStringLiteral("mode"), QStringLiteral("dry_run")},
        {QStringLiteral("program"), QStringLiteral("git")},
        {QStringLiteral("arguments"), QJsonArray::fromStringList(arguments)},
        {QStringLiteral("working_directory"), worktree},
        {QStringLiteral("exit_code"), started ? process.exitCode() : -1},
        {QStringLiteral("exit_status"), started && process.exitStatus() == QProcess::NormalExit ? QStringLiteral("normal") : QStringLiteral("crash_or_not_started")},
        {QStringLiteral("elapsed_ms"), static_cast<double>(timer.elapsed())},
        {QStringLiteral("success"), success},
        {QStringLiteral("stdout"), QStringLiteral("logs/%1").arg(stdoutName)},
        {QStringLiteral("stderr"), QStringLiteral("logs/%1").arg(stderrName)},
        {QStringLiteral("conflict_hint"), success ? QString() : stderrText.left(1200)},
    };
    const QJsonArray conflicts = patchConflictHints(stderrText + QLatin1Char('\n') + stdoutText);
    if (!conflicts.isEmpty())
    {
        json.insert(QStringLiteral("conflicts"), conflicts);
    }

    QFile artifactFile(artifactPath);
    if (artifactFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        artifactFile.write(QJsonDocument(json).toJson(QJsonDocument::Indented));
    }
    else if (m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[patch] failed to write %1: %2").arg(artifactPath, artifactFile.errorString()));
    }

    const QString status = QStringLiteral("%1 dry-run %2").arg(action, success ? QStringLiteral("passed") : QStringLiteral("failed"));
    setVerificationMetric(QStringLiteral("patch dry-run"), status);
    if (m_verificationPanel != nullptr)
    {
        m_verificationPanel->setItems(m_data.verificationItems);
    }

    QString dryRunLogFile = QStringLiteral("logs/%1").arg(stderrName);
    int dryRunLogLine = firstDrawErrorLine(stderrText);
    if (dryRunLogLine <= 0)
    {
        const int stdoutLine = firstDrawErrorLine(stdoutText);
        if (stdoutLine > 0)
        {
            dryRunLogFile = QStringLiteral("logs/%1").arg(stdoutName);
            dryRunLogLine = stdoutLine;
        }
    }

    occtdebug::EvidenceRecord dryRunEvidence;
    dryRunEvidence.type = QStringLiteral("patch");
    dryRunEvidence.title = QStringLiteral("Patch %1 dry-run").arg(action);
    dryRunEvidence.summary = QStringLiteral("status=%1 exit=%2")
        .arg(success ? QStringLiteral("ok") : QStringLiteral("failed"))
        .arg(started ? process.exitCode() : -1);
    dryRunEvidence.link = QStringLiteral("artifacts/%1").arg(artifactName);
    dryRunEvidence.logFile = dryRunLogLine > 0 ? dryRunLogFile : QString();
    dryRunEvidence.logLine = dryRunLogLine;
    if (!conflicts.isEmpty())
    {
        const QJsonObject firstConflict = conflicts.first().toObject();
        dryRunEvidence.sourceFile = firstConflict.value(QStringLiteral("source_file")).toString();
        dryRunEvidence.sourceLine = firstConflict.value(QStringLiteral("source_line")).toInt();
        dryRunEvidence.summary += QStringLiteral(" conflicts=%1").arg(conflicts.size());
    }
    appendEvidenceRecord(dryRunEvidence);

    if (!success)
    {
        m_data.patchApplyStatus = status;
        m_data.patchApplyLog = QStringLiteral("artifacts/%1").arg(artifactName);
        m_data.manifest.patchApplyStatus = m_data.patchApplyStatus;
        m_data.manifest.patchApplyLog = m_data.patchApplyLog;
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[patch] %1; worktree was not modified. See %2")
                    .arg(status, QDir::toNativeSeparators(artifactPath)));
            if (!stderrText.trimmed().isEmpty())
            {
                m_cmakeConsole->append(stderrText.trimmed().left(2000));
            }
        }
        return false;
    }

    if (m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[patch] %1; starting real %2").arg(status, action));
    }
    return true;
}

void WorkbenchWindow::persistPatchCandidateGenerationResult(const occtdebug::CommandResult& result)
{
    const bool commandSuccess = commandSucceeded(result);
    const bool candidateAvailable = commandSuccess && !result.stdoutText.trimmed().isEmpty();
    const QByteArray diffBytes = result.stdoutText.toUtf8();
    const QString sha256 = candidateAvailable
        ? QString::fromLatin1(QCryptographicHash::hash(diffBytes, QCryptographicHash::Sha256).toHex())
        : QString();
    const QString artifactName = QStringLiteral("patch_generate_result.json");
    const QString stdoutName = QStringLiteral("patch_generate.stdout.log");
    const QString stderrName = QStringLiteral("patch_generate.stderr.log");
    const QString artifactPath = QDir(runtimeArtifactDirectory()).filePath(artifactName);
    const QString stdoutPath = QDir(runtimeLogDirectory()).filePath(stdoutName);
    const QString stderrPath = QDir(runtimeLogDirectory()).filePath(stderrName);

    QDir dir;
    dir.mkpath(runtimeArtifactDirectory());
    dir.mkpath(runtimeLogDirectory());

    QString writeError;
    writeTextFile(stdoutPath, result.stdoutText, &writeError);
    writeTextFile(stderrPath, result.stderrText, &writeError);

    const QJsonObject json {
        {QStringLiteral("schema_version"), 1},
        {QStringLiteral("case_id"), m_data.caseId},
        {QStringLiteral("action"), QStringLiteral("generate_candidate_from_worktree")},
        {QStringLiteral("program"), result.program},
        {QStringLiteral("arguments"), QJsonArray::fromStringList(result.arguments)},
        {QStringLiteral("working_directory"), result.workingDirectory},
        {QStringLiteral("exit_code"), result.exitCode},
        {QStringLiteral("exit_status"), result.exitStatus == QProcess::NormalExit ? QStringLiteral("normal") : QStringLiteral("crash")},
        {QStringLiteral("elapsed_ms"), static_cast<double>(result.elapsedMs)},
        {QStringLiteral("canceled"), result.canceled},
        {QStringLiteral("timed_out"), result.timedOut},
        {QStringLiteral("timeout_ms"), result.timeoutMs},
        {QStringLiteral("success"), commandSuccess},
        {QStringLiteral("candidate_available"), candidateAvailable},
        {QStringLiteral("candidate_patch"), candidateAvailable ? QStringLiteral("artifacts/candidate_patch.diff") : QString()},
        {QStringLiteral("candidate_bytes"), diffBytes.size()},
        {QStringLiteral("candidate_sha256"), sha256},
        {QStringLiteral("stdout"), QStringLiteral("logs/%1").arg(stdoutName)},
        {QStringLiteral("stderr"), QStringLiteral("logs/%1").arg(stderrName)},
    };

    if (!writeTextFile(artifactPath, QString::fromUtf8(QJsonDocument(json).toJson(QJsonDocument::Indented)), &writeError)
        && m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[patch] %1").arg(writeError));
    }

    const QString generationStatus = candidateAvailable
        ? QStringLiteral("generated bytes=%1 sha256=%2").arg(diffBytes.size()).arg(sha256.left(12))
        : (commandSuccess ? QStringLiteral("no diff against HEAD") : QStringLiteral("failed exit=%1").arg(result.exitCode));
    setVerificationMetric(QStringLiteral("patch generation"), generationStatus);

    appendEvidenceRecord({
        QStringLiteral("patch"),
        QStringLiteral("Patch candidate generation"),
        QStringLiteral("status=%1 elapsed=%2ms").arg(generationStatus).arg(result.elapsedMs),
        QStringLiteral("artifacts/patch_generate_result.json"),
    });

    if (candidateAvailable)
    {
        if (m_patchDiffEdit != nullptr)
        {
            m_patchDiffEdit->setPlainText(result.stdoutText);
        }
        savePatchCandidateDiff();
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[patch] generated candidate from git diff --binary HEAD; %1").arg(generationStatus));
        }
        if (!result.stderrText.trimmed().isEmpty() && m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(result.stderrText.trimmed().left(1200));
        }
        return;
    }

    updatePatchReviewStatus();
    saveCurrentCaseManifest();
    refreshEvidenceBundle();
    refreshVerificationReport();

    if (!commandSuccess)
    {
        QMessageBox::warning(this, QStringLiteral("Generate patch candidate"), QStringLiteral("git diff failed: %1").arg(result.stderrText.left(1200)));
    }
    else
    {
        QMessageBox::information(this, QStringLiteral("Generate patch candidate"), QStringLiteral("worktree has no diff against HEAD."));
    }
}

void WorkbenchWindow::persistPatchCommandResult(const occtdebug::CommandResult& result)
{
    const bool isUndo = m_patchRunMode == PatchRunMode::Undo;
    const QString action = isUndo ? QStringLiteral("undo") : QStringLiteral("apply");
    const bool success = commandSucceeded(result);
    const QString artifactName = QStringLiteral("patch_%1_result.json").arg(action);
    const QString stdoutName = QStringLiteral("patch_%1.stdout.log").arg(action);
    const QString stderrName = QStringLiteral("patch_%1.stderr.log").arg(action);
    const QString artifactPath = QDir(runtimeArtifactDirectory()).filePath(artifactName);
    const QString stdoutPath = QDir(runtimeLogDirectory()).filePath(stdoutName);
    const QString stderrPath = QDir(runtimeLogDirectory()).filePath(stderrName);

    QDir dir;
    dir.mkpath(runtimeArtifactDirectory());
    dir.mkpath(runtimeLogDirectory());

    QString writeError;
    writeTextFile(stdoutPath, result.stdoutText, &writeError);
    writeTextFile(stderrPath, result.stderrText, &writeError);

    const QJsonObject json {
        {QStringLiteral("schema_version"), 1},
        {QStringLiteral("case_id"), m_data.caseId},
        {QStringLiteral("action"), action},
        {QStringLiteral("program"), result.program},
        {QStringLiteral("arguments"), QJsonArray::fromStringList(result.arguments)},
        {QStringLiteral("working_directory"), result.workingDirectory},
        {QStringLiteral("exit_code"), result.exitCode},
        {QStringLiteral("exit_status"), result.exitStatus == QProcess::NormalExit ? QStringLiteral("normal") : QStringLiteral("crash")},
        {QStringLiteral("elapsed_ms"), static_cast<double>(result.elapsedMs)},
        {QStringLiteral("canceled"), result.canceled},
        {QStringLiteral("timed_out"), result.timedOut},
        {QStringLiteral("timeout_ms"), result.timeoutMs},
        {QStringLiteral("success"), success},
        {QStringLiteral("stdout"), QStringLiteral("logs/%1").arg(stdoutName)},
        {QStringLiteral("stderr"), QStringLiteral("logs/%1").arg(stderrName)},
    };

    QFile artifactFile(artifactPath);
    if (artifactFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        artifactFile.write(QJsonDocument(json).toJson(QJsonDocument::Indented));
    }
    else if (m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[patch] failed to write %1: %2").arg(artifactPath, artifactFile.errorString()));
    }

    m_data.patchApplyStatus = QStringLiteral("%1 %2").arg(action, success ? QStringLiteral("passed") : QStringLiteral("failed"));
    m_data.patchApplyLog = QStringLiteral("artifacts/%1").arg(artifactName);
    m_data.manifest.patchApplyStatus = m_data.patchApplyStatus;
    m_data.manifest.patchApplyLog = m_data.patchApplyLog;

    setVerificationMetric(QStringLiteral("patch apply"), m_data.patchApplyStatus);
    m_data.manifest.verificationItems = m_data.verificationItems;
    if (m_verificationPanel != nullptr)
    {
        m_verificationPanel->setItems(m_data.verificationItems);
    }

    QString commandLogFile = QStringLiteral("logs/%1").arg(stderrName);
    int commandLogLine = firstDrawErrorLine(result.stderrText);
    if (commandLogLine <= 0)
    {
        const int stdoutLine = firstDrawErrorLine(result.stdoutText);
        if (stdoutLine > 0)
        {
            commandLogFile = QStringLiteral("logs/%1").arg(stdoutName);
            commandLogLine = stdoutLine;
        }
    }

    occtdebug::EvidenceRecord patchEvidence;
    patchEvidence.type = QStringLiteral("patch");
    patchEvidence.title = QStringLiteral("Patch %1 result").arg(action);
    patchEvidence.summary = QStringLiteral("exit=%1 status=%2 worktree=%3")
        .arg(result.exitCode)
        .arg(success ? QStringLiteral("ok") : QStringLiteral("failed"))
        .arg(QDir::toNativeSeparators(result.workingDirectory));
    patchEvidence.link = m_data.patchApplyLog;
    patchEvidence.logFile = commandLogLine > 0 ? commandLogFile : QString();
    patchEvidence.logLine = commandLogLine;
    appendEvidenceRecord(patchEvidence);

    updatePatchReviewStatus();
    saveCurrentCaseManifest();
}

void WorkbenchWindow::setVerificationMetric(const QString& labelText, const QString& valueText)
{
    for (auto& itemValue : m_data.verificationItems)
    {
        if (itemValue.label == labelText)
        {
            itemValue.value = valueText;
            m_data.manifest.verificationItems = m_data.verificationItems;
            return;
        }
    }
    m_data.verificationItems.push_back({labelText, valueText});
    m_data.manifest.verificationItems = m_data.verificationItems;
}

void WorkbenchWindow::updateReproStatusMetric()
{
    const occtdebug::ReproStatus& status = m_data.reproStatus;
    const QString value = QStringLiteral("overall=%1 draw=%2 cpp=%3 testgrid=%4")
        .arg(status.overall.isEmpty() ? QStringLiteral("unknown") : status.overall,
             status.draw.isEmpty() ? QStringLiteral("unknown") : status.draw,
             status.cpp.isEmpty() ? QStringLiteral("unknown") : status.cpp,
             status.testgrid.isEmpty() ? QStringLiteral("unknown") : status.testgrid);
    setVerificationMetric(QStringLiteral("repro status"), value);
    m_data.manifest.reproStatus = m_data.reproStatus;
}

void WorkbenchWindow::exportPatchReviewReport()
{
    recordPatchReviewState(QStringLiteral("Patch review report exported."));
    refreshVerificationReport();

    QDir dir;
    if (!dir.mkpath(runtimeReportDirectory()))
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[patch] failed to create report directory"));
        }
        return;
    }

    const auto cell = [](QString value) {
        value.replace(QLatin1Char('|'), QStringLiteral("\\|"));
        value.replace(QLatin1Char('\n'), QStringLiteral("<br>"));
        return value;
    };

    QStringList lines;
    lines << QStringLiteral("# Patch Review Report");
    lines << QString();
    lines << QStringLiteral("- Case ID: `%1`").arg(m_data.caseId);
    lines << QStringLiteral("- Generated at: %1").arg(QDateTime::currentDateTime().toString(Qt::ISODate));
    lines << QStringLiteral("- Review status: %1").arg(m_patchReview.statusText());
    lines << QStringLiteral("- Signoff status: %1").arg(m_data.patchSignoffStatus.isEmpty() ? QStringLiteral("not requested") : m_data.patchSignoffStatus);
    if (!m_data.patchSignoffNote.trimmed().isEmpty())
    {
        lines << QStringLiteral("- Signoff note: %1").arg(m_data.patchSignoffNote);
    }
    lines << QString();
    lines << QStringLiteral("## Candidate Diff");
    lines << QString();
    lines << QStringLiteral("```diff");
    lines << m_data.patchDiff;
    lines << QStringLiteral("```");
    lines << QString();
    lines << QStringLiteral("## Review Steps");
    lines << QString();
    lines << QStringLiteral("| Step | State / Note |");
    lines << QStringLiteral("|---|---|");
    for (const auto& step : m_data.patchReviewItems)
    {
        lines << QStringLiteral("| %1 | %2 |").arg(cell(step.label), cell(step.value));
    }
    lines << QString();
    lines << QStringLiteral("## Verification Binding");
    lines << QString();
    lines << QStringLiteral("- Verification report: [verification_report.md](verification_report.md)");
    lines << QStringLiteral("- Structured verification: [../verification/verification_report.json](../verification/verification_report.json)");
    lines << QString();
    lines << QStringLiteral("| Item | Value |");
    lines << QStringLiteral("|---|---|");
    for (const auto& itemValue : m_data.verificationItems)
    {
        lines << QStringLiteral("| %1 | %2 |").arg(cell(itemValue.label), cell(itemValue.value));
    }
    lines << QString();
    lines << QStringLiteral("## testgrid Summary");
    lines << QString();
    lines << QStringLiteral("| Module | Run | Pass | Fail | Rate |");
    lines << QStringLiteral("|---|---:|---:|---:|---:|");
    if (m_data.testgridRows.isEmpty())
    {
        lines << QStringLiteral("| _none_ | 0 | 0 | 0 | n/a |");
    }
    for (const auto& rowValue : m_data.testgridRows)
    {
        lines << QStringLiteral("| %1 | %2 | %3 | %4 | %5 |")
                .arg(cell(rowValue.module), cell(rowValue.runCount), cell(rowValue.passCount), cell(rowValue.failCount), cell(rowValue.passRate));
    }
    lines << QString();
    lines << QStringLiteral("## Limitations");
    lines << QString();
    lines << QStringLiteral("- This report records a candidate, review decision, optional worktree apply/undo result, and signoff state.");
    lines << QStringLiteral("- Signoff requires a passing VerificationReport gate; it does not create a git commit or merge the fix.");
    lines << QStringLiteral("- Approval means the direction is acceptable for verification planning, not that the fix is merged.");
    lines << QString();

    const QString reportPath = QDir(runtimeReportDirectory()).filePath(QStringLiteral("patch_review.md"));
    QString error;
    if (!writeTextFile(reportPath, lines.join(QLatin1Char('\n')), &error))
    {
        if (m_cmakeConsole != nullptr)
        {
            m_cmakeConsole->append(QStringLiteral("[patch] %1").arg(error));
        }
        return;
    }

    appendEvidenceRecord({
        QStringLiteral("patch_review"),
        QStringLiteral("Patch review report"),
        QStringLiteral("status=%1 verification_items=%2").arg(m_patchReview.statusText()).arg(m_data.verificationItems.size()),
        QStringLiteral("report/patch_review.md"),
    });
    saveCurrentCaseManifest();

    if (m_cmakeConsole != nullptr)
    {
        m_cmakeConsole->append(QStringLiteral("[patch] generated %1").arg(QDir::toNativeSeparators(reportPath)));
    }
}

void WorkbenchWindow::updatePatchReviewStatus()
{
    if (m_patchReviewStatus == nullptr)
    {
        return;
    }

    QStringList lines;
    lines << QStringLiteral("Review status: %1").arg(m_patchReview.statusText());
    lines << QStringLiteral("Patch apply: %1").arg(m_data.patchApplyStatus.isEmpty() ? QStringLiteral("not run") : m_data.patchApplyStatus);
    lines << QStringLiteral("Patch signoff: %1").arg(m_data.patchSignoffStatus.isEmpty() ? QStringLiteral("not requested") : m_data.patchSignoffStatus);
    if (!m_data.patchSignoffNote.trimmed().isEmpty())
    {
        lines << QStringLiteral("Signoff note: %1").arg(m_data.patchSignoffNote);
    }
    lines << QStringLiteral("Worktree: %1").arg(m_data.patchWorktreeRoot.isEmpty() ? QStringLiteral("not configured") : QDir::toNativeSeparators(m_data.patchWorktreeRoot));
    if (!m_data.patchApplyLog.isEmpty())
    {
        lines << QStringLiteral("Log: %1").arg(m_data.patchApplyLog);
    }
    for (const occtdebug::PatchReviewStep& step : m_patchReview.steps())
    {
        lines << QStringLiteral("- %1: %2").arg(step.title, step.state);
    }
    m_patchReviewStatus->setText(lines.join(QLatin1Char('\n')));
    m_data.patchReviewStatus = m_patchReview.statusText();
}

void WorkbenchWindow::applyWorkbenchTheme()
{
    qApp->setStyleSheet(QStringLiteral(R"css(
        QMainWindow, QWidget {
            background: #07111c;
            color: #d8e5f2;
            font-family: "Microsoft YaHei UI", "Segoe UI";
            font-size: 13px;
        }
        #TitleBar {
            background: #050c14;
            border-bottom: 1px solid #142235;
        }
        #WorkflowToolbar {
            background: #0a1724;
            border-bottom: 1px solid #17283b;
        }
        #AppIcon {
            color: #55a7ff;
            font-size: 22px;
            font-weight: 700;
        }
        #AppTitle {
            font-size: 17px;
            font-weight: 700;
            color: #f2f7ff;
        }
        #Badge, #SuccessBadge {
            border: 1px solid #24374d;
            border-radius: 4px;
            padding: 2px 10px;
            background: #0d1a29;
            color: #aebbd0;
        }
        #SuccessBadge {
            color: #65df8b;
            border-color: #23553c;
            background: #0b2118;
        }
        #ToolButton, #PrimaryToolButton {
            border: 1px solid #26394f;
            border-radius: 5px;
            padding: 6px 14px;
            background: #101d2b;
            color: #c7d5e7;
        }
        #PrimaryToolButton {
            background: #123968;
            border-color: #2862a8;
            color: #eaf4ff;
        }
        #ToolButton:hover, #PrimaryToolButton:hover {
            background: #17304b;
        }
        #Panel {
            background: #0a1623;
            border: 1px solid #1a2b40;
            border-radius: 6px;
        }
        #PanelTitle {
            color: #b9d9ff;
            font-weight: 700;
        }
        #SearchBox, #EvidenceCard {
            border: 1px solid #20354d;
            border-radius: 4px;
            padding: 7px 9px;
            background: #081320;
            color: #8fa3ba;
        }
        #GeometryViewport {
            border: 1px solid #213954;
            border-radius: 4px;
            background: #06101b;
            color: #9db4c9;
            min-height: 230px;
        }
        #MutedText {
            color: #7f91a8;
        }
        QTabWidget::pane {
            border: 1px solid #182a3f;
            background: #081421;
        }
        QTabBar::tab {
            background: #0c1927;
            border: 1px solid #182a3f;
            padding: 7px 18px;
            color: #8fa3ba;
        }
        QTabBar::tab:selected {
            background: #12365b;
            color: #e5f1ff;
            border-bottom-color: #2b83d8;
        }
        QListWidget, QTreeWidget, QTableWidget, QPlainTextEdit, QTextEdit {
            background: #07121e;
            border: 1px solid #1a2b40;
            border-radius: 4px;
            selection-background-color: #154d86;
            selection-color: #ffffff;
            color: #d1dfef;
        }
        QPlainTextEdit, QTextEdit {
            font-family: "Cascadia Code", Consolas, monospace;
            font-size: 13px;
        }
        QHeaderView::section {
            background: #0e1d2c;
            border: 0;
            border-right: 1px solid #1a2b40;
            padding: 5px;
            color: #9db4c9;
        }
        QProgressBar {
            background: #132031;
            border: 1px solid #26394f;
            border-radius: 4px;
            text-align: center;
            color: #d8e5f2;
        }
        QProgressBar::chunk {
            background: #57bf73;
            border-radius: 3px;
        }
        QSplitter::handle {
            background: #07111c;
        }
    )css"));
}
