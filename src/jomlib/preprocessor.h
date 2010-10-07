/****************************************************************************
 **
 ** Copyright (C) 2008-2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include <QRegExp>
#include <QStack>
#include <QStringList>

class PPExprParser;

namespace NMakeFile {

class MacroTable;
class MakefileLineReader;

class Preprocessor
{
public:
    Preprocessor();
    ~Preprocessor();

    void setMacroTable(MacroTable* macroTable);
    MacroTable* macroTable() { return m_macroTable; }
    bool openFile(const QString& filename);
    QString readLine();
    uint lineNumber() const;
    QString currentFileName() const;
    int evaluateExpression(const QString& expr);

    static void removeInlineComments(QString& line);

private:
    bool internalOpenFile(QString fileName);
    void basicReadLine(QString& line);
    bool parseMacro(const QString& line);
    bool parsePreprocessingDirective(const QString& line);
    bool isPreprocessingDirective(const QString& line, QString& directive, QString& value);
    void skipUntilNextMatchingConditional();
    void error(const QString& msg);
    void enterConditional(bool followElseBranch);
    void exitConditional();
    int conditionalDepth() { return m_conditionalStack.count(); }

private:
    struct TextFile
    {
        MakefileLineReader* reader;
        QString fileDirectory;

        TextFile()
            : reader(0)
        {}

        TextFile(const TextFile& rhs)
            : reader(rhs.reader), fileDirectory(rhs.fileDirectory)
        {}

        TextFile& operator=(const TextFile& rhs)
        {
            reader = rhs.reader;
            fileDirectory = rhs.fileDirectory;
            return *this;
        }
    };

    QStack<TextFile>    m_fileStack;
    MacroTable*         m_macroTable;
    QRegExp             m_rexPreprocessingDirective;
    QStack<bool>        m_conditionalStack;
    PPExprParser*       m_expressionParser;
    QStringList         m_linesPutBack;
};

} //namespace NMakeFile
