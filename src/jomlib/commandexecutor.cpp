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
#include "commandexecutor.h"
#include "options.h"
#include "exception.h"
#include "helperfunctions.h"
#include "fileinfo.h"

#include <QDir>
#include <QTemporaryFile>
#include <QDebug>
#include <windows.h>

namespace NMakeFile {

ulong CommandExecutor::m_startUpTickCount = 0;
QString CommandExecutor::m_tempPath;

CommandExecutor::CommandExecutor(QObject* parent, const QStringList& environment)
:   QObject(parent),
    m_pTarget(0),
    m_ignoreProcessErrors(false)
{
    if (m_startUpTickCount == 0)
        m_startUpTickCount = GetTickCount();

    if (m_tempPath.isNull()) {
        WCHAR buf[MAX_PATH];
        DWORD count = GetTempPathW(MAX_PATH, buf);
        if (count) {
            m_tempPath = QString::fromWCharArray(buf, count);
            if (!m_tempPath.endsWith('\\')) m_tempPath.append('\\');
        }
    }

    m_process.setEnvironment(environment);
    connect(&m_process, SIGNAL(error(QProcess::ProcessError)), SLOT(onProcessError(QProcess::ProcessError)));
    connect(&m_process, SIGNAL(finished(int, QProcess::ExitStatus)), SLOT(onProcessFinished(int, QProcess::ExitStatus)));
    connect(&m_process, SIGNAL(readyReadStandardError()), SLOT(onProcessReadyReadStandardError()));
    connect(&m_process, SIGNAL(readyReadStandardOutput()), SLOT(onProcessReadyReadStandardOutput()));
}

CommandExecutor::~CommandExecutor()
{
    cleanupTempFiles();
}

void CommandExecutor::start(DescriptionBlock* target)
{
    m_pTarget = target;

    if (target->m_commands.isEmpty()) {
        emit finished(this, false);
        return;
    }

    target->expandFileNameMacros();
    cleanupTempFiles();
    createTempFiles();

    m_ignoreProcessErrors = false;
    m_currentCommandIdx = 0;
    m_nextWorkingDir.clear();
    m_process.setWorkingDirectory(m_nextWorkingDir);
    executeCurrentCommandLine();
}

void CommandExecutor::onProcessError(QProcess::ProcessError error)
{
    //qDebug() << "onProcessError" << error;
    if (!m_ignoreProcessErrors)
        onProcessFinished(2, (error == QProcess::Crashed) ? QProcess::CrashExit : QProcess::NormalExit);
}

void CommandExecutor::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    //qDebug() << "onProcessFinished" << m_pTarget->m_targetName;

    if (exitStatus != QProcess::NormalExit)
        exitCode = 2;
    if (exitCode != 0) {
        QByteArray msg = "command failed with exit code ";
        msg += QByteArray::number(exitCode);
        msg += "\n";
        writeToStandardError(msg);
        bool abortMakeProcess = !m_pTarget->makefile()->options()->buildUnrelatedTargetsOnError;
        emit finished(this, abortMakeProcess);
        return;
    }

    ++m_currentCommandIdx;
    if (m_currentCommandIdx < m_pTarget->m_commands.count()) {
        executeCurrentCommandLine();
    } else {
        emit finished(this, false);
    }
}

void CommandExecutor::onProcessReadyReadStandardError()
{
    writeToStandardError(m_process.readAllStandardError());
}

void CommandExecutor::onProcessReadyReadStandardOutput()
{
    writeToStandardOutput(m_process.readAllStandardOutput());
}

void CommandExecutor::waitForFinished()
{
    m_process.waitForFinished();
}

void CommandExecutor::executeCurrentCommandLine()
{
    const Command& cmd = m_pTarget->m_commands.at(m_currentCommandIdx);
    QString commandLine = cmd.m_commandLine;
    bool spawnJOM = false;
    if (g_options.maxNumberOfJobs > 1) {
        int idx = commandLine.indexOf(m_pTarget->makefile()->options()->fullAppPath);
        if (idx > -1) {
            spawnJOM = true;
            const int appPathLength = m_pTarget->makefile()->options()->fullAppPath.length();
            QString arg = " -nologo -j " + QString().setNum(g_options.maxNumberOfJobs);
            if (m_pTarget->makefile()->options()->displayBuildInfo)
                arg += " /D";

            // Check if the jom call is enclosed by double quotes.
            const int idxRight = idx + appPathLength;
            if (idx > 0 && commandLine.at(idx-1) == '"' &&
                idxRight < commandLine.length() && commandLine.at(idxRight) == '"')
            {
                idx++;  // to insert behind the double quote
            }

            commandLine.insert(idx + appPathLength, arg);
        }
    }

    if (!cmd.m_silent && !m_pTarget->makefile()->options()->suppressExecutedCommandsDisplay) {
        QByteArray output = commandLine.toLocal8Bit();
        output.append('\n');
        writeToStandardOutput(output);
    }

    if (m_pTarget->makefile()->options()->dryRun)
        return;

    if (!m_nextWorkingDir.isEmpty()) {
        m_process.setWorkingDirectory(m_nextWorkingDir);
        m_nextWorkingDir.clear();
    }

    m_cmdLexer.setInput(commandLine);
    QList<CmdLexer::Token> cmdLineTokens = m_cmdLexer.lexTokens();
    const bool simpleCmdLine = isSimpleCommandLine(cmdLineTokens);
    CmdLexer::Token program = cmdLineTokens.takeFirst();

    if (program.type == CmdLexer::TArgument
        && program.value.compare(QLatin1String("cd"), Qt::CaseInsensitive) == 0)
    {
        bool success = exec_cd(cmdLineTokens, simpleCmdLine);
        if (simpleCmdLine) {
            onProcessFinished(success ? 0 : 1, QProcess::NormalExit);
            return;
        }
    }

    bool executionSucceeded = false;
    if (simpleCmdLine) {
        //qDebug("+++ direct exec");
        QStringList args;
        foreach (const CmdLexer::Token& t, cmdLineTokens)
            args.append(t.value);
        m_ignoreProcessErrors = true;
        m_process.start(program.value, args);
        executionSucceeded = m_process.waitForStarted();
        m_ignoreProcessErrors = false;
    }

    if (!executionSucceeded) {
        //qDebug("+++ shell exec");
        m_process.start(QLatin1String("cmd \"/C ") + commandLine + QLatin1String("\""));
        executionSucceeded = m_process.waitForStarted();
    }

    if (!executionSucceeded)
        qFatal("Can't start command.");
    if (spawnJOM)
        m_process.waitForFinished();
}

