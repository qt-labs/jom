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

#ifndef JOBCLIENT_H
#define JOBCLIENT_H

#include "processenvironment.h"
#include <QObject>

QT_BEGIN_NAMESPACE
class QSystemSemaphore;
class QThread;
QT_END_NAMESPACE

namespace NMakeFile {

class JobClientAcquireHelper;

class JobClient : public QObject
{
    Q_OBJECT
public:
    explicit JobClient(ProcessEnvironment *environment, QObject *parent = 0);
    ~JobClient();

    bool start();
    void asyncAcquire();
    bool isAcquiring() const;
    void release();
    QString errorString() const;

signals:
    void startAcquisition();
    void acquired();

private slots:
    void onHelperAcquired();

private:
    void setError(const QString &errorMessage);

    ProcessEnvironment *m_environment;
    QString m_errorString;
    QSystemSemaphore *m_semaphore;
    QThread *m_acquireThread;
    JobClientAcquireHelper *m_acquireHelper;
    bool m_isAcquiring;
};

} // namespace NMakeFile

#endif // JOBCLIENT_H
