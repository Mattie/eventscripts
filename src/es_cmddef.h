#ifndef ES_MACROS_H
#define ES_MACROS_H


// This file defines a number of custom macros and functions that ES 
//   uses for different variations of console commands.

#include "es_defines.h"
#include "es_debug.h"
#include "es_util.h"
#include "convar.h"
#ifndef ORANGEBOX
#include "VirtualMachine/BlockArgs.h"
#endif

#ifdef PYTHON_SUPPORT
#include "Python.h"
#endif

extern const CCommand * g_pCmdArg;

#ifdef ORANGEBOX
#define DISPATCH_ARGS args
#define DISPATCH_PARAM const CCommand &args
#define DISPATCH_PARAM_COMMA ,DISPATCH_PARAM
#else
#define DISPATCH_ARGS
#define DISPATCH_PARAM 
#define DISPATCH_PARAM_COMMA ,DISPATCH_PARAM
#endif


#ifdef PYTHON_SUPPORT

extern PyMethodDef g_pEsMethods[];

// This special class is used to quickly add Python commands
//   which will be autoregistered later.
class PyCommand
{
public:
    PyCommand(char *name, PyCFunction pFunc, unsigned int method, char *desc)
    {
        static int count = 0;
        g_pEsMethods[count].ml_name = name;
        g_pEsMethods[count].ml_meth = pFunc;
        g_pEsMethods[count].ml_flags = method;
        g_pEsMethods[count].ml_doc = desc;
        count++;
        g_pEsMethods[count].ml_name = 0;
        g_pEsMethods[count].ml_meth = 0;
        g_pEsMethods[count].ml_flags = 0;
        g_pEsMethods[count].ml_doc = 0;
    }
};
extern PyObject * g_pPyNone;
extern PyObject * g_pPyReturn;

char *GetPythonCommandLine(char *cmdname, PyObject *args);

// Handy define to register a typical vararg Python method with the PyCommand class and g_pEsMethods array
#define PY_COMMAND_VARARGS(name, description) \
static PyObject* espy_##name (PyObject *self, PyObject *args); \
static PyCommand espy_##name##_register(#name, espy_##name, METH_VARARGS,description); \
static PyObject* espy_##name(PyObject *self, PyObject *args)

