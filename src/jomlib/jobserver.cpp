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

#include "jobserver.h"
#include "filetime.h"
#include "helperfunctions.h"
#include <QByteArray>
#include <QCoreApplication>
#include <QSystemSemaphore>

namespace NMakeFile {

JobServer::JobServer(ProcessEnvironment *environment)
    : m_semaphore(0)
    , m_environment(environment)
{
}

JobServer::~JobServer()
{
    delete m_semaphore;
}

bool JobServer::start(int maxNumberOfJobs)
{
    Q_ASSERT(m_environment);

    const quint64 randomId = (FileTime::currentTime().internalRepresentation() % UINT_MAX)
        ^ reinterpret_cast<quint64>(&maxNumberOfJobs);
    const QString semaphoreKey = QLatin1String("jomsrv-")
            + QString::number(QCoreApplication::applicationPid()) + QLatin1Char('-')
            + QString::number(randomId);
    m_semaphore = new QSystemSemaphore(semaphoreKey, maxNumberOfJobs - 1, QSystemSemaphore::Create);
    if (m_semaphore->error() != QSystemSemaphore::NoError) {
        setError(m_semaphore->errorString());
        return false;
    }
    m_environment->insert(QLatin1String("_JOMSRVKEY_"), semaphoreKey);
    m_environment->insert(QLatin1String("_JOMJOBCOUNT_"), QString::number(maxNumberOfJobs));
    return true;
}

QString JobServer::errorString() const
{
    return m_errorString;
}

void JobServer::setError(const QString &errorMessage)
{
    m_errorString = errorMessage;
}

} // namespace NMakeFile
