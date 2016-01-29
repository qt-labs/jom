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

#include "application.h"
#include <iocompletionport.h>

#include <QtCore/QDebug>
#include <QtCore/QFileInfo>
#include <QtCore/QSet>
#include <qt_windows.h>
#include <Tlhelp32.h>

namespace NMakeFile {

static bool isSubJOM(const QString &processExeName)
{
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
        return false;
    bool result = false;
    QHash<DWORD, PROCESSENTRY32> processEntries;
    QSet<DWORD> seenProcessIds;
    PROCESSENTRY32 pe = {0};
    pe.dwSize = sizeof(pe);
    if (!Process32First(hSnapshot, &pe)) {
        qWarning("Process32First failed with error code %d.", GetLastError());
        goto done;
    }
    do {
        processEntries.insert(pe.th32ProcessID, pe);
    } while (Process32Next(hSnapshot, &pe));

    const DWORD dwCurrentProcessId = GetCurrentProcessId();
    DWORD dwProcessId = dwCurrentProcessId;
    while (dwProcessId && !seenProcessIds.contains(dwProcessId)) {
        seenProcessIds.insert(dwProcessId);
        QHash<DWORD, PROCESSENTRY32>::iterator it = processEntries.find(dwProcessId);
        if (it == processEntries.end())
            break;

        PROCESSENTRY32 &pe = it.value();
        QString exeName = QString::fromUtf16((const ushort*)pe.szExeFile);
        if (pe.th32ProcessID != dwCurrentProcessId && exeName == processExeName) {
            result = true;
            goto done;
        }

        dwProcessId = pe.th32ParentProcessID;
        processEntries.erase(it);
    }

done:
    CloseHandle(hSnapshot);
    return result;
}

Application::Application(int &argc, char **argv)
:   QCoreApplication(argc, argv)
{
    QString exeName = QFileInfo(QCoreApplication::applicationFilePath()).fileName();
    m_bIsSubJOM = NMakeFile::isSubJOM(exeName);
}

Application::~Application()
{
    IoCompletionPort::destroyInstance();
}

void Application::exit(int exitCode)
{
    QCoreApplication::exit(exitCode);
}

} // namespace NMakeFile
