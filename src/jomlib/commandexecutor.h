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

#ifndef COMMANDEXECUTOR_H
#define COMMANDEXECUTOR_H

#include "makefile.h"
#include "process.h"
#include <QFile>
#include <QString>

QT_BEGIN_NAMESPACE
class QStringList;
QT_END_NAMESPACE

namespace NMakeFile {

class CommandExecutor : public QObject
{
    Q_OBJECT
public:
    CommandExecutor(QObject* parent, const ProcessEnvironment &environment);
    ~CommandExecutor();

    void start(DescriptionBlock* target);
    DescriptionBlock* target() { return m_pTarget; }
    bool isActive() const { return m_active; }
    void waitForFinished();
    void cleanupTempFiles();
    void setBufferedOutput(bool b) { m_process.setBufferedOutput(b); }
    bool isBufferedOutputSet() const { return m_process.isBufferedOutputSet(); }

public slots:
    void setEnvironment(const ProcessEnvironment &environment);

signals:
    void environmentChanged(const ProcessEnvironment &environment);
    void finished(CommandExecutor* process, bool abortMakeProcess);

private slots:
    void onProcessError(Process::ProcessError error);
    void onProcessFinished(int exitCode, Process::ExitStatus exitStatus);

private:
    void finishExecution(bool commandFailed);
    void executeCurrentCommandLine();
    void createTempFiles();
    void writeToChannel(const QByteArray& data, FILE *channel);
    void writeToStandardOutput(const QByteArray& data);
    void writeToStandardError(const QByteArray& data);
    bool isSimpleCommandLine(const QString &cmdLine);
    bool exec_cd(const QString &commandLine);

private:
    static ulong        m_startUpTickCount;
    static QString      m_tempPath;
    Process             m_process;
    DescriptionBlock*   m_pTarget;

    struct TempFile
    {
        QFile* file;
        bool   keep;
    };

    QList<TempFile>     m_tempFiles;
    int                 m_currentCommandIdx;
    QString             m_nextWorkingDir;
    bool                m_ignoreProcessErrors;
    bool                m_active;
};

} // namespace NMakeFile

#endif // COMMANDEXECUTOR_H
