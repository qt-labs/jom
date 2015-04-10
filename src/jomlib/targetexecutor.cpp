/****************************************************************************
 **
 ** Copyright (C) 2015 The Qt Company Ltd.
 ** Contact: http://www.qt.io/licensing/
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
 ** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 ** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 **
 ****************************************************************************/

#include "targetexecutor.h"
#include "commandexecutor.h"
#include "dependencygraph.h"
#include "jobclient.h"
#include "options.h"
#include "exception.h"

#include <QDebug>
#include <QTextStream>
#include <QCoreApplication>

namespace NMakeFile {

TargetExecutor::TargetExecutor(const ProcessEnvironment &environment)
    : m_environment(environment)
    , m_jobClient(0)
    , m_bAborted(false)
    , m_allCommandsSuccessfullyExecuted(true)
{
    m_makefile = 0;
    m_depgraph = new DependencyGraph();

    for (int i = 0; i < g_options.maxNumberOfJobs; ++i) {
        CommandExecutor* executor = new CommandExecutor(this, environment);
        connect(executor, SIGNAL(finished(CommandExecutor*, bool)),
                this, SLOT(onChildFinished(CommandExecutor*, bool)));

        foreach (CommandExecutor *other, m_processes) {
            connect(executor, SIGNAL(environmentChanged(const ProcessEnvironment &)),
                    other, SLOT(setEnvironment(const ProcessEnvironment &)));
            connect(other, SIGNAL(environmentChanged(const ProcessEnvironment &)),
                    executor, SLOT(setEnvironment(const ProcessEnvironment &)));
        }
        m_processes.append(executor);
    }
    m_availableProcesses = m_processes;
    m_availableProcesses.first()->setBufferedOutput(false);
}

TargetExecutor::~TargetExecutor()
{
    delete m_depgraph;
}

void TargetExecutor::apply(Makefile* mkfile, const QStringList& targets)
{
    m_bAborted = false;
    m_allCommandsSuccessfullyExecuted = true;
    m_makefile = mkfile;
    m_jobAcquisitionCount = 0;
    m_nextTarget = 0;

    if (!m_jobClient) {
        m_jobClient = new JobClient(&m_environment, this);
        if (!m_jobClient->start()) {
            const QString msg = QLatin1String("Can't connect to job server: %1");
            throw Exception(msg.arg(m_jobClient->errorString()));
        }
        connect(m_jobClient, &JobClient::acquired, this, &TargetExecutor::buildNextTarget);
    }

    DescriptionBlock* descblock;
    if (targets.isEmpty()) {
        if (mkfile->targets().isEmpty())
            throw Exception(QLatin1String("no targets in makefile"));

        descblock = mkfile->firstTarget();
    } else {
        const QString targetName = targets.first();
        descblock = mkfile->target(targetName);
        if (!descblock) {
            QString msg = QLatin1String("Target %1 does not exist in %2.");
            throw Exception(msg.arg(targetName, mkfile->fileName()));
        }
        for (int i=1; i < targets.count(); ++i) {
            m_pendingTargets.append( mkfile->target(targets.at(i)) );
        }
    }

    m_depgraph->build(descblock);
    if (m_makefile->options()->dumpDependencyGraph) {
        if (m_makefile->options()->dumpDependencyGraphDot)
            m_depgraph->dotDump();
        else
            m_depgraph->dump();
        finishBuild(0);
        return;
    }

    QMetaObject::invokeMethod(this, "startProcesses", Qt::QueuedConnection);
}

void TargetExecutor::startProcesses()
{
    if (m_bAborted || m_jobClient->isAcquiring() || m_availableProcesses.isEmpty())
        return;

    try {
        if (!m_nextTarget)
            findNextTarget();

        if (m_nextTarget) {
            if (numberOfRunningProcesses() == 0) {
                // Use up the internal job token.
                buildNextTarget();
            } else {
                // Acquire a job token from the server. Will call buildNextTarget() when done.
                m_jobAcquisitionCount++;
                m_jobClient->asyncAcquire();
            }
        } else {
            if (numberOfRunningProcesses() == 0) {
                if (m_pendingTargets.isEmpty()) {
                    finishBuild(0);
                } else {
                    m_depgraph->clear();
                    m_makefile->invalidateTimeStamps();
                    m_depgraph->build(m_pendingTargets.takeFirst());
                    QMetaObject::invokeMethod(this, "startProcesses", Qt::QueuedConnection);
                }
            }
        }
    } catch (Exception &e) {
        m_bAborted = true;
        fprintf(stderr, "Error: %s\n", qPrintable(e.message()));
        finishBuild(1);
    }
}

void TargetExecutor::buildNextTarget()
{
    Q_ASSERT(m_nextTarget);

    if (m_bAborted)
        return;

    try {
        CommandExecutor *executor = m_availableProcesses.takeFirst();
        executor->start(m_nextTarget);
        m_nextTarget = 0;
        QMetaObject::invokeMethod(this, "startProcesses", Qt::QueuedConnection);
    } catch (const Exception &e) {
        m_bAborted = true;
        fprintf(stderr, "Error: %s\n", qPrintable(e.message()));
        finishBuild(1);
    }
}

void TargetExecutor::waitForProcesses()
{
    foreach (CommandExecutor* process, m_processes)
        process->waitForFinished();
}

void TargetExecutor::finishBuild(int exitCode)
{
    if (exitCode == 0
        && !m_allCommandsSuccessfullyExecuted
        && m_makefile->options()->buildUnrelatedTargetsOnError)
    {
        // /k specified and some command failed
        exitCode = 1;
    }
    emit finished(exitCode);
}

void TargetExecutor::findNextTarget()
{
    forever {
        m_nextTarget = m_depgraph->findAvailableTarget();
        if (m_nextTarget && m_nextTarget->m_commands.isEmpty()) {
            // Short cut for targets without commands.
            m_depgraph->removeLeaf(m_nextTarget);
        } else {
            return;
        }
    }
}

void TargetExecutor::onChildFinished(CommandExecutor* executor, bool commandFailed)
{
    Q_CHECK_PTR(executor->target());
    FastFileInfo::clearCacheForFile(executor->target()->targetName());
    m_depgraph->removeLeaf(executor->target());
    if (m_jobAcquisitionCount > 0) {
        m_jobClient->release();
        m_jobAcquisitionCount--;
    }
    m_availableProcesses.append(executor);
    if (!executor->isBufferedOutputSet()) {
        executor->setBufferedOutput(true);
        bool found = false;
        foreach (CommandExecutor *cmdex, m_processes) {
            if (cmdex->isActive()) {
                cmdex->setBufferedOutput(false);
                found = true;
            }
        }
        if (!found)
            m_availableProcesses.first()->setBufferedOutput(false);
    }

    if (commandFailed)
        m_allCommandsSuccessfullyExecuted = false;

    bool abortMakeProcess = commandFailed && !m_makefile->options()->buildUnrelatedTargetsOnError;
    if (abortMakeProcess) {
        m_bAborted = true;
        m_depgraph->clear();
        m_pendingTargets.clear();
        waitForProcesses();
        finishBuild(2);
    }

    QMetaObject::invokeMethod(this, "startProcesses", Qt::QueuedConnection);
}

int TargetExecutor::numberOfRunningProcesses() const
{
    return m_processes.count() - m_availableProcesses.count();
}

void TargetExecutor::removeTempFiles()
{
    foreach (QObject* child, children()) {
        CommandExecutor* cmdex = qobject_cast<CommandExecutor*>(child);
        if (!cmdex)
            continue;

        cmdex->cleanupTempFiles();
    }
}

} //namespace NMakeFile
