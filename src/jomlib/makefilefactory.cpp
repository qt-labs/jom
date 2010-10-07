#include "exception.h"
#include "makefilefactory.h"
#include "macrotable.h"
#include "makefile.h"
#include "options.h"
#include "parser.h"
#include "preprocessor.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QProcess>

namespace NMakeFile {

MakefileFactory::MakefileFactory()
:   m_makefile(0),
    m_errorType(NoError)
{
}

void MakefileFactory::clear()
{
    m_makefile = 0;
    m_errorType = NoError;
    m_errorString.clear();
    m_activeTargets.clear();
}

static void readEnvironment(const QStringList& environment, MacroTable *macroTable, bool forceReadOnly)
{
    foreach (const QString& env, environment) {
        QString lhs, rhs;
        int idx = env.indexOf('=');
        lhs = env.left(idx);
        rhs = env.right(env.length() - idx - 1);
        macroTable->defineEnvironmentMacroValue(lhs, rhs, forceReadOnly);
    }
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
        QString result = "\"";
        result.append(path);
        result.append('"');
        return result;
    }
    return path;
}

bool MakefileFactory::apply(const QStringList& commandLineArguments)
{
    if (m_makefile)
        clear();

    Options *options = new Options;
    MacroTable *macroTable = new MacroTable;
    macroTable->setEnvironment(m_environment);
    m_makefile = new Makefile;
    m_makefile->setOptions(options);
    m_makefile->setMacroTable(macroTable);

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
            m_errorString = "Cannot open stderr file for writing.";
            m_errorType = IOError;
            return false;
        }
        fclose(f);
        if (!_wfreopen(wszFileName, L"w", stderr)) {
            m_errorString = "Cannot reopen stderr handle for writing.";
            m_errorType = IOError;
            return false;
        }
    }

    options->fullAppPath = QCoreApplication::applicationFilePath();
    options->fullAppPath.replace(QLatin1Char('/'), QDir::separator());

    readEnvironment(m_environment, macroTable, options->overrideEnvVarMacros);
    if (!options->ignorePredefinedRulesAndMacros) {
        macroTable->setMacroValue("MAKE", encloseInDoubleQuotesIfNeeded(options->fullAppPath));
        macroTable->setMacroValue("MAKEDIR", encloseInDoubleQuotesIfNeeded(QDir::currentPath()));
        macroTable->setMacroValue("AS", "ml");       // Macro Assembler
        macroTable->setMacroValue("ASFLAGS", QString());
        macroTable->setMacroValue("BC", "bc");       // Basic Compiler
        macroTable->setMacroValue("BCFLAGS", QString());
        macroTable->setMacroValue("CC", "cl");       // C Compiler
        macroTable->setMacroValue("CCFLAGS", QString());
        macroTable->setMacroValue("COBOL", "cobol"); // COBOL Compiler
        macroTable->setMacroValue("COBOLFLAGS", QString());
        macroTable->setMacroValue("CPP", "cl");      // C++ Compiler
        macroTable->setMacroValue("CPPFLAGS", QString());
        macroTable->setMacroValue("CXX", "cl");      // C++ Compiler
        macroTable->setMacroValue("CXXFLAGS", QString());
        macroTable->setMacroValue("FOR", "fl");      // FORTRAN Compiler
        macroTable->setMacroValue("FORFLAGS", QString());
        macroTable->setMacroValue("PASCAL", "pl");   // Pascal Compiler
        macroTable->setMacroValue("PASCALFLAGS", QString());
        macroTable->setMacroValue("RC", "rc");       // Resource Compiler
        macroTable->setMacroValue("RCFLAGS", QString());
    }

    try {
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
