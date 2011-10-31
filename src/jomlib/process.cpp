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

#include "process.h"
#include <QByteArray>
#include <qt_windows.h>
#include <QMap>
#include <QDir>

struct Pipe
{
    Pipe()
        : hWrite(INVALID_HANDLE_VALUE)
        , hRead(INVALID_HANDLE_VALUE)
    {}

    ~Pipe()
    {
        if (hWrite != INVALID_HANDLE_VALUE)
            CloseHandle(hWrite);
        if (hRead != INVALID_HANDLE_VALUE)
            CloseHandle(hRead);
    }

    HANDLE hWrite;
    HANDLE hRead;
};

struct ProcessPrivate
{
    ProcessPrivate()
        : hWatcherThread(INVALID_HANDLE_VALUE),
          hProcess(INVALID_HANDLE_VALUE),
          hProcessThread(INVALID_HANDLE_VALUE)
    {}

    void closeAndReset(HANDLE &h)
    {
        ::CloseHandle(h);
        h = 0;
    }

    void reset()
    {
        closeAndReset(hProcess);
        closeAndReset(hProcessThread);
        closeAndReset(hWatcherThread);
    }

    HANDLE hWatcherThread;
    HANDLE hProcess;
    HANDLE hProcessThread;
    Pipe stdoutPipe;
    Pipe stderrPipe;
    QByteArray outputBuffer;
};

Process::Process(QObject *parent)
    : QObject(parent),
      d(new ProcessPrivate),
      m_state(NotRunning),
      m_exitCode(0),
      m_exitStatus(NormalExit),
      m_directOutput(false)
{
}

Process::~Process()
{
    waitForFinished();
}

void Process::setWorkingDirectory(const QString &path)
{
    m_workingDirectory = path;
}

static QByteArray createEnvBlock(const QMap<QString,QString> &environment)
{
    QByteArray envlist;
    if (!environment.isEmpty()) {
        QMap<QString,QString> copy = environment;

        // add PATH if necessary (for DLL loading)
        QString pathKey(QLatin1String("PATH"));
        if (!copy.contains(pathKey)) {
            QByteArray path = qgetenv("PATH");
            if (!path.isEmpty())
                copy.insert(pathKey, QString::fromLocal8Bit(path));
        }

        // add systemroot if needed
        QString rootKey(QLatin1String("SystemRoot"));
        if (!copy.contains(rootKey)) {
            QByteArray systemRoot = qgetenv("SystemRoot");
            if (!systemRoot.isEmpty())
                copy.insert(rootKey, QString::fromLocal8Bit(systemRoot));
        }

        int pos = 0;
        QMap<QString,QString>::ConstIterator it = copy.constBegin(),
                                             end = copy.constEnd();

        static const wchar_t equal = L'=';
        static const wchar_t nul = L'\0';

        for ( ; it != end; ++it) {
            uint tmpSize = sizeof(wchar_t) * (it.key().length() + it.value().length() + 2);
            // ignore empty strings
            if (tmpSize == sizeof(wchar_t) * 2)
                continue;
            envlist.resize(envlist.size() + tmpSize);

            tmpSize = it.key().length() * sizeof(wchar_t);
            memcpy(envlist.data()+pos, it.key().utf16(), tmpSize);
            pos += tmpSize;

            memcpy(envlist.data()+pos, &equal, sizeof(wchar_t));
            pos += sizeof(wchar_t);

            tmpSize = it.value().length() * sizeof(wchar_t);
            memcpy(envlist.data()+pos, it.value().utf16(), tmpSize);
            pos += tmpSize;

            memcpy(envlist.data()+pos, &nul, sizeof(wchar_t));
            pos += sizeof(wchar_t);
        }
        // add the 2 terminating 0 (actually 4, just to be on the safe side)
        envlist.resize( envlist.size()+4 );
        envlist[pos++] = 0;
        envlist[pos++] = 0;
        envlist[pos++] = 0;
        envlist[pos++] = 0;
    }
    return envlist;
}

void Process::setEnvironment(const QStringList &environment)
{
    m_environment = environment;

    QMap<QString,QString> envmap;
    foreach (const QString &str, m_environment) {
        int idx = str.indexOf(QLatin1Char('='));
        if (idx < 0)
            continue;
        QString name = str.left(idx);
        QString value = str.mid(idx + 1);
        envmap.insert(name, value);
    }

    m_envBlock = createEnvBlock(envmap);
}

