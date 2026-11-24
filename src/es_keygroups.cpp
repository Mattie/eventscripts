#include "es_cmddef.h"
#include "es_convar.h"
#include "es_keygroups.h"
#include "es_util.h"
#include "filesystem.h"
#include "string.h"
#include "snip_str.h"

#include "es_strutils.h"

extern ConVar datadir_cvar;
extern ConVar scriptdir_cvar;
extern IFileSystem		*filesystem;
void ImmediateExecuteCommand(char *szCommand);

KeyValues *g_pUserKeys = NULL;

void InitKeyGroups() {
    if (!g_pUserKeys)
    {
        g_pUserKeys = new KeyValues(KEYNAME_USERKEYS);
        g_pUserKeys->AddSubKey(new KeyValues(KEYNAME_USERGROUPS));
        g_pUserKeys->AddSubKey(new KeyValues(KEYNAME_UNGROUPED));
    }
}

// RETURNVAL
ES_CON_COMMAND2(keygetvalue, 
               "Gets a value within a given key (where the key could be free-floating or associated with a group).\n",
               4,
               "<var> [group] <key> <valuename>")
    int nFirstArg = bUseServerVariablesForReturn ? 0 : -1;
    nCount+=abs(nFirstArg);
    KeyValues *pGroup = NULL;
    char szKeyName[MAX_FORMATTED];
    char szValueName[MAX_FORMATTED];
    if (!g_pUserKeys)
    {
        ErrMsg(0,"ERROR: Eventscripts key access attempted before keys have been initialized.\n");
        ErrVar("Keys not ready, yet");
        return;
    }
    if (nCount == 5)
    {
        KeyValues *pUserGroups = g_pUserKeys->FindKey(KEYNAME_USERGROUPS);
        pGroup = pUserGroups->FindKey(args.Arg(nFirstArg+2));
        if (!pGroup)
        {
            ErrMsg(0,"ERROR: Eventscripts cannot find the %s group!\n", args.Arg(nFirstArg+2));
            ErrVar("Keygroup not found");
            return;
        }
        strcpy(szKeyName, args.Arg(nFirstArg+3));
        strcpy(szValueName, args.Arg(nFirstArg+4));
    }
    else
    {
        // use the ungrouped group.
        pGroup = g_pUserKeys->FindKey(KEYNAME_UNGROUPED);
        strcpy(szKeyName, args.Arg(nFirstArg+2));
        strcpy(szValueName, args.Arg(nFirstArg+3));
    }
    KeyValues *pNewKey = pGroup->FindKey(szKeyName);
    if (!pNewKey)
    {
        int keynum = atoi(szKeyName);
        if (keynum>0) 
        {
            int i = 1;
            for(pNewKey = pGroup->GetFirstTrueSubKey();i<keynum && pNewKey;i++)
            {
                pNewKey = pNewKey->GetNextTrueSubKey();
            }
        }
        if (!pNewKey)
        {
            ErrMsg(0,"ERROR: Eventscripts cannot find the %s key!\n", szKeyName);
            ErrVar("Key not found");
            return;
        }
    }

    RETURNVAL(args.Arg(1), pNewKey->GetString(szValueName), "s");
}


KeyValues *pForEachVal = NULL;

ES_CON_COMMAND2(_foreachval, 
               "EXPERIMENTAL. Loops through a keygroup and performs a single command on each key, providing a single variable with the key name.\n",
               6,
               "<ptr> <var> in <groupname> <keyname> <command>")
    pForEachVal = (KeyValues *)atoi(args.Arg(1));
    pForEachVal = pForEachVal->GetNextValue();
    const char *pArgs = strstr(args.ArgS(), " ")+1;

    if (pForEachVal && !FStrEq("", pForEachVal->GetName()))
    {
        char szCommand[MAX_COMMAND_STRING];
        sprintf(szCommand, "es_xsetinfo %s \"%s\";%s\n;%s %d %s\n", 
            args.Arg(2), pForEachVal->GetName(), 
            args.Arg(6),
            args.Arg(0), (unsigned int)pForEachVal, pArgs
            );
        ImmediateExecuteCommand(szCommand);
    }
}

KeyValues *pForEachKey = NULL;

ES_CON_COMMAND2(_foreachkey, 
               "EXPERIMENTAL. Loops through a keygroup and performs a single command on each key, providing a single variable with the key name.\n",
               6,
               "<ptr> <var> in <groupname> <command>")
    pForEachKey = (KeyValues *)atoi(args.Arg(1));
    pForEachKey = pForEachKey->GetNextTrueSubKey();
    const char *pArgs = strstr(args.ArgS(), " ")+1;

    if (pForEachKey && !FStrEq("", pForEachKey->GetName()))
    {
        char szCommand[MAX_COMMAND_STRING];
        sprintf(szCommand, "es_xsetinfo %s \"%s\";%s\n;%s %d %s\n", 
            args.Arg(2), pForEachKey->GetName(), 
            args.Arg(5),
            args.Arg(0), (unsigned int)pForEachKey, pArgs
            );
        ImmediateExecuteCommand(szCommand);
    }
}



