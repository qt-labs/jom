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

#ifndef HELPERFUNCTIONS_H
#define HELPERFUNCTIONS_H

#include <QtCore/QString>
#include <QtCore/QStringList>

inline QString fileNameFromFilePath(const QString& filePath)
{
    int idx = qMax(filePath.lastIndexOf(QLatin1Char('/')), filePath.lastIndexOf(QLatin1Char('\\')));
    if (idx == -1)
        return filePath;
    QString fileName = filePath;
    fileName.remove(0, idx+1);
    return fileName;
}

inline bool isSpaceOrTab(const QChar& ch)
{
    return ch == QLatin1Char(' ') || ch == QLatin1Char('\t');
}

inline bool startsWithSpaceOrTab(const QString& str)
{
    Q_ASSERT(!str.isEmpty());
    return isSpaceOrTab(str.at(0));
}

inline void removeDirSeparatorAtEnd(QString& directory)
{
    if (directory.endsWith(QLatin1Char('/')) || directory.endsWith(QLatin1Char('\\')))
        directory.chop(1);
}

inline void removeDoubleQuotes(QString& targetName)
{
    const QChar dblQuote = QLatin1Char('"');
    if (targetName.startsWith(dblQuote) && targetName.endsWith(dblQuote)) {
        targetName.chop(1);
        targetName.remove(0, 1);
    }
}

/**
 * Splits the string, respects "foo bar" and "foo ""knuffi"" bar".
 */
QStringList splitCommandLine(QString commandLine);

/**
 * Returns a copy of s with all whitespace removed from the left.
 */
QString trimLeft(const QString &s);

QString qGetEnvironmentVariable(const wchar_t *lpName);
bool qSetEnvironmentVariable(const QString &name, const QString &value);

#endif // HELPERFUNCTIONS_H
