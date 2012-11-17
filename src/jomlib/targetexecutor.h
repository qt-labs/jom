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

class TargetExecutor : public QObject {
    Q_OBJECT
public:
    TargetExecutor(const ProcessEnvironment &environment);
    ~TargetExecutor();

    void apply(Makefile* mkfile, const QStringList& targets);
    void removeTempFiles();
    bool hasPendingTargets() const;

signals:
    void finished(int exitCode);

public slots:
   void startProcesses();

private slots:
    void onChildFinished(CommandExecutor*, bool commandFailed);

private:
    void waitForProcesses();
    void finishBuild(int exitCode);

private:
    Makefile* m_makefile;
    DependencyGraph* m_depgraph;
    QList<DescriptionBlock*> m_pendingTargets;
    bool m_bAborted;
    QList<CommandExecutor*> m_availableProcesses;
    QList<CommandExecutor*> m_processes;
    bool m_allCommandsSuccessfullyExecuted;
};

} //namespace NMakeFile

#endif // TARGETEXECUTOR_H
