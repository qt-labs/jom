/****************************************************************************
 **
 ** Copyright (C) 2008-2012 Nokia Corporation and/or its subsidiary(-ies).
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
