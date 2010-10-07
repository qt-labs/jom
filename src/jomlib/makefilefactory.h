#pragma once

#include <QtCore/QStringList>

namespace NMakeFile {

class Makefile;

class MakefileFactory
{
public:
    MakefileFactory();
    void setEnvironment(const QStringList& env) { m_environment = env; }
    bool apply(const QStringList& commandLineArguments);

    enum ErrorType {
        NoError,
        CommandLineError,
        ParserError,
        IOError
    };

    Makefile* makefile() { return m_makefile; }
    const QStringList& activeTargets() const { return m_activeTargets; }
    const QString& errorString() const { return m_errorString; }
    ErrorType errorType() const { return m_errorType; }

private:
    void clear();

private:
    Makefile*   m_makefile;
    QStringList m_environment;
    QStringList m_activeTargets;
    QString     m_errorString;
    ErrorType   m_errorType;
};

} // namespace NMakeFile
