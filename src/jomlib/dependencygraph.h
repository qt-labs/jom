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

#ifndef DEPENDENCYGRAPH_H
#define DEPENDENCYGRAPH_H

#include <QtCore/QHash>
#include <QtCore/QSet>

namespace NMakeFile {

class DescriptionBlock;

class DependencyGraph
{
public:
    DependencyGraph();
    ~DependencyGraph();

    void build(DescriptionBlock* target);
    void markParentsRecursivlyUnbuildable(DescriptionBlock *target);
    bool isUnbuildable(DescriptionBlock *target) const;
    bool isEmpty() const;
    void removeLeaf(DescriptionBlock* target);
    DescriptionBlock *findAvailableTarget(bool ignoreTimeStamps);
    void dump();
    void dotDump();
    void clear();

private:
    bool isTargetUpToDate(DescriptionBlock* target);

    struct Node
    {
        enum State {UnknownState, ExecutingState, Unbuildable};

        State state;
        DescriptionBlock* target;
        QList<Node*> children;
        QList<Node*> parents;
    };

    Node* createNode(DescriptionBlock* target, Node* parent);
    void deleteNode(Node* node);
    void removeLeaf(Node* node);
    void internalBuild(Node *node, QSet<Node *> &seen);
    void addEdge(Node* parent, Node* child);
    void internalDump(Node* node, QString& indent);
    void internalDotDump(Node* node, const QString& parent);
    void displayNodeBuildInfo(Node* node, bool isUpToDate);
    static void markParentsRecursivlyUnbuildable(Node *node);

private:
    Node* m_root;
    QHash<DescriptionBlock*, Node*> m_nodeContainer;
    QList<Node *> m_leaves;
    bool m_bDirtyLeaves;
};

} // namespace NMakeFile

#endif // DEPENDENCYGRAPH_H
