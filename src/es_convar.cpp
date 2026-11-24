//=============================================================================//
//
// Mattie's EventScripts
//
// The MIT License
//
// Copyright © 2009, Mattie Casper
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// Except as contained in this notice, the name(s) of the above copyright holders 
// shall not be used in advertising or otherwise to promote the sale, use or other 
// dealings in this Software without prior written authorization.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
//
//=============================================================================//
#include "commonmacros.h"
#include "memalloc.h"
#include "icvar.h"
#include "es_cmddef.h"
#include "es_convar.h"
#include "es_util.h"
#include "es_debug.h"
#include "utlhash.h"
#include "eiface.h"
#include "es_strutils.h"

extern IVEngineServer * engine;
extern ICvar * cvarmanager;
//extern ICvar * g_pCVar;
#ifndef ORANGEBOX
ICvar *g_pCVar;

class CPluginConVarAccessor : public IConCommandBaseAccessor
{
public:
	virtual bool	RegisterConCommandBase( ConCommandBase *pCommand )
	{
		pCommand->AddFlags( FCVAR_PLUGIN );

		// Unlink from plugin only list
		pCommand->SetNext( 0 );

		// Link to engine's list instead
		g_pCVar->RegisterConCommandBase( pCommand );
		return true;
	}

};

CPluginConVarAccessor g_ConVarAccessor;

void InitCVars( CreateInterfaceFn cvarFactory )
{
	g_pCVar = (ICvar*)cvarFactory( VENGINE_CVAR_INTERFACE_VERSION, NULL );
	if ( g_pCVar )
	{
		ConCommandBaseMgr::OneTimeInit( &g_ConVarAccessor );
	}
}

#endif

ConVar * SetConVar(const char * szName, const char * szValue, bool bCreate)
{
    ConVar *temp = GetVar(szName, bCreate);

    if (temp)
    {
        if (CanChange(temp))
        {
            if (szValue) 
            {
                temp->SetValue(szValue);
				DxMsg(4, __FUNCTION__, __LINE__, "\"%s\" set to \"%s\"\n", szName, szValue);
            }
            else
            {
                temp->SetValue("");
				DxMsg(4, __FUNCTION__, __LINE__, "\"%s\" set to \"\"\n", szName);
            }
        }
		else 
		{
			DxMsg(1, __FUNCTION__, __LINE__, "Can't change variable: %s\n", szName);
		}

    }
    return temp;
}

static ConVar autocreate_cvar("eventscripts_autocreate", 0, 0, "Automatically create new console variables when EventScripts commands are used on nonexistent variables.");
ConVar * GetVarDescription(const char * szName, const char *szDescription, bool bCreate)
{
    ConVar *test = FindVar(szName);
    if (!test)
    {
        if (bCreate || autocreate_cvar.GetBool())
        {
            return CreateVar(szName, "", 0, strdup(szDescription));
         }
        else
        {
            ErrMsg(0,"ERROR: Variable does not exist.\n");
            ErrVar("Variable does not exist.");
        }
    }
    return test;
}

ConVar * GetVar(const char * szName, bool bCreate)
{
    return GetVarDescription(szName, "Custom server variable.", bCreate);
}

// protects against changing cheat server_vars unless on dedicated server
bool CanChange(ConCommandBase * var)
{
    if (var->m_nFlags & FCVAR_CHEAT)
    {
        if (engine->IsDedicatedServer() || FindVar("sv_cheats")->GetInt())
        {
            return true;
        }
        else
        {
			ErrMsg(0,"WARNING: Cannot change a cheat variable/command (%s) while sv_cheats = 0 (requires dedicated server).\n", var->GetName());
            return false;
        }
    }
    else
    {
        if (FStrEq(var->GetName(), "sv_cheats") && !engine->IsDedicatedServer())
        {
			ErrMsg(0,"WARNING: Cannot change sv_cheats on a non-dedicated server.\n");
            return false;
        }
        else
        {
            return true;
        }
    }
}

