#ifndef IOCOMPLETIONPORT_H
#define IOCOMPLETIONPORT_H

#include <qset.h>
#include <qmutex.h>
#include <qthread.h>
#include <qt_windows.h>

namespace NMakeFile {

class IoCompletionPortObserver
{
public:
    virtual void completionPortNotified(DWORD numberOfBytes, DWORD errorCode) = 0;
};

class IoCompletionPort : protected QThread
{
public:
    IoCompletionPort();
    ~IoCompletionPort();

    void registerObserver(IoCompletionPortObserver *notifier, HANDLE hFile);
    void unregisterObserver(IoCompletionPortObserver *notifier);

protected:
    void run();

private:
    HANDLE hPort;
    QSet<IoCompletionPortObserver *> observers;
    QMutex mutex;
};

} // namespace NMakeFile

#endif // IOCOMPLETIONPORT_H
