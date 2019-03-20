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

#ifndef MACROTABLE_H
#define MACROTABLE_H

#include "processenvironment.h"

#include <QtCore/QSet>
#include <QtCore/QStringList>

namespace NMakeFile {

class MacroTable
{
public:
    static const QChar fileNameMacroMagicEscape;

    MacroTable();
    ~MacroTable();

    void setEnvironment(const ProcessEnvironment &e) { m_environment = e; }
    const ProcessEnvironment &environment() const { return m_environment; }

    bool isMacroDefined(const QString& name) const;
    bool isMacroNameValid(const QString& name) const;
    QString macroValue(const QString& macroName) const;
    void defineEnvironmentMacroValue(const QString& name, const QString& value, bool readOnly = false);
    void defineCommandLineMacroValue(const QString &name, const QString &value);
    void defineImplicitCommandLineMacroValue(const QString &name, const QString &value);
    void setMacroValue(const QString& name, const QString& value);
    void setMacroValue(const char *szStr, const QString& value) { setMacroValue(QString::fromLatin1(szStr), value); }
    void setMacroValue(const char *szStr, const char *szValue) { setMacroValue(QString::fromLatin1(szStr), QString::fromLatin1(szValue)); }
    void predefineValue(const QString &name, const QString &value);
    void predefineValue(const char *szStr, const QString &value) { predefineValue(QString::fromLatin1(szStr), value); }
    void predefineValue(const char *szStr, const char *szValue) { predefineValue(QString::fromLatin1(szStr), QString::fromLatin1(szValue)); }
    void undefineMacro(const QString& name);
    QString expandMacros(const QString& str, bool inDependentsLine = false) const;
    void dump() const;

    struct Substitution
    {
        QString before;
        QString after;
    };

    static Substitution parseSubstitutionStatement(const QString &str, int substitutionStartIdx, int &macroInvokationEndIdx);
    static void applySubstitution(const Substitution &substitution, QString &value);

private:
    enum class MacroSource
    {
        CommandLine,
        CommandLineImplicit,
        MakeFile,
        Environment,
        Predefinition
    };

    struct MacroData
    {
        MacroSource source = MacroSource::MakeFile;
        bool isReadOnly = false;
        QString value;
    };

    void setMacroValueImpl(const QString &name, const QString &value, MacroSource source);
    void defineCommandLineMacroValueImpl(const QString &name, const QString &value,
                                         MacroSource source);
    MacroData* internalSetMacroValue(const QString &name, const QString &value,
                                     bool ignoreReadOnly = false);
    void setEnvironmentVariable(const QString& name, const QString& value);
    QString expandMacros(const QString& str, bool inDependentsLine, QSet<QString>& usedMacros) const;
    QString cycleCheckedMacroValue(const QString& macroName, QSet<QString>& usedMacros) const;

    QHash<QString, MacroData>   m_macros;
    ProcessEnvironment          m_environment;
};

} // namespace NMakeFile

#endif // MACROTABLE_H