ConVar * SetConVarDescription(const char * szName, const char * szValue, const char * szDescription, bool bCreate)
{
    ConVar *temp = GetVarDescription(szName, szDescription, bCreate);
    if (temp)
    {
        if (CanChange(temp))
        {
            if (szValue) 
            {
                temp->SetValue(szValue);
            }
            else
            {
                temp->SetValue("");
            }
        }

    }
    return temp;
}

ConVar * SetConVar(const char * szName, int nValue, bool bCreate)
{
    // bug where 0 values can't be set if value is a string
    if (nValue == 0) return SetConVar(szName, "0", bCreate);
    
    ConVar *temp = GetVar(szName, bCreate);
    if (temp)
    {
        if (CanChange(temp))
        {
            temp->SetValue(nValue);
			DxMsg(4, __FUNCTION__, __LINE__, "\"%s\" set to \"%d\"\n", szName, nValue);
        }
    }
    return temp;
}

ConVar * SetConVar(const char * szName, float fValue, bool bCreate)
{
    // bug where 0 values can't be set if value is a string
    if (fValue == 0) return SetConVar(szName, "0.0", bCreate);

    ConVar *temp = GetVar(szName, bCreate);
    if (temp)
    {
        if (CanChange(temp))
        {
            temp->SetValue(fValue);
			DxMsg(4, __FUNCTION__, __LINE__, "\"%s\" set to \"%f\"\n", szName, fValue);
        }
    }
    return temp;
}

void SetConVarVal(const char * szName, const char *szValue)
{
    ConVar* cvar = SetConVar( szName, szValue); 
    if (!cvar){
        DxMsg(0, __FUNCTION__, __LINE__,"The var \"%s\" could not be set\n", szName); 
        return; // If it doesn t find the var
    }
    else
    {
        DxMsg(1, __FUNCTION__, __LINE__,"%s = '%s'.\n", szName, szValue);
    }
}

// use a hash table to store the name of console variables
struct HashData {
    char * name;
    ConVar * pVar;
};

bool HashDataCompare(const HashData &p1, const HashData &p2)
{
    return (FStrEq(p1.name, p2.name));
}
unsigned int HashKeyFunc(const HashData &p)
{
    char *str = p.name;

    unsigned int hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash;
}
CUtlHash<HashData> g_ConVarHashTable(65536,0,0,HashDataCompare, HashKeyFunc);

ConVar * FindVar(const char * name)
{
    if (name==NULL)
    {
        return NULL;
    }
    HashData pFind;
    pFind.name = (char*)name;
    pFind.pVar = NULL;
    ConVar * ctest = NULL;
    UtlHashHandle_t hKey = g_ConVarHashTable.Find(pFind);
    if (hKey != g_ConVarHashTable.InvalidHandle())
    {
        ctest = g_ConVarHashTable.Element(hKey).pVar;
    }
    else
    {
        ctest = g_pCVar->FindVar(name);
        if (ctest)
        {
            HashData *pNew = new HashData();
            pNew->name = strdup(name);
            pNew->pVar = ctest;
            g_ConVarHashTable.Insert(*pNew);
        }
    }
    return ctest;
}

ConVar * CreateVar(const char *szName, const char *szDefault, int flags, const char *szDescription)
{
    char * str = strdup(szName);
    char * def = strdup(szDefault);
    ConVar *test =new ConVar(str, def, flags, szDescription);
    if (!test)
    {
        if (str)
        {
            free(str);
        }
        if (def)
        {
            free(def);
        }
        DxMsg(0, __FUNCTION__, __LINE__,"Wild strange error creating ConVar: %s.\n", szName);
        ErrVar("Wild strange error creating ConVar");
        return NULL;
    }

    // add it to the hash table, too.
    HashData *pNew = new HashData();
    pNew->name = str;
    pNew->pVar = test;
    g_ConVarHashTable.Insert(*pNew);
    return test;
}


