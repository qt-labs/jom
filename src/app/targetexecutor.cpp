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
#include "targetexecutor.h"
#include "commandexecutor.h"
#include "dependencygraph.h"
#include "options.h"
#include "exception.h"

#include <QDebug>
#include <QTemporaryFile>
#include <QTextStream>
#include <QCoreApplication>

namespace NMakeFile {

TargetExecutor::TargetExecutor(const QStringList& environment)
:   m_bAborted(false)
{
    m_makefile = 0;
    m_depgraph = new DependencyGraph();

    for (int i=0; i < g_options.maxNumberOfJobs; ++i) {
        CommandExecutor* process = new CommandExecutor(this, environment);
        connect(process, SIGNAL(finished(CommandExecutor*, bool)), this, SLOT(onChildFinished(CommandExecutor*, bool)));
        m_availableProcesses.append(process);
    }
}

TargetExecutor::~TargetExecutor()
{
    delete m_depgraph;
}

void TargetExecutor::apply(Makefile* mkfile, const QStringList& targets)
{
    m_bAborted = false;
    m_makefile = mkfile;

    DescriptionBlock* descblock;
    if (targets.isEmpty()) {
        if (mkfile->targets().isEmpty())
            throw Exception("no targets in makefile");

        descblock = mkfile->firstTarget();
    } else {
        descblock = mkfile->target(targets.first());
        for (int i=1; i < targets.count(); ++i) {
            m_pendingTargets.append( mkfile->target(targets.at(i)) );
        }
    }

    mkfile->updateTimeStamps(descblock);
    m_depgraph->build(mkfile, descblock);
    if (g_options.dumpDependencyGraph) {
        if (g_options.dumpDependencyGraphDot)
            m_depgraph->dotDump();
        else
            m_depgraph->dump();
        QCoreApplication::exit();
    }
}

bool TargetExecutor::event(QEvent* e)
{
    if (e->type() == QEvent::User) {
        startProcesses();
        return true;
    }
    return QObject::event(e);
}

void TargetExecutor::startProcesses()
{
    if (m_bAborted)
        return;

    DescriptionBlock* nextTarget = 0;
    while (!m_availableProcesses.isEmpty() && (nextTarget = m_depgraph->findAvailableTarget())) {
        if (nextTarget->m_commands.isEmpty()) {
            // Short cut for targets without commands.
            // We're not really interested in these.
            m_depgraph->remove(nextTarget);
            continue;
        }

        CommandExecutor* process = m_availableProcesses.takeFirst();
        process->start(nextTarget);
        if (m_bAborted)
            return;
    }

    if (m_availableProcesses.count() == g_options.maxNumberOfJobs) {
        if (m_pendingTargets.isEmpty()) {
            QCoreApplication::exit();
        } else {
            m_depgraph->clear();
            nextTarget = m_pendingTargets.takeFirst();
            m_makefile->invalidateTimeStamps();
            m_makefile->updateTimeStamps(nextTarget);
            m_depgraph->build(m_makefile, nextTarget);
            qApp->postEvent(this, new StartEvent);
        }
    }
}

void TargetExecutor::waitForProcesses()
{
    foreach (QObject* child, children()) {
        CommandExecutor* process = qobject_cast<CommandExecutor*>(child);
        if (process)
            process->waitForFinished();
    }
}

void TargetExecutor::onChildFinished(CommandExecutor* executor, bool abortMakeProcess)
{
    Q_CHECK_PTR(executor->target());
    m_depgraph->remove(executor->target());
    m_availableProcesses.append(executor);

    if (abortMakeProcess) {
        m_bAborted = true;
        m_depgraph->clear();
        m_pendingTargets.clear();
        waitForProcesses();
        QCoreApplication::exit(2);
    }

    qApp->postEvent(this, new StartEvent);
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
