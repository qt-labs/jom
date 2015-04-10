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

#ifndef JOBSERVER_H
#define JOBSERVER_H

#include "processenvironment.h"

QT_BEGIN_NAMESPACE
class QSystemSemaphore;
QT_END_NAMESPACE

namespace NMakeFile {

class JobServer
{
public:
    JobServer(ProcessEnvironment *environment);
    ~JobServer();

    bool start(int maxNumberOfJobs);
    QString errorString() const;

private:
    void setError(const QString &errorMessage);

    QString m_errorString;
    QSystemSemaphore *m_semaphore;
    ProcessEnvironment *m_environment;
};

} // namespace NMakeFile

#endif // JOBSERVER_H
