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

    const uint randomId = (FileTime::currentTime().internalRepresentation() % UINT_MAX)
        ^ reinterpret_cast<uint>(&maxNumberOfJobs);
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
