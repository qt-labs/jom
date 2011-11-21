/****************************************************************************
 **
 ** Copyright (C) 2008-2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "application.h"
#include <QtCore/QDebug>
#include <QtCore/QFileInfo>
#include <qt_windows.h>
#include <Tlhelp32.h>

namespace NMakeFile {

class ProcessEntry
{
public:
    ProcessEntry()
    {
        ZeroMemory(&data, sizeof(data));
        data.dwSize = sizeof(data);
    }

    bool isValid() const { return data.szExeFile[0] != 0; }

    PROCESSENTRY32 data;
};

static bool isSubJOM(const QString &processExeName)
{
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
        return false;
    bool result = false;
    QHash<DWORD, ProcessEntry> processEntries;
    ProcessEntry pe;
    if (!Process32First(hSnapshot, &pe.data))
        goto done;
    do {
        processEntries.insert(pe.data.th32ProcessID, pe);
    } while (Process32Next(hSnapshot, &pe.data));

    const DWORD dwCurrentProcessId = GetCurrentProcessId();
    DWORD dwProcessId = dwCurrentProcessId;
    while (dwProcessId) {
        const ProcessEntry &process = processEntries.value(dwProcessId);
        if (!process.isValid())
            break;

        QString exeName = QString::fromUtf16(process.data.szExeFile);
        if (process.data.th32ProcessID != dwCurrentProcessId && exeName == processExeName) {
            result = true;
            goto done;
        }

        dwProcessId = process.data.th32ParentProcessID;
    }

done:
    CloseHandle(hSnapshot);
    return result;
}

Application::Application(int argc, char** argv)
:   QCoreApplication(argc, argv)
{
    QString exeName = QFileInfo(QCoreApplication::applicationFilePath()).fileName();
    m_bIsSubJOM = NMakeFile::isSubJOM(exeName);
}

} // namespace NMakeFile