KeyValues * GetKeyGroup(const char *szName, bool bCreate)
{
    if (!g_pUserKeys)
    {
        ErrMsg(0,"ERROR: Eventscripts key access attempted before keys have been initialized.\n");
        ErrVar("Keys not ready yet");
        return NULL;
    }
    KeyValues *pUserGroups = g_pUserKeys->FindKey(KEYNAME_USERGROUPS);
    if (!pUserGroups)
    {
        ErrMsg(0,"ERROR: Eventscripts cannot find any UserGroups!\n");
        return NULL;
    }
    KeyValues *pNewGroup = pUserGroups->FindKey(szName, bCreate);
    return pNewGroup;
}

KeyValues * CreateKeyGroupConCommand(const CCommand &args,int position = 1)
{
    return GetKeyGroup(args.Arg(position), true);
}

KeyValues * CreateKeyGroupEx(const char *szName)
{
    return GetKeyGroup(szName, true);
}


ES_CON_COMMAND2(keygroupcreate, 
               "Creates a keygroup that can be loaded and saved to a file. Must call es_keygroupdelete to free this memory!\n",
               2,
               "<groupname>")
    KeyValues *pNewGroup = CreateKeyGroupEx(args.Arg(1));
    if (!pNewGroup)
    {
        ErrMsg(0,"ERROR: EventScripts couldn't create the user group.\n");
    }

    return;
}

void keygroupdelete(const char *szKeyGroupName)
{
    if (!g_pUserKeys)
    {
        ErrMsg(0,"ERROR: Eventscripts key access attempted before keys have been initialized.\n");
        ErrVar("Keys not ready, yet");
        return;
    }
    KeyValues *pUserGroups = g_pUserKeys->FindKey(KEYNAME_USERGROUPS);
    if (!pUserGroups)
    {
        ErrMsg(0,"ERROR: Eventscripts cannot find any UserGroups!\n");
        return;
    }
    KeyValues *pNewGroup = pUserGroups->FindKey(szKeyGroupName, false);
    if (!pNewGroup)
    {
        ErrMsg(1,"EventScripts couldn't find the user group %s\n", szKeyGroupName);
        ErrVar("Key not found");
        return;
    }
    pUserGroups->RemoveSubKey(pNewGroup);
    pNewGroup->deleteThis();
    return;
}

ES_CON_COMMAND2(keygroupdelete, 
               "Deletes a keygroup from memory so that it's not leaked.\n",
               2,
               "<groupname>")
    keygroupdelete(args.Arg(1));
}

ES_CON_COMMAND2(keygrouprename, 
               "Renames an existing keygroup.\n",
               3,
               "<groupname> <newname>")
    if (!g_pUserKeys)
    {
        ErrMsg(0,"ERROR: Eventscripts key access attempted before keys have been initialized.\n");
        ErrVar("Keys not ready, yet");
        return;
    }
    KeyValues *pUserGroups = g_pUserKeys->FindKey(KEYNAME_USERGROUPS);
    if (!pUserGroups)
    {
        ErrMsg(0,"ERROR: Eventscripts cannot find any UserGroups!\n");
        return;
    }
    KeyValues *pNewGroup = pUserGroups->FindKey(args.Arg(1), false);
    if (!pNewGroup)
    {
        ErrMsg(0,"EventScripts couldn't find the user group %s\n", args.Arg(1));
        ErrVar("Key not found");
        return;
    }
    pNewGroup->SetName(args.Arg(2));
    return;
}


CON_COMMAND(es_keygrouplist, "Lists all in-memory keygroups.\n")
{
    if (!g_pUserKeys)
    {
        ErrMsg(0,"ERROR: Eventscripts key access attempted before keys have been initialized.\n");
        ErrVar("Keys not ready, yet");
        return;
    }
    KeyValues *pUserGroups = g_pUserKeys->FindKey(KEYNAME_USERGROUPS);
    if (!pUserGroups)
    {
        ErrMsg(0,"ERROR: Eventscripts cannot find any UserGroups!\n");
        return;
    }
    DxMsg(0, __FUNCTION__, __LINE__, "----------------------\n");
	for ( KeyValues *pKey = pUserGroups->GetFirstTrueSubKey(); pKey; pKey = pKey->GetNextTrueSubKey() )
	{
		DxMsg(0, __FUNCTION__, __LINE__, "Key: %s\n", pKey->GetName() );
	    for ( KeyValues *pValue = pKey->GetFirstValue(); pValue; pValue = pValue->GetNextValue() )
	    {
            DxMsg(0, __FUNCTION__, __LINE__, "   Name: %s\n  Value: %s\n", pValue->GetName(), pValue->GetString() );  // Assuming pValue->GetDataType() == TYPE_INT...
	    }
	} 
    DxMsg(0, __FUNCTION__, __LINE__, "----------------------\n");
}

