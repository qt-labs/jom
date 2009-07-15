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
#include "preprocessor.h"
#ifdef ANTLR_AVAILABLE
#   include "ppexpr/ppexpression.h"
#endif
#include "macrotable.h"
#include "exception.h"
#include "makefilelinereader.h"

#include <QDir>
#include <QDebug>

namespace NMakeFile {

Preprocessor::Preprocessor()
:   m_macroTable(0),
    m_expressionParser(0)
{
    m_rexMacro.setPattern("^(\\S+)\\s*=(.*)");
    m_rexPreprocessingDirective.setPattern("^!\\s*(\\S+)(.*)");
}

Preprocessor::~Preprocessor()
{
}

void Preprocessor::setMacroTable(MacroTable* macroTable)
{
    m_macroTable = macroTable;
}

bool Preprocessor::openFile(const QString& fileName)
{
    m_conditionalStack.clear();
    if (!m_fileStack.isEmpty())
        m_fileStack.clear();

    return internalOpenFile(fileName);
}

bool Preprocessor::internalOpenFile(QString fileName)
{
    if (fileName.startsWith('"') && fileName.endsWith('"'))
        fileName = fileName.mid(1, fileName.length() - 2);
    else if (fileName.startsWith('<') && fileName.endsWith('>')) {
        fileName = fileName.mid(1, fileName.length() - 2);
        QString includeVar = m_macroTable->macroValue("INCLUDE").replace('\t', ' ');
        QStringList includeDirs = includeVar.split(' ', QString::SkipEmptyParts);
        QString fullFileName;
        foreach (const QString& includeDir, includeDirs) {
            fullFileName = includeDir + '\\' + fileName;
            if (QFile::exists(fullFileName)) {
                fileName = fullFileName;
                break;
            }
        }
    } else {
        QString fullFileName = fileName;
        QStack<TextFile> tmpStack;
        while (m_fileStack.count() >= 1) {
            if (QFile::exists(fullFileName)) {
                fileName = fullFileName;
                break;
            }
            TextFile textFile = m_fileStack.pop();
            tmpStack.push(textFile);
            fullFileName = textFile.fileDirectory + '\\' + fileName;
        }
        while (!tmpStack.isEmpty())
            m_fileStack.push(tmpStack.pop());
    }

    // make file name canonical for safe cycle detection
    const QString origFileName = fileName;
    QFileInfo fileInfo(fileName);
    if (!fileInfo.exists())
        error(QString("File %1 doesn't exist.").arg(origFileName));
    fileName = fileInfo.canonicalFilePath();

    // detect include cycles
    foreach (const TextFile& tf, m_fileStack) {
        if (tf.reader->fileName() == fileName) {
            error(QLatin1String("cycle in include files: ") + fileInfo.fileName());
            return false;
        }
    }

    MakefileLineReader* reader = new MakefileLineReader(fileName);
    if (!reader->open()) {
        delete reader;
        error(QLatin1String("Can't open ") + origFileName);
        return false;
    }

    m_fileStack.push(TextFile());
    TextFile& textFile = m_fileStack.top();
    textFile.reader = reader;
    textFile.fileDirectory = fileInfo.absolutePath();
    return true;
}

QString Preprocessor::readLine()
{
    QString line;
    basicReadLine(line);

    if (parseMacro(line) || parsePreprocessingDirective(line)) {
        return readLine();
    }

    return line;
}

uint Preprocessor::lineNumber() const
{
    if (m_fileStack.isEmpty())
        return 0;
    return m_fileStack.top().reader->lineNumber();
}

QString Preprocessor::currentFileName() const
{
    if (m_fileStack.isEmpty())
        return QString();
    return m_fileStack.top().reader->fileName();
}

inline void removingLeadingCharacterFromSubstring(QString& str, const QString& substring)
{
    QStringMatcher sm(substring);
    int idx = 0;
    while (idx = sm.indexIn(str, idx) > -1)
        str.remove(idx, 1);
}

void Preprocessor::basicReadLine(QString& line)
{
    if (m_fileStack.isEmpty()) {
        line = QString();
        return;
    }
    line = m_fileStack.top().reader->readLine();
    while (line.isNull()) {
        delete m_fileStack.top().reader;
        m_fileStack.pop();
        if (m_fileStack.isEmpty())
            return;
        line = m_fileStack.top().reader->readLine();
    }

    line.replace("$$", "$");
    line.replace("^\\", "\\");
}

bool Preprocessor::parseMacro(const QString& line)
{
    if (!m_rexMacro.exactMatch(line))
        return false;

    QString name = m_rexMacro.cap(1);
    QString value = m_rexMacro.cap(2);

    value = value.trimmed();
    //qDebug() << "parseMacro" << name << value;
    m_macroTable->setMacroValue(name, value);
    return true;
}

bool Preprocessor::parsePreprocessingDirective(const QString& line)
{
    QString directive, value;
    if (!isPreprocessingDirective(line, directive, value))
        return false;

    if (directive == "CMDSWITCHES") {
    } else if (directive == "ERROR") {
        error("ERROR: " + value);
    } else if (directive == "MESSAGE") {
        printf(qPrintable(value));
    } else if (directive == "INCLUDE") {
        internalOpenFile(value);
    } else if (directive == "IF") {
        bool followElseBranch = evaluateExpression(value) == 0;
        enterConditional(followElseBranch);
        if (followElseBranch) {
            skipUntilNextMatchingConditional();
            return true;
        }
    } else if (directive == "IFDEF") {
        bool followElseBranch = !m_macroTable->isMacroDefined(value);
        enterConditional(followElseBranch);
        if (followElseBranch) {
            skipUntilNextMatchingConditional();
            return true;
        }
    } else if (directive == "IFNDEF") {
        bool followElseBranch = m_macroTable->isMacroDefined(value);
        enterConditional(followElseBranch);
        if (followElseBranch) {
            skipUntilNextMatchingConditional();
            return true;
        }
    } else if (directive == "ELSE") {
        if (conditionalDepth() == 0) {
            error("unexpected ELSE");
            return true;
        }
        if (!m_conditionalStack.top()) {
            skipUntilNextMatchingConditional();
            return true;
        }
    } else if (directive == "ELSEIF") {
        if (conditionalDepth() == 0) {
            error("unexpected ELSE");
            return true;
        }
        if (!m_conditionalStack.top() || evaluateExpression(value) == 0) {
            skipUntilNextMatchingConditional();
            return true;
        }
    } else if (directive == "ELSEIFDEF") {
        if (conditionalDepth() == 0) {
            error("unexpected ELSE");
            return true;
        }
        if (!m_conditionalStack.top() || !m_macroTable->isMacroDefined(value)) {
            skipUntilNextMatchingConditional();
            return true;
        }
    } else if (directive == "ELSEIFNDEF") {
        if (conditionalDepth() == 0) {
            error("unexpected ELSE");
            return true;
        }
        if (!m_conditionalStack.top() || m_macroTable->isMacroDefined(value)) {
            skipUntilNextMatchingConditional();
            return true;
        }
    } else if (directive == "ENDIF") {
        exitConditional();
    } else if (directive == "UNDEF") {
        m_macroTable->undefineMacro(value);
    }

    return true;
}

bool Preprocessor::isPreprocessingDirective(const QString& line, QString& directive, QString& value)
{
    if (line.isEmpty())
        return false;

    const QChar firstChar = line.at(0);
    if (firstChar == ' ' || firstChar == '\t')
        return false;

    bool oldStyleIncludeDirectiveFound = false;
    if (firstChar != '!' && line.length() > 8) {
        const char ch = line.at(7).toLatin1();
        if (ch != ' ' && ch != '\t')
            return false;

        if (line.left(7).toLower() == QLatin1String("include"))
            oldStyleIncludeDirectiveFound = true;
        else
            return false;
    }

    bool result = true;
    if (oldStyleIncludeDirectiveFound) {
        directive = "INCLUDE";
        value = line.mid(8);
    } else {
        result = m_rexPreprocessingDirective.exactMatch(line);
        if (result) {
            directive = m_rexPreprocessingDirective.cap(1).toUpper();
            value = m_rexPreprocessingDirective.cap(2).trimmed();
        }
    }

    return result;
}

void Preprocessor::skipUntilNextMatchingConditional()
{
    uint depth = 0;
    QString line, directive, value;

    enum DirectiveToken { TOK_IF, TOK_ENDIF, TOK_ELSE, TOK_UNINTERESTING };
    DirectiveToken token;
    do {
        basicReadLine(line);
        if (line.isNull())
            return;

        if (!isPreprocessingDirective(line, directive, value))
            continue;

        if (directive == "ENDIF")
            token = TOK_ENDIF;
        else if (directive.startsWith("IF"))
            token = TOK_IF;
        else if (directive.startsWith("ELSE"))
            token = TOK_ELSE;
        else
            token = TOK_UNINTERESTING;

        if (token == TOK_UNINTERESTING)
            continue;

        if (depth == 0) {
            if (token == TOK_ELSE)
                return;  // found the next matching ELSE
            if (token == TOK_ENDIF) {
                exitConditional();
                return;  // found the next matching ENDIF
            }
        }

        if (token == TOK_ENDIF)
            --depth;
        else if (token == TOK_IF)
            ++depth;

    } while (!line.isNull());
}

void Preprocessor::enterConditional(bool followElseBranch)
{
    m_conditionalStack.push(followElseBranch);
}

void Preprocessor::exitConditional()
{
    m_conditionalStack.pop();
}

int Preprocessor::evaluateExpression(const QString& expr)
{
#ifdef ANTLR_AVAILABLE
    if (!m_expressionParser)
        m_expressionParser = new PPExpression(this);

    return m_expressionParser->evaluate(m_macroTable->expandMacros(expr));
#else
    qFatal("Sorry, expressions in preprocessor directives are not supported. Compile with ANTLR_AVAILABLE.");
    return 0;
#endif
}

void Preprocessor::error(const QString& msg)
{
    throw Exception(msg, currentFileName(), lineNumber());
}

} // namespace NMakeFile
