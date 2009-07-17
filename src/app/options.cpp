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
#include "macrotable.h"
#include "exception.h"
#include <QThread>

namespace NMakeFile {

Options g_options;

Options::Options()
:   buildAllTargets(false),
    buildIfTimeStampsAreEqual(false),
    showLogo(true),
    suppressOutputMessages(false),
    displayIncludeFileNames(false),
    dryRun(false),
    stopOnErrors(true),
    buildUnrelatedTargetsOnError(false),
    checkTimeStampsButDoNotBuild(false),
    changeTimeStampsButDoNotBuild(false),
    ignorePredefinedRulesAndMacros(false),
    suppressExecutedCommandsDisplay(false),
    batchModeEnabled(true),
    dumpInlineFiles(false),
    maxNumberOfJobs(QThread::idealThreadCount()),
    dumpDependencyGraph(false),
    dumpDependencyGraphDot(false),
    displayMakeInformation(false),
    showUsageAndExit(false)
{
}

/**
 * This function reads the following from to the command line arguments:
 *
 * - name of the Makefile
 * - fill the Options structure
 * - fill the MAKEFLAGS variable (and translate long option names to short option names)
 * - set macro values
 * - generate list of targets
 */
bool Options::readCommandLineArguments(QStringList arguments, QString& makefile,
                                       QStringList& targets, MacroTable& macroTable)
{
    QString makeflags;
    arguments.removeAt(0);

    while (!arguments.isEmpty()) {
        QString arg = arguments.takeFirst();
        if (arg.at(0) == QLatin1Char('-') ||
            arg.at(0) == QLatin1Char('/'))
        {
            // handle option
            arg.remove(0, 1);
            if (!handleCommandLineOption(arg, arguments, makefile, makeflags))
                return false;
        } else if (arg.contains(QLatin1Char('='))) {
            // handle macro definition
            int idx = arg.indexOf(QLatin1Char('='));
            QString name = arg.left(idx);
            if (!macroTable.isMacroNameValid(name)) {
                fprintf(stderr, "ERROR: The macro name %s is invalid.", qPrintable(name));
                exit(128);
            }
            macroTable.defineEnvironmentMacroValue(name, arg.mid(idx+1));
        } else {
            // handle target
            if (!targets.contains(arg))
                targets.append(arg);
        }
    }

    macroTable.setMacroValue(QLatin1String("MAKEFLAGS"), makeflags);
    if (makefile.isNull())
        makefile = QLatin1String("Makefile");

    return true;
}

bool Options::handleCommandLineOption(QString arg, QStringList& arguments, QString& makefile, QString& makeflags)
{
    while (!arg.isEmpty()) {
        QString upperArg = arg.toUpper();
        if (arg.length() > 1) {
            if (upperArg.startsWith(QLatin1String("NOLOGO"))) {
                makeflags.append(QLatin1Char('L'));
                arg.remove(0, 6);
                showLogo = false;
            } else if (upperArg.startsWith(QLatin1String("DUMPGRAPH"))) {
                arg.remove(0, 9);
                dumpDependencyGraph = true;
                showLogo = false;
            } else if (upperArg.startsWith(QLatin1String("DUMPGRAPHDOT"))) {
                arg.remove(0, 12);
                dumpDependencyGraph = true;
                dumpDependencyGraphDot = true;
                showLogo = false;
            } else if (upperArg.startsWith(QLatin1String("ERRORREPORT"))) {
                arg.remove(0, 11);
                // ignore - we don't send stuff to Microsoft :)
            }
        }

        if (arg.isEmpty())
            return true;

        QChar ch = arg.at(0);
        if (!makeflags.contains(ch))
            makeflags.append(ch);
        arg.remove(0, 1);
        switch (ch.toUpper().toLatin1()) {
            case 'F':
                makeflags.chop(1);
                if (arg.isEmpty()) {
                    if (arguments.isEmpty()) {
                        fprintf(stderr, "Error: no filename specified for option -f\n");
                        return false;
                    }
                    makefile = arguments.takeFirst();
                } else {
                    makefile = arg;
                    arg = QString();
                }
                break;
            case 'X':
                makeflags.chop(1);
                if (arg.isEmpty()) {
                    if (arguments.isEmpty()) {
                        fprintf(stderr, "Error: no filename specified for option -x\n");
                        return false;
                    }
                    stderrFile = arguments.takeFirst();
                } else {
                    stderrFile = arg;
                    arg = QString();
                }
                break;
            case 'A':
                buildAllTargets = true;
                break;
            case 'B':
                buildIfTimeStampsAreEqual = true;
                break;
            case 'C':
                suppressOutputMessages = true;
                break;
            case 'D':
                // display build information
                break;
            case 'E':
                //TODO: "Override env-var macros" What does that mean?
                break;
            case 'G':
                displayIncludeFileNames = true;
                break;
            case 'I':
                stopOnErrors = false;
                break;
            case 'J':
                {
                    bool ok;
                    if (arg.isEmpty()) {
                        if (arguments.isEmpty()) {
                            fprintf(stderr, "Error: no process count specified for option -j\n");
                            return false;
                        }
                        maxNumberOfJobs = arguments.takeFirst().toUInt(&ok);
                    } else {
                        maxNumberOfJobs = arg.toUInt(&ok);
                        arg = QString();
                    }
                    if (!ok) {
                        fprintf(stderr, "Error: option -j expects a numerical argument\n");
                        return false;
                    }
                    makeflags.chop(1);
                    break;
                }
            case 'K':
                buildUnrelatedTargetsOnError = true;
                break;
            case 'L':
                showLogo = false;
                break;
            case 'N':
                dryRun = true;
                break;
            case 'P':
                displayMakeInformation = true;
                break;
            case 'Q':
                checkTimeStampsButDoNotBuild = true;
                break;
            case 'R':
                ignorePredefinedRulesAndMacros = true;
                break;
            case 'S':
                suppressExecutedCommandsDisplay = true;
                break;
            case 'T':
                changeTimeStampsButDoNotBuild = true;
                break;
            case 'U':
                dumpInlineFiles = true;
                break;
            case 'Y':
                batchModeEnabled = false;
                break;
            case '?':
                showUsageAndExit = true;
                break;
            default:
                fprintf(stderr, "Error: unknown command line option\n");
                return false;
        }
    }

    return true;
}

} // namespace NMakeFile