ES_CON_COMMAND2(keylist, 
               "Lists all key values in memory that aren't groups. Optionally can look up a group, if you provide one.\n", 
               1, 
               "[group]")
    KeyValues *pGroup = NULL;
    if (!g_pUserKeys)
    {
        ErrMsg(0,"ERROR: Eventscripts key access attempted before keys have been initialized.\n");
        ErrVar("Keys not ready, yet.");
        return;
    }
    if (args.ArgC() == 2)
    {
        KeyValues *pUserGroups = g_pUserKeys->FindKey(KEYNAME_USERGROUPS);
        pGroup = pUserGroups->FindKey(args.Arg(1));
        if (!pGroup)
        {
            ErrMsg(0,"ERROR: Eventscripts cannot find the %s group!\n", args.Arg(1));
            ErrVar("Key not found");
            return;
        }
    }
    else
    {
        // use the ungrouped group.
        pGroup = g_pUserKeys->FindKey(KEYNAME_UNGROUPED);
        if (!pGroup)
        {
            ErrMsg(0,"ERROR: Eventscripts cannot find the %s group!\n", KEYNAME_UNGROUPED);
            ErrVar("Key not found");
            return;
        }
    } 

    DxMsg(0, __FUNCTION__, __LINE__, "----------------------\n");
	for ( KeyValues *pKey = pGroup->GetFirstTrueSubKey(); pKey; pKey = pKey->GetNextTrueSubKey() )
	{
		DxMsg(0, __FUNCTION__, __LINE__, "Key: %s\n", pKey->GetName() );
	    for ( KeyValues *pValue = pKey->GetFirstValue(); pValue; pValue = pValue->GetNextValue() )
	    {
            DxMsg(0, __FUNCTION__, __LINE__, "   Name: %s\n  Value: %s\n", pValue->GetName(), pValue->GetString() );  // Assuming pValue->GetDataType() == TYPE_INT...
	    }
	}
    DxMsg(0, __FUNCTION__, __LINE__, "----------------------\n");
}

ES_CON_COMMAND2(keygroupcopy, 
               "Copies a keygroup.\n", 
               2, 
               "<FromKeygroup> <ToKeygroup>")
    KeyValues *pGroup = NULL;
    if (!g_pUserKeys)
    {
        ErrMsg(0,"ERROR: Eventscripts key access attempted before keys have been initialized.\n");
        ErrVar("Keys not ready, yet.");
        return;
    }
    KeyValues *pUserGroups = g_pUserKeys->FindKey(KEYNAME_USERGROUPS);
    pGroup = pUserGroups->FindKey(args.Arg(1));
    if (!pGroup)
    {
        ErrMsg(0,"ERROR: Eventscripts cannot find the %s group!\n", args.Arg(1));
        ErrVar("Key not found");
        return;
    }

    KeyValues *pNewGroup = pGroup->MakeCopy();
    pNewGroup->SetName(args.Arg(2));
    pNewGroup->SetNextKey(pGroup->m_pPeer);
    pGroup->SetNextKey(pNewGroup);
}



#define KEYGROUP_LOAD_FORMAT "%s/%s/es_%s_db.txt"

ES_CON_COMMAND2(keygroupload, 
               "Loads a keygroup from file based on its name.\n",
               2,
               "<groupname> [path]")
    char szFullPath[MAX_PATH];
    if (args.ArgC() == 3)
    {
        if (*args.Arg(2)=='|')
        {
            sprintf(szFullPath, KEYGROUP_LOAD_FORMAT, "addons/eventscripts", args.Arg(2)+1, args.Arg(1));
        }
        else
        {
            sprintf(szFullPath, KEYGROUP_LOAD_FORMAT, datadir_cvar.GetString(), args.Arg(2), args.Arg(1));
        }
    }
    else
    {
        sprintf(szFullPath, KEYGROUP_LOAD_FORMAT, datadir_cvar.GetString(), scriptdir_cvar.GetString(), args.Arg(1));
    }
    if (!g_pUserKeys)
    {
        ErrMsg(0,"ERROR: Eventscripts key access attempted before keys have been initialized.\n");
        ErrVar("Keys not ready, yet");
        return;
    }

    KeyValues *pUserGroups = g_pUserKeys->FindKey(KEYNAME_USERGROUPS);
    if (!pUserGroups)
    {
        ErrMsg(0,"ERROR: Eventscripts cannot find any UserGroups!\n");
        return;
    }

    KeyValues *pNewGroup = pUserGroups->FindKey(args.Arg(1), true);
    if (!pNewGroup->LoadFromFile(filesystem, szFullPath))
    {
        ErrMsg(0,"ERROR: Could not load keygroup: %s!\n", szFullPath);
        ErrVar("Keygroup not found");
    } 
    DxMsg(1, __FUNCTION__, __LINE__,"Loaded: %s\n", szFullPath);
    return;
}

