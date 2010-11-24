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

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QRegExp>
#include <QtCore/QTemporaryFile>
#include <windows.h>

namespace NMakeFile {

ulong CommandExecutor::m_startUpTickCount = 0;
QString CommandExecutor::m_tempPath;

CommandExecutor::CommandExecutor(QObject* parent, const QStringList& environment)
:   QObject(parent),
    m_pTarget(0),
    m_ignoreProcessErrors(false),
    m_active(false),
    m_outputMode(BufferingOutput)
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
    m_active = true;

    if (target->m_commands.isEmpty()) {
        finishExecution(false);
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

    const Command &currentCommand = m_pTarget->m_commands.at(m_currentCommandIdx);
    if (exitCode > currentCommand.m_maxExitCode) {
        QByteArray msg = "command failed with exit code ";
        msg += QByteArray::number(exitCode);
        msg += "\n";
        writeToStandardError(msg);
        bool abortMakeProcess = !m_pTarget->makefile()->options()->buildUnrelatedTargetsOnError;
        finishExecution(abortMakeProcess);
        return;
    }

    ++m_currentCommandIdx;
    if (m_currentCommandIdx < m_pTarget->m_commands.count()) {
        executeCurrentCommandLine();
    } else {
        finishExecution(false);
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

void CommandExecutor::finishExecution(bool abortMakeProcess)
{
    m_active = false;
    emit finished(this, abortMakeProcess);
}

void CommandExecutor::waitForFinished()
{
    m_process.waitForFinished();
    setOutputMode(DirectOutput);
}

inline bool commandLineStartsWithCommand(const QString &str, const QString &searchString)
{
    return str.length() > searchString.length()
        && str.at(searchString.length()).isSpace()
        && str.startsWith(searchString, Qt::CaseInsensitive);
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

    // Unescape commandline characters.
    commandLine.replace("%%", "%");

    if (!cmd.m_silent && !m_pTarget->makefile()->options()->suppressExecutedCommandsDisplay) {
        QByteArray output = commandLine.toLocal8Bit();
        output.prepend('\t');
        output.append('\n');
        writeToStandardOutput(output);
    }

    if (m_pTarget->makefile()->options()->dryRun) {
        onProcessFinished(0, QProcess::NormalExit);
        return;
    }

    if (!m_nextWorkingDir.isEmpty()) {
        m_process.setWorkingDirectory(m_nextWorkingDir);
        m_nextWorkingDir.clear();
    }

    const bool simpleCmdLine = isSimpleCommandLine(commandLine);
    if (simpleCmdLine)
    {
        // handle builtins
        if (commandLineStartsWithCommand(commandLine, QLatin1String("cd"))) {
            bool success = exec_cd(commandLine);
            onProcessFinished(success ? 0 : 1, QProcess::NormalExit);
            return;
        } else if (commandLineStartsWithCommand(commandLine, QLatin1String("set"))) {
            QString variableAssignment = commandLine;
            variableAssignment = variableAssignment.remove(0, 4).trimmed();
            int idx = variableAssignment.indexOf(QLatin1Char('='));
            if (idx >= 0) {
                QString variableName = variableAssignment;
                variableName.truncate(idx);
                QStringList environment = m_process.environment();
                bool found = false;
                for (int i=0; i < environment.count(); ++i) {
                    if (environment.at(i).compare(variableName, Qt::CaseInsensitive) == 0) {
                        environment.replace(i, variableAssignment);
                        found = true;
                        break;
                    }
                }
                if (!found)
                    environment.append(variableAssignment);
                setEnvironment(environment);
                emit environmentChanged(environment);
            }
        }
    }

    bool executionSucceeded = false;
    if (simpleCmdLine) {
        //qDebug("+++ direct exec");
        m_ignoreProcessErrors = true;
#if QT_VERSION >= QT_VERSION_CHECK(4,7,0)
        m_process.setNativeArguments(commandLine);
        m_process.start(QString());
#else
        m_process.start(commandLine);
#endif
        executionSucceeded = m_process.waitForStarted();
        m_ignoreProcessErrors = false;
#if QT_VERSION >= QT_VERSION_CHECK(4,7,0)
        m_process.setNativeArguments(QString());
#endif
    }

    if (!executionSucceeded) {
        //qDebug("+++ shell exec");

        int doubleQuoteCount(0), idx(0);
        const QChar doubleQuote = QLatin1Char('"');
        while (doubleQuoteCount < 3 && (commandLine.indexOf(doubleQuote, idx) >= 0))
            ++doubleQuoteCount;

        if (doubleQuoteCount >= 3) {
            // There are more than three double quotes in the command. We must properly escape it.
            commandLine.prepend(QLatin1String("\" "));
            commandLine.append(QLatin1String(" \""));
        }

#if QT_VERSION >= QT_VERSION_CHECK(4,7,0)
        m_process.setNativeArguments(commandLine);
        m_process.start("cmd", QStringList() << "/c");
#else
        m_process.start(QLatin1String("cmd /C ") + commandLine);
#endif
        executionSucceeded = m_process.waitForStarted();
#if QT_VERSION >= QT_VERSION_CHECK(4,7,0)
        m_process.setNativeArguments(QString());
#endif
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

void CommandExecutor::writeToChannel(const QByteArray& data, FILE *channel)
{
    if (m_outputMode == DirectOutput) {
        fputs(data, channel);
        fflush(channel);
    } else {
        OutputChunk chunk;
        chunk.data = data;
        chunk.channel = channel;
        m_outputBuffer.append(chunk);
    }
}

void CommandExecutor::writeToStandardOutput(const QByteArray& output)
{
    writeToChannel(output, stdout);
}

void CommandExecutor::writeToStandardError(const QByteArray& output)
{
    writeToChannel(output, stderr);
}

bool CommandExecutor::isSimpleCommandLine(const QString &commandLine)
{
    static QRegExp rex("\\||>|<|&");
    return rex.indexIn(commandLine) == -1;
}

bool CommandExecutor::exec_cd(const QString &commandLine)
{
    QString args = commandLine.right(commandLine.count() - 3);    // cut of "cd "
    args = args.trimmed();

    if (args.isEmpty()) {
        // cd, called without arguments, prints the current working directory.
        QString cwd = m_process.workingDirectory();
        if (cwd.isNull())
            cwd = QDir::currentPath();
        writeToStandardOutput(QDir::toNativeSeparators(cwd).toLocal8Bit());
        return true;
    }

    bool changeDrive = false;
    if (args.startsWith(QLatin1String("/d"), Qt::CaseInsensitive)) {
        changeDrive = true;
        args.remove(0, 3);
        args = args.trimmed();
        if (args.isEmpty()) {
            // "cd /d" does nothing.
            return true;
        }
    }

    FileInfo fi(args);
    if (!fi.exists()) {
        QString msg = QLatin1String("Couldn't change working directory to %0.");
        writeToStandardError(msg.arg(args).toLocal8Bit());
        return false;
    }

    if (!changeDrive) {
        // ### handle the case where we cd into a dir on a drive != current drive 
    }

    m_nextWorkingDir = fi.absoluteFilePath();
    return true;
}

void CommandExecutor::setOutputMode(CommandExecutor::OutputMode mode)
{
    switch (mode) {
    case DirectOutput:
        flushOutput();
        break;
    }

    m_outputMode = mode;
}

void CommandExecutor::flushOutput()
{
    if (!m_outputBuffer.isEmpty()) {
        foreach (const OutputChunk &chunk, m_outputBuffer) {
            fputs(chunk.data.data(), chunk.channel);
            fflush(chunk.channel);
        }
        m_outputBuffer.clear();
    }
}

void CommandExecutor::setEnvironment(const QStringList &environment)
{
    m_process.setEnvironment(environment);
}

} // namespace NMakeFile
