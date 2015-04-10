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

#ifndef JOBCLIENTACQUIRETHREAD_H
#define JOBCLIENTACQUIRETHREAD_H

#include <QObject>
#include <QSystemSemaphore>

namespace NMakeFile {

class JobClientAcquireHelper : public QObject
{
    Q_OBJECT
public:
    explicit JobClientAcquireHelper(QSystemSemaphore *semaphore);

public slots:
    void acquire();

signals:
    void acquired();

private:
    QSystemSemaphore *m_semaphore;
};

} // namespace NMakeFile

#endif // JOBCLIENTACQUIRETHREAD_H