ES_CON_COMMAND2(keygroupsave, 
               "Saves a keygroup to a file based on its name.\n",
               2,
               "<groupname> [path]")
    char szFullPath[MAX_PATH];
    if (args.ArgC() == 3)
    {
        if (*args.Arg(2)=='|')
        {
            sprintf(szFullPath, KEYGROUP_LOAD_FORMAT, "addons/eventscripts", args.Arg(2)+1, args.Arg(1));
        }
        else
        {
            sprintf(szFullPath, KEYGROUP_LOAD_FORMAT, datadir_cvar.GetString(), args.Arg(2), args.Arg(1));
        }
    }
    else
    {
        sprintf(szFullPath, KEYGROUP_LOAD_FORMAT, datadir_cvar.GetString(), scriptdir_cvar.GetString(), args.Arg(1));
    }
    if (!g_pUserKeys)
    {
        ErrMsg(0,"ERROR: Eventscripts key access attempted before keys have been initialized.\n");
        ErrVar("Keys not ready, yet");
        return;
    }
    KeyValues *pUserGroups = g_pUserKeys->FindKey(KEYNAME_USERGROUPS);
    if (!pUserGroups)
    {
        ErrMsg(0,"ERROR: Eventscripts cannot find any UserGroups!\n");
        return;
    }
    KeyValues *pNewGroup = pUserGroups->FindKey(args.Arg(1));
    if (!pNewGroup)
    {
        ErrMsg(0,"EventScripts couldn't find the user group %s\n", args.Arg(1));
        ErrVar("Key not found");
        return;
    }
    if (!pNewGroup->SaveToFile(filesystem, szFullPath))
    {
        ErrMsg(0,"ERROR: Could not save keygroup file: %s!\n", szFullPath);
        ErrVar("Keygroup could not be saved");
    }
    DxMsg(1, __FUNCTION__, __LINE__,"Saved: %s\n", szFullPath);
    return;
}

ES_CON_COMMAND2(keycreate, 
               "Creates a key that can be free-floating or associated with a group. Must call es_keydelete to free this memory when you're done.\n",
               2,
               "[group] <key>")
    KeyValues *pGroup = NULL;
    char szKeyName[MAX_FORMATTED];
    if (!g_pUserKeys)
    {
        ErrMsg(0,"ERROR: Eventscripts key access attempted before keys have been initialized.\n");
        ErrVar("Keys not ready, yet");
        return;
    }
    if (args.ArgC() == 3)
    {
        KeyValues *pUserGroups = g_pUserKeys->FindKey(KEYNAME_USERGROUPS);
        pGroup = pUserGroups->FindKey(args.Arg(1));
        if (!pGroup)
        {
            ErrMsg(0,"ERROR: Eventscripts cannot find the %s group!\n", args.Arg(1));
            ErrVar("Keygroup not found");
            return;
        }
        strcpy(szKeyName, args.Arg(2));
    }
    else
    {
        // use the ungrouped group.
        pGroup = g_pUserKeys->FindKey(KEYNAME_UNGROUPED);
        strcpy(szKeyName, args.Arg(1));
    }
    KeyValues *pNewKey = pGroup->FindKey(szKeyName, true);
    if (!pNewKey)
    {
        ErrMsg(0,"ERROR: Eventscripts cannot find/create the %s key!\n", szKeyName);
        ErrVar("Key could not be found or created");
        return;
    }
    return;
}

ES_CON_COMMAND2(keydelete, 
               "Deletes a key from memory so that it's not leaked when you're done with it.\n",
               2,
               "[group] <key>")
    KeyValues *pGroup = NULL;
    char szKeyName[MAX_FORMATTED];
    if (!g_pUserKeys)
    {
        ErrMsg(0,"ERROR: Eventscripts key access attempted before keys have been initialized.\n");
        ErrVar("Keys not ready, yet");
        return;
    }
    if (args.ArgC() == 3)
    {
        KeyValues *pUserGroups = g_pUserKeys->FindKey(KEYNAME_USERGROUPS);
        pGroup = pUserGroups->FindKey(args.Arg(1));
        if (!pGroup)
        {
            ErrMsg(0,"ERROR: Eventscripts cannot find the %s group!\n", args.Arg(1));
            ErrVar("Keygroup not found");
            return;
        }
        strcpy(szKeyName, args.Arg(2));
    }
    else
    {
        // use the ungrouped group.
        pGroup = g_pUserKeys->FindKey(KEYNAME_UNGROUPED);
        strcpy(szKeyName, args.Arg(1));
    }
    KeyValues *pNewKey = pGroup->FindKey(szKeyName);
    if (!pNewKey)
    {
        ErrMsg(0,"ERROR: Eventscripts cannot find the %s key.\n", szKeyName);
        ErrVar("Key not found");
        return;
    }
    pGroup->RemoveSubKey(pNewKey);
    pNewKey->deleteThis();
    return;
}

void KeyGroupFilter(KeyValues *pGroup, bool bOnly, const char * pValueName, const char * pValueData)
{
    if (!pGroup)
    {
        ErrMsg(0,"ERROR: Eventscripts cannot find the keygroup.\n");
        ErrVar("Key not found");
        return;
    }

    KeyValues *pKey = pGroup->GetFirstTrueSubKey();
	while (pKey)
	{
        bool bFound = false;
	    for ( KeyValues *pValue = pKey->GetFirstValue(); pValue; pValue = pValue->GetNextValue() )
	    {
            DxMsg(4, __FUNCTION__, __LINE__, "--%s/%s: %s/%s\n---%d: %s", pValue->GetName(), pValueName, pValue->GetString(), pValueData, FStrEq(pValueData, pValue->GetString()), Q_stristr(pValue->GetName(),pValueName));
            if (FStrEq(pValueData, pValue->GetString()) && Q_stristr(pValue->GetName(),pValueName))
            {
                DxMsg(2, __FUNCTION__, __LINE__, "KeyGroupFilter: Found a match for key %s on %s: %s\n", pKey->GetName(), pValue->GetName(), pValue->GetString());
                bFound = true;
                break;
            }
	    }
        if (bFound)
        {
            if (bOnly)
            {
                // only = finders keepers
                pKey = pKey->GetNextTrueSubKey();
            }
            else
            {
                // not = finders deleters
                KeyValues *pTemp = pKey;
                pKey = pKey->GetNextTrueSubKey();
                pGroup->RemoveSubKey(pTemp);
                pTemp->deleteThis();                
                
            }
        }
        else
        {
            if (bOnly)
            {
                // only = absent deleters
                KeyValues *pTemp = pKey;
                pKey = pKey->GetNextTrueSubKey();
                pGroup->RemoveSubKey(pTemp);
                pTemp->deleteThis();                
            }
            else
            {
                // not = absent keepers
                pKey = pKey->GetNextTrueSubKey();
            }
        }
        
	}
}

