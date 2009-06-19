/****************************************************************************
 **
 ** Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies).
 ** Contact: Qt Software Information (qt-info@nokia.com)
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
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QCoreApplication>

#include <windows.h>
#include <Tlhelp32.h>

using namespace NMakeFile;

const int nVersionMajor = 0;
const int nVersionMinor = 6;
const int nVersionPatch = 5;

static void displayMakeInformation()
{
}

static void showLogo()
{
    fprintf(stderr, "jom %d.%d.%d - empower your cores\n\n",
        nVersionMajor, nVersionMinor, nVersionPatch);
}

static void showUsage()
{
    showLogo();
    printf("This is how you do it!\n\n"
           "/DUMPGRAPH show the generated dependency graph\n"
           "/DUMPGRAPHDOT dump dependency graph in dot format\n");
}

static void readEnvironment(MacroTable& macroTable)
{
    wchar_t buf[1];
    size_t retsize;
    _wgetenv_s(&retsize, buf, 1, L""); // initialize _wenviron
    int i = 0;
    if (_wenviron) {
        while (_wenviron[i]) {
            QString env = QString::fromUtf16(reinterpret_cast<ushort*>(_wenviron[i]));
            QString lhs, rhs;
            int idx = env.indexOf('=');
            lhs = env.left(idx);
            rhs = env.right(env.length() - idx - 1);
            //qDebug() << lhs << rhs;
            macroTable.setMacroValue(lhs, rhs, true);
            ++i;
        }
    }
}

static void traverseChildProcesses(DWORD dwProcessId, const QMultiHash<DWORD, DWORD>& processIds)
{
    foreach (DWORD dwChildPID, processIds.values(dwProcessId)) {
        traverseChildProcesses(dwChildPID, processIds);
        //printf("=====> CHILD PID %d\n", dwChildPID);
        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, TRUE, dwChildPID);
        if (hProcess == INVALID_HANDLE_VALUE)
            continue;

        TerminateProcess(hProcess, 5);
        CloseHandle(hProcess);
    }
}

static TargetExecutor* g_pTargetExecutor = 0;

BOOL WINAPI ConsoleCtrlHandlerRoutine(__in  DWORD /*dwCtrlType*/)
{
    fprintf(stderr, "jom terminated by user\n");

    // whatever the event is, we should stop processing
    QMultiHash<DWORD, DWORD> processIds;
    HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapShot == INVALID_HANDLE_VALUE)
        return 0;
    BOOL bSuccess;
    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(processEntry);
    bSuccess = Process32First(hSnapShot, &processEntry);
    while (bSuccess) {
        if (processEntry.th32ParentProcessID > 0)
            processIds.insert(processEntry.th32ParentProcessID, processEntry.th32ProcessID);
        bSuccess = Process32Next(hSnapShot, &processEntry);
    }
    CloseHandle(hSnapShot);

    DWORD dwProcessId = QCoreApplication::applicationPid();
    traverseChildProcesses(dwProcessId, processIds);

    if (g_pTargetExecutor)
        g_pTargetExecutor->removeTempFiles();

    exit(2);
    return TRUE;
}

int main(int argc, char* argv[])
{
    SetConsoleCtrlHandler(&ConsoleCtrlHandlerRoutine, TRUE);
    QCoreApplication app(argc, argv);

    QString filename;
    QStringList targets;
    MacroTable macroTable;
    if (!g_options.readCommandLineArguments(qApp->arguments(), filename, targets, macroTable)) {
        showUsage();
        return 128;
    }
    if (g_options.showUsageAndExit) {
        showUsage();
        return 0;
    }
    g_options.fullAppPath = QCoreApplication::applicationFilePath();
    g_options.fullAppPath.replace(QLatin1Char('/'), QDir::separator());

    if (g_options.showLogo)
        showLogo();

    QFile file(filename);
    if (!file.open(QFile::ReadOnly)) {
        printf(qPrintable(QString("Cannot open %1.\n").arg(filename)));
        return 128;
    }

    readEnvironment(macroTable);
    if (!g_options.ignorePredefinedRulesAndMacros) {
        macroTable.setMacroValue("MAKE", g_options.fullAppPath);
        macroTable.setMacroValue("MAKEDIR", QDir::currentPath());
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
    preprocessor.openFile(filename);

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

        printf(qPrintable(output));
        return 2;
    }

    TargetExecutor executor;
    g_pTargetExecutor = &executor;
    try {
        executor.apply(mkfile, targets);
    }
    catch (Exception e) {
        QString msg = "Error in executor: " + e.message() + "\n";
        printf(qPrintable(msg));
    }

    app.postEvent(&executor, new TargetExecutor::StartEvent());
    int result = app.exec();
    g_pTargetExecutor = 0;
    return result;
}
