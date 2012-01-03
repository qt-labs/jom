#include "iocompletionport.h"

namespace NMakeFile {

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
