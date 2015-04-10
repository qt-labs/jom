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

#include "jobclient.h"
#include "jobclientacquirehelper.h"
#include "helperfunctions.h"

#include <QSystemSemaphore>
#include <QThread>

namespace NMakeFile {

JobClient::JobClient(ProcessEnvironment *environment, QObject *parent)
    : QObject(parent)
    , m_environment(environment)
    , m_semaphore(0)
    , m_acquireThread(new QThread(this))
    , m_acquireHelper(0)
    , m_isAcquiring(false)
{
}

JobClient::~JobClient()
{
    if (isAcquiring())
        qWarning("JobClient destroyed while still acquiring.");
    m_acquireThread->quit();
    m_acquireThread->wait(2500);
    delete m_acquireHelper;
    delete m_semaphore;
}

bool JobClient::start()
{
    Q_ASSERT(!m_semaphore && !m_acquireHelper);
    Q_ASSERT(!m_acquireThread->isRunning());

    const QString semaphoreKey = m_environment->value(QLatin1String("_JOMSRVKEY_"));
    if (semaphoreKey.isEmpty()) {
        setError(QLatin1String("Cannot determine jobserver name."));
        return false;
    }
    m_semaphore = new QSystemSemaphore(semaphoreKey);
    if (m_semaphore->error() != QSystemSemaphore::NoError) {
        setError(m_semaphore->errorString());
        return false;
    }

    m_acquireHelper = new JobClientAcquireHelper(m_semaphore);
    m_acquireHelper->moveToThread(m_acquireThread);
    connect(this, &JobClient::startAcquisition, m_acquireHelper, &JobClientAcquireHelper::acquire);
    connect(m_acquireHelper, &JobClientAcquireHelper::acquired, this, &JobClient::onHelperAcquired);
    m_acquireThread->start();
    return true;
}

void JobClient::asyncAcquire()
{
    Q_ASSERT(m_semaphore);
    Q_ASSERT(m_acquireHelper);
    Q_ASSERT(m_acquireThread->isRunning());

    m_isAcquiring = true;
    emit startAcquisition();
}

void JobClient::onHelperAcquired()
{
    m_isAcquiring = false;
    emit acquired();
}

bool JobClient::isAcquiring() const
{
    return m_isAcquiring;
}

void JobClient::release()
{
    Q_ASSERT(m_semaphore);

    if (!m_semaphore->release())
        qWarning("QSystemSemaphore::release failed: %s (%d)",
                 qPrintable(m_semaphore->errorString()), m_semaphore->error());
}

QString JobClient::errorString() const
{
    return m_errorString;
}

void JobClient::setError(const QString &errorMessage)
{
    m_errorString = errorMessage;
}

} // namespace NMakeFile