//-----------------------------------------------------------------------------
// Purpose: Forces the change of a ConVar's string value
//-----------------------------------------------------------------------------
void ChangeStringValue(ConVar * cvar, char const *tempVal )
{
    int len = strlen(tempVal) + 1;

#ifdef ALIENSWARM
    ConVar::CVValue_t * cvardata = &(cvar->GetRawValue());
#else
    ConVar *cvardata = cvar;
#endif
	if ( len > cvardata->m_StringLength)
	{
		if (cvardata->m_pszString)
			delete[] cvardata->m_pszString;

		cvardata->m_pszString	= new char[len];
		cvardata->m_StringLength = len;
	}

	memcpy( cvardata->m_pszString, tempVal, len );
}



ES_CON_COMMAND2(set, "Adds/sets a new server/global variable.", 3, "<key> <value> [description]")
	//DxMsg(0, __FUNCTION__, __LINE__, "es_set: %d, %s, %s\n", nCount, args.Arg(0), args.Arg(1));
	if (nCount>3)
    {
        SetConVarDescription(args.Arg(1), args.Arg(2), args.Arg(3), true);
    }
    else
    {
        SetConVar(args.Arg(1), args.Arg(2), true);
    }
}

void ForceGlobalChangeCallbacks(const char *varname)
{
#ifdef ORANGEBOX
    ConVar * cvar = FindVar(varname);
    if (cvar)
    {
        cvarmanager->CallGlobalChangeCallbacks(cvar, cvar->GetString(), cvar->GetFloat());
    }
    else
    {
        DxMsg(1, __FUNCTION__, __LINE__,"The var \"%s\" could not be found.\n", varname);
    }
#endif
}

ES_CON_COMMAND2(forcecallbacks, "Calls all global convar callbacks for a particular server variable.", 2, "<varname>")

    ForceGlobalChangeCallbacks(args.Arg(1));
}



ES_CON_COMMAND2(setinfo, 
                "Adds a new server/global variable.",
                3,
                "<variable> <value>")
    SetConVar(args.Arg(1), args.Arg(2), true);
}
// only register this on Linux
#ifndef _WIN32
static ConCommand setinfo_command( "setinfo", es_xsetinfo, "Adds a new user info value\n");
#endif


ES_CON_COMMAND2(copy,
                "Reads the server variable referenced by varname2 and copies it into the variable referenced by varname.\n",
                3,
                "<varname> <varname2>")
    if (args.ArgC() != 3)
    {
        DxMsg(0, __FUNCTION__, __LINE__,"Syntax: es_xcopy <varname> <varname2>\n");
        ErrVar("Not enough arguments.");
        return;
    }
    if (FStrEq(args.Arg(1),"") || FStrEq(args.Arg(2),""))
    {
        DxMsg(0, __FUNCTION__, __LINE__,"Specify a variable!\n"); // If i made a findvar with a non specified arg that cause crashs
        ErrVar("Not enough arguments.");
        return;
    }
    ConVar* cvar2 = FindVar(args.Arg(2));
    if (!cvar2)
    {
        DxMsg(0, __FUNCTION__, __LINE__,"The var \"%s\" could not be found\n", args.Arg(2)); 
        return; // If it doesn t find the var
    }
    ConVar* cvar = SetConVar( args.Arg(1), cvar2->GetString());
    if (!cvar)
    {
        DxMsg(0, __FUNCTION__, __LINE__,"The var \"%s\" could not be set.\n", args.Arg(1)); 
        return; // If it doesn t find the var
    }
    else
    {
        DxMsg(1, __FUNCTION__, __LINE__,"%s = \"%s\"\n",cvar->GetName(), cvar->GetString());
    }
}



