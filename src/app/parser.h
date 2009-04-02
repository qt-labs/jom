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
#pragma once

#include <QRegExp>
#include <QHash>
#include <QVector>
#include <QStack>
#include <QStringList>
#include <QSharedPointer>

#include "makefile.h"

namespace NMakeFile {

class Preprocessor;
class PPExpression;

class Parser
{
public:
    Parser();
    virtual ~Parser();

    Makefile* apply(Preprocessor* pp, const QStringList& activeTargets = QStringList());
    MacroTable* macroTable();

protected:
    void clear();

private:
    void readLine();
    bool isEmptyLine();
    bool isDescriptionBlock(int& separatorPos, int& separatorLength);
    bool isInferenceRule();
    bool isDotDirective();
    DescriptionBlock* createTarget(const QString& targetName);
    void parseDescriptionBlock(int separatorPos, int separatorLength);
    void parseInferenceRule();
    void parseDotDirective();
    bool parseCommand(QList<Command>& commands, bool inferenceRule);
    void parseInlineFile(Command& cmd);
    void checkForCycles(DescriptionBlock* target);
    void updateTimeStamps();
    void updateTimeStamp(DescriptionBlock* db);
    QList<InferenceRule*> findRulesByTargetExtension(const QString& suffixes);
    void filterRulesByTargetName(QList<InferenceRule*>& rules, const QString& targetName);
    void preselectInferenceRules();
    void preselectInferenceRules(const QString& targetName, QList<InferenceRule*>& rules, const QStringList& suffixes);
    void preselectInferenceRulesRecursive(DescriptionBlock* target);
    void error(const QString& msg);

private:
    Preprocessor*           m_preprocessor;
    QString                 m_line;
    bool                    m_silentCommands;
    bool                    m_ignoreExitCodes;

    QRegExp                 m_rexDotDirective;
    QRegExp                 m_rexInferenceRule;
    QRegExp                 m_rexSingleWhiteSpace;
    QRegExp                 m_rexInlineMarkerOption;

    Makefile                m_makefile;
    QSharedPointer<QStringList> m_suffixes;
    QStringList             m_activeTargets;

    uint                    m_conditionalDepth;
    bool                    m_followElseBranch;
    QDateTime               m_latestTimeStamp;
};

} // namespace NMakeFile
