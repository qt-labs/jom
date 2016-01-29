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

#include "helperfunctions.h"
#include <qt_windows.h>

/**
 * Splits the string, respects "foo bar" and "foo ""knuffi"" bar".
 */
QStringList splitCommandLine(QString commandLine)
{
    QString str;
    QStringList arguments;
    commandLine.append(QLatin1Char(' '));   // append artificial space
    bool escapedQuote = false;
    bool insideQuotes = false;
    for (int i=0; i < commandLine.count(); ++i) {
        if (commandLine.at(i).isSpace() && !insideQuotes) {
            escapedQuote = false;
            str = str.trimmed();
            if (!str.isEmpty()) {
                arguments.append(str);
                str.clear();
            }
        } else {
            if (commandLine.at(i) == QLatin1Char('"')) {
                if (escapedQuote)  {
                    str.append(QLatin1Char('"'));
                    escapedQuote = false;
                } else {
                    escapedQuote = true;
                }
                insideQuotes = !insideQuotes;
            } else {
                str.append(commandLine.at(i));
                escapedQuote = false;
            }
        }
    }
    return arguments;
}

QString trimLeft(const QString &s)
{
    QString result = s;
    while (!result.isEmpty() && result[0].isSpace())
        result.remove(0, 1);
    return result;
}

QString qGetEnvironmentVariable(const wchar_t *lpName)
{
    const size_t bufferSize = 32767;
    TCHAR buffer[bufferSize];
    if (GetEnvironmentVariable(lpName, buffer, bufferSize))
        return QString::fromWCharArray(buffer);
    return QString();
}

bool qSetEnvironmentVariable(const QString &name, const QString &value)
{
    return SetEnvironmentVariable(
            reinterpret_cast<const wchar_t *>(name.utf16()),
            reinterpret_cast<const wchar_t *>(value.utf16()));
}