DWORD WINAPI Process::processWatcherThread(void *lpParameter)
{
    Process *process = reinterpret_cast<Process*>(lpParameter);
    ProcessPrivate *d = process->d;

    DWORD dwRead, dwWritten;
    const size_t buflen = 4096;
    char chBuf[buflen];
    BOOL bSuccess = FALSE;
    HANDLE hParentStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

    for (;;)
    {
        bSuccess = ReadFile(d->stdoutPipe.hRead, chBuf, buflen, &dwRead, NULL);
        if (!bSuccess || dwRead == 0)
            break;

        if (process->m_directOutput) {
            bSuccess = WriteFile(hParentStdOut, chBuf, dwRead, &dwWritten, NULL);
            if (!bSuccess)
                break;
        } else {
            d->outputBuffer.append(QByteArray(chBuf, dwRead));
        }
    }

    CloseHandle(d->stdoutPipe.hRead);
    WaitForSingleObject(d->hProcess, INFINITE);

    DWORD exitCode = 0;
    bool crashed;
    if (GetExitCodeProcess(process->d->hProcess, &exitCode)) {
        //### for now we assume a crash if exit code is less than -1 or the magic number
        crashed = (exitCode == 0xf291 || (int)exitCode < 0);
    }

    QMetaObject::invokeMethod(process, "onProcessFinished", Qt::QueuedConnection, Q_ARG(int, exitCode), Q_ARG(bool, crashed));
    return 0;
}

void Process::onProcessFinished(int exitCode, bool crashed)
{
    if (!d->outputBuffer.isEmpty()) {
        printf(d->outputBuffer.data());
        d->outputBuffer.clear();
    }
    d->reset();
    ExitStatus exitStatus = crashed ? Process::CrashExit : Process::NormalExit;
    emit finished(exitCode, exitStatus);
}

static bool setupPipe(Pipe &pipe, SECURITY_ATTRIBUTES *sa)
{
    if (!CreatePipe(&pipe.hRead, &pipe.hWrite, sa, 0)) {
        qWarning("CreatePipe failed with error code %d.", GetLastError());
        return false;
    }
    if (!SetHandleInformation(pipe.hRead, HANDLE_FLAG_INHERIT, 0)) {
        qWarning("SetHandleInformation failed with error code %d.", GetLastError());
        return false;
    }
    return true;
}

void Process::start(const QString &commandLine)
{
    m_state = Starting;

    SECURITY_ATTRIBUTES sa = {0};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    if (!setupPipe(d->stdoutPipe, &sa))
        qFatal("Cannot setup pipe for stdout.");

    // Let the child process write to the same handle, like QProcess::MergedChannels.
    DuplicateHandle(GetCurrentProcess(), d->stdoutPipe.hWrite, GetCurrentProcess(),
                    &d->stderrPipe.hWrite, 0, TRUE, DUPLICATE_SAME_ACCESS);

    STARTUPINFO si = {0};
    si.cb = sizeof(si);
    si.hStdOutput = d->stdoutPipe.hWrite;
    si.hStdError = d->stderrPipe.hWrite;
    si.dwFlags = STARTF_USESTDHANDLES;

    DWORD dwCreationFlags = CREATE_UNICODE_ENVIRONMENT;
    PROCESS_INFORMATION pi;
    wchar_t *strCommandLine = _wcsdup(commandLine.utf16());     // CreateProcess can modify this string
    const wchar_t *strWorkingDir = 0;
    if (!m_workingDirectory.isEmpty()) {
        m_workingDirectory = QDir::toNativeSeparators(m_workingDirectory);
        strWorkingDir = m_workingDirectory.utf16();
    }
    void *envBlock = (m_envBlock.isEmpty() ? 0 : m_envBlock.data());
    BOOL bResult = CreateProcess(NULL, strCommandLine,
                                 0, 0, TRUE, dwCreationFlags, envBlock,
                                 strWorkingDir, &si, &pi);
    free(strCommandLine);
    strCommandLine = 0;
    if (!bResult) {
        m_state = NotRunning;
        emit error(FailedToStart);
        return;
    }

    // Close the pipe handles. This process doesn't need them anymore.
    CloseHandle(d->stdoutPipe.hWrite);
    d->stdoutPipe.hWrite = INVALID_HANDLE_VALUE;
    CloseHandle(d->stderrPipe.hWrite);
    d->stderrPipe.hWrite = INVALID_HANDLE_VALUE;

    // TODO: use a global notifier object instead of creating a thread for every starting process
    d->hProcess = pi.hProcess;
    d->hProcessThread = pi.hThread;
    HANDLE hThread = CreateThread(NULL, 4096, processWatcherThread, this, 0, NULL);
    if (!hThread) {
        d->reset();
        emit error(FailedToStart);
        return;
    }
    d->hWatcherThread = hThread;
    m_state = Running;
}

bool Process::waitForFinished(int ms)
{
    if (m_state != Running)
        return true;
    if (WaitForSingleObject(d->hWatcherThread, ms) == WAIT_TIMEOUT)
        return false;
    d->reset();
    return true;
}
