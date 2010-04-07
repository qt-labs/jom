/****************************************************************************
 **
 ** Copyright (C) 2008-2010 Nokia Corporation and/or its subsidiary(-ies).
 ** Contact: Nokia Corporation (qt-info@nokia.com)
 **
 ** This file is part of the jom project on Trolltech Labs.
 **
 ** This file may be used under the terms of the GNU General Public
 ** License version 2.0 or 3.0 as published by the Free Software Foundation
 ** and appearing in the file LICENSE.GPL included in the packaging of
 ** this file.  Please review the following information to ensure GNU
 ** General Public Licensing requirements will be met:
 ** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
 ** http://www.gnu.org/copyleft/gpl.html.
 **
 ** If you are unsure which license is appropriate for your use, please
 ** contact the sales department at qt-sales@nokia.com.
 **
 ** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 ** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 **
 ****************************************************************************/
#include "options.h"
#include "parser.h"
#include "preprocessor.h"
#include "targetexecutor.h"
#include "exception.h"

#include <QDebug>
#include <QDir>
#include <QTextStream>
#include <QCoreApplication>
#include <QProcess>
#include <QTextCodec>

#include <windows.h>
#include <Tlhelp32.h>

using namespace NMakeFile;

const int nVersionMajor = 0;
const int nVersionMinor = 9;
const int nVersionPatch = 1;

static void showLogo()
{
    fprintf(stderr, "\njom %d.%d.%d - empower your cores\n\n",
        nVersionMajor, nVersionMinor, nVersionPatch);
}

static void showUsage()
{
    printf("Usage: jom @commandfile\n"
           "       jom [options] [/f makefile] [macro definitions] [targets]\n\n"
           "This tool is meant to be an nmake clone.\n"
           "Please see the Microsoft nmake documentation for more options.\n"
           "/DUMPGRAPH show the generated dependency graph\n"
           "/DUMPGRAPHDOT dump dependency graph in dot format\n");
}

static void readEnvironment(const QStringList& environment, MacroTable& macroTable)
{
    foreach (const QString& env, environment) {
        QString lhs, rhs;
        int idx = env.indexOf('=');
        lhs = env.left(idx);
        rhs = env.right(env.length() - idx - 1);
        //qDebug() << lhs << rhs;
        macroTable.defineEnvironmentMacroValue(lhs, rhs);
    }
}

/**
 * Returns true, if the path contains some whitespace.
 */
static bool isComplexPathName(const QString& path)
{
    for (int i=path.length()-1; i > 0; i--) {
        const QChar ch = path.at(i);
        if (ch.isSpace())
            return true;
    }
    return false;
}

/**
 * Enclose the path in double quotes, if its a complex one.
 */
static QString encloseInDoubleQuotesIfNeeded(const QString& path)
{
    if (isComplexPathName(path)) {
        QString result = "\"";
        result.append(path);
        result.append('"');
        return result;
    }
    return path;
}

static TargetExecutor* g_pTargetExecutor = 0;

BOOL WINAPI ConsoleCtrlHandlerRoutine(__in  DWORD /*dwCtrlType*/)
{
    fprintf(stderr, "jom terminated by user (pid=%u)\n", QCoreApplication::applicationPid());

    HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapShot == INVALID_HANDLE_VALUE)
        return 0;
    BOOL bSuccess;
    const DWORD dwThisPID = QCoreApplication::applicationPid();
    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(processEntry);
    bSuccess = Process32First(hSnapShot, &processEntry);
    while (bSuccess) {
        if (processEntry.th32ParentProcessID == dwThisPID) {
            HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, TRUE, processEntry.th32ProcessID);
            if (hProcess != INVALID_HANDLE_VALUE) {
                //fprintf(stderr, "terminating process %u\n", processEntry.th32ProcessID);
                TerminateProcess(hProcess, 5);
                CloseHandle(hProcess);
            }
        }
        bSuccess = Process32Next(hSnapShot, &processEntry);
    }
    CloseHandle(hSnapShot);

    if (g_pTargetExecutor)
        g_pTargetExecutor->removeTempFiles();

    exit(2);
    return TRUE;
}

