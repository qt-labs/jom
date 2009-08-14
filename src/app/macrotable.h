/****************************************************************************
 **
 ** Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies).
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
#pragma once

#include <QHash>

namespace NMakeFile {

class MacroTable
{
public:
    MacroTable(QStringList* environment = 0);
    ~MacroTable();

    bool isMacroDefined(const QString& name) const;
    bool isMacroNameValid(const QString& name) const;
    QString macroValue(const QString& macroName) const;
    void defineEnvironmentMacroValue(const QString& name, const QString& value, bool forceReadOnly = false);
    void setMacroValue(const QString& name, const QString& value);
    void undefineMacro(const QString& name);
    QString expandMacros(QString str) const;
    void dump();

private:
    struct MacroData
    {
        MacroData()
            : isEnvironmentVariable(false), isReadOnly(false)
        {}

        bool isEnvironmentVariable;
        bool isReadOnly;
        QString value;
    };

    MacroData* internalSetMacroValue(const QString& name, const QString& value);
    void setEnvironmentVariable(const QString& name, const QString& value);

    QHash<QString, MacroData> m_macros;
    QStringList* m_environment;
};

} // namespace NMakeFile