ES_CON_COMMAND2(keygroupfilter, 
               "Deletes keys from a keygroup that match or don't match a certain value.\n",
               2,
               "[group] <not/only> <value-name> <value>")
    KeyValues *pGroup = NULL;

    const char * pOp = NULL;
    const char * pValueName = NULL;
    const char * pValue = NULL;
    if (!g_pUserKeys)
    {
        ErrMsg(0,"ERROR: Eventscripts key access attempted before keys have been initialized.\n");
        ErrVar("Keys not ready, yet");
        return;
    }
    if (args.ArgC() > 4)
    {
        KeyValues *pUserGroups = g_pUserKeys->FindKey(KEYNAME_USERGROUPS);
        pGroup = pUserGroups->FindKey(args.Arg(1));
        if (!pGroup)
        {
            ErrMsg(0,"ERROR: Eventscripts cannot find the %s group!\n", args.Arg(1));
            ErrVar("Keygroup not found");
            return;
        }
        pOp = args.Arg(2);
        pValueName = args.Arg(3);
        pValue = args.Arg(4);
        //strcpy(szKeyName, args.Arg(2));

    }
    else
    {
        // use the ungrouped group.
        pGroup = g_pUserKeys->FindKey(KEYNAME_UNGROUPED);
        pOp = args.Arg(1);
        pValueName = args.Arg(2);
        pValue = args.Arg(3);
        //strcpy(szKeyName, args.Arg(1));
    }

    KeyGroupFilter(pGroup, FStrEq(pOp, "only") ? true : false, pValueName, pValue);
    return;
}

ES_CON_COMMAND2(keyrename, 
               "Rename a key.",
               3,
               "[group] <key> <newkeyname>")
    KeyValues *pGroup = NULL;
    char szKeyName[MAX_FORMATTED];
    if (!g_pUserKeys)
    {
        ErrMsg(0,"ERROR: Eventscripts key access attempted before keys have been initialized.\n");
        ErrVar("Keys not ready, yet");
        return;
    }
    if (args.ArgC() == 4)
    {
        KeyValues *pUserGroups = g_pUserKeys->FindKey(KEYNAME_USERGROUPS);
        pGroup = pUserGroups->FindKey(args.Arg(1));
        if (!pGroup)
        {
            ErrMsg(0,"ERROR: Eventscripts cannot find the %s group!\n", args.Arg(1));
            ErrVar("Keygroup not found");
            return;
        }
        strcpy(szKeyName, args.Arg(2));
    }
    else
    {
        // use the ungrouped group.
        pGroup = g_pUserKeys->FindKey(KEYNAME_UNGROUPED);
        strcpy(szKeyName, args.Arg(1));
    }
    KeyValues *pNewKey = pGroup->FindKey(szKeyName);
    if (!pNewKey)
    {
        ErrMsg(0,"ERROR: Eventscripts cannot find the %s key.\n", szKeyName);
        ErrVar("Key not found");
        return;
    }
    if (args.ArgC() == 4)
    {
        pNewKey->SetName(args.Arg(3));
    }
    else
    {
        pNewKey->SetName(args.Arg(2));
    }
    return;
}

ES_CON_COMMAND2(foreachval, 
               "EXPERIMENTAL. Loops through a keygroup and performs a single command on each key, providing a single variable with the key name.\n",
               5,
               "<var> in <groupname> <keyname> <command>")
    pForEachVal = NULL; // reset the global
    if (args.ArgC() < 5 || !FStrEq(args.Arg(2), "in"))
    {
        DxMsg(0, __FUNCTION__, __LINE__," Invalid syntax: es_xforeachval <var> in <groupname> <command>\n");
        ErrVar("Syntax error");
        return;
           
    }
    if (!g_pUserKeys)
    {
        ErrMsg(0,"ERROR: Eventscripts key access attempted before keys have been initialized.\n");
        ErrVar("Keys not ready, yet");
        return;
    }
    KeyValues *pUserGroups = g_pUserKeys->FindKey(KEYNAME_USERGROUPS);
    if (!pUserGroups)
    {
        ErrMsg(0,"ERROR: Eventscripts cannot find any UserGroups!\n");
        return;
    }
    KeyValues *pGroup = pUserGroups->FindKey(args.Arg(3));
    if (!pGroup)
    {
        DxMsg(0, __FUNCTION__, __LINE__,"%s\nERROR: Eventscripts cannot find keygroup: %s!\n", args.Arg(0), args.Arg(3));
        ErrVar("Keygroup not found");
        return;
    }
    pForEachVal = pGroup->FindKey(args.Arg(4));
    if (pForEachVal)
    {
	    pForEachVal = pForEachVal->GetFirstValue();
        if (pForEachVal && !FStrEq("", pForEachVal->GetName()))
        {
            char szCommand[MAX_COMMAND_STRING];
            sprintf(szCommand, "es_xsetinfo %s \"%s\";%s\n;%s %d %s\n", 
                args.Arg(1), pForEachVal->GetName(), 
                args.Arg(5),
                "es_x_foreachval", (unsigned int)pForEachVal, args.ArgS()
                );
            ImmediateExecuteCommand(szCommand);
        }
    }
    else
    {
        DxMsg(0, __FUNCTION__, __LINE__,"%s\nERROR: Eventscripts cannot find key: %s!\n", args.Arg(0), args.Arg(4));
        ErrVar("Key not found");
        return;
    }
}

