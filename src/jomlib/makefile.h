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

#ifndef MAKEFILE_H
#define MAKEFILE_H

#include "fastfileinfo.h"
#include "macrotable.h"
#include <QStringList>
#include <QHash>
#include <QVector>

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

    void evaluateModifiers();

    QString m_commandLine;
    QList<InlineFile*> m_inlineFiles;
    unsigned int m_maxExitCode;  // greatest allowed exit code
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
     * Returns the makefile, this target belongs to.
     */
    Makefile* makefile() const { return m_pMakefile; }

    QStringList m_dependents;
    FileTime m_timeStamp;
    bool m_bFileExists;
    bool m_bVisitedByCycleCheck;
    bool m_bNoCyclesRootedHere;
    bool m_bVisitedByPreselectInferenceRules;
    QVector<InferenceRule*> m_inferenceRules;

    enum AddCommandsState { ACSUnknown, ACSEnabled, ACSDisabled };
    AddCommandsState m_canAddCommands;

private:
    void expandFileNameMacros(Command& command, int depIdx);
    void expandFileNameMacros(QString& str, int depIdx, bool dependentsForbidden);
    QStringList getFileNameMacroValues(const QStringRef& str, int& replacementLength, int depIdx,
                                       bool dependentsForbidden);

private:
    QString m_targetName;
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
    Makefile(const QString &fileName);
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
        DescriptionBlock* result = 0;
        const QString lowerName = name.toLower();
        result = m_targets.value(lowerName, 0);
        if (result)
            return result;

        QString systemName = lowerName;
        result = m_targets.value(systemName.replace(QLatin1Char('/'), QLatin1Char('\\')), 0);
        if (result)
            return result;

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

    const QVector<InferenceRule *>& inferenceRules() const
    {
        return m_inferenceRules;
    }

    const QString &fileName() const { return m_fileName; }
    const QString &dirPath() const;
    void setOptions(Options *o) { m_options = o; }
    const Options* options() const { return m_options; }
    void setParallelExecutionDisabled(bool disabled) { m_parallelExecutionDisabled = disabled; }
    bool isParallelExecutionDisabled() const { return m_parallelExecutionDisabled; }

    void setMacroTable(MacroTable *mt) { m_macroTable = mt; }
    const MacroTable* macroTable() const { return m_macroTable; }

    void dumpTarget(DescriptionBlock*, uchar level = 0) const;
    void dumpTargets() const;
    void dumpInferenceRules() const;
    void invalidateTimeStamps();
    void applyInferenceRules(QList<DescriptionBlock*> targets);
    void addInferenceRule(InferenceRule *rule);
    void calculateInferenceRulePriorities(const QStringList &suffixes);
    void addPreciousTarget(const QString& targetName);

private:
    void filterRulesByDependent(QVector<InferenceRule*>& rules, const QString& targetName);
    QStringList findInferredDependents(InferenceRule* rule, const QStringList& dependents);
    void applyInferenceRules(DescriptionBlock* target);
    void applyInferenceRule(DescriptionBlock* target, const InferenceRule *rule, bool applyingBatchMode = false);
    void applyInferenceRule(QList<DescriptionBlock*> &batch, const InferenceRule *rule);

private:
    QString m_fileName;
    mutable QString m_dirPath;
    DescriptionBlock* m_firstTarget;
    QHash<QString, DescriptionBlock*> m_targets;
    QStringList m_preciousTargets;
    QVector<InferenceRule *> m_inferenceRules;
    MacroTable* m_macroTable;
    Options* m_options;
    QSet<const InferenceRule*> m_batchModeRules;
    QMultiHash<const InferenceRule*, DescriptionBlock*> m_batchModeTargets;
    bool m_parallelExecutionDisabled;
};

} // namespace NMakeFile

#endif // MAKEFILE_H
