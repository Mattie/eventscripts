#ifndef ES_CONVAR_H
#define ES_CONVAR_H

#ifdef _WIN32
#pragma once
#endif

#include "convar.h"

ConVar * GetVar(const char * szName, bool bCreate = false);
ConVar * GetVarDescription(const char * szName, const char *szDescription, bool bCreate);
ConVar * SetConVar(const char * szName, const char * szValue, bool bCreate = false);
ConVar * SetConVar(const char * szName, float fValue, bool bCreate = false);
ConVar * SetConVar(const char * szName, int nValue, bool bCreate = false);
ConVar * SetConVarDescription(const char * szName, const char * szValue, const char * szDescription, bool bCreate = true);
ConVar * CreateVar(const char *szName, const char *szDefault, int flags, const char *szDescription);
void SetConVarVal(const char * szName, const char *szValue);
bool CanChange(ConCommandBase * var);
ConVar * FindVar(const char * name);
void ChangeStringValue(ConVar * cvar, char const *tempVal );

// Loop through each concommandbase and invoke a callback. Callback returns false if it wants to break iteration
bool ForEachConCommandBase(bool (*callback)(ConCommandBase *));
// Used with ForEachConCommandBase() 
bool RefreshNotifyVars(ConCommandBase *cnext);
#endif

