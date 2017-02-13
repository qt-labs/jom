/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of jom.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "application.h"
#include <helperfunctions.h>
#include <jobserver.h>
#include <options.h>
#include <parser.h>
#include <preprocessor.h>
#include <targetexecutor.h>
#include <exception.h>
#include <makefilefactory.h>

#include <QDebug>
#include <QDir>
#include <QProcess>
#include <QScopedPointer>
#include <QTextCodec>

#include <windows.h>
#include <Tlhelp32.h>

using namespace NMakeFile;

const int nVersionMajor = 1;
const int nVersionMinor = 1;
const int nVersionPatch = 3;

static void showLogo()
{
    fprintf(stderr, "\njom %d.%d.%d - empower your cores\n\n",
        nVersionMajor, nVersionMinor, nVersionPatch);
    fflush(stderr);
}

static void showUsage()
{
    printf("Usage: jom @commandfile\n"
           "       jom [options] [/f makefile] [macro definitions] [targets]\n\n"
           "nmake compatible options:\n"
           "/A build all targets\n"
           "/D display build information\n"
           "/E override environment variable macros\n"
           "/F <filename> use the specified makefile\n"
           "/G display included makefiles\n"
           "/H show help\n"
           "/I ignore all exit codes\n"
           "/K keep going - build unrelated targets on error\n"
           "/N dry run - just print commands\n"
           "/NOLOGO do not print logo\n"
           "/P print makefile info\n"
           "/R ignore predefined rules and macros\n"
           "/S silent mode\n"
           "/U print content of inline files\n"
           "/L same as /NOLOGO\n"
           "/W print the working directory before and after other processing\n"
           "/X <filename> write stderr to file.\n"
           "/Y disable batch mode inference rules\n\n"
           "jom only options:\n"
           "/DUMPGRAPH show the generated dependency graph\n"
           "/DUMPGRAPHDOT dump dependency graph in dot format\n"
           "/J <n> use up to n processes in parallel\n"
           "/KEEPTEMPFILES keep all temporary files\n"
           "/VERSION print version and exit\n");
}

static TargetExecutor* g_pTargetExecutor = 0;

BOOL WINAPI ConsoleCtrlHandlerRoutine(DWORD dwCtrlType)
{
    Q_UNUSED(dwCtrlType);
    fprintf(stderr, "jom terminated by user (pid=%lld)\n", QCoreApplication::applicationPid());
    fflush(stderr);

    GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0);
    if (g_pTargetExecutor)
        g_pTargetExecutor->removeTempFiles();

    exit(2);
    return TRUE;
}

QStringList getCommandLineArguments()
{
    QStringList commandLineArguments = qApp->arguments().mid(1);
    QString makeFlags = qGetEnvironmentVariable(L"JOMFLAGS");
    if (makeFlags.isEmpty())
        makeFlags = qGetEnvironmentVariable(L"MAKEFLAGS");
    if (!makeFlags.isEmpty())
        commandLineArguments.prepend(QLatin1Char('/') + makeFlags);
    return commandLineArguments;
}

static bool initJobServer(const Application &app, ProcessEnvironment *environment,
                          JobServer **outJobServer)
{
    bool mustCreateJobServer = false;
    if (app.isSubJOM()) {
        int inheritedMaxNumberOfJobs = g_options.maxNumberOfJobs;
        const QString str = environment->value(QLatin1String("_JOMJOBCOUNT_"));
        if (!str.isEmpty()) {
            bool ok;
            const int n = str.toInt(&ok);
            if (ok && n > 0)
                inheritedMaxNumberOfJobs = n;
        }
        if (g_options.isMaxNumberOfJobsSet
                && g_options.maxNumberOfJobs != inheritedMaxNumberOfJobs)
        {
            fprintf(stderr, "jom: Overriding inherited number of jobs %d with %d. "
                    "New jobserver created.\n",
                    inheritedMaxNumberOfJobs, g_options.maxNumberOfJobs);
            mustCreateJobServer = true;
        }
    } else {
        mustCreateJobServer = true;
    }

    if (mustCreateJobServer) {
        JobServer *jobServer = new JobServer(environment);
        *outJobServer = jobServer;
        if (!jobServer->start(g_options.maxNumberOfJobs)) {
            fprintf(stderr, "Cannot start job server: %s.", qPrintable(jobServer->errorString()));
            return false;
        }
    }
    return true;
}

int main(int argc, char* argv[])
{
    int result = 0;
    try {
        SetConsoleCtrlHandler(&ConsoleCtrlHandlerRoutine, TRUE);
        Application app(argc, argv);
        QTextCodec::setCodecForLocale(QTextCodec::codecForName("IBM 850"));
        MakefileFactory mf;
        Options* options = 0;
        mf.setEnvironment(QProcess::systemEnvironment());
        if (!mf.apply(getCommandLineArguments(), &options)) {
            switch (mf.errorType()) {
            case MakefileFactory::CommandLineError:
                showUsage();
                return 128;
            case MakefileFactory::ParserError:
            case MakefileFactory::IOError:
                fprintf(stderr, "Error: %s\n", qPrintable(mf.errorString()));
                return 2;
            }
        }

        if (options->showUsageAndExit) {
            if (options->showLogo)
                showLogo();
            showUsage();
            return 0;
        } else if (options->showVersionAndExit) {
            printf("jom version %d.%d.%d\n", nVersionMajor, nVersionMinor, nVersionPatch);
            return 0;
        }

        if (options->showLogo && !app.isSubJOM())
            showLogo();

        QScopedPointer<Makefile> mkfile(mf.makefile());
        if (options->displayMakeInformation) {
            printf("MACROS:\n\n");
            mkfile->macroTable()->dump();
            printf("\nINFERENCE RULES:\n\n");
            mkfile->dumpInferenceRules();
            printf("\nTARGETS:\n\n");
            mkfile->dumpTargets();
        }

        JobServer *jobServer = 0;
        ProcessEnvironment processEnvironment = mkfile->macroTable()->environment();
        if (!initJobServer(app, &processEnvironment, &jobServer))
            return 3;
        QScopedPointer<JobServer> jobServerDeleter(jobServer);

        if (options->printWorkingDir) {
            printf("jom: Entering directory '%s\n",
                   qPrintable(QDir::toNativeSeparators(QDir::currentPath())));
            fflush(stdout);
        }

        if (mkfile->isParallelExecutionDisabled()) {
            printf("jom: parallel job execution disabled for %s\n", qPrintable(mkfile->fileName()));
            g_options.maxNumberOfJobs = 1;
        }

        TargetExecutor executor(processEnvironment);
        QObject::connect(&executor, SIGNAL(finished(int)), &app, SLOT(exit(int)));
        g_pTargetExecutor = &executor;
        executor.apply(mkfile.data(), mf.activeTargets());

        QMetaObject::invokeMethod(&executor, "startProcesses", Qt::QueuedConnection);
        result = app.exec();
        g_pTargetExecutor = 0;
        if (options->printWorkingDir) {
            printf("jom: Leaving directory '%s'\n",
                   qPrintable(QDir::toNativeSeparators(QDir::currentPath())));
            fflush(stdout);
        }
    } catch (const Exception &e) {
        fprintf(stderr, "jom: %s\n", qPrintable(e.message()));
        result = 2;
    }
    return result;
}
