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

#include "jobclientacquirehelper.h"

namespace NMakeFile {

JobClientAcquireHelper::JobClientAcquireHelper(QSystemSemaphore *semaphore)
    : m_semaphore(semaphore)
{
}

void JobClientAcquireHelper::acquire()
{
    if (!m_semaphore->acquire()) {
        qWarning("QSystemSemaphore::acquire failed: %s (%d)",
                 qPrintable(m_semaphore->errorString()), m_semaphore->error());
        return;
    }
    emit acquired();
}

} // namespace NMakeFile
