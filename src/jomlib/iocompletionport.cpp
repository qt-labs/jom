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

#include "iocompletionport.h"

namespace NMakeFile {

IoCompletionPort *IoCompletionPort::m_instance = 0;

IoCompletionPort::IoCompletionPort()
    : hPort(INVALID_HANDLE_VALUE)
{
    setObjectName(QLatin1String("I/O completion port thread"));
    HANDLE hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
    if (!hIOCP) {
        qWarning("CreateIoCompletionPort failed with error code %d.\n", GetLastError());
        return;
    }
    hPort = hIOCP;
}

IoCompletionPort::~IoCompletionPort()
{
    PostQueuedCompletionStatus(hPort, 0, NULL, NULL);
    QThread::wait();
    CloseHandle(hPort);
}

IoCompletionPort *IoCompletionPort::instance()
{
    if (!m_instance)
        m_instance = new IoCompletionPort;
    return m_instance;
}

void IoCompletionPort::destroyInstance()
{
    delete m_instance;
    m_instance = 0;
}

void IoCompletionPort::registerObserver(IoCompletionPortObserver *observer, HANDLE hFile)
{
    HANDLE hIOCP = CreateIoCompletionPort(hFile, hPort, reinterpret_cast<ULONG_PTR>(observer), 0);
    if (!hIOCP) {
        qWarning("Can't associate file handle with I/O completion port. Error code %d.\n", GetLastError());
        return;
    }
    mutex.lock();
    observers.insert(observer);
    mutex.unlock();
    if (!QThread::isRunning())
        QThread::start();
}

void IoCompletionPort::unregisterObserver(IoCompletionPortObserver *observer)
{
    mutex.lock();
    observers.remove(observer);
    mutex.unlock();
}

void IoCompletionPort::run()
{
    DWORD dwBytesRead;
    ULONG_PTR pulCompletionKey;
    OVERLAPPED *overlapped;

    for (;;) {
        BOOL success = GetQueuedCompletionStatus(hPort,
                                                &dwBytesRead,
                                                &pulCompletionKey,
                                                &overlapped,
                                                INFINITE);

        DWORD errorCode = success ? ERROR_SUCCESS : GetLastError();
        if (!success && !overlapped) {
            printf("GetQueuedCompletionStatus failed with error code %d.\n", errorCode);
            return;
        }

        if (success && !(dwBytesRead || pulCompletionKey || overlapped)) {
            // We've posted null values via PostQueuedCompletionStatus to end this thread.
            return;
        }

        IoCompletionPortObserver *observer = reinterpret_cast<IoCompletionPortObserver *>(pulCompletionKey);
        mutex.lock();
        if (observers.contains(observer))
            observer->completionPortNotified(dwBytesRead, errorCode);
        mutex.unlock();
    }
}

} // namespace NMakeFile
