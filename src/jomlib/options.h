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

#ifndef OPTIONS_H
#define OPTIONS_H

#include <QtCore/QStringList>
#include <QtCore/QSharedPointer>

namespace NMakeFile {

class MacroTable;

class Options
{
public:
    Options();

    bool readCommandLineArguments(QStringList arguments, QString& makefile,
                                  QStringList& targets, MacroTable& macroTable);

    bool buildAllTargets;
    bool buildIfTimeStampsAreEqual;
    bool showLogo;
    bool suppressOutputMessages;
    bool overrideEnvVarMacros;
    bool displayIncludeFileNames;
    bool dryRun;
    bool stopOnErrors;
    bool buildUnrelatedTargetsOnError;
    bool checkTimeStampsButDoNotBuild;
    bool changeTimeStampsButDoNotBuild;
    bool ignorePredefinedRulesAndMacros;
    bool suppressExecutedCommandsDisplay;
    bool printWorkingDir;
    bool batchModeEnabled;
    bool dumpInlineFiles;
    bool dumpDependencyGraph;
    bool dumpDependencyGraphDot;
    bool displayMakeInformation;
    bool showUsageAndExit;
    bool displayBuildInfo;
    bool debugMode;
    bool showVersionAndExit;
    QString fullAppPath;
    QString stderrFile;

private:
    bool expandCommandFiles(QStringList& arguments);
    bool handleCommandLineOption(const QStringList &originalArguments, QString arg, QStringList& arguments, QString& makefile, QString& makeflags);
};

class GlobalOptions
{
public:
    GlobalOptions();
    int maxNumberOfJobs;
    bool isMaxNumberOfJobsSet;
};

extern GlobalOptions g_options;

} // namespace NMakeFile

#endif // OPTIONS_H
