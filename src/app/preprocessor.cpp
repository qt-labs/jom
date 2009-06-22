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
    m_lineNumber = 0;
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
    QFileInfo fileInfo(fileName);
    fileName = fileInfo.canonicalFilePath();

    // detect include cycles
    foreach (const TextFile& tf, m_fileStack) {
        if (tf.file->fileName() == fileName) {
            throw Exception(QLatin1String("cycle in include files: ") + fileInfo.fileName(), m_lineNumber);
            return false;
        }
    }

    QFile* file = new QFile(fileName);
    if (!file->open(QFile::ReadOnly)) {
        throw Exception(QLatin1String("Can't open ") + fileName, m_lineNumber);
        delete file;
        return false;
    }

    m_fileStack.push(TextFile());
    TextFile& textFile = m_fileStack.top();
    textFile.file = file;
    textFile.stream = new QTextStream(file);
    textFile.fileDirectory = fileInfo.absolutePath();
    return true;
}

QString Preprocessor::readLine()
{
    QString line;
    basicReadLine(line);

    if (parseMacro(line) || parsePreprocessingDirective(line)) {
        m_lineNumber++;
        return readLine();
    }

    return line;
}

void Preprocessor::basicReadLine(QString& line)
{
    if (m_fileStack.isEmpty()) {
        line = QString();
        return;
    }
    line = m_fileStack.top().stream->readLine();
    while (line.isNull()) {
        delete m_fileStack.top().stream;
        delete m_fileStack.top().file;
        m_fileStack.pop();
        if (m_fileStack.isEmpty())
            return;
        line = m_fileStack.top().stream->readLine();
    }

    // filter comments
    int idx = line.indexOf('#');
    bool escapedCommentSpecifierFound = false;
    bool commentAtEndOfLine = false;
    while (idx > -1) {
        if (idx == 0) {
            m_lineNumber++;
            basicReadLine(line);
            return;
        }
        else if (line.at(idx-1) == '^') {
            escapedCommentSpecifierFound = true;
            idx = line.indexOf('#', idx + 1);
            continue;
        }
        line.resize(idx);
        line = line.trimmed();
        commentAtEndOfLine = true;
        break;
    }
    line.replace("$$", "$");
    if (escapedCommentSpecifierFound)
        line.replace("^#", "#");

    // concatenate lines ending with ^
    while (line.endsWith('^') && !commentAtEndOfLine) {
        line = line.left(line.count() - 1);
        line.append(QLatin1Char('\n'));
        QString appendix;
        basicReadLine(appendix);
        line.append(appendix);
    }

    // concatenate multi lines
    while (line.endsWith('\\') && !commentAtEndOfLine) {
        if (line.count() > 2 && line.at(line.count() - 2) == '^')
            break;
        line.resize(line.length() - 1);
        line.append(' ');
        QString appendix;
        basicReadLine(appendix);
        // find the first non-whitespace character in appendix
        const int appendixLength = appendix.length();
        for (idx=0; idx < appendixLength; ++idx) {
            QChar ch = appendix.at(idx);
            if (ch != QLatin1Char(' ') && ch != QLatin1Char('\t'))
                break;
        }
        line.append(appendix.midRef(idx));
    }
    line.replace("^\\", "\\");

    m_lineNumber++;
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
        throw Exception("ERROR: " + value);
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
    qWarning("Sorry, expressions in preprocessor directives are not supported. Compile with ANTLR_AVAILABLE.");
    return 0;
#endif
}

void Preprocessor::error(const QString& msg)
{
    throw Exception("Preprocessor: " + msg, m_lineNumber);
}

} // namespace NMakeFile
