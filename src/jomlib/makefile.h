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

#include "fileinfo.h"
#include "macrotable.h"
#include <QStringList>
#include <QHash>

namespace NMakeFile {

class Options;

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
class Makefile;

class DescriptionBlock : public CommandContainer {
public:
    DescriptionBlock(Makefile* mkfile);

    void expandFileNameMacrosForDependents();
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

    /**
     * Returns the makefile, this target belongs to.
     */
    Makefile* makefile() const { return m_pMakefile; }

    QStringList m_dependents;
    FileTime m_timeStamp;
    bool m_bFileExists;
    bool m_bVisitedByCycleCheck;
    QList<InferenceRule*> m_inferenceRules;

    enum AddCommandsState { ACSUnknown, ACSEnabled, ACSDisabled };
    AddCommandsState m_canAddCommands;

private:
    void expandFileNameMacros(Command& command, int depIdx);
    void expandFileNameMacros(QString& str, int depIdx, bool dependentsForbidden);
    QString getFileNameMacroValue(const QStringRef& str, int& replacementLength, int depIdx,
                                  bool dependentsForbidden, bool& returnsFileName);

private:
    QString m_targetName;
    QString m_targetFilePath;
    Makefile* m_pMakefile;
};

class InferenceRule : public CommandContainer {
public:
    InferenceRule();
    InferenceRule(const InferenceRule& rhs);

    bool operator == (const InferenceRule& rhs) const;

    QString inferredDependent(const QString &targetName) const;

    bool m_batchMode;
    QString m_fromSearchPath;
    QString m_fromExtension;
    QString m_toSearchPath;
    QString m_toExtension;
    int m_priority; // priority < 0 means: not applicable
};

class Makefile
{
public:
    Makefile();
    ~Makefile();

    void clear();

    void append(DescriptionBlock* target)
    {
        m_targets[target->targetName().toLower()] = target;
        if (!m_firstTarget) m_firstTarget = target;
    }

    DescriptionBlock* firstTarget()
    {
        return m_firstTarget;
    }

    DescriptionBlock* target(const QString& name) const
    {
        const QString lowerName = name.toLower();
        DescriptionBlock* result = m_targets.value(lowerName, 0);
        if(!result) {
            QString systemName = lowerName;
            result = m_targets.value(systemName.replace(QLatin1Char('/'), QLatin1Char('\\')), 0);
        }
        if (!result) {
            QString alternativeName = lowerName;
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

    const QStringList& preciousTargets() const
    {
        return m_preciousTargets;
    }

    const QList<InferenceRule>& inferenceRules() const
    {
        return m_inferenceRules;
    }

    void setOptions(Options *o) { m_options = o; }
    const Options* options() const { return m_options; }

    void setMacroTable(MacroTable *mt) { m_macroTable = mt; }
    const MacroTable* macroTable() const { return m_macroTable; }

    void dumpTarget(DescriptionBlock*, uchar level = 0) const;
    void dumpTargets() const;
    void dumpInferenceRules() const;
    void invalidateTimeStamps();
    void applyInferenceRules(QList<DescriptionBlock*> targets);
    void addInferenceRule(const InferenceRule& rule);
    void calculateInferenceRulePriorities(const QStringList &suffixes);
    void addPreciousTarget(const QString& targetName);

private:
    void filterRulesByDependent(QList<InferenceRule*>& rules, const QString& targetName);
    QStringList findInferredDependents(InferenceRule* rule, const QStringList& dependents);
    void applyInferenceRules(DescriptionBlock* target);
    void applyInferenceRule(DescriptionBlock* target, const InferenceRule *rule, bool applyingBatchMode = false);
    void applyInferenceRule(QList<DescriptionBlock*> &batch, const InferenceRule *rule);

private:
    DescriptionBlock* m_firstTarget;
    QHash<QString, DescriptionBlock*> m_targets;
    QStringList m_preciousTargets;
    QList<InferenceRule> m_inferenceRules;
    MacroTable* m_macroTable;
    Options* m_options;
    QSet<const InferenceRule*> m_batchModeRules;
    QMultiHash<const InferenceRule*, DescriptionBlock*> m_batchModeTargets;
};

} // namespace NMakeFile
