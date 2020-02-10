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

#include "exception.h"
#include "makefilefactory.h"
#include "macrotable.h"
#include "makefile.h"
#include "options.h"
#include "parser.h"
#include "preprocessor.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>

namespace NMakeFile {

MakefileFactory::MakefileFactory()
:   m_makefile(0),
    m_errorType(NoError)
{
}

void MakefileFactory::setEnvironment(const QStringList &env)
{
    for (QStringList::const_iterator it = env.begin(); it != env.end(); ++it) {
        int idx = it->indexOf(QLatin1Char('='));
        if (idx >= 0)
            m_environment.insert(it->left(idx), it->mid(idx + 1));
    }
}

void MakefileFactory::clear()
{
    m_makefile = 0;
    m_errorType = NoError;
    m_errorString.clear();
    m_activeTargets.clear();
}

static void readEnvironment(const ProcessEnvironment &environment, MacroTable *macroTable, bool forceReadOnly)
{
    ProcessEnvironment::const_iterator it = environment.begin();
    for (; it != environment.end(); ++it)
        macroTable->defineEnvironmentMacroValue(it.key().toQString(), it.value(), forceReadOnly);
}

/**
 * Returns true, if the path contains some whitespace.
 */
static bool isComplexPathName(const QString& path)
{
    for (int i=path.length()-1; i > 0; i--) {
        const QChar ch = path.at(i);
        if (ch.isSpace())
            return true;
    }
    return false;
}

/**
 * Enclose the path in double quotes, if its a complex one.
 */
static QString encloseInDoubleQuotesIfNeeded(const QString& path)
{
    if (isComplexPathName(path)) {
        QString result(QLatin1Char('"'));
        result.append(path);
        result.append(QLatin1Char('"'));
        return result;
    }
    return path;
}

bool MakefileFactory::apply(const QStringList& commandLineArguments, Options **outopt)
{
    if (m_makefile)
        clear();

    Options *options = new Options;
    if (outopt)
        *outopt = options;
    MacroTable *macroTable = new MacroTable;
    macroTable->setEnvironment(m_environment);

    QString filename;
    if (!options->readCommandLineArguments(commandLineArguments, filename, m_activeTargets, *macroTable)) {
        m_errorType = CommandLineError;
        return false;
    }
    if (options->showUsageAndExit || options->showVersionAndExit)
        return true;

    if (!options->stderrFile.isEmpty()) {
        // Try to open the file for writing.
        const wchar_t *wszFileName = reinterpret_cast<const wchar_t*>(options->stderrFile.utf16());
        FILE *f = _wfopen(wszFileName, L"w");
        if (!f) {
            m_errorString = QLatin1String("Cannot open stderr file for writing.");
            m_errorType = IOError;
            return false;
        }
        fclose(f);
        if (!_wfreopen(wszFileName, L"w", stderr)) {
            m_errorString = QLatin1String("Cannot reopen stderr handle for writing.");
            m_errorType = IOError;
            return false;
        }
    }

    options->fullAppPath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());

    readEnvironment(m_environment, macroTable, options->overrideEnvVarMacros);
    if (!options->ignorePredefinedRulesAndMacros) {
        macroTable->predefineValue("MAKE", encloseInDoubleQuotesIfNeeded(options->fullAppPath));
        macroTable->predefineValue("MAKEDIR", encloseInDoubleQuotesIfNeeded(
                                       QDir::toNativeSeparators(QDir::currentPath())));
        macroTable->predefineValue("AS", "ml");       // Macro Assembler
        macroTable->predefineValue("ASFLAGS", QString());
        macroTable->predefineValue("BC", "bc");       // Basic Compiler
        macroTable->predefineValue("BCFLAGS", QString());
        macroTable->predefineValue("CC", "cl");       // C Compiler
        macroTable->predefineValue("CCFLAGS", QString());
        macroTable->predefineValue("COBOL", "cobol"); // COBOL Compiler
        macroTable->predefineValue("COBOLFLAGS", QString());
        macroTable->predefineValue("CPP", "cl");      // C++ Compiler
        macroTable->predefineValue("CPPFLAGS", QString());
        macroTable->predefineValue("CXX", "cl");      // C++ Compiler
        macroTable->predefineValue("CXXFLAGS", QString());
        macroTable->predefineValue("FOR", "fl");      // FORTRAN Compiler
        macroTable->predefineValue("FORFLAGS", QString());
        macroTable->predefineValue("PASCAL", "pl");   // Pascal Compiler
        macroTable->predefineValue("PASCALFLAGS", QString());
        macroTable->predefineValue("RC", "rc");       // Resource Compiler
        macroTable->predefineValue("RCFLAGS", QString());
    }

    try {
        m_makefile = new Makefile(filename);
        m_makefile->setOptions(options);
        m_makefile->setMacroTable(macroTable);
        Preprocessor preprocessor;
        preprocessor.setMacroTable(macroTable);
        preprocessor.openFile(filename);
        Parser parser;
        parser.apply(&preprocessor, m_makefile, m_activeTargets);
    } catch (Exception &e) {
        m_errorType = ParserError;
        m_errorString = e.toString();
    }

    return m_errorType == NoError;
}

} //namespace NMakeFile
