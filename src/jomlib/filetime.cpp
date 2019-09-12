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

#include "filetime.h"
#include <windows.h>
#include <strsafe.h>

namespace NMakeFile {

static_assert(sizeof(FileTime::InternalType) == sizeof(FILETIME),
              "FileTime::InternalType has wrong size");

FileTime::FileTime()
    : m_fileTime(0)
{
}

bool FileTime::operator < (const FileTime &rhs) const
{
    const FILETIME *const t1 = reinterpret_cast<const FILETIME *>(&m_fileTime);
    const FILETIME *const t2 = reinterpret_cast<const FILETIME *>(&rhs.m_fileTime);
    return CompareFileTime(t1, t2) < 0;
}

void FileTime::clear()
{
    m_fileTime = 0;
}

bool FileTime::isValid() const
{
    return m_fileTime != 0;
}

FileTime FileTime::currentTime()
{
    FileTime result;
    SYSTEMTIME st;
    GetSystemTime(&st);
    FILETIME *const ft = reinterpret_cast<FILETIME *>(&result.m_fileTime);
    SystemTimeToFileTime(&st, ft);
    return result;
}

QString FileTime::toString() const
{
    const FILETIME *const ft = reinterpret_cast<const FILETIME *>(&m_fileTime);
    SYSTEMTIME stUTC, stLocal;
    FileTimeToSystemTime(ft, &stUTC);
    SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);
    WCHAR szString[512];
    HRESULT hr = StringCchPrintf(szString, sizeof(szString) / sizeof(WCHAR),
                                 L"%02d.%02d.%d %02d:%02d:%02d",
                                 stLocal.wDay, stLocal.wMonth, stLocal.wYear,
                                 stLocal.wHour, stLocal.wMinute, stLocal.wSecond);
    return SUCCEEDED(hr) ? QString::fromUtf16((unsigned short*)szString) : QString();
}

} // namespace NMakeFile