ES_CON_COMMAND2(foreachkey, 
               "EXPERIMENTAL. Loops through a keygroup and performs a single command on each key, providing a single variable with the key name.\n",
               5,
               "<var> in <groupname> <command>")
    pForEachKey = NULL; // reset the global
    if (args.ArgC() < 5 || !FStrEq(args.Arg(2), "in"))
    {
        DxMsg(0, __FUNCTION__, __LINE__," Invalid syntax: es_xforeachkey <var> in <groupname> <command>");
        ErrVar("Syntax error");
        return;
           
    }
    if (!g_pUserKeys)
    {
        ErrMsg(0,"ERROR: Eventscripts key access attempted before keys have been initialized.\n");
        ErrVar("Keys not ready, yet");
        return;
    }
    KeyValues *pUserGroups = g_pUserKeys->FindKey(KEYNAME_USERGROUPS);
    if (!pUserGroups)
    {
        ErrMsg(0,"ERROR: Eventscripts cannot find any UserGroups!\n");
        return;
    }
    KeyValues *pGroup = pUserGroups->FindKey(args.Arg(3));
    if (!pGroup)
    {
        DxMsg(0, __FUNCTION__, __LINE__,"%s\nERROR: Eventscripts cannot find keygroup: %s!\n", args.Arg(0), args.Arg(3));
        ErrVar("Keygroup not found");
        return;
    }
	pForEachKey = pGroup->GetFirstTrueSubKey();
    if (pForEachKey && !FStrEq("", pForEachKey->GetName()))
    {
        char szCommand[MAX_COMMAND_STRING];
        sprintf(szCommand, "es_xsetinfo %s \"%s\";%s\n;%s %d %s\n", 
            args.Arg(1), pForEachKey->GetName(), 
            args.Arg(4),
            "es_x_foreachkey", (unsigned int)pForEachKey, args.ArgS()
            );
        ImmediateExecuteCommand(szCommand);
    }
}


// Python Keygroups wrappers


PY_COMMAND_VARARGS(keygroupgetpointer,
                   "Returns the C++ pointer to a keygroup.")
{
    char * szString;
    if(!PyArg_ParseTuple(args, "s", &szString))
    {
        return NULL;
    }
    return Py_BuildValue("i", (int)GetKeyGroup(szString,false));
}


PY_COMMAND_VARARGS(keypcreate,
                   "Returns the C++ pointer to a new keyvalues object.")
{
    return Py_BuildValue("i", (int)new KeyValues(NULL));
}

PY_COMMAND_VARARGS(keypdelete,
                   "Deletes a key by pointer (not recommended)")
{
    KeyValues *pKey = NULL;
    if(!PyArg_ParseTuple(args, "i", &pKey))
    {
        return NULL;
    }
    if (pKey)
        pKey->deleteThis();
    Py_INCREF(g_pPyNone);
    return g_pPyNone;
}
PY_COMMAND_VARARGS(keypgetname,
                   "Gets a key name by pointer")
{
    KeyValues *pKey = NULL;
    if(!PyArg_ParseTuple(args, "i", &pKey))
    {
        return NULL;
    }
    return Py_BuildValue("s", pKey ? pKey->GetName() : "");
}

PY_COMMAND_VARARGS(keypsetname,
                   "Sets a key name by pointer")
{
    KeyValues *pKey = NULL;
    char *szString;
    if(!PyArg_ParseTuple(args, "is", &pKey, &szString))
    {
        return NULL;
    }
    if (pKey)
        pKey->SetName(szString);
    Py_INCREF(g_pPyNone);
    return g_pPyNone;
}

PY_COMMAND_VARARGS(keypcreatesubkey,
                   "Creates a subkey as an integer.")
{
    KeyValues *pKey = NULL;
    if(!PyArg_ParseTuple(args, "i", &pKey))
    {
        return NULL;
    }
    if (pKey)
    {
        int nReturn = (int)pKey->CreateNewKey();
        if (nReturn != 0)
        {
            return Py_BuildValue("i", nReturn);
        }
        else
        {
            Py_INCREF(g_pPyNone);
            return g_pPyNone;
        }
    }
    else
    {
        Py_INCREF(g_pPyNone);
        return g_pPyNone;
    }
}