void CommandExecutor::createTempFiles()
{
    QList<Command>::iterator it = m_pTarget->m_commands.begin();
    QList<Command>::iterator itEnd = m_pTarget->m_commands.end();
    for (; it != itEnd; ++it) {
        Command& cmd = *it;
        foreach (InlineFile* inlineFile, cmd.m_inlineFiles) {
            QString fileName;
            if (inlineFile->m_filename.isEmpty()) {
                do {
                    QString simplifiedTargetName = m_pTarget->targetFilePath();
                    simplifiedTargetName = fileNameFromFilePath(simplifiedTargetName);
                    fileName = m_tempPath + QString("%1.%2.%3.jom").arg(simplifiedTargetName)
                                                                   .arg(GetCurrentProcessId())
                                                                   .arg(GetTickCount() - m_startUpTickCount);
                } while (QFile::exists(fileName));
            } else
                fileName = inlineFile->m_filename;

            TempFile tempFile;
            tempFile.keep = inlineFile->m_keep;
            tempFile.file = new QFile(fileName);
            if (!tempFile.file->open(QFile::WriteOnly)) {
                delete tempFile.file;
                throw Exception(QString("cannot open %1 for write").arg(fileName));
            }

            // TODO: do something with inlineFile->m_unicode;
            tempFile.file->write(inlineFile->m_content.toLocal8Bit());
            tempFile.file->close();

            QString replacement = QString(tempFile.file->fileName()).replace('/', '\\');
            if (replacement.contains(' ') || replacement.contains('\t')) {
                replacement.prepend("\"");
                replacement.append("\"");
            }

            int idx = cmd.m_commandLine.indexOf(QLatin1String("<<"));
            if (idx > 0)
                cmd.m_commandLine.replace(idx, 2, replacement);

            m_tempFiles.append(tempFile);
        }
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

void CommandExecutor::writeToStandardOutput(const QByteArray& output)
{
    printf(output.data());
}

void CommandExecutor::writeToStandardError(const QByteArray& output)
{
    fprintf(stderr, output.data());
}

bool CommandExecutor::isSimpleCommandLine(const QList<CmdLexer::Token>& tokens)
{
    foreach (const CmdLexer::Token& t, tokens)
        if (t.type != CmdLexer::TArgument)
            return false;
    return true;
}

bool CommandExecutor::findExecutableInPath(QString& fileName) // ### needed?
{
    FileInfo fi(fileName);
    if (fi.isAbsolute() && fi.exists())
        return true;

    return false;
}

bool CommandExecutor::exec_cd(const QList<CmdLexer::Token>& args, bool isSimpleCmdLine)
{
    QList<CmdLexer::Token>::const_iterator it = args.begin();
    if (it == args.end() || it->type != CmdLexer::TArgument) {
        // cd, called without arguments, prints the current working directory.
        if (isSimpleCmdLine)
            writeToStandardOutput(QDir::toNativeSeparators(QDir::currentPath()).toLocal8Bit());
        return true;
    }

    bool changeDrive = false;
    if (it->value.compare(QLatin1String("/d"), Qt::CaseInsensitive) == 0) {
        changeDrive = true;
        ++it;
        if (it == args.end() || it->type != CmdLexer::TArgument) {
            // "cd /d" does nothing.
            return true;
        }
    }

    if (!FileInfo(it->value).exists()) {
        QString msg = QLatin1String("Couldn't change working directory to %0.");
        writeToStandardError(msg.arg(it->value).toLocal8Bit());
        return false;
    }

    if (!changeDrive) {
        // ### handle the case where we cd into a dir on a drive != current drive 
    }

    m_nextWorkingDir = it->value;
    return true;
}

} // namespace NMakeFile