int main(int argc, char* argv[])
{
    SetConsoleCtrlHandler(&ConsoleCtrlHandlerRoutine, TRUE);
    QCoreApplication app(argc, argv);
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("IBM 850"));

    QString filename;
    QStringList targets;
    QStringList systemEnvironment = QProcess::systemEnvironment();
    MacroTable macroTable(&systemEnvironment);
    if (!g_options.readCommandLineArguments(qApp->arguments(), filename, targets, macroTable)) {
        showUsage();
        return 128;
    }
    if (g_options.showUsageAndExit) {
        if (g_options.showLogo)
            showLogo();
        showUsage();
        return 0;
    } else if (g_options.showVersionAndExit) {
        printf("jom version %d.%d.%d\n", nVersionMajor, nVersionMinor, nVersionPatch);
        return 0;
    }
    g_options.fullAppPath = QCoreApplication::applicationFilePath();
    g_options.fullAppPath.replace(QLatin1Char('/'), QDir::separator());

    if (g_options.showLogo)
        showLogo();

    readEnvironment(systemEnvironment, macroTable);
    if (!g_options.ignorePredefinedRulesAndMacros) {
        macroTable.setMacroValue("MAKE", encloseInDoubleQuotesIfNeeded(g_options.fullAppPath));
        macroTable.setMacroValue("MAKEDIR", encloseInDoubleQuotesIfNeeded(QDir::currentPath()));
        macroTable.setMacroValue("AS", "ml");       // Macro Assembler
        macroTable.setMacroValue("ASFLAGS", QString::null);
        macroTable.setMacroValue("BC", "bc");       // Basic Compiler
        macroTable.setMacroValue("BCFLAGS", QString::null);
        macroTable.setMacroValue("CC", "cl");       // C Compiler
        macroTable.setMacroValue("CCFLAGS", QString::null);
        macroTable.setMacroValue("COBOL", "cobol"); // COBOL Compiler
        macroTable.setMacroValue("COBOLFLAGS", QString::null);
        macroTable.setMacroValue("CPP", "cl");      // C++ Compiler
        macroTable.setMacroValue("CPPFLAGS", QString::null);
        macroTable.setMacroValue("CXX", "cl");      // C++ Compiler
        macroTable.setMacroValue("CXXFLAGS", QString::null);
        macroTable.setMacroValue("FOR", "fl");      // FORTRAN Compiler
        macroTable.setMacroValue("FORFLAGS", QString::null);
        macroTable.setMacroValue("PASCAL", "pl");   // Pascal Compiler
        macroTable.setMacroValue("PASCALFLAGS", QString::null);
        macroTable.setMacroValue("RC", "rc");       // Resource Compiler
        macroTable.setMacroValue("RCFLAGS", QString::null);
    }

    Preprocessor preprocessor;
    preprocessor.setMacroTable(&macroTable);
    try {
        preprocessor.openFile(filename);
    }
    catch (Exception e) {
        fprintf(stderr, qPrintable(e.message()));
        fprintf(stderr, "\n");
        return 128;
    }

    Parser parser;
    Makefile* mkfile;
    try {
        mkfile = parser.apply(&preprocessor, targets);
    } catch (Exception e) {
        QString output;
        if (e.line() != 0)
            output = QString("ERROR in line %1: %2\n").arg(e.line()).arg(e.message());
        else
            output = QString("ERROR: %1\n").arg(e.message());

        fprintf(stderr, qPrintable(output));
        return 2;
    }

    if (g_options.displayMakeInformation) {
        printf("MACROS:\n\n");
        macroTable.dump();
        printf("\nINFERENCE RULES:\n\n");
        mkfile->dumpInferenceRules();
        printf("\nTARGETS:\n\n");
        mkfile->dumpTargets();
    }

    TargetExecutor executor(systemEnvironment);
    g_pTargetExecutor = &executor;
    try {
        executor.apply(mkfile, targets);
    }
    catch (Exception e) {
        QString msg = "Error in executor: " + e.message() + "\n";
        fprintf(stderr, qPrintable(msg));
    }

    app.postEvent(&executor, new TargetExecutor::StartEvent());
    int result = app.exec();
    g_pTargetExecutor = 0;
    return result;
}
