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
#include "targetexecutor.h"
#include "commandexecutor.h"
#include "consolecolorrestorer.h"
#include "dependencygraph.h"
#include "options.h"
#include "exception.h"

#include <QDebug>
#include <QTextStream>
#include <QCoreApplication>

namespace NMakeFile {

TargetExecutor::TargetExecutor(const QStringList& environment)
:   m_consoleColorRestorer(new ConsoleColorRestorer),
    m_bAborted(false),
    m_blockingCommand(0)
{
    m_makefile = 0;
    m_depgraph = new DependencyGraph();

    for (int i=0; i < g_options.maxNumberOfJobs; ++i) {
        CommandExecutor* process = new CommandExecutor(this, environment);
        connect(process, SIGNAL(finished(CommandExecutor*, bool)), this, SLOT(onChildFinished(CommandExecutor*, bool)));
        connect(process, SIGNAL(subJomStarted()), this, SLOT(onSubJomStarted()));
        m_availableProcesses.append(process);
        m_processes.append(process);
    }

    foreach (CommandExecutor *process, m_processes)
        foreach (CommandExecutor *otherProcess, m_processes)
            if (process != otherProcess)
                connect(process, SIGNAL(environmentChanged(const QStringList&)),
                        otherProcess, SLOT(setEnvironment(const QStringList&)));
}

TargetExecutor::~TargetExecutor()
{
    delete m_depgraph;
    delete m_consoleColorRestorer;
    m_consoleColorRestorer = 0;
}

bool TargetExecutor::hasPendingTargets() const
{
    return !m_pendingTargets.isEmpty() || m_availableProcesses.count() < m_processes.count();
}

void TargetExecutor::apply(Makefile* mkfile, const QStringList& targets)
{
    m_bAborted = false;
    m_blockingCommand = 0;
    m_makefile = mkfile;

    DescriptionBlock* descblock;
    if (targets.isEmpty()) {
        if (mkfile->targets().isEmpty())
            throw Exception(QLatin1String("no targets in makefile"));

        descblock = mkfile->firstTarget();
    } else {
        descblock = mkfile->target(targets.first());
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
        QCoreApplication::exit();
    }
}

void TargetExecutor::startProcesses()
{
    if (m_bAborted || m_blockingCommand)
        return;

    try {
        DescriptionBlock* nextTarget = 0;
        while (!m_availableProcesses.isEmpty() && (nextTarget = m_depgraph->findAvailableTarget())) {
            if (nextTarget->m_commands.isEmpty()) {
                // Short cut for targets without commands.
                // We're not really interested in these.
                m_depgraph->removeLeaf(nextTarget);
                continue;
            }

            CommandExecutor* process = m_availableProcesses.takeFirst();
            process->start(nextTarget);
            if (m_bAborted || m_blockingCommand)
                return;
        }

        if (m_availableProcesses.count() == g_options.maxNumberOfJobs) {
            if (m_pendingTargets.isEmpty()) {
                QCoreApplication::exit();
            } else {
                m_depgraph->clear();
                nextTarget = m_pendingTargets.takeFirst();
                m_makefile->invalidateTimeStamps();
                m_depgraph->build(nextTarget);
                QMetaObject::invokeMethod(this, "startProcesses", Qt::QueuedConnection);
            }
        }
    } catch (Exception &e) {
        m_bAborted = true;
        QString msg = QLatin1Literal("Error: ") + e.message() + QLatin1Char('\n');
        fprintf(stderr, qPrintable(msg));
        QCoreApplication::exit(1);
    }
}

void TargetExecutor::waitForProcesses()
{
    foreach (CommandExecutor* process, findChildren<CommandExecutor*>())
        process->waitForFinished();
}

void TargetExecutor::onSubJomStarted()
{
    //qDebug() << "BLOCK" << QCoreApplication::applicationPid();
    m_blockingCommand = sender();
    foreach (CommandExecutor *cmdex, m_processes)
        if (cmdex != sender())
            cmdex->block();
}

void TargetExecutor::onChildFinished(CommandExecutor* executor, bool abortMakeProcess)
{
    Q_CHECK_PTR(executor->target());
    m_depgraph->removeLeaf(executor->target());
    m_availableProcesses.append(executor);

    if (m_consoleColorRestorer)
        m_consoleColorRestorer->restore();

    if (m_blockingCommand && m_blockingCommand == executor) {
        //qDebug() << "UNBLOCK" << QCoreApplication::applicationPid();
        m_blockingCommand = 0;
        foreach (CommandExecutor *cmdex, m_processes)
            cmdex->unblock();
    }

    if (abortMakeProcess) {
        m_bAborted = true;
        m_depgraph->clear();
        m_pendingTargets.clear();
        waitForProcesses();
        QCoreApplication::exit(2);
    }

    QMetaObject::invokeMethod(this, "startProcesses", Qt::QueuedConnection);
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
