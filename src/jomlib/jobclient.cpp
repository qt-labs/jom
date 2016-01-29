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
