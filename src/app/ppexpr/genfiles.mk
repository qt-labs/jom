all: ppexpr_grammar.cpp

ppexpr-lex.incl: ppexpr.l
    flex ppexpr.l

ppexpr_grammar.cpp: ppexpr.g ppexpr-lex.incl
    qlalr ppexpr.g