ES_CON_COMMAND2(forcevalue, 
               "Forces a variable to a particular value\n",
               3,
               "<var> <value>")
    ConVar* cvar = FindVar(args.Arg(1));

    if (!cvar){
        DxMsg(0, __FUNCTION__, __LINE__,"The var \"%s\" could not be found\n", args.Arg(1)); 
        return; // If it doesn t find the var
    }
    else
    {
		if (!CanChange(cvar)){
			return;
		}
        const char * value = args.Arg(2);
	    float fNewValue;
	    char  *val;
	    val = (char *)value;
	    fNewValue = ( float )atof( value );
#ifdef ALIENSWARM
    ConVar::CVValue_t * cvardata = &(cvar->GetRawValue());
#else
    ConVar *cvardata = cvar;
#endif
	    // Redetermine value
	    cvardata->m_fValue		= fNewValue;
	    cvardata->m_nValue		= ( int )( cvardata->m_fValue );


		if ( !(cvar->m_nFlags & FCVAR_NEVER_AS_STRING) )
		    ChangeStringValue( cvar, val );

    }
}

ES_CON_COMMAND2(makepublic, 
               "Makes a console variable public such that changes to it are announced to clients.\n",
               2,
               "<var>")
    ConVar* cvar = FindVar(args.Arg(1));
  
    if (!cvar)
    {
        DxMsg(0, __FUNCTION__, __LINE__,"The var \"%s\" could not be found\n", args.Arg(1)); 
        ErrVar("Variable couldn't be found.");
    } 
    else
    {
        cvar->AddFlags(FCVAR_NOTIFY);        
        ForceGlobalChangeCallbacks(args.Arg(1));
    }
    return;
}

bool ForEachConCommandBase(bool (*callback)(ConCommandBase *))
{
#ifdef LEFT4DEAD
    ICvar::Iterator iter(g_pCVar); 
	for ( iter.SetFirst() ; iter.IsValid() ; iter.Next() )
    {  
	    ConCommandBase *cbase = iter.Get();
        if (!callback(cbase))
            return false;
	} 
#else
    ConCommandBase *cbase = cvarmanager->GetCommands();
    while (cbase != NULL)
    {
        if (!callback(cbase))
            return false;
        cbase = cbase->GetNext();
    }
#endif
    return true;
}

int _dumpcount = 0;
int _dumpcmdcount = 0;
bool DumpConCommandBase(ConCommandBase *cnext)
{
    if (cnext->IsCommand()) _dumpcmdcount++;
    _dumpcount++;
    Msg("%s: %s\n\t%s\n", cnext->IsCommand() ? "CMD" : "VAR", cnext->GetName(), cnext->GetHelpText()); 
    return true;
}

ES_CON_COMMAND2(dumpconcommandbase, 
               "Outputs all the console commands and variables.\n",
               1,
               "[prefix]")
    ForEachConCommandBase(DumpConCommandBase);
    Msg("Total: %d\tCommands: %d\tVariables: %d\n", _dumpcount, _dumpcmdcount, _dumpcount-_dumpcmdcount);
}

bool RefreshNotifyVars(ConCommandBase *cnext)
{
    // only want variables
    if (!cnext->IsCommand())
    {
        if (cnext->IsFlagSet(FCVAR_NOTIFY))
        {
            ConVar *cvar = (ConVar*) cnext;
            cvarmanager->CallGlobalChangeCallbacks(cvar, cvar->GetString(), cvar->GetFloat());
        }
    }
    return true;
}

ES_CON_COMMAND2(refreshpublicvars, 
               "Outputs all the console commands and variables.\n",
               1,
               "")
    ForEachConCommandBase(RefreshNotifyVars);
	DxMsg(3, MSGLOC, " Refreshed public variables.\n");
}


// ***********************************
// Begin Python convar-related commands
// ***********************************
PY_COMMAND_VARARGS(getInt, "Gets the int value for a server variable")
{
    char * szVar;
    if(!PyArg_ParseTuple(args, "s", &szVar))
        return g_pPyNone;
    ConVar* cvar = FindVar( szVar ); 
    if (!cvar){
        DxMsg(1, __FUNCTION__, __LINE__,"The var \"%s\" could not be found\n", szVar); 
        return g_pPyNone; // If it doesn t find the var
    }
    return Py_BuildValue("i", cvar->GetInt());
}

