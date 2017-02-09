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

#include "jomprocess.h"
#include <cstdio>

namespace NMakeFile {

Process::Process(QObject *parent)
    : QProcess(parent)
{
    connect(this, SIGNAL(error(QProcess::ProcessError)), SLOT(forwardError(QProcess::ProcessError)));
    connect(this, SIGNAL(finished(int, QProcess::ExitStatus)), SLOT(forwardFinished(int, QProcess::ExitStatus)));
}

void Process::setBufferedOutput(bool bufferedOutput)
{
    QProcess::setProcessChannelMode(bufferedOutput ? SeparateChannels : ForwardedChannels);
}

bool Process::isBufferedOutputSet() const
{
    return QProcess::processChannelMode() == SeparateChannels;
}

void Process::setEnvironment(const ProcessEnvironment &e)
{
    QProcessEnvironment qpenv;
    for (ProcessEnvironment::const_iterator it = e.constBegin(); it != e.constEnd(); ++it)
        qpenv.insert(it.key().toQString(), it.value());
    QProcess::setProcessEnvironment(qpenv);
}

ProcessEnvironment Process::environment() const
{
    ProcessEnvironment env;
    const QProcessEnvironment qpenv = QProcess::processEnvironment();
    foreach (const QString &key, qpenv.keys())
        env.insert(key, qpenv.value(key));
    return env;
}

bool Process::isRunning() const
{
    return QProcess::state() == QProcess::Running;
}

void Process::start(const QString &commandLine)
{
    QProcess::start(commandLine);
    QProcess::waitForStarted();
}

void Process::writeToStdOutBuffer(const QByteArray &output)
{
    fputs(output.data(), stdout);
    fflush(stdout);
}

void Process::writeToStdErrBuffer(const QByteArray &output)
{
    fputs(output.data(), stderr);
    fflush(stderr);
}

Process::ExitStatus Process::exitStatus() const
{
    return static_cast<Process::ExitStatus>(QProcess::exitStatus());
}

void Process::forwardError(QProcess::ProcessError qe)
{
    ProcessError e;
    switch (qe) {
    case QProcess::FailedToStart:
        e = FailedToStart;
        break;
    case QProcess::Crashed:
        e = Crashed;
        break;
    default:
        e = UnknownError;
    }
    emit error(e);
}

void Process::forwardFinished(int exitCode, QProcess::ExitStatus status)
{
    emit finished(exitCode, static_cast<Process::ExitStatus>(status));
}

} // namespace NMakeFile
