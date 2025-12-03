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

#include "preprocessor.h"
#include "ppexprparser.h"
#include "macrotable.h"
#include "exception.h"
#include "helperfunctions.h"
#include "fastfileinfo.h"

#include <QDir>
#include <QDebug>

namespace NMakeFile {

Preprocessor::Preprocessor()
:   m_macroTable(0),
    m_expressionParser(0),
    m_bInlineFileMode(false)
{
    m_rexPreprocessingDirective.setPattern(QLatin1String("^!\\s*(\\S+)(.*)"));
}

Preprocessor::~Preprocessor()
{
    delete m_expressionParser;
}

void Preprocessor::setMacroTable(MacroTable* macroTable)
{
    m_macroTable = macroTable;
    if (m_expressionParser)
        m_expressionParser->setMacroTable(m_macroTable);
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
    // make file name absolute for safe cycle detection
    const QString origFileName = fileName;
    QFileInfo fileInfo(fileName);
    if (!fileInfo.exists()) {
        QString msg = QLatin1String("File %1 doesn't exist.");
        error(msg.arg(origFileName));
    }
    fileName = fileInfo.absoluteFilePath();

    // detect include cycles
    foreach (const TextFile& tf, m_fileStack)
        if (tf.reader->fileName() == fileName)
            error(QLatin1String("cycle in include files: ") + fileInfo.fileName());

    MakefileLineReader* reader = new MakefileLineReader(fileName);
    if (!reader->open()) {
        delete reader;
        error(QLatin1Literal("Can't open ") + origFileName);
    }

    m_fileStack.push(TextFile());
    TextFile& textFile = m_fileStack.top();
    textFile.reader = reader;
    textFile.fileDirectory = fileInfo.absolutePath();
    return true;
}

QString Preprocessor::readLine()
{
    MakefileLine line;
    for (;;) {
        MakefileLine next = basicReadLine();
        if (next.content.startsWith(QLatin1Char('!'))) {
            completePreprocessingDirectiveLine(next);
            parsePreprocessingDirective(next.content);
            continue;
        }
        if (isComplete(line))
            line = std::move(next);
        else
            joinLines(line, next);
        if (!isComplete(line))
            continue;
        if (!m_bInlineFileMode && parseMacro(line.content))
            continue;
        if (parsePreprocessingDirective(line.content))
            continue;
        break;
    }

    if (line.content.isNull() && conditionalDepth())
        error(QLatin1Literal("Missing !ENDIF directive."));

    return line.content;
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

MakefileLine Preprocessor::basicReadLine()
{
    if (!m_linesPutBack.isEmpty())
        return { m_linesPutBack.takeFirst() };

    if (m_fileStack.isEmpty())
        return {};

    MakefileLine line = m_fileStack.top().reader->readLine(m_bInlineFileMode);
    while (line.content.isNull()) {
        delete m_fileStack.top().reader;
        m_fileStack.pop();
        if (m_fileStack.isEmpty())
            break;
        line = m_fileStack.top().reader->readLine(m_bInlineFileMode);
    }
    return line;
}

static QString leftTrimmed(const QString &str)
{
    if (str.isEmpty())
        return str;
    int n = 0;
    for (; n < str.length() && str.at(n).isSpace(); ++n);
    return str.mid(n);
}

void Preprocessor::joinLines(MakefileLine &line, const MakefileLine &next)
{
    if (line.continuation == LineContinuationType::Backslash)
        line.content += QLatin1Char(' ');
    else if (line.continuation == LineContinuationType::Caret)
        line.content += QLatin1Char('\n');
    line.content += leftTrimmed(next.content);
    line.continuation = next.continuation;
}

void Preprocessor::joinPreprocessingDirectiveLines(MakefileLine &line, const MakefileLine &next)
{
    if (line.continuation == LineContinuationType::Caret)
        line.content += QLatin1Char('^');
    line.content += next.content;
    line.continuation = next.continuation;
}

void Preprocessor::completeLineImpl(MakefileLine &line, JoinFunc join)
{
    while (!isComplete(line)) {
        MakefileLine next = basicReadLine();
        if (next.content.isNull()) {
            line.continuation = LineContinuationType::None;
            break;
        }
        join(line, next);
    }
}

void Preprocessor::completeLine(MakefileLine &line)
{
    completeLineImpl(line, &joinLines);
}

void Preprocessor::completePreprocessingDirectiveLine(MakefileLine &line)
{
    completeLineImpl(line, &joinPreprocessingDirectiveLines);
}

bool Preprocessor::parseMacro(const QString& line)
{
    if (line.isEmpty())
        return false;

    static const QRegExp rex(QLatin1String("^(?:_|[a-z]|[0-9]|\\$)(?:[a-z]|[0-9]|\\$|=|\\()?"),
                             Qt::CaseInsensitive, QRegExp::RegExp2);
    if (rex.indexIn(line) != 0)
        return false;

    int equalsSignPos = -1;
    int parenthesisDepth = 0;
    for (int i=1; i < line.count(); ++i) {
        const QChar &ch = line.at(i);
        if (ch == QLatin1Char('(')) {
            ++parenthesisDepth;
        } else if (ch == QLatin1Char(')')) {
            --parenthesisDepth;
        } else if (parenthesisDepth == 0) {
            if (ch == QLatin1Char('=')) {
                equalsSignPos = i;
                break;
            } else if (ch == QLatin1Char(':')) {
                // A colon (outside parenthesis) to the left of an equals sign is not a valid
                // macro assignment. This is likely to be a description block.
                break;
            }
        }
    }

    if (equalsSignPos < 0)
        return false;

    QString name = line.left(equalsSignPos).trimmed();
    QString value = line.mid(equalsSignPos + 1).trimmed();
    removeInlineComments(value);
    //qDebug() << "parseMacro" << name << value;
    m_macroTable->setMacroValue(name, value);
    return true;
}

bool Preprocessor::parsePreprocessingDirective(const QString& line)
{
    QString directive, value;
    QString expandedLine = m_macroTable->expandMacros(line);
    if (!isPreprocessingDirective(expandedLine, directive, value))
        return false;

    if (directive == QLatin1String("CMDSWITCHES")) {
    } else if (directive == QLatin1String("ERROR")) {
        error(QLatin1Literal("ERROR: ") + value);
    } else if (directive == QLatin1String("MESSAGE")) {
        puts(qPrintable(value));
    } else if (directive == QLatin1String("INCLUDE")) {
        internalOpenFile(findIncludeFile(value));
    } else if (directive == QLatin1String("IF")) {
        bool followElseBranch = evaluateExpression(value) == 0;
        enterConditional(followElseBranch);
        if (followElseBranch) {
            skipUntilNextMatchingConditional();
        }
    } else if (directive == QLatin1String("IFDEF")) {
        bool followElseBranch = !m_macroTable->isMacroDefined(value);
        enterConditional(followElseBranch);
        if (followElseBranch) {
            skipUntilNextMatchingConditional();
        }
    } else if (directive == QLatin1String("IFNDEF")) {
        bool followElseBranch = m_macroTable->isMacroDefined(value);
        enterConditional(followElseBranch);
        if (followElseBranch) {
            skipUntilNextMatchingConditional();
        }
    } else if (directive == QLatin1String("ELSE")) {
        if (conditionalDepth() == 0)
            error(QLatin1String("unexpected ELSE"));
        if (!m_conditionalStack.top()) {
            skipUntilNextMatchingConditional();
        }
    } else if (directive == QLatin1String("ELSEIF")) {
        if (conditionalDepth() == 0)
            error(QLatin1String("unexpected ELSE"));
        if (!m_conditionalStack.top() || evaluateExpression(value) == 0) {
            skipUntilNextMatchingConditional();
        } else {
            m_conditionalStack.pop();
            m_conditionalStack.push(false);
        }
    } else if (directive == QLatin1String("ELSEIFDEF")) {
        if (conditionalDepth() == 0)
            error(QLatin1String("unexpected ELSE"));
        if (!m_conditionalStack.top() || !m_macroTable->isMacroDefined(value)) {
            skipUntilNextMatchingConditional();
        } else {
            m_conditionalStack.pop();
            m_conditionalStack.push(false);
        }
    } else if (directive == QLatin1String("ELSEIFNDEF")) {
        if (conditionalDepth() == 0)
            error(QLatin1String("unexpected ELSE"));
        if (!m_conditionalStack.top() || m_macroTable->isMacroDefined(value)) {
            skipUntilNextMatchingConditional();
        } else {
            m_conditionalStack.pop();
            m_conditionalStack.push(false);
        }
    } else if (directive == QLatin1String("ENDIF")) {
        exitConditional();
    } else if (directive == QLatin1String("UNDEF")) {
        m_macroTable->undefineMacro(value);
    } else {
        error(QString(QStringLiteral("Unknown preprocessor directive !%1")).arg(directive));
    }

    return true;
}

QString Preprocessor::findIncludeFile(const QString &filePathToInclude)
{
    QString filePath = filePathToInclude;
    bool angleBrackets = false;
    if (filePath.startsWith(QLatin1Char('<')) && filePath.endsWith(QLatin1Char('>'))) {
        angleBrackets = true;
        filePath.chop(1);
        filePath.remove(0, 1);
    }
    removeDoubleQuotes(filePath);

    QFileInfo fi(filePath);
    if (fi.exists())
        return fi.absoluteFilePath();

    // Search recursively through all directories of all parent makefiles.
    for (QStack<TextFile>::const_iterator it = m_fileStack.constEnd();
         it != m_fileStack.constBegin();) {
        --it;
        fi.setFile(it->fileDirectory + QLatin1Char('/') + filePath);
        if (fi.exists())
            return fi.absoluteFilePath();
    }

    if (angleBrackets) {
        // Search through all directories in the INCLUDE macro.
        const QString includeVar = m_macroTable->macroValue(QLatin1String("INCLUDE"))
                .replace(QLatin1Char('\t'), QLatin1Char(' '));
        const QStringList includeDirs = includeVar.split(QLatin1Char(';'), QString::SkipEmptyParts);
        foreach (const QString& includeDir, includeDirs) {
            fi.setFile(includeDir + QLatin1Char('/') + filePath);
            if (fi.exists())
                return fi.absoluteFilePath();
        }
    }

    const QString msg = QLatin1String("File %1 cannot be found.");
    error(msg.arg(filePathToInclude));
    return QString();
}

bool Preprocessor::isPreprocessingDirective(const QString& line, QString& directive, QString& value)
{
    if (line.isEmpty())
        return false;

    const QChar firstChar = line.at(0);
    if (isSpaceOrTab(firstChar))
        return false;

    bool oldStyleIncludeDirectiveFound = false;
    if (firstChar != QLatin1Char('!') && line.length() > 8) {
        const char ch = line.at(7).toLatin1();
        if (!isSpaceOrTab(QLatin1Char(ch)))
            return false;

        if (line.left(7).toLower() == QLatin1String("include"))
            oldStyleIncludeDirectiveFound = true;
        else
            return false;
    }

    bool result = true;
    if (oldStyleIncludeDirectiveFound) {
        directive = QLatin1String("INCLUDE");
        value = line.mid(8);
    } else {
        result = m_rexPreprocessingDirective.exactMatch(line);
        if (result) {
            directive = m_rexPreprocessingDirective.cap(1).toUpper();
            value = m_rexPreprocessingDirective.cap(2).trimmed();

            // Handle "!else if", "!else ifdef", "!else ifndef" as "!ELSEIF" etc.
            if (directive == QLatin1String("ELSE")) {
                static const QRegExp rex(QLatin1String("^(ifn?def|if)\\s+(.*)$"),
                                         Qt::CaseInsensitive);
                if (rex.exactMatch(value)) {
                    directive.append(rex.cap(1).toUpper());
                    value = rex.cap(2);
                }
            }
        }
    }

    value = m_macroTable->expandMacros(value);
    removeInlineComments(value);
    return result;
}

void Preprocessor::skipUntilNextMatchingConditional()
{
    uint depth = 0;
    MakefileLine line;
    QString directive, value;

    enum DirectiveToken { TOK_IF, TOK_ENDIF, TOK_ELSE, TOK_UNINTERESTING };
    DirectiveToken token;
    do {
        line = basicReadLine();
        if (line.content.isNull())
            return;

        if (line.content.startsWith(QLatin1Char('!')))
            completePreprocessingDirectiveLine(line);

        QString expandedLine = m_macroTable->expandMacros(line.content);
        if (!isPreprocessingDirective(expandedLine, directive, value))
            continue;

        if (directive == QLatin1String("ENDIF"))
            token = TOK_ENDIF;
        else if (directive.startsWith(QLatin1String("IF")))
            token = TOK_IF;
        else if (directive.startsWith(QLatin1String("ELSE")))
            token = TOK_ELSE;
        else
            token = TOK_UNINTERESTING;

        if (token == TOK_UNINTERESTING)
            continue;

        if (depth == 0) {
            if (token == TOK_ELSE) {
                m_linesPutBack.append(expandedLine);
                return;  // found the next matching ELSE
            }
            if (token == TOK_ENDIF) {
                exitConditional();
                return;  // found the next matching ENDIF
            }
        }

        if (token == TOK_ENDIF)
            --depth;
        else if (token == TOK_IF)
            ++depth;

    } while (!line.content.isNull());
}

void Preprocessor::enterConditional(bool followElseBranch)
{
    m_conditionalStack.push(followElseBranch);
}

void Preprocessor::exitConditional()
{
    if (m_conditionalStack.isEmpty())
        error(QLatin1String("unexpected ENDIF"));
    m_conditionalStack.pop();
}

int Preprocessor::evaluateExpression(const QString& expr)
{
    if (!m_expressionParser) {
        m_expressionParser = new PPExprParser;
        m_expressionParser->setMacroTable(m_macroTable);
    }

    if (!m_expressionParser->parse(qPrintable(m_macroTable->expandMacros(expr)))) {
        QString msg = QLatin1String("Can't evaluate preprocessor expression.");
        msg += QLatin1String("\nerror: ");
        msg += QString::fromLatin1(m_expressionParser->errorMessage());
        msg += QLatin1String("\nexpression: ");
        msg += expr;
        error(msg);
    }

    return m_expressionParser->expressionValue();
}

void Preprocessor::error(const QString& msg)
{
    throw FileException(msg, currentFileName(), lineNumber());
}

void Preprocessor::removeInlineComments(QString& line)
{
    int idx = -1;
    while (true) {
        idx = line.indexOf(QLatin1Char('#'), idx + 1);
        if (idx > 0 && line.at(idx - 1) == QLatin1Char('^')) {
            line.remove(idx - 1, 1);
            continue;
        }
        break;
    }
    if (idx >= 0) {
        line.truncate(idx);
        line = line.trimmed();
    }
}

} // namespace NMakeFile
