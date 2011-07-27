/****************************************************************************
 **
 ** Copyright (C) 2008-2010 Nokia Corporation and/or its subsidiary(-ies).
 ** Contact: Nokia Corporation (qt-info@nokia.com)
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

#include "application.h"
#include <QtCore/QDebug>
#include <QtCore/QFileInfo>
#include <qt_windows.h>

namespace NMakeFile {

Application::Application(int argc, char** argv)
:   QCoreApplication(argc, argv),
    m_ctrl_c_generated(false)
{
    m_shutDownByUserMessage = RegisterWindowMessage(L"jom_shut_down_by_user");
    m_exeName = QFileInfo(QCoreApplication::applicationFilePath()).fileName();
}

bool Application::winEventFilter(MSG *msg, long *result)
{
    Q_UNUSED(result);
    if (m_shutDownByUserMessage == msg->message &&
        msg->lParam != GetCurrentProcessId())
    {
        m_ctrl_c_generated = true;
        GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0);
        return true;
    }
    return false;
}

} // namespace NMakeFile