PY_COMMAND_VARARGS(getFloat, "Gets the float value for a server variable")
{
    char * szVar;
    if(!PyArg_ParseTuple(args, "s", &szVar))
        return g_pPyNone;
    ConVar* cvar = FindVar( szVar ); 
    if (!cvar){
        DxMsg(1, __FUNCTION__, __LINE__,"The var \"%s\" could not be found\n", szVar); 
        return g_pPyNone; // If it doesn t find the var
    }
    return Py_BuildValue("f", cvar->GetFloat());
}

PY_COMMAND_VARARGS(getString, "Gets the string value for a server variable")
{
    char * szVar;
    if(!PyArg_ParseTuple(args, "s", &szVar))
        return g_pPyNone;
    ConVar* cvar = FindVar( szVar ); 
    if (!cvar){
        DxMsg(1, __FUNCTION__, __LINE__,"The var \"%s\" could not be found\n", szVar); 
        return Py_BuildValue("s", ""); // If it doesn t find the var
    }
    return Py_BuildValue("s", cvar->GetString());
}

PY_COMMAND_VARARGS(setString, "Sets the server variable to the given string  value. Creating it if necessary.")
{
    char * szVar;
    char * szValue;
    if(!PyArg_ParseTuple(args, "ss", &szVar, &szValue))
        return NULL;
    ConVar* cvar = SetConVar(szVar, szValue, true);
    if (!cvar){
        DxMsg(0, __FUNCTION__, __LINE__,"Couldn't create \"%s\" and could not be found\n", szVar); 
        return NULL; // If it doesn t find the var
    }
    return Py_BuildValue("s", szValue);
}


PY_COMMAND_VARARGS(setInt, "Sets the server variable to the given integer value. Creating it if necessary.")
{
    char * szVar;
    int nValue = 0;
    if(!PyArg_ParseTuple(args, "si", &szVar, &nValue))
        return NULL;
    DxMsg(4, __FUNCTION__, __LINE__, "setint: %s, %d\n", szVar, nValue); 
    ConVar* cvar = SetConVar(szVar, nValue, true);
    if (!cvar){
        DxMsg(0, __FUNCTION__, __LINE__,"Couldn't create \"%s\" and could not be found\n", szVar); 
        return NULL; // If it doesn t find the var
    }
    return Py_BuildValue("i", nValue);
}

PY_COMMAND_VARARGS(setFloat,"Sets the server variable to the given float value. Creating it if necessary.")
{
    char * szVar;
    float fValue = 0;
    if(!PyArg_ParseTuple(args, "sf", &szVar, &fValue))
        return NULL;
    ConVar* cvar = SetConVar(szVar, fValue, true);
    if (!cvar){
        DxMsg(0, __FUNCTION__, __LINE__,"Couldn't create \"%s\" and could not be found\n", szVar); 
        return NULL; // If it doesn t find the var
    }
    return Py_BuildValue("f", fValue);
}

PY_COMMAND_VARARGS(getFlags, "Gets the flags value for a command")
{
    char * szVar;
    if(!PyArg_ParseTuple(args, "s", &szVar))
        return g_pPyNone;
    ConCommandBase * cvar = g_pCVar->FindCommandBase( szVar ); 
    if (!cvar){
        DxMsg(1, __FUNCTION__, __LINE__,"The command or variable \"%s\" could not be found\n", szVar); 
        return g_pPyNone; // If it doesn t find the var
    }
#ifdef LEFT4DEAD
    int nFlags = cvar->GetFlags();
#else
    int nFlags = cvar->m_nFlags;
#endif
    return Py_BuildValue("i", nFlags);
}

PY_COMMAND_VARARGS(getHelpText, "Gets the help text for a console command or server variable.")
{
    char * szVar;
    if(!PyArg_ParseTuple(args, "s", &szVar))
        return g_pPyNone;
    ConCommandBase * cvar = g_pCVar->FindCommandBase( szVar ); 
    if (!cvar){
        DxMsg(1, __FUNCTION__, __LINE__,"The var \"%s\" could not be found\n", szVar); 
        return g_pPyNone; // If it doesn t find the var
    }
    return Py_BuildValue("s", cvar->GetHelpText());
}
