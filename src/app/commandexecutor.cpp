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

#include <QTemporaryFile>
#include <QDebug>
#include <windows.h>

namespace NMakeFile {

ulong CommandExecutor::m_startUpTickCount = 0;
QByteArray CommandExecutor::m_globalCommandLines;
QString CommandExecutor::m_tempPath;

CommandExecutor::CommandExecutor(QObject* parent, const QStringList& environment)
:   QObject(parent),
    m_pTarget(0)
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
    connect(&m_process, SIGNAL(readyReadStandardError()), SLOT(processReadyReadStandardError()));
    connect(&m_process, SIGNAL(readyReadStandardOutput()), SLOT(processReadyReadStandardOutput()));
}

CommandExecutor::~CommandExecutor()
{
    if (!g_options.debugMode && !m_commandScript.fileName().isEmpty())
        m_commandScript.remove();
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
    openCommandScript();
    bool spawnJOM = false;
    fillCommandScript(spawnJOM);

    QString commandScriptFileName = m_commandScript.fileName().replace("/", "\\");
    if (g_options.debugMode)
        qDebug() << "--- executing batch file" << commandScriptFileName;

    if (spawnJOM) {
        QByteArray cmdLine = "cmd /C \"";
        cmdLine += commandScriptFileName;
        cmdLine += "\"";
        int exitCode = system(cmdLine);
        onProcessFinished(exitCode, QProcess::NormalExit);
    } else {
        m_process.start("cmd", QStringList() << "/C" << commandScriptFileName);
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
    if (exitCode != 0) {
        fprintf(stderr, "command failed with exit code %d\n", exitCode);
        bool abortMakeProcess = !g_options.buildUnrelatedTargetsOnError;
        emit finished(this, abortMakeProcess);
        return;
    }

    emit finished(this, false);
}

void CommandExecutor::processReadyReadStandardError()
{
    fputs(m_process.readAllStandardError().data(), stderr);
    fflush(stderr);
    //printf("\n");
}

void CommandExecutor::processReadyReadStandardOutput()
{
    fputs(m_process.readAllStandardOutput().data(), stdout);
    fflush(stdout);
    //printf("\n");
}

void CommandExecutor::waitForFinished()
{
    m_process.waitForFinished();
}

void CommandExecutor::openCommandScript()
{
    if (!m_commandScript.exists()) {
        // create an unique temp file
        QTemporaryFile* tempFile = new QTemporaryFile(this);
        tempFile->setFileTemplate(m_tempPath + "jomex");
        tempFile->open();
        QString fileName = tempFile->fileName() + ".cmd";
        m_commandScript.setFileName(fileName);
    }

    if (!m_commandScript.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        const QString msg = "Can't create command script file %1.";
        throw Exception(msg.arg(m_commandScript.fileName()));
    }

    Q_ASSERT(m_commandScript.isOpen());
    Q_ASSERT(m_commandScript.isWritable());
    m_commandScript.resize(0);
}

void CommandExecutor::fillCommandScript(bool& spawnJOM)
{
    Q_ASSERT(!m_pTarget->m_commands.isEmpty());

    spawnJOM = false;
    m_commandScript.write("@echo off\n");
    if (!m_globalCommandLines.isEmpty())
        m_commandScript.write(m_globalCommandLines);
    foreach (const Command& cmd, m_pTarget->m_commands)
        writeCommandToCommandScript(cmd, spawnJOM);
    m_commandScript.write("exit 0\n");
    m_commandScript.close();
}

void CommandExecutor::handleSetCommand(const QString& commandLine)
{
    if (commandLine.length() < 4)
        return;

    if (!commandLine.startsWith("set", Qt::CaseInsensitive))
        return;

    if (!commandLine.at(3).isSpace())
        return;

    m_globalCommandLines.append(commandLine);
    m_globalCommandLines.append("\n");
}

void CommandExecutor::writeCommandToCommandScript(const Command& cmd, bool& spawnJOM)
{
    QString commandLine = cmd.m_commandLine;
    if (g_options.maxNumberOfJobs > 1) {
        int idx = commandLine.indexOf(g_options.fullAppPath);
        if (idx > -1) {
            spawnJOM = true;
            const int appPathLength = g_options.fullAppPath.length();
            QString arg = " -nologo -j " + QString().setNum(g_options.maxNumberOfJobs);
            if (g_options.displayBuildInfo)
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

    handleSetCommand(commandLine);

    if (!cmd.m_silent && !g_options.suppressExecutedCommandsDisplay) {
        QByteArray echoLine = commandLine.toLocal8Bit();
        // we must quote all special characters properly
        echoLine.replace("^", "^^");
        echoLine.replace("&", "^&");
        echoLine.replace("|", "^|");
        echoLine.replace("<", "^<");
        echoLine.replace(">", "^>");
        m_commandScript.write("echo ");
        m_commandScript.write(echoLine);
        m_commandScript.write("\n");
    }

    if (g_options.dryRun)
        return;

    // write the actual command
    m_commandScript.write(commandLine.toLocal8Bit());
    m_commandScript.write("\n");

    // write the exit code check
    if (cmd.m_maxExitCode < 255) {
        m_commandScript.write("IF %ERRORLEVEL% GTR ");
        m_commandScript.write(QByteArray::number(cmd.m_maxExitCode));
        m_commandScript.write(" EXIT %ERRORLEVEL%\n");
        m_commandScript.write("IF %ERRORLEVEL% LSS 0 EXIT %ERRORLEVEL%\n");
    } else {
        m_commandScript.write("cmd /c exit /b 0\n");
    }
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

} // namespace NMakeFile
