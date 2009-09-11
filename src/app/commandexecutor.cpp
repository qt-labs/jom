/****************************************************************************
 **
 ** Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies).
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
#include "commandexecutor.h"
#include "options.h"
#include "exception.h"
#include "helperfunctions.h"

#include <QTemporaryFile>
#include <QTextStream>
#include <QDebug>
#include <windows.h>

namespace NMakeFile {

QString CommandExecutor::m_tempPath;

CommandExecutor::CommandExecutor(QObject* parent, const QStringList& environment)
:   QObject(parent),
    m_pTarget(0),
    m_nCommandIdx(0),
    m_pLastCommand(0)
{
    if (m_tempPath.isNull()) {
        WCHAR buf[MAX_PATH];
        DWORD count = GetTempPathW(MAX_PATH, buf);
        if (count) {
            m_tempPath = QString::fromUtf16(reinterpret_cast<const ushort*>(buf), count);
            if (!m_tempPath.endsWith('\\')) m_tempPath.append('\\');
        }
    }

    m_process.setEnvironment(environment);
    connect(&m_process, SIGNAL(error(QProcess::ProcessError)), SLOT(onProcessError(QProcess::ProcessError)));
    connect(&m_process, SIGNAL(finished(int, QProcess::ExitStatus)), SLOT(onProcessFinished(int, QProcess::ExitStatus)));
    connect(&m_process, SIGNAL(readyReadStandardError()), SLOT(processReadyReadStandardError()));
    connect(&m_process, SIGNAL(readyReadStandardOutput()), SLOT(processReadyReadStandardOutput()));
}

CommandExecutor::~CommandExecutor()
{
    cleanupTempFiles();
}

void CommandExecutor::start(DescriptionBlock* target)
{
    m_nCommandIdx = 0;
    m_pLastCommand = 0;
    m_pTarget = target;
    target->expandFileNameMacros();
    cleanupTempFiles();
    createTempFiles();

    bool mergeCommands = false;
    QList<Command>::iterator it = target->m_commands.begin();
    QList<Command>::iterator itEnd = target->m_commands.end();
    for (; it != itEnd; ++it) {
        Command& cmd = *it;
        // TODO: this check is a stupid hack!
        if (cmd.m_commandLine.startsWith("cd ") && !cmd.m_commandLine.contains("&&")) {
            mergeCommands = true;
        }
    }

    if (mergeCommands) {
        Command cmd;
        bool firstLoop = true;
        for (it = target->m_commands.begin(); it != itEnd; ++it) {
            if (firstLoop)
                firstLoop = false;
            else
                cmd.m_commandLine += " && ";

            cmd.m_commandLine += (*it).m_commandLine;
        }
        target->m_commands.clear();
        target->m_commands.append(cmd);
    }

    emit started(this);
    if (!executeNextCommand()) {
        emit finished(this, 0);
    }
}

void CommandExecutor::onProcessError(QProcess::ProcessError error)
{
    //qDebug() << "onProcessError" << error;
    onProcessFinished(2, (error == QProcess::Crashed) ? QProcess::CrashExit : QProcess::NormalExit);
}

void CommandExecutor::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    //qDebug() << "onProcessFinished" << m_pTarget->m_targetName;

    if (exitStatus != QProcess::NormalExit)
        exitCode = 2;
    if (exitCode > m_pLastCommand->m_maxExitCode) {
        fprintf(stderr, "command failed with exit code %d\n", exitCode);
        emit finished(this, true);  // abort make process
        return;
    }

    if (!executeNextCommand()) {
        emit finished(this, false);
    }
}

void CommandExecutor::processReadyReadStandardError()
{
    fprintf(stderr, m_process.readAllStandardError().data());
    //printf("\n");
}

void CommandExecutor::processReadyReadStandardOutput()
{
    printf(m_process.readAllStandardOutput().data());
    //printf("\n");
}

bool CommandExecutor::executeNextCommand()
{
    if (m_nCommandIdx >= m_pTarget->m_commands.count()) {
        return false;
    }

    Command& cmd = m_pTarget->m_commands[m_nCommandIdx];
    m_pLastCommand = &cmd;

    if (cmd.m_commandLine.startsWith('@'))
        cmd.m_commandLine = cmd.m_commandLine.right(cmd.m_commandLine.length() - 1);

    bool spawnJOM = false;
    if (g_options.maxNumberOfJobs > 1) {
        int idx = cmd.m_commandLine.indexOf(g_options.fullAppPath);
        if (idx > -1) {
            spawnJOM = true;
            const int appPathLength = g_options.fullAppPath.length();
            QString arg = " -nologo -j " + QString().setNum(g_options.maxNumberOfJobs);
            if (g_options.incredibuildSupport)
                arg.prepend(" -incrediBuildSupport");

            // Check if the jom call is enclosed by double quotes.
            const int idxRight = idx + appPathLength;
            if (idx > 0 && cmd.m_commandLine.at(idx-1) == '"' &&
                idxRight < cmd.m_commandLine.length() && cmd.m_commandLine.at(idxRight) == '"')
            {
                idx++;  // to insert behind the double quote
            }

            cmd.m_commandLine.insert(idx + appPathLength, arg);
        }
    }

    if (!cmd.m_silent && !g_options.suppressExecutedCommandsDisplay) {
        printf(qPrintable(cmd.m_commandLine));
        printf("\n");
    }

    if (g_options.dryRun) {
        m_nCommandIdx++;
        onProcessFinished(0, QProcess::NormalExit);
        return true;
    }

    // transform quotes to a form that cmd.exe can understand
    cmd.m_commandLine.replace("\\\"", "\"\"\"");

    if (spawnJOM) {
        int ret = _wsystem(reinterpret_cast<const WCHAR*>(cmd.m_commandLine.utf16()));
        if (ret == -1)
            throw Exception("cannot spawn jom subprocess");
        m_nCommandIdx++;
        onProcessFinished(ret, QProcess::NormalExit);
    } else {
        if (g_options.incredibuildSupport)
            m_process.start("cmd /c " + cmd.m_commandLine);
        else
            m_process.start("cmd \"/c " + cmd.m_commandLine + "\"");
        m_nCommandIdx++;
    }

    return true;
}

void CommandExecutor::waitForFinished()
{
    m_process.waitForFinished();
}

void CommandExecutor::createTempFiles()
{
    QList<Command>::iterator it = m_pTarget->m_commands.begin();
    QList<Command>::iterator itEnd = m_pTarget->m_commands.end();
    for (; it != itEnd; ++it) {
        Command& cmd = *it;
        if (!cmd.m_inlineFile)
            continue;

        QString fileName;
        if (cmd.m_inlineFile->m_filename.isEmpty()) {
            do {
                QString simplifiedTargetName = fileNameFromFilePath(m_pTarget->m_targetName);
                fileName = m_tempPath + QString("%1.%2.jom").arg(simplifiedTargetName).arg(GetTickCount());
            } while (QFile::exists(fileName));
        } else
            fileName = cmd.m_inlineFile->m_filename;

        TempFile tempFile;
        tempFile.file = new QFile(fileName);
        if (!tempFile.file->open(QFile::WriteOnly)) {
            delete tempFile.file;
            throw Exception(QString("cannot open %1 for write").arg(tempFile.file->fileName()));
        }

        tempFile.keep = cmd.m_inlineFile->m_keep;

        QString content = cmd.m_inlineFile->m_content;
        if (content.contains("$<")) {
            // TODO: handle more file macros here
            content.replace("$<", m_pTarget->m_dependents.join(" "));
        }

        QTextStream textstream(tempFile.file);
        textstream << content;
        tempFile.file->close();

        cmd.m_commandLine.replace("<<",
            QString(tempFile.file->fileName()).replace('/', '\\'));

        m_tempFiles.append(tempFile);
    }
}

void CommandExecutor::cleanupTempFiles()
{
    while (!m_tempFiles.isEmpty()) {
        const TempFile& tempfile = m_tempFiles.takeLast();
        if (!tempfile.keep) tempfile.file->remove();
        delete tempfile.file;
    }
}

} // namespace NMakeFile
