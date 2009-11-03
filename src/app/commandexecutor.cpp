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

#include <QTextStream>
#include <QTemporaryFile>
#include <QDebug>
#include <windows.h>

namespace NMakeFile {

QString CommandExecutor::m_tempPath;
static const bool g_bKeepCommandScriptFiles = false;

CommandExecutor::CommandExecutor(QObject* parent, const QStringList& environment)
:   QObject(parent),
    m_pTarget(0)
{
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
    if (!g_bKeepCommandScriptFiles && !m_commandScript.fileName().isEmpty())
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
    //qDebug() << "--- executing batch file" << commandScriptFileName;
    m_process.start("cmd", QStringList() << "/C" << commandScriptFileName);

    if (spawnJOM)
        m_process.waitForFinished(-1);
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
    if (exitCode > 0) {
        fprintf(stderr, "command failed with exit code %d\n", exitCode);
        emit finished(this, true);  // abort make process
        return;
    }

    emit finished(this, false);
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
    foreach (const Command& cmd, m_pTarget->m_commands)
        writeCommandToCommandScript(cmd, spawnJOM);
    m_commandScript.write("exit 0\n");
    m_commandScript.close();
}

void CommandExecutor::writeCommandToCommandScript(const Command& cmd, bool& spawnJOM)
{
    QString commandLine = cmd.m_commandLine;
    if (commandLine.startsWith('@'))
        commandLine = commandLine.right(commandLine.length() - 1);

    if (g_options.maxNumberOfJobs > 1) {
        int idx = commandLine.indexOf(g_options.fullAppPath);
        if (idx > -1) {
            spawnJOM = true;
            const int appPathLength = g_options.fullAppPath.length();
            QString arg = " -nologo -j " + QString().setNum(g_options.maxNumberOfJobs);

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
    }
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