PY_COMMAND_VARARGS(keypfindsubkey,
                   "Finds or creates a subkey by a particular name.")
{
    KeyValues *pKey = NULL;
    bool bCreate = false;
    char *szString;
    if(!PyArg_ParseTuple(args, "isb", &pKey, &szString, &bCreate))
    {
        return NULL;
    }
    if (pKey)
    {
        int nReturn = (int)pKey->FindKey(szString, bCreate);
        if (nReturn != 0)
        {
            return Py_BuildValue("i", nReturn);
        }
        else
        {
            Py_INCREF(g_pPyNone);
            return g_pPyNone;
        }
    }
    else
    {
        Py_INCREF(g_pPyNone);
        return g_pPyNone;
    }
}

PY_COMMAND_VARARGS(keypdetachsubkey,
                   "Detaches a subkey by pointer.")
{
    KeyValues *pKey = NULL;
    KeyValues *pKeyToRemove = NULL;
    if(!PyArg_ParseTuple(args, "ii", &pKey, &pKeyToRemove))
    {
        return NULL;
    }
    if (pKey && pKeyToRemove)
    {
        pKey->RemoveSubKey(pKeyToRemove);
    }
    Py_INCREF(g_pPyNone);
    return g_pPyNone;
}

PY_COMMAND_VARARGS(keypgetfirstsubkey,
                   "Retrieves the first subkey underneath this pointer")
{
    KeyValues *pKey = NULL;
    if(!PyArg_ParseTuple(args, "i", &pKey))
    {
        return NULL;
    }
    if (pKey)
    {
        return Py_BuildValue("i", (int)pKey->GetFirstSubKey());
    }
    else
    {
        Py_INCREF(g_pPyNone);
        return g_pPyNone;
    }
}

PY_COMMAND_VARARGS(keypgetnextkey,
                   "Retrieves the next key (peer) to this pointer.")
{
    KeyValues *pKey = NULL;
    if(!PyArg_ParseTuple(args, "i", &pKey))
    {
        return NULL;
    }
    if (pKey)
    {
#ifdef ORANGEBOX
        return Py_BuildValue("i", (int)pKey->GetNextKey());
#else
		// TODO, Remember: Why did I use a GetNextKey2?
        return Py_BuildValue("i", (int)pKey->GetNextKey2());
#endif
    }
    else
    {
        Py_INCREF(g_pPyNone);
        return g_pPyNone;
    }
}


PY_COMMAND_VARARGS(keypgetfirsttruesubkey,
                   "Retrieves the first true subkey for this pointer.")
{
    KeyValues *pKey = NULL;
    if(!PyArg_ParseTuple(args, "i", &pKey))
    {
        return NULL;
    }
    if (pKey)
    {
        return Py_BuildValue("i", (int)pKey->GetFirstTrueSubKey());
    }
    else
    {
        Py_INCREF(g_pPyNone);
        return g_pPyNone;
    }
}

PY_COMMAND_VARARGS(keypgetnexttruesubkey,
                   "Retrieves the next true subkey to this pointer (ignores 'values')")
{
    KeyValues *pKey = NULL;
    if(!PyArg_ParseTuple(args, "i", &pKey))
    {
        return NULL;
    }
    if (pKey)
    {
        return Py_BuildValue("i", (int)pKey->GetNextTrueSubKey());
    }
    else
    {
        Py_INCREF(g_pPyNone);
        return g_pPyNone;
    }
}

PY_COMMAND_VARARGS(keypgetnextvaluekey,
                   "Retrieves the next value in this pointer.")
{
    KeyValues *pKey = NULL;
    if(!PyArg_ParseTuple(args, "i", &pKey))
    {
        return NULL;
    }
    if (pKey)
    {
        return Py_BuildValue("i", (int)pKey->GetNextValue());
    }
    else
    {
        Py_INCREF(g_pPyNone);
        return g_pPyNone;
    }
}

PY_COMMAND_VARARGS(keypgetfirstvaluekey,
                   "Retrieves the first value in this pointer.")
{
    KeyValues *pKey = NULL;
    if(!PyArg_ParseTuple(args, "i", &pKey))
    {
        return NULL;
    }
    if (pKey)
    {
        return Py_BuildValue("i", (int)pKey->GetFirstValue());
    }
    else
    {
        Py_INCREF(g_pPyNone);
        return g_pPyNone;
    }
}

