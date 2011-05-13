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
#pragma once

#include "makefile.h"
#include <QProcess>
#include <QFile>

namespace NMakeFile {

class CommandExecutor : public QObject
{
    Q_OBJECT
public:
    CommandExecutor(QObject* parent, const QStringList& environment);
    ~CommandExecutor();

    void start(DescriptionBlock* target);
    DescriptionBlock* target() { return m_pTarget; }
    void block();
    void unblock();
    bool isActive() const { return m_active; }
    void waitForFinished();
    void cleanupTempFiles();

    enum OutputMode
    {
        DirectOutput,
        BufferingOutput
    };

    void setOutputMode(OutputMode mode);
    OutputMode outputMode() const { return m_outputMode; }
    void flushOutput();

public slots:
    void setEnvironment(const QStringList &environment);

signals:
    void environmentChanged(const QStringList &environment);
    void finished(CommandExecutor* process, bool abortMakeProcess);
    void subJomStarted();

private slots:
    void onProcessReadyReadStandardError();
    void onProcessReadyReadStandardOutput();
    void onProcessError(QProcess::ProcessError error);
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void finishExecution(bool abortMakeProcess);
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
    QProcess            m_process;
    DescriptionBlock*   m_pTarget;
    bool                m_blocked;
    bool                m_processFinishedWhileBlocked;

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

    struct OutputChunk
    {
        QByteArray data;
        FILE *channel;
    };

    OutputMode          m_outputMode;
    QList<OutputChunk>  m_outputBuffer;
};

} // namespace NMakeFile
