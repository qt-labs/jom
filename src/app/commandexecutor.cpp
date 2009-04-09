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
#include "commandexecutor.h"
#include "options.h"
#include "exception.h"

#include <QTemporaryFile>
#include <QTextStream>
#include <QDebug>
#include <windows.h>

namespace NMakeFile {

QString CommandExecutor::m_tempPath;

CommandExecutor::CommandExecutor(QObject* parent)
:   QObject(parent),
    m_pTarget(0),
    m_nCommandIdx(0),
    m_pLastCommand(0),
    m_pTempFile(0)
{
    if (m_tempPath.isNull()) {
        WCHAR buf[MAX_PATH];
        DWORD count = GetTempPathW(MAX_PATH, buf);
        if (count) {
            m_tempPath = QString::fromUtf16(buf, count);
            if (!m_tempPath.endsWith('\\')) m_tempPath.append('\\');
        }
    }

    connect(&m_process, SIGNAL(error(QProcess::ProcessError)), SLOT(onProcessError(QProcess::ProcessError)));
    connect(&m_process, SIGNAL(finished(int, QProcess::ExitStatus)), SLOT(onProcessFinished(int, QProcess::ExitStatus)));
    connect(&m_process, SIGNAL(readyReadStandardError()), SLOT(processReadyReadStandardError()));
    connect(&m_process, SIGNAL(readyReadStandardOutput()), SLOT(processReadyReadStandardOutput()));
}

CommandExecutor::~CommandExecutor()
{
}

void CommandExecutor::start(DescriptionBlock* target)
{
    m_nCommandIdx = 0;
    m_pLastCommand = 0;
    m_pTarget = target;
    target->expandFileNameMacros();
    emit started(this);
    if (!executeNextCommand()) {
        emit finished(this, 0);
    }
}

void CommandExecutor::onProcessError(QProcess::ProcessError error)
{
    qDebug() << "onProcessError" << error;
    // TODO: pass error upwards and stop build if not prevented by command line option
}

void CommandExecutor::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    //qDebug() << "onProcessFinished" << m_pTarget->m_target;

    if (m_pTempFile) {
        if (!m_bKeepTempFile) m_pTempFile->remove();
        delete m_pTempFile;
        m_pTempFile = 0;
    }

    if (exitStatus != QProcess::NormalExit)
        exitCode = 2;
    if (exitCode > m_pLastCommand->m_maxExitCode) {
        printf("command failed with exit code %d\n", exitCode);
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
    if (cmd.m_inlineFile) {
        QString fileName;
        if (cmd.m_inlineFile->m_filename.isEmpty())
            fileName = m_tempPath + QString("nmp%1.tmp").arg(GetTickCount());
        else
            fileName = cmd.m_inlineFile->m_filename;

        m_pTempFile = new QFile(fileName);
        if (!m_pTempFile->open(QFile::WriteOnly)) {
            delete m_pTempFile;
            throw Exception(QString("cannot open %1 for write").arg(m_pTempFile->fileName()));
        }

        m_bKeepTempFile = cmd.m_inlineFile->m_keep;

        QString content = cmd.m_inlineFile->m_content;
        if (content.contains("$<")) {
            // TODO: handle more file macros here
            content.replace("$<", m_pTarget->m_dependents.join(" "));
        }

        QTextStream textstream(m_pTempFile);
        textstream << content;
        m_pTempFile->close();

        cmd.m_commandLine.replace("<<",
            QString(m_pTempFile->fileName()).replace('/', '\\'));
    }

    if (cmd.m_commandLine.startsWith('@'))
        cmd.m_commandLine = cmd.m_commandLine.right(cmd.m_commandLine.length() - 1);

    bool spawnNMP = false;
    if (g_options.maxNumberOfJobs > 1) {
        int idx = cmd.m_commandLine.indexOf(g_options.nmpFullPath);
        if (idx > -1) {
            spawnNMP = true;
            const QString arg = " -nologo -j " + QString().setNum(g_options.maxNumberOfJobs);
            cmd.m_commandLine.insert(idx + g_options.nmpFullPath.length(), arg);
        }
    }

    if (!cmd.m_silent) {
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

    if (spawnNMP) {
        if (cmd.m_commandLine.startsWith(g_options.nmpFullPath))
            m_process.start(cmd.m_commandLine);
        else
            m_process.start("cmd /c " + cmd.m_commandLine);
        m_nCommandIdx++;
        m_process.waitForFinished(-1);
    } else {
        m_process.start("cmd /c " + cmd.m_commandLine);
        m_nCommandIdx++;
    }

    return true;
}

void CommandExecutor::waitForFinished()
{
    m_process.waitForFinished();
}

} // namespace NMakeFile