PY_COMMAND_VARARGS(keypgetint,
                   "Retrieves the int value in this pointer by name.")
{
    KeyValues *pKey = NULL;
    char * szString = NULL;
    if(!PyArg_ParseTuple(args, "is", &pKey, &szString))
    {
        return NULL;
    }
    if (pKey)
    {
        return Py_BuildValue("i", pKey->GetInt(szString));
    }
    else
    {
        Py_INCREF(g_pPyNone);
        return g_pPyNone;
    }
}
PY_COMMAND_VARARGS(keypgetfloat,
                   "Retrieves the float value in this pointer by name.")
{
    KeyValues *pKey = NULL;
    char * szString = NULL;
    if(!PyArg_ParseTuple(args, "is", &pKey, &szString))
    {
        return NULL;
    }
    if (pKey)
    {
        return Py_BuildValue("f", pKey->GetFloat(szString));
    }
    else
    {
        Py_INCREF(g_pPyNone);
        return g_pPyNone;
    }
}
/*
		TYPE_NONE = 0,
		TYPE_STRING,
		TYPE_INT,
		TYPE_FLOAT,
		TYPE_PTR,
		TYPE_WSTRING,
		TYPE_COLOR,
		TYPE_NUMTYPES, 
*/
//types_t GetDataType(const char *keyName = NULL);
PY_COMMAND_VARARGS(keypgetdatatype,
                   "Returns the data type id of the value in the key.")
{
    KeyValues *pKey = NULL;
    if(!PyArg_ParseTuple(args, "i", &pKey))
    {
        return NULL;
    }
    if (pKey)
    {
        return Py_BuildValue("i", (int)pKey->GetDataType());
    }
    else
    {
        Py_INCREF(g_pPyNone);
        return g_pPyNone;
    }
}
PY_COMMAND_VARARGS(keypgetstring,
                   "Retrieves the string value in this pointer by name.")
{
    KeyValues *pKey = NULL;
    char * szString = NULL;
    if(!PyArg_ParseTuple(args, "is", &pKey, &szString))
    {
        return NULL;
    }
    if (pKey)
    {
        return Py_BuildValue("s", pKey->GetString(szString));
    }
    else
    {
        Py_INCREF(g_pPyNone);
        return g_pPyNone;
    }
}
PY_COMMAND_VARARGS(keypsetstring,
                   "Sets the string value in this pointer by name.")
{
    KeyValues *pKey = NULL;
    char * szString = NULL;
    char * szSetString = NULL;
    if(!PyArg_ParseTuple(args, "iss", &pKey, &szString, &szSetString))
    {
        return NULL;
    }
    if (pKey)
        pKey->SetString(szString, szSetString);
    Py_INCREF(g_pPyNone);
    return g_pPyNone;
}
PY_COMMAND_VARARGS(keypsetint,
                   "Sets the int value in this pointer by name.")
{
    KeyValues *pKey = NULL;
    char * szString = NULL;
    int nSetInt = 0;
    if(!PyArg_ParseTuple(args, "isi", &pKey, &szString, &nSetInt))
    {
        return NULL;
    }
    if (pKey)
        pKey->SetInt(szString, nSetInt);
    Py_INCREF(g_pPyNone);
    return g_pPyNone;
}
PY_COMMAND_VARARGS(keypsetfloat,
                   "Sets the float value in this pointer by name.")
{
    KeyValues *pKey = NULL;
    char * szString = NULL;
    double fSetFloat = 0.0;
    if(!PyArg_ParseTuple(args, "isd", &pKey, &szString, &fSetFloat))
    {
        return NULL;
    }
    if (pKey)
    {
        pKey->SetFloat(szString, fSetFloat);
    }
    Py_INCREF(g_pPyNone);
    return g_pPyNone;
}
PY_COMMAND_VARARGS(keypisempty,
                   "Check if the keyvalue pointer is empty.")
{
    KeyValues *pKey = NULL;
    if(!PyArg_ParseTuple(args, "i", &pKey))
    {
        return NULL;
    }
    if (pKey)
    {
        return Py_BuildValue("i", (int)pKey->IsEmpty());
    }
    else
    {
        Py_INCREF(g_pPyNone);
        return g_pPyNone;
    }
}

PY_COMMAND_VARARGS(keypsavetofile,
                   "Saves the keyvalue pointer to filepath with all subkeys and values")
{
    KeyValues *pKey = NULL;
    char * szString = NULL;
    if(!PyArg_ParseTuple(args, "is", &pKey, &szString))
    {
        return NULL;
    }
    if (pKey)
    {
        pKey->SaveToFile(filesystem, szString);
    }
    Py_INCREF(g_pPyNone);
    return g_pPyNone;
}

PY_COMMAND_VARARGS(keyploadfromfile,
                   "Saves the keyvalue pointer to filepath with all subkeys and values")
{
    KeyValues *pKey = NULL;
    char * szString = NULL;
    if(!PyArg_ParseTuple(args, "is", &pKey, &szString))
    {
        return NULL;
    }
    if (pKey)
    {
        pKey->LoadFromFile(filesystem, szString);
    }
    Py_INCREF(g_pPyNone);
    return g_pPyNone;
}

// TODO: Re-implement RecursiveCopyKeyValues so that we don't need a custom KeyValues.cpp.
//       For Alien Swarm, we don't have their KeyValues.cpp so are out of luck
#ifndef ALIENSWARM
PY_COMMAND_VARARGS(keyprecursivekeycopy,
                   "Recursively copies a key into another key")
{
    KeyValues *pKey = NULL;
    KeyValues *pKeySource = NULL;
    if(!PyArg_ParseTuple(args, "ii", &pKey, &pKeySource))
    {
        return NULL;
    }
    if (pKey && pKeySource)
    {
        pKey->RecursiveCopyKeyValues(*pKeySource);
    }
    Py_INCREF(g_pPyNone);
    return g_pPyNone;
}
#endif

