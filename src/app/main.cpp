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
const int nVersionPatch = 4;

static void displayNMakeInformation()
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
    //TODO156: command line options can also be specified as -ACF ...
    //         even -fmakeflags.mk /nNoLogo /nologon works with nmake

    g_options.fullAppPath = QCoreApplication::applicationFilePath();
    g_options.fullAppPath.replace(QLatin1Char('/'), QDir::separator());

    const QStringList arguments = app.arguments();
    QString filename, stdErrFile;
    QStringList otherParams;
    for (int i=1; i < argc; ++i) {
        const char* param = argv[i];
        if (param[0] == '-' || param[0] == '/') {
            if (strlen(param) > 2) {
                QString str = QString::fromLocal8Bit(param+1).toUpper();
                if (str == "HELP") {
                    showUsage();
                    return 0;
                } else if (str == "NOLOGO") {
                    g_options.showLogo = false;
                } else if (str == "DUMPGRAPH") {
                    g_options.dumpDependencyGraph = true;
                    g_options.showLogo = false;
                } else if (str == "DUMPGRAPHDOT") {
                    g_options.dumpDependencyGraph = true;
                    g_options.dumpDependencyGraphDot = true;
                    g_options.showLogo = false;
                } else if (str.startsWith("ERRORREPORT")) {
                    // ignore - we don't send stuff to Microsoft :)
                }
                continue;
            }

            switch (param[1]) {
                case 'f':
                case 'F':
                    ++i;
                    if (i >= argc) {
                        printf("Error: no filename specified.\n");
                        showUsage();
                        return 128;
                    }
                    filename = QString(argv[i]);
                    break;
                case 'x':
                case 'X':
                    ++i;
                    if (i >= argc) {
                        printf("Error: no filename specified.\n");
                        showUsage();
                        return 128;
                    }
                    stdErrFile = QString(argv[i]);
                    break;
                case 'a':
                case 'A':
                    g_options.buildAllTargets = true;
                    break;
                case 'b':
                case 'B':
                    g_options.buildIfTimeStampsAreEqual = true;
                    break;
                case 'c':
                case 'C':
                    g_options.suppressOutputMessages = true;
                    break;
                case 'd':
                case 'D':
                    // display build information
                    break;
                case 'e':
                case 'E':
                    //TODO: "Override env-var macros" What does that mean?
                    break;
                case 'g':
                case 'G':
                    g_options.displayIncludeFileNames = true;
                    break;
                case 'i':
                case 'I':
                    g_options.stopOnErrors = false;
                    break;
                case 'j':
                case 'J':
                    {
                        int j = i+1;
                        QString s = QString::fromLocal8Bit(argv[j]);
                        bool ok;
                        int jobs = s.toUInt(&ok);
                        if (!ok)
                            continue;
                        i=j;
                        g_options.maxNumberOfJobs = jobs;
                    }
                    break;
                case 'k':
                case 'K':
                    g_options.buildUnrelatedTargetsOnError = true;
                    break;
                case 'l':
                case 'L':
                    g_options.showLogo = false;
                    break;
                case 'n':
                case 'N':
                    g_options.dryRun = true;
                    break;
                case 'p':
                case 'P':
                    displayNMakeInformation();
                    return 0;
                case 'q':
                case 'Q':
                    g_options.checkTimeStampsButDoNotBuild = true;
                    break;
                case 'r':
                case 'R':
                    g_options.ignorePredefinedRulesAndMacros = true;
                    break;
                case 's':
                case 'S':
                    break;
                case 't':
                case 'T':
                    g_options.changeTimeStampsButDoNotBuild = true;
                    break;
                case 'u':
                case 'U':
                    g_options.dumpInlineFiles = true;
                    break;
                case 'y':
                case 'Y':
                    g_options.batchModeEnabled = false;
                    break;
                case '?':
                    showUsage();
                    return 0;
                    break;
            }
        } else {
            otherParams.append(QString(argv[i]));
        }
    }

    QStringList targets;
    foreach (const QString& param, otherParams) {
        if (param.contains('=')) {
            // TODO: handle macro definition
            qFatal("macro definition on command line not implemented :-P");
        } else {
            targets.append(param);
        }
    }

    if (g_options.showLogo)
        showLogo();

    if (filename.isEmpty())
        filename = "Makefile";

    QFile file(filename);
    if (!file.open(QFile::ReadOnly)) {
        printf(qPrintable(QString("Cannot open %1.\n").arg(filename)));
        return 128;
    }

    MacroTable macroTable;
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

        QString makeflags;
        bool firstFlag = true;
        foreach (const QString& arg, arguments) {
            if (arg.length() < 2)
                continue;

            QChar ch0 = arg.at(0);
            QChar ch1 = arg.at(1).toLower();
            if ((ch0 == QLatin1Char('/') || ch0 == QLatin1Char('-')) &&
                ch1 != QLatin1Char('f') && ch1 != QLatin1Char('j'))
            {
                QString str = arg.mid(1).toUpper();
                if (str.length() > 1) {
                    // translate long option to short option
                    // TODO: add more translations?
                    if (str == "NOLOGO")
                        str = "L";
                }
                if (!firstFlag)
                    makeflags.append(" /"); // remove when TODO156 is implemented!
                makeflags.append(str);
                firstFlag = false;
            }
        }
        macroTable.setMacroValue("MAKEFLAGS", makeflags);
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
