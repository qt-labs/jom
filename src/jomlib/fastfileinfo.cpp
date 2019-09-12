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

#include "fastfileinfo.h"

#include <QtCore/QDebug>
#include <QtCore/QHash>
#include <windows.h>

namespace NMakeFile {

static_assert(sizeof(FastFileInfo::InternalType) == sizeof(WIN32_FILE_ATTRIBUTE_DATA),
              "FastFileInfo::InternalType has wrong size");

inline WIN32_FILE_ATTRIBUTE_DATA* z(FastFileInfo::InternalType &internalData)
{
    return reinterpret_cast<WIN32_FILE_ATTRIBUTE_DATA*>(&internalData);
}

inline const WIN32_FILE_ATTRIBUTE_DATA* z(const FastFileInfo::InternalType &internalData)
{
    return reinterpret_cast<const WIN32_FILE_ATTRIBUTE_DATA*>(&internalData);
}

static WIN32_FILE_ATTRIBUTE_DATA createInvalidFAD()
{
    WIN32_FILE_ATTRIBUTE_DATA fad = {0};
    fad.dwFileAttributes = INVALID_FILE_ATTRIBUTES;
    return fad;
}

static QHash<QString, WIN32_FILE_ATTRIBUTE_DATA> fadHash;

FastFileInfo::FastFileInfo(const QString &fileName)
{
    static const WIN32_FILE_ATTRIBUTE_DATA invalidFAD = createInvalidFAD();
    *z(m_attributes) = fadHash.value(fileName, invalidFAD);
    if (z(m_attributes)->dwFileAttributes != INVALID_FILE_ATTRIBUTES)
        return;

    if (!GetFileAttributesEx(reinterpret_cast<const TCHAR*>(fileName.utf16()),
                             GetFileExInfoStandard, &m_attributes))
    {
        z(m_attributes)->dwFileAttributes = INVALID_FILE_ATTRIBUTES;
        return;
    }

    fadHash.insert(fileName, *z(m_attributes));
}

bool FastFileInfo::exists() const
{
    return z(m_attributes)->dwFileAttributes != INVALID_FILE_ATTRIBUTES; 
}

FileTime FastFileInfo::lastModified() const
{
    const WIN32_FILE_ATTRIBUTE_DATA *fattr = z(m_attributes);
    if (fattr->dwFileAttributes == INVALID_FILE_ATTRIBUTES) {
        return FileTime();
    } else {
        return FileTime(reinterpret_cast<const FileTime::InternalType&>(fattr->ftLastWriteTime));
    }
}

void FastFileInfo::clearCacheForFile(const QString &fileName)
{
    fadHash.remove(fileName);
}

} // NMakeFile
