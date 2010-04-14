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

#include "macrotable.h"
#include <QSharedPointer>
#include <QStringList>
#include <QDateTime>
#include <QHash>

namespace NMakeFile {

class InlineFile {
public:
    InlineFile();
    InlineFile(const InlineFile& rhs);

    bool m_keep;
    bool m_unicode;
    QString m_filename;   // optional
    QString m_content;
};

class Command {
public:
    Command();
    Command(const Command& rhs);
    ~Command();

    QString m_commandLine;
    QList<InlineFile*> m_inlineFiles;
    unsigned char m_maxExitCode;  // greatest allowed exit code
    bool m_silent;
    bool m_singleExecution;       // Execute this command for each dependent, if the command contains $** or $?.
};

class CommandContainer {
public:
    CommandContainer() {};
    CommandContainer(const CommandContainer& rhs)
        : m_commands(rhs.m_commands)
    {}

    QList<Command> m_commands;
};

class CommandExecutor;
class InferenceRule;

class DescriptionBlock : public CommandContainer {
public:
    DescriptionBlock();

    size_t id() const { return m_id; }
    bool isExecuting() const { return m_bExecuting; }
    void setExecuting() { m_bExecuting = true; }
    void expandFileNameMacros();

    void setTargetName(const QString& name);

    /**
     * Returns the name of the target.
     * This string may be surrounded by double quotes!
     */
    QString targetName() const
    {
        return m_targetName;
    }

    /**
     * Returns this target's name as a file name, i.e. unquoted.
     */
    QString targetFilePath() const
    {
        return m_targetFilePath;
    }

    QStringList m_dependents;
    QDateTime m_timeStamp;
    QSharedPointer<QStringList> m_suffixes;
    bool m_bFileExists;
    bool m_bVisitedByCycleCheck;
    QList<InferenceRule*> m_inferenceRules;

    enum AddCommandsState { ACSUnknown, ACSEnabled, ACSDisabled };
    AddCommandsState m_canAddCommands;

private:
    void expandFileNameMacros(Command& command, int depIdx);
    void expandFileNameMacros(QString& str, int depIdx);
    QString getFileNameMacroValue(const QStringRef& str, int& replacementLength, int depIdx);

private:
    static size_t m_nextId;
    size_t m_id;
    bool m_bExecuting;
    QString m_targetName;
    QString m_targetFilePath;
};

class InferenceRule : public CommandContainer {
public:
    InferenceRule();
    InferenceRule(const InferenceRule& rhs);

    bool operator == (const InferenceRule& rhs) const;

    bool m_batchMode;
    QString m_fromSearchPath;
    QString m_fromExtension;
    QString m_toSearchPath;
    QString m_toExtension;
};

class Makefile
{
public:
    Makefile();
    ~Makefile()
    {
    }

    void clear();
    void setMacroTable(MacroTable* macroTable) { m_macroTable = macroTable; }

    void append(DescriptionBlock* target)
    {
        m_targets[target->targetName()] = target;
        if (!m_firstTarget) m_firstTarget = target;
    }

    DescriptionBlock* firstTarget()
    {
        return m_firstTarget;
    }

    DescriptionBlock* target(const QString& name) const
    {
        DescriptionBlock* result = m_targets.value(name, 0);
        if (!result) {
            QString alternativeName = name;
            if (name.startsWith('"') && name.endsWith('"')) {
                alternativeName.remove(0, 1);
                alternativeName.chop(1);
            } else {
                alternativeName.prepend('"');
                alternativeName.append('"');
            }
            result = m_targets.value(alternativeName, 0);
        }
        return result;
    }

    const QHash<QString, DescriptionBlock*>& targets() const
    {
        return m_targets;
    }

    const QList<QStringList>& suffixes()
    {
        return m_suffixesLists;
    }

    const QStringList& preciousTargets()
    {
        return m_preciousTargets;
    }

    const QList<InferenceRule>& inferenceRules() const
    {
        return m_inferenceRules;
    }

    void dumpTarget(DescriptionBlock*, uchar level = 0) const;
    void dumpTargets() const;
    void dumpInferenceRules() const;
    void invalidateTimeStamps();
    void updateTimeStamps(DescriptionBlock* target);
    void applyInferenceRules(DescriptionBlock* target);
    void addInferenceRule(const InferenceRule& rule);
    void addPreciousTarget(const QString& targetName);

private:
    void filterRulesByDependent(QList<InferenceRule*>& rules, const QString& targetName);
    void sortRulesBySuffixes(QList<InferenceRule*>& rules, const QStringList& suffixes);
    QStringList findInferredDependents(InferenceRule* rule, const QStringList& dependents);
    void applyInferenceRule(DescriptionBlock* target, const InferenceRule* rule);

private:
    DescriptionBlock* m_firstTarget;
    QHash<QString, DescriptionBlock*> m_targets;
    QList<QStringList> m_suffixesLists;
    QStringList m_preciousTargets;
    QList<InferenceRule> m_inferenceRules;
    MacroTable* m_macroTable;
};

} // namespace NMakeFile
