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

#include <QStringList>
#include <QRegExp>
#include <QDebug>
#include <windows.h>

namespace NMakeFile {

MacroTable::MacroTable(QStringList* environment)
:   m_environment(environment)
{
}

MacroTable::~MacroTable()
{
}

QString MacroTable::macroValue(const QString& macroName) const
{
    return m_macros.value(macroName).value;
}

/**
 * Sets the value of a macro and marks it as environment variable.
 * That means changing the macro value changes the environment.
 * Note that environment macro names are converted to upper case.
 */
void MacroTable::defineEnvironmentMacroValue(const QString& name, const QString& value)
{
    MacroData* macroData = internalSetMacroValue(name.toUpper(), value);
    if (!macroData)
        return;
    macroData->isEnvironmentVariable = true;
    setEnvironmentVariable(name, value);
}

bool MacroTable::isMacroNameValid(const QString& name) const
{
    static QRegExp rexMacroIdentifier;
    if (rexMacroIdentifier.isEmpty()) {
        rexMacroIdentifier.setPattern("([A-Z]|_|)(\\w|\\.)+");
        rexMacroIdentifier.setCaseSensitivity(Qt::CaseInsensitive);
    }

    return rexMacroIdentifier.exactMatch(name);
}

/**
 * Sets the value of a macro. If the macro doesn't exist, it is defines as
 * a normal macro (no environment variable) - changing the macro doesn't affect
 * the environment.
 * If the macros exists and is an environment variable then the corresponding
 * environment variable is set to the new macro value.
 */
void MacroTable::setMacroValue(const QString& name, const QString& value)
{
    MacroData* macroData = internalSetMacroValue(name, value);
    if (!macroData)
        throw Exception(QString("macro name %1 is invalid").arg(name));

    if (macroData->isEnvironmentVariable)
        setEnvironmentVariable(name, value);
}

/**
 * Sets the value of an environment variable.
 * The environment will be passed to the QProcess instances.
 */
void MacroTable::setEnvironmentVariable(const QString& name, const QString& value)
{
    //### Changing the actual environment can be removed when we don't call system() anymore.
    ::SetEnvironmentVariable(reinterpret_cast<const WCHAR*>(name.utf16()),
                             reinterpret_cast<const WCHAR*>(value.utf16()));

    if (!m_environment)
        return;

    const QString namePlusEq = name + "=";
    QStringList::iterator it = m_environment->begin();
    QStringList::iterator itEnd = m_environment->end();
    for (; it != itEnd; ++it) {
        if ((*it).startsWith(namePlusEq, Qt::CaseInsensitive)) {
            m_environment->erase(it);
            break;
        }
    }
    m_environment->append(namePlusEq + value);
}

MacroTable::MacroData* MacroTable::internalSetMacroValue(const QString& name, const QString& value)
{
    QString expandedName = expandMacros(name);
    if (!isMacroNameValid(expandedName))
        return 0;

    MacroData* result = 0;
    const QString instantiatedName = "$(" + expandedName + ")";
    if (value.contains(instantiatedName)) {
        QString str = value;
        str.replace(instantiatedName, macroValue(expandedName));
        result = &m_macros[expandedName];
        result->value = str;
    } else {
        result = &m_macros[expandedName];
        result->value = value;
    }

    return result;
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
    while ((i = str.indexOf("$(")) != -1) {
        const int strLength = str.length();
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
