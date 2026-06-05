#include "core/app/AppContext.h"
#include "core/case/CaseWorkspaceService.h"
#include "core/Logger.h"
#include "workbench/WorkbenchWindow.h"

#include <QApplication>
#include <QDir>

#include <Standard_Version.hxx>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("OCCT Kernel Expert Workbench"));
    QApplication::setOrganizationName(QStringLiteral("OCCTDebug"));
    QApplication::setApplicationDisplayName(QString::fromUtf8("OCCT 内核专家工作台"));

    Logger::info(QStringLiteral("OCCT %1").arg(QString::fromLatin1(OCC_VERSION_STRING)));
    occtdebug::WorkbenchMockData initialData = occtdebug::createMockWorkbenchData();
    QString contextError;
    const auto appContext = occtdebug::AppContext::createDefault(&contextError);
    if (appContext.has_value())
    {
        const occtdebug::WorkbenchConfig& config = appContext->config();
        Logger::info(QStringLiteral("Repo root: %1").arg(config.repoRoot));
        Logger::info(QStringLiteral("Case root: %1").arg(config.caseRoot));
        Logger::info(QStringLiteral("Build root: %1").arg(config.buildRoot));
        if (!config.drawExe.isEmpty())
        {
            Logger::info(QStringLiteral("DRAWEXE: %1").arg(config.drawExe));
        }
        if (!config.testgridExecutable.isEmpty())
        {
            Logger::info(QStringLiteral("testgrid executable: %1").arg(config.testgridExecutable));
        }
        if (!config.patchWorktreeRoot.isEmpty())
        {
            Logger::info(QStringLiteral("patch worktree: %1").arg(config.patchWorktreeRoot));
        }
        for (const QString& warning : config.warnings)
        {
            Logger::warning(warning);
        }

        occtdebug::CaseWorkspaceService caseWorkspace(config);
        const QString sampleCaseDirectory = QDir(config.repoRoot).absoluteFilePath(QStringLiteral("sample_cases/OCC-LOCAL-2026-0001"));
        QString workspaceError;
        if (caseWorkspace.createFromSample(sampleCaseDirectory, &workspaceError))
        {
            const occtdebug::CaseWorkspaceInfo sampleInfo = caseWorkspace.workspaceInfo(QStringLiteral("OCC-LOCAL-2026-0001"));
            Logger::info(QStringLiteral("Sample case workspace ready: %1").arg(sampleInfo.rootPath));

            QString caseLoadError;
            initialData = occtdebug::createWorkbenchDataFromCaseDirectory(sampleInfo.rootPath, &caseLoadError);
            if (!caseLoadError.isEmpty())
            {
                Logger::warning(QStringLiteral("Sample case workspace load warning: %1").arg(caseLoadError));
            }
            if (initialData.verificationPlan.testgridRoot.isEmpty())
            {
                initialData.verificationPlan.testgridRoot = config.testgridRoot;
            }
            if (initialData.verificationPlan.testgridExecutable.isEmpty())
            {
                initialData.verificationPlan.testgridExecutable = config.testgridExecutable;
            }
            if (initialData.verificationPlan.testgridArguments.isEmpty())
            {
                initialData.verificationPlan.testgridArguments = config.testgridArguments;
            }
            if (initialData.verificationPlan.testgridGroup.isEmpty())
            {
                initialData.verificationPlan.testgridGroup = config.testgridGroup;
            }
            if (initialData.verificationPlan.testgridGrid.isEmpty())
            {
                initialData.verificationPlan.testgridGrid = config.testgridGrid;
            }
            if (initialData.verificationPlan.testgridCase.isEmpty())
            {
                initialData.verificationPlan.testgridCase = config.testgridCase;
            }
            if (initialData.verificationPlan.testdiffExecutable.isEmpty())
            {
                initialData.verificationPlan.testdiffExecutable = config.testdiffExecutable;
            }
            if (initialData.verificationPlan.testdiffArguments.isEmpty())
            {
                initialData.verificationPlan.testdiffArguments = config.testdiffArguments;
            }
            if (initialData.verificationPlan.testdiffOutputRoot.isEmpty())
            {
                initialData.verificationPlan.testdiffOutputRoot = config.testdiffOutputRoot;
            }
            if (initialData.patchWorktreeRoot.isEmpty())
            {
                initialData.patchWorktreeRoot = config.patchWorktreeRoot;
                initialData.manifest.patchWorktreeRoot = config.patchWorktreeRoot;
            }
            initialData.manifest.verificationPlan = initialData.verificationPlan;
        }
        else
        {
            Logger::warning(QStringLiteral("Sample case workspace was not initialized: %1").arg(workspaceError));
        }
    }
    else
    {
        Logger::warning(QStringLiteral("AppContext was not loaded: %1").arg(contextError));
    }

    WorkbenchWindow w(initialData);
    w.show();
    return app.exec();
}