#define PYTHON_CON_COMMAND(name, description) \
    PY_COMMAND_VARARGS(name, description) { \
    char *szPythonCommandLine = GetPythonCommandLine("es_x"#name, args); \
	CCommand goteam; \
	goteam.Tokenize(szPythonCommandLine); \
    g_pPyReturn = NULL; \
    do_es_x##name(goteam, false); \
	free(szPythonCommandLine); \
    if (g_pPyReturn) return g_pPyReturn; \
    Py_INCREF(g_pPyNone); \
    return g_pPyNone;\
}
#else
#define PYTHON_CON_COMMAND(name, description) ;
#endif

#ifdef WIN32
// allows a special name for the resulting function.
#define ES_CON_COMMAND4_PREFIX( name, mintokens, syntax) \
void do_es_x##name(const CCommand &args, bool bUseServerVariablesForReturn) { \
        g_pCmdArg = &args; \
    DxMsg(5, __FUNCTION__, __LINE__, "Command: %s %s;\n", args.Arg(0), args.ArgS()); \
    int nCount = args.ArgC(); \
    if (bUseServerVariablesForReturn && nCount < mintokens) { \
    DxMsg(0, __FUNCTION__, __LINE__, "Syntax: %s " ##syntax "\n", args.Arg(0)); ErrVar("Syntax error"); return; }
#else
// allows a special name for the resulting function.
#define ES_CON_COMMAND4_PREFIX( name, mintokens, syntax) \
void do_es_x##name(const CCommand &args, bool bUseServerVariablesForReturn) { \
        g_pCmdArg = &args; \
    DxMsg(5, __FUNCTION__, __LINE__, "Command: %s;\n", args.ArgS()); \
    int nCount = args.ArgC(); \
    if (bUseServerVariablesForReturn && nCount < mintokens) { \
    DxMsg(0, __FUNCTION__, __LINE__, "Syntax: %s " #syntax "\n", args.Arg(0)); ErrVar("Syntax error"); return; }
#endif

// FIX THIS FOR NORANGEBOX


#ifndef ORANGEBOX
#ifdef WIN32
#define ES_CON_COMMAND4_NON_ESX(name, name2, description) \
static ConCommand es_##name##_command( #name2, old_es_##name, "" ##description " Expands event and server variables."); 
#else
#define ES_CON_COMMAND4_NON_ESX(name, name2, description) \
static ConCommand es_##name##_command( #name2, es_##name, #description " Expands event and server variables."); 
#endif

// this version allows you to provide a different es_cmd name (e.g. "if") in name2
#define ES_CON_COMMAND4( name, name2, description, mintokens, syntax) \
void do_es_x##name(const CCommand &args, bool bUseServerVariablesForReturn); \
/*	prototype an old FnCommandCallback wrapper */ \
static void old_es_x##name(); \
static void es_x##name(const CCommand &args); \
/*	call the old FnCommandCallback wrapper */ \
static ConCommand es_x##name##_command( "es_x" #name, old_es_x##name, description ); \
static void es_x##name(const CCommand &args) { do_es_x##name(args, true); } \
static void old_es_x##name() {  \
	CCommand args; \
	char szPreparseCommand[MAX_COMMAND_STRING]; \
    sprintf(szPreparseCommand, "%s %s\n", engine->Cmd_Argv(0), engine->Cmd_Args()); \
	args.Tokenize(szPreparseCommand); \
	es_x##name(args); \
} \
static void es_##name(const CCommand &args); \
static void old_es_##name(); \
static void old_es_##name() {  \
CCommand args; \
char szPreparseCommand[MAX_COMMAND_STRING]; \
sprintf(szPreparseCommand, "%s %s\n", engine->Cmd_Argv(0), engine->Cmd_Args()); \
args.Tokenize(szPreparseCommand); \
es_##name(args); \
} \
ES_CON_COMMAND4_NON_ESX(name, name2, description) \
static void es_##name(const CCommand &args) { \
	g_pCmdArg = &args; \
	char szPreparseCommand[MAX_COMMAND_STRING]; \
    sprintf(szPreparseCommand, "%s %s\n", args.Arg(0), args.ArgS()); \
    char szExpandCommand[MAX_COMMAND_STRING]; \
    szExpandCommand[0]='\0'; \
    if (ParseCommand(args, szExpandCommand, 0, args.ArgC(),'\0')>0)  { \
		CCommand goteam; \
		goteam.Tokenize(szExpandCommand); \
        do_es_x##name(goteam, true); \
        return; \
    } \
    else    { \
        ErrMsg(0,"es: Error parsing command string: %s\n", args.ArgS()); \
        ErrVar("Error while parsing command string."); \
        return; \
    } \
} 
#else
#ifdef WIN32
#define ES_CON_COMMAND4_NON_ESX(name, name2, description) \
static ConCommand es_##name##_command( #name2, es_##name, "" ##description " Expands event and server variables."); 
#else
#define ES_CON_COMMAND4_NON_ESX(name, name2, description) \
static ConCommand es_##name##_command( #name2, es_##name, #description " Expands event and server variables."); 
#endif

// this version allows you to provide a different es_cmd name (e.g. "if") in name2
#define ES_CON_COMMAND4( name, name2, description, mintokens, syntax) \
void do_es_x##name(const CCommand &args, bool bUseServerVariablesForReturn); \
static void es_x##name(const CCommand &args); \
static ConCommand es_x##name##_command( "es_x" #name, es_x##name, description ); \
static void es_x##name(const CCommand &args) { do_es_x##name(args, true); } \
static void es_##name(const CCommand &args); \
ES_CON_COMMAND4_NON_ESX(name, name2, description) \
static void es_##name(const CCommand &args) { \
	g_pCmdArg = &args; \
	char szPreparseCommand[MAX_COMMAND_STRING]; \
    sprintf(szPreparseCommand, "%s %s\n", args.Arg(0), args.ArgS()); \
    char szExpandCommand[MAX_COMMAND_STRING]; \
    szExpandCommand[0]='\0'; \
    if (ParseCommand(args, szExpandCommand, 0, args.ArgC(),'\0')>0)  { \
		CCommand goteam; \
		goteam.Tokenize(szExpandCommand); \
        do_es_x##name(goteam, true); \
        return; \
    } \
    else    { \
        ErrMsg(0,"es: Error parsing command string: %s\n", args.ArgS()); \
        ErrVar("Error while parsing command string."); \
        return; \
    } \
}
#endif
// Used for defining new EventScripts console commands. Will declare two CON_COMMANDS
//   the es_blah and es_xblah variations.
// This version does the in-line parsing.
#define ES_CON_COMMAND2( name, description, mintokens, syntax ) \
    ES_CON_COMMAND4( name, es_##name, description, mintokens, syntax ) \
    PYTHON_CON_COMMAND(name, description) \
    ES_CON_COMMAND4_PREFIX( name, mintokens, syntax )

// Used when you don't want a Python command.
#define ES_CON_COMMAND3( name, description, mintokens, syntax ) \
    ES_CON_COMMAND4( name, es_##name, description, mintokens, syntax ) \
    ES_CON_COMMAND4_PREFIX( name, mintokens, syntax )

// Used when you want to specify what your es_ name is and don't want Python.
#define ES_CON_COMMAND5( name, name2, description, mintokens, syntax ) \
    ES_CON_COMMAND4( name, name2, description, mintokens, syntax ) \
    ES_CON_COMMAND4_PREFIX( name, mintokens, syntax )

#define RETURNVAL(varname, returnval, type) \
    if (bUseServerVariablesForReturn) \
    { \
        ConVar * cvar = NULL; \
        cvar = SetConVar( varname, returnval); \
        if (!cvar){ \
            DxMsg(0, __FUNCTION__, __LINE__,"The var \"%s\" could not be set\n", varname);  \
            return; \
        } \
        else \
        { \
            DxMsg(1, __FUNCTION__, __LINE__,"%s = %s\n", cvar->GetName(), cvar->GetString()); \
        } \
    } \
    else \
    { \
        g_pPyReturn = Py_BuildValue(type, returnval); \
    }

#ifdef WIN32
#define RETURNVAL3_BUILD(returnval1, type1, returnval2, type2, returnval3,type3) g_pPyReturn = Py_BuildValue(type1##type2##type3, returnval1, returnval2, returnval3);
#else
#define RETURNVAL3_BUILD(returnval1, type1, returnval2, type2, returnval3,type3) \
   g_pPyReturn = Py_BuildValue(type1 type2 type3, returnval1, returnval2, returnval3); \
   DxMsg(6, __FUNCTION__, __LINE__, "RETURNVAL3 BUILD for '%s' g_pPyReturn: %d", type1 type2 type3, g_pPyReturn);
#endif

#define RETURNVAL3(varname1, returnval1, type1, varname2, returnval2, type2, varname3, returnval3,type3) \
    if (bUseServerVariablesForReturn) \
    { \
        ConVar * cvar = NULL; \
        cvar = SetConVar( varname1, returnval1); \
        if (!cvar){ \
            DxMsg(0, __FUNCTION__, __LINE__,"The var \"%s\" could not be set\n", varname1);  \
            return; \
        } \
        else \
        { \
            DxMsg(1, __FUNCTION__, __LINE__,"%s = %s\n", cvar, cvar->GetString()); \
        } \
        cvar = SetConVar( varname2, returnval2); \
        if (!cvar){ \
            DxMsg(0, __FUNCTION__, __LINE__,"The var \"%s\" could not be set\n", varname3);  \
            return; \
        } \
        else \
        { \
            DxMsg(1, __FUNCTION__, __LINE__,"%s = %s\n", cvar, cvar->GetString()); \
        } \
        cvar = SetConVar( varname3, returnval3); \
        if (!cvar){ \
            DxMsg(0, __FUNCTION__, __LINE__,"The var \"%s\" could not be set\n", varname3);  \
            return; \
        } \
        else \
        { \
            DxMsg(1, __FUNCTION__, __LINE__,"%s = %s\n", cvar->GetName(), cvar->GetString()); \
        } \
    } \
    else \
    { \
        RETURNVAL3_BUILD(returnval1, type1, returnval2, type2, returnval3,type3) \
    }
#define CHANGE_FIRST_ARG_INDEX_FOR_N_RETURNVALS(num_return_vars) \
    int nFirstArg = bUseServerVariablesForReturn ? 0 : -num_return_vars; \
    nCount+=abs(nFirstArg);


#ifdef ORANGEBOX
#define ARGV(val) args.Arg(val)
#define ARGS() args.ArgS()
#define ARGC() args.ArgC()
#else
#define ARGV(val) engine->Cmd_Argv(val)
#define ARGS() engine->Cmd_Args()
#define ARGC() engine->Cmd_Argc()
#endif

#ifdef ORANGEBOX
#define PARSE_ARGS_FROM_ENGINE()
#else
#define PARSE_ARGS_FROM_ENGINE() \
	CCommand args; \
	char szPreparseCommandx[MAX_COMMAND_STRING]; \
	sprintf(szPreparseCommandx, "%s %s\n", ARGV(0), ARGS()); \
	args.Tokenize(szPreparseCommandx);
#endif

int ParseCommand(const CCommand &args, char * szDestination, int nToken, int nMaxTokens, char appendChar = '\n');




#endif




