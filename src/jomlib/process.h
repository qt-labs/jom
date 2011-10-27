/****************************************************************************
 **
 ** Copyright (C) 2008-2011 Nokia Corporation and/or its subsidiary(-ies).
 ** Contact: Nokia Corporation (info@qt.nokia.com)
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

#ifndef PROCESS_H
#define PROCESS_H

#include <QObject>

class Process : public QObject
{
    Q_OBJECT
public:
    explicit Process(bool directOutput, QObject *parent = 0);
    ~Process();

    enum ProcessError
    {
        UnknownError,
        FailedToStart,
        Crashed
    };

    enum ExitStatus
    {
        NormalExit,
        CrashExit
    };

    enum ProcessState
    {
        NotRunning,
        Starting,
        Running
    };

    void setWorkingDirectory(const QString &path);
    const QString &workingDirectory() const { return m_workingDirectory; }
    void setEnvironment(const QStringList &environment);
    const QStringList &environment() const { return m_environment; }
    int exitCode() const { return m_exitCode; }
    ExitStatus exitStatus() const { return m_exitStatus; }
    bool isStarted() const { return m_state == Running; }

signals:
    void error(Process::ProcessError);
    void finished(int, Process::ExitStatus);

public slots:
    void start(const QString &commandLine);
    bool waitForFinished(int ms = 30000);

private:
    static unsigned long __stdcall processWatcherThread(void *lpParameter);
    Q_INVOKABLE void onProcessFinished(int, bool);

private:
    struct ProcessPrivate *d;
    QString m_workingDirectory;
    QStringList m_environment;
    QByteArray m_envBlock;
    ProcessState m_state;
    int m_exitCode;
    ExitStatus m_exitStatus;
    bool m_directOutput;
};

#endif // PROCESS_H
