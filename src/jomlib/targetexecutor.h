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

#ifndef TARGETEXECUTOR_H
#define TARGETEXECUTOR_H

#include "makefile.h"
#include <QEvent>
#include <QtCore/QMap>

QT_BEGIN_NAMESPACE
class QFile;
QT_END_NAMESPACE

namespace NMakeFile {

class CommandExecutor;
class DependencyGraph;
class JobClient;

class TargetExecutor : public QObject {
    Q_OBJECT
public:
    TargetExecutor(const ProcessEnvironment &environment);
    ~TargetExecutor();

    void apply(Makefile* mkfile, const QStringList& targets);
    void removeTempFiles();

signals:
    void finished(int exitCode);

private slots:
    void startProcesses();
    void buildNextTarget();
    void onChildFinished(CommandExecutor*, bool commandFailed);

private:
    int numberOfRunningProcesses() const;
    void waitForProcesses();
    void finishBuild(int exitCode);
    void findNextTarget();

private:
    ProcessEnvironment m_environment;
    Makefile* m_makefile;
    DependencyGraph* m_depgraph;
    QList<DescriptionBlock*> m_pendingTargets;
    JobClient *m_jobClient;
    bool m_bAborted;
    int m_jobAcquisitionCount;
    QList<CommandExecutor*> m_availableProcesses;
    QList<CommandExecutor*> m_processes;
    DescriptionBlock *m_nextTarget;
    bool m_allCommandsSuccessfullyExecuted;
};

} //namespace NMakeFile

#endif // TARGETEXECUTOR_H
