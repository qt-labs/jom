/****************************************************************************
 **
 ** Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies).
 ** Contact: Qt Software Information (qt-info@nokia.com)
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
#include "macrotable.h"
#include "exception.h"

#include <QRegExp>

namespace NMakeFile {

MacroTable::MacroTable()
{
}

MacroTable::~MacroTable()
{
}

QString MacroTable::macroValue(const QString& macroName) const
{
    return m_macros.value(macroName);
}

void MacroTable::setMacroValue(const QString& name, const QString& value, bool silentErrors)
{
    static QRegExp rexMacroIdentifier;
    if (rexMacroIdentifier.isEmpty()) {
        rexMacroIdentifier.setPattern("([A-Z]|_|)(\\w|\\.)+");
        rexMacroIdentifier.setCaseSensitivity(Qt::CaseInsensitive);
    }

    QString expandedName = expandMacros(name);
    if (!rexMacroIdentifier.exactMatch(expandedName)) {
        if (!silentErrors)
            throw Exception(QString("macro name %1 is invalid").arg(name));
        return;
    }
    m_macros[expandedName] = expandMacros(value);
}

bool MacroTable::isMacroDefined(const QString& name) const
{
    return m_macros.contains(name);
}

void MacroTable::undefineMacro(const QString& name)
{
    m_macros.remove(name);
}

QString MacroTable::expandMacros(QString str) const
{
    int i;
    const int strLength = str.length();
    while ((i = str.indexOf("$(")) != -1) {
        if (strLength <= i+2)
            throw Exception("single $( at end of line found");

        if (str.at(i+2) == QLatin1Char('@'))    // we don't handle filename macros here
            return str;

        int j = str.indexOf(')', i);
        if (j == -1)
            throw Exception("found $( without matching )");

        str = str.left(i) + macroValue(str.mid(i+2, j-i-2)) + str.right(strLength - j - 1);
    }
    return str;
}

} // namespace NMakeFile
