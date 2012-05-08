/****************************************************************************
 **
 ** Copyright (C) 2008-2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "fileinfo.h"

#include <QtCore/QDebug>
#include <QtCore/QHash>
#include <windows.h>

namespace NMakeFile {

template<bool> struct CompileTimeAssert;
template<> struct CompileTimeAssert<true> {};
static CompileTimeAssert<
    sizeof(FastFileInfo::InternalType) == sizeof(WIN32_FILE_ATTRIBUTE_DATA)
        > internal_type_has_wrong_size;

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

static QString correctFileName(const QString &fileName)
{
    QString result = fileName;
    if (result.startsWith(QLatin1Char('\"'))) {
        result.remove(0, 1);
        result.chop(1);
    }
    return result;
}

static QHash<QString, WIN32_FILE_ATTRIBUTE_DATA> fadHash;

FastFileInfo::FastFileInfo(const QString &fileName)
{
    QString correctedFileName = correctFileName(fileName);

    static const WIN32_FILE_ATTRIBUTE_DATA invalidFAD = createInvalidFAD();
    *z(m_attributes) = fadHash.value(correctedFileName, invalidFAD);
    if (z(m_attributes)->dwFileAttributes != INVALID_FILE_ATTRIBUTES)
        return;

    if (!GetFileAttributesEx(reinterpret_cast<const TCHAR*>(correctedFileName.utf16()),
                             GetFileExInfoStandard, &m_attributes))
    {
        z(m_attributes)->dwFileAttributes = INVALID_FILE_ATTRIBUTES;
        return;
    }

    fadHash.insert(correctedFileName, *z(m_attributes));
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
    QString correctedFileName = correctFileName(fileName);
    fadHash.remove(correctedFileName);
}

} // NMakeFile
