//=============================================================================//
//
// Mattie's EventScripts
//
// The MIT License
//
// Copyright © 2010, Mattie Casper
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

#pragma region includes
#ifdef PYTHON_SUPPORT
#include "Python.h"
#endif
// On Windows, this source file was built with VS 2005 Express.
#ifdef WIN32
#include <windows.h>
#endif
class CBasePlayer;
#include "interface.h"
#include "filesystem.h"
#include "engine/iserverplugin.h"
#ifndef ORANGEBOX
#include "dlls/iplayerinfo.h"
#else
#include "game/server/iplayerinfo.h"
#endif
#include "eiface.h"
#include "igameevents.h"
#include "basehandle.h"
#include "es_convar.h"
#include "vstdlib/random.h"
#include "engine/IEngineTrace.h"
#include "bitbuf.h"
#include "engine/IEngineSound.h" 
#include "iservernetworkable.h"
#include "utlqueue.h"
#include "es_util.h"
#include "strtools.h"
#include <time.h>
#include "shareddefs.h"
#include "icommandline.h"
#include "utlhash.h"
#include "server_class.h"
#include "ehandle.h"
#include "es_debug.h"
#include "VirtualMachine/VirtualMachine.h"
#include "shake.h"
#include "IEffects.h"
#include "inetchannelinfo.h"
#include "muParser/muParser.h"
#include "esoptimize.h"
#include "utlstack.h"
#include "vphysics_interface.h"
#include "string.h"
#include "ivoiceserver.h"
#include "es_strings.h"
#include "snip_str.h"
#ifndef max
#define max(a1,a2) a1 > a2 ? a1 : a2
#endif
#ifndef min
#define min(a1,a2) a1 < a2 ? a1 : a2
#endif
#ifdef ORANGEBOX
#include "tier2/tier2.h"
#include "toolframework/itoolentity.h"
#include "mathlib/vector.h"
#endif
#include "es_version.h"
#include "es_cmddef.h"
#include "es_command_buffer.h"
#include "es_keygroups.h"
#include "es_strutils.h"
#include "es_vecstring.h"
#include "es_usermsg.h"
#include "es_rcon.h"
#include "es_mathparse.h"
#include "es_playerutil.h"
#include "es_hashtable.h"

extern "C"
{ 
#include "es_regex_adapter.h"
}
#pragma endregion Main EventScripts header dependencies

extern ICvar * g_pCVar;
extern CVirtualMachine g_ShellVM;
extern VMInstruction g_LatestInstruction;

#ifndef WIN32
// we don't want sleep on Linux
#define Sleep(a)
#endif
#ifndef snprintf
#define snprintf _snprintf
#endif



#include "MRecipientFilter.h"



#ifdef PYTHON_SUPPORT
// An array of all the methods we want in the ES module
PyMethodDef g_pEsMethods[300];

// Quick references to core objects/methods
PyObject * g_pEsAddonsTick = NULL;
PyObject * g_pEsAddonsSayWrap = NULL;
PyObject * g_pEsAddonsClientCommand = NULL;
PyObject * g_pEsAddonsLoadModuleAddon = NULL;
PyObject * g_pEsAddonsTriggerEvent = NULL;
PyObject * g_pEsAddonsCallBlock = NULL;
PyObject * g_pEsAddonsHasEvent = NULL;

#endif

#pragma region interfaces
// Interfaces from the engine
IVEngineServer	*engine = NULL; // helper functions (messaging clients, loading content, making entities, running commands, etc)
IFileSystem		*filesystem = NULL; // file I/O 
IGameEventManager2 *gameeventmanager = NULL;
IGameEventListener2 *listener = NULL;
IPlayerInfoManager *playerinfomanager = NULL; // game dll interface to interact with players
IServerPluginHelpers *helpers = NULL; // special 3rd party plugin helpers from the engine
IUniformRandomStream *randomStr = NULL;
IEngineTrace *enginetrace = NULL;
IEngineSound *esounds = NULL; // sound
IEffects * effects = NULL;
IPhysics * physics = NULL;
IServerGameEnts * gameents = NULL;
INetworkStringTableContainer * networkstringtable = NULL;
IVoiceServer * voiceserver = NULL;
#ifdef ORANGEBOX
IServerTools * tools = NULL;
#endif
#pragma endregion Interfaces from the Source engine

// We store all events fired by the engine in this queue until we have time
//  to pass them to scripts to use. 
CUtlQueue<IGameEvent*> g_pEventQueue;
KeyValues *g_pRegistered;

// This is useful as an alternative to engine->Cmd*() functions we had in CS:S
const CCommand * g_pCmdArg = NULL;


struct EntityInfo
{
    char szName[2048];
};
EntityInfo * g_pESEntityList;

ICvar *cvarmanager = NULL;

unsigned int g_nDelayeds = 0;
int maxplayers = 0;


int g_nEifaceLevel = 1; // to help us find InsertServerCommand()
int g_nTraceLevel = 0;
int g_NumTickListenersRegistered = 0;
CGlobalVars *gpGlobals = NULL;
ConCommand * g_pTopConCommand = NULL;
int g_nEngineVersion = 21;
bool g_bFirstLevelLoaded = false;



#ifdef ORANGEBOX
#define CONVARPARAM IConVar *
#define CONVARCALLBACKPARAM , float flOldValue
#else
#define CONVARPARAM ConVar *
#define CONVARCALLBACKPARAM
#endif

float GetTime( void )
{
	return Plat_FloatTime();
}


#pragma region callbacks
#define INT_CVARCALLBACK(variable, name) \
    static void name ( CONVARPARAM var, const char *pOldValue CONVARCALLBACKPARAM) { variable = ((ConVar*)var)->GetInt();}
extern int g_nDebugLevel;
extern int g_nDebugLog;
extern int g_nDebugShowFunctions;
bool g_bPythonTried = false;
int g_nESEventRegEnabled = 1;
int g_nESNoisy = 0;
int g_nESEnabled = 1;
int g_nFrameTimer = 0;
int g_nPythonEnabled = 1;
// Callbacks for cvar changes we care to watch
INT_CVARCALLBACK(g_nDebugLevel,           SetDebugLevel)
INT_CVARCALLBACK(g_nDebugLog,             SetDebugLog)
INT_CVARCALLBACK(g_nDebugShowFunctions,   SetDebugShowFunctions)
INT_CVARCALLBACK(g_nTraceLevel,           SetTraceOn)
INT_CVARCALLBACK(g_nEifaceLevel,          SetEifaceLevel)
INT_CVARCALLBACK(g_nESEventRegEnabled,    SetESEventReg)
INT_CVARCALLBACK(g_nESNoisy,              SetESNoisy)
INT_CVARCALLBACK(g_nESEnabled,            SetESEnabled)
INT_CVARCALLBACK(g_nFrameTimer,           SetFTEnabled)
INT_CVARCALLBACK(g_nPythonEnabled,        SetPythonEnabled)
#pragma endregion CvarCallbacks

#pragma region ConsoleVars
//---------------------------------------------------------------------------------
// EventScripts server-side console variables
//---------------------------------------------------------------------------------
static ConVar eventscripts_cvar("mattie_eventscripts", "1", FCVAR_REPLICATED|FCVAR_NOTIFY, "Turns on scripts that run on every server event.", SetESEnabled);
static ConVar noisy_cvar("eventscripts_noisy", "0", 0, "Turns on script handling for noisy GameEvents (footsteps, reloads, etc).",SetESNoisy);
static ConVar debug_cvar("eventscripts_debug", "0", 0, "Turns on debug print output for EventScripts. Use only if you want to debug problems.", SetDebugLevel);
static ConVar debugfunctions_cvar("eventscripts_debug_showfunctions", "0", 0, "Adds internal ES function names and line numbers to debug strings for parsing.", SetDebugShowFunctions);
static ConVar scripttrace_cvar("eventscripts_scripttrace", "0", 0, "Turns on script tracing.", SetTraceOn);
static ConVar debuglog_cvar("eventscripts_debuglog", "0", 0, "Will send debug messages to server log as well as console if this is set to 1.", SetDebugLog);
ConVar scriptdir_cvar("eventscripts_subdirectory", "", 0, "Specifies an alternate directory for configuration files. Must be a subdirectory of your cfg directory.");
ConVar datadir_cvar("eventscripts_datadirectory", "cfg", 0, "Specifies an alternate directory for output files.");
static ConVar escape_cvar("eventscripts_escapechars", "; {}()':", 0, "Lists the characters for that cause variable expansion to require quotes to prevent parsing issues. (Leave this alone unless you know what you're doing.)");
static ConVar cflags_cvar("eventscripts_cflags", "902", 0, "Obsolete. eventscripts_cflags");
static ConVar timeformat_cvar("eventscripts_timeformat", "%Y-%m-%d %H:%M:%S (%Z)", 0, "Format for time strings retrieved by es_gettimestring.");
static ConVar currentmap_cvar("eventscripts_currentmap", "", 0, "Current map.");
static ConVar lastgive_cvar("eventscripts_lastgive", "", 0, "Last given entity index");
static ConVar deadflag_cvar("eventscripts_deadflag", "1", 0, "If set to 1, uses fast but less stable method of detecting player's dead state. If set to 0, reverts to using Valve's method (delayed).");
static ConVar cmdprefix_cvar("eventscripts_cmdprefixes", "!", 0, "A list of single character prefixes which will be passed to es_client_command.cfg when fired.");
static ConVar interface_cvar("eventscripts_eiface", "1", 0, "Internal function for helping EventScripts choose the right ImmediateExecuteCommand for command ordering.", SetEifaceLevel);
static ConVar execmd_cvar("eventscripts_exec-cmd", "exec", 0, "The name of the 'exec' command that EventScripts uses internally.");
static ConVar protectrcon_cvar("eventscripts_protectrcon", "1", 0, "Add FCVAR_PROTECTED flag to rcon_password so that it will resolve properly.");
static ConVar botcexec_cvar("eventscripts_bot-cexec", "0", 0, "If set to 1, es_cexec* commands attempt to apply to bots. If set to 0, es_cexec* commands do not work on bots (old style)");
static ConVar serverdll_cvar("eventscripts_servergamedll_ver", "", 0, "Version of ServerGameDLL used by EventScripts. (read only)");
static ConVar serverclients_cvar("eventscripts_servergameclients_ver", "", 0, "Version of ServerGameClients interface used by EventScripts. (read only)");
static ConVar sayevents_cvar("eventscripts_chatevent", "1", 0, "If set to 1, EventScripts will trigger the es_player_chat event, otherwise it will not.");
static ConVar mapcommands_cvar("eventscripts_maphandler", "1", 0, "If set to 1, EventScripts will allow level change replacement. Must be set in autoexec.cfg or on command-line.");
static ConVar maxmessages_cvar("eventscripts_maxmsg", "28", 0, "Maximum number of usermessages supported by the mod. Do not change unless instructed to do so.");
static ConVar quote_cvar("eventscripts_quote", "\"", 0, "A quotation mark. A tool, not a variable.");
static ConVar nextmap_cvar("eventscripts_nextmapoverride", "", 0, "Set this to a map name that will override the next changelevel command to use this map.");
static ConVar addondir_cvar("eventscripts_addondir", "", 0, "Full path to EventScripts addon directory (read-only)");
static ConVar gamedir_cvar("eventscripts_gamedir", "", 0, "Full path to the Source mod you're playing.");
static ConVar lang_cvar("eventscripts_language", "english", 0, "Name of the default language for EventScripts scripts to use. Supports the same values as cl_language for clients.");
static ConVar frametimer_cvar("eventscripts_frametimecheck", "0", 0, "If set to 1, EventScripts will output timing warnings when a GameFrame takes longer than 0.01 seconds inside scripts.", SetFTEnabled);
static ConVar defaultevents_cvar("eventscripts_defaultevents", "1", 0, "If set to 1, EventScripts will try to register for the default event files on each new map.", SetESEventReg);
#ifndef ORANGEBOX
static ConVar autorefreshvars_cvar("eventscripts_autorefreshvars", "0", 0, "If set to 1, EventScripts will try to refresh global change callbacks for every public cvar on each new map.");
#else
#ifdef LEFT4DEAD
static ConVar autorefreshvars_cvar("eventscripts_autorefreshvars", "0", 0, "If set to 1, EventScripts will try to refresh global change callbacks for every public cvar on each new map.");
#else
static ConVar autorefreshvars_cvar("eventscripts_autorefreshvars", "1", 0, "If set to 1, EventScripts will try to refresh global change callbacks for every public cvar on each new map.");
#endif
#endif

#ifdef LEFT4DEAD
// Using the new ESC shell engine for Left4Dead engines and beyond
static ConVar shellengine_cvar("eventscripts_shellengine", "1", 0, "Turns on Python-based ESC script engine");
#else
static ConVar shellengine_cvar("eventscripts_shellengine", "0", 0, "Turns on Python-based ESC script engine");
#endif
static ConVar managed_servercmd_cvar("eventscripts_managed_servercmd", "0", 0, "Run single registered commands to completion; compound buffers remain engine-deferred");
static int g_nManagedServerCmdDepth = 0;
bool g_bManagedServerCmdExecution = false;
static ConVar setipcmdline_cvar("eventscripts_setipcmdline", "1", 0, "EventScripts will set the IP for you passed from a +ip command-line.");
#ifdef PYTHON_SUPPORT
static ConVar python_cvar("mattie_python", "1", 0, "Enable Python support for EventScripts",SetPythonEnabled);
#endif
#pragma endregion EventScripts server-side console variables

// function to initialize any cvars/command in this plugin
#ifndef ORANGEBOX
void InitCVars( CreateInterfaceFn cvarFactory );
#endif

char g_szCommand[MAX_COMMAND_STRING];
char * GetCommandFromString(char *szCommand)
{
    char *pTemp = szCommand;
    char *pStart = g_szCommand;
    int n = 0;
    while(pTemp && *pTemp && *pTemp!=' ' && *pTemp !='\n' && *pTemp!=';' && *pTemp!='\"')
    {
        g_szCommand[n] = *pTemp;
        pTemp++;
        n++;
    }
    g_szCommand[n]='\0';
    return pStart;
}

char *GetPythonCommandLine(char *cmdname, PyObject *args)
{
    char szCommandString[8000];
    strcpy(szCommandString, cmdname);
	int cmd_argc = 0;
    cmd_argc++;
    // loop through argvs
    int nArg = 0;
	PyObject *a;
    int sizetup = PyTuple_GET_SIZE(	args);
    while (nArg < sizetup) 
    {
        a = PyTuple_GetItem(	args, nArg);
        //   Get the string version of it
        PyObject *s;
        s = PyObject_Str(a);
        if (s)
        {
            char *orig = PyString_AsString(s);
            if (orig)
            {
                if (cmd_argc>0)
                    strcat(szCommandString, " ");
                if(strcspn(orig, escape_cvar.GetString())!=strlen(orig) || strstr(orig, "//"))  {
                    strcat(szCommandString, "\"");
                    strcat(szCommandString, orig);   
                    strcat(szCommandString, "\"");
				}
				else
				{
					strcat(szCommandString, orig);
				}
			    cmd_argc++;
            }
            Py_XDECREF(s);
        }
        nArg++;
    }
    return strdup(szCommandString);
}
void ServerCommand(const char *szCommand)
{
#ifdef ORANGEBOX
    char szCommandWait[MAX_COMMAND_STRING];
    sprintf(szCommandWait, "wait;%s\n", szCommand);
    engine->ServerCommand(szCommandWait);
#else
    engine->ServerCommand(szCommand);
#endif
    return;
}

void InsertServerCommand(const char *szCommand)
{
#ifdef ORANGEBOX
	engine->ServerCommand(szCommand);
#else
	engine->InsertServerCommand(szCommand);
#endif
}

void ExecuteCommandNow(char *szCommandOrig);

// Tell the Source engine to execute this command immediately on the Source console.
void ImmediateExecuteCommand(char *szCommand)
{
    if (g_bManagedServerCmdExecution && !EsCommandNeedsEngineBuffer(szCommand))
    {
        ExecuteCommandNow(szCommand);
        return;
    }
    if (g_nTraceLevel == 2)
    {
		// Log everything that goes through the console from ES
		DxMsg(0, __FUNCTION__, __LINE__, "[trace] %s", szCommand);
    }
    DxMsg(7, __FUNCTION__, __LINE__,"ImmediateExecuteCommand executing %d:\n\t%s\n", g_nEifaceLevel, szCommand);

    engine->InsertServerCommand(szCommand);
}

void ExecuteCommandNow(char *szCommandOrig)
{
#ifdef ORANGEBOX
	DxMsg(1, __FUNCTION__, __LINE__, "%s", szCommandOrig);
    // I dislike this for performance reasons, but the
    // current version of CCommand doesn't properly trim \n from
    // the end of the line. We need a copy so we can do that.
    char szCommand[MAX_COMMAND_STRING];
    strcpy(szCommand, szCommandOrig);
    TrimEol(szCommand);
	CCommand c;
	if (c.Tokenize(szCommand))
	{
		const char *name = c.Arg(0);
		ConCommandBase *pPtr = cvarmanager->FindCommand(name);
		if (pPtr)
		{
			if (pPtr->IsCommand())
			{
				ConCommand *pCommand = (ConCommand *)pPtr;
				pCommand->Dispatch(c);
				return;
			}
            
		}
        else {
            // not a command
            ConVar *pVar = FindVar(name);
            if (pVar)
            {
                pVar->SetValue(c.ArgS());
            }
            else
            {
                char szBufferedCommand[MAX_COMMAND_STRING + 2];
                // Unknown commands and aliases remain engine-owned and deferred.
                DxMsg(2, __FUNCTION__, __LINE__, "Passing command onto the server, possibly an alias.\n");
                sprintf(szBufferedCommand, "%s\n", szCommand);
                engine->ServerCommand(szBufferedCommand);
            }
        }
		if (pPtr) DxMsg(1, __FUNCTION__, __LINE__, "%s", pPtr->GetName());
	}
	DxMsg(1, __FUNCTION__, __LINE__, "--- %s\n", szCommand);
#else
    char szPreparseCommand[MAX_COMMAND_STRING];
    strcpy(szPreparseCommand, szCommandOrig);
    strcat(szPreparseCommand, "\n");
	
	ImmediateExecuteCommand(szPreparseCommand);
#endif
}


#ifdef DISABLE_WIN32
#include <signal.h>
#include <setjmp.h>

static volatile sig_atomic_t segv_flag;
static jmp_buf jmp_env;

static void segv_handler(int dummy)
{
    segv_flag = 1;
    longjmp(jmp_env, 1);

}

int IsBufferReadable(void *ptr_buffer, unsigned long buffer_size)
{
    struct sigaction oldaction, newaction;
    volatile char dummy;
    char *ptr = ptr_buffer;

    if (!buffer_size)
        return 0;

    newaction.sa_handler = segv_handler;
    newaction.sa_flags = 0;
    sigemptyset(&newaction.sa_mask);

    sigaction(SIGSEGV, &newaction, &oldaction);

    segv_flag = 0;

    if (setjmp(jmp_env) == 0) {
        dummy = ptr[0];
        dummy = ptr[buffer_size-1];
    }

    sigaction(SIGSEGV, &oldaction, NULL);

    return !segv_flag;

} 
#endif

void LoadAddonFile(const char *szAddonName);

IServerGameClients *g_pGameClients = NULL;
ConVar * g_Cheats = NULL;

#ifdef ORANGEBOX
void dummy(DISPATCH_PARAM)
{
	return;
}
ConCommandBase * FindCommandEx(const char *szCommandName, ConCommandBase *notthis = NULL)
{
	return cvarmanager->FindCommand(szCommandName);
}
#else
#define dummy NULL
ConCommandBase * FindCommandEx(const char *szCommandName, ConCommandBase *notthis = NULL)
{
      ConCommandBase *pPtr = g_pCVar->GetCommands();
      while (pPtr)
      {
         if (pPtr != notthis && pPtr->IsCommand() && strcmp(pPtr->GetName(), szCommandName) == 0)
            break;
         // Nasty
         pPtr = const_cast<ConCommandBase*>(pPtr->GetNext());
      }
	  return pPtr;
}
#endif

// Special hook for all commands
class CCmdWrap : public ConCommand
{
   // This will hold the pointer original gamedll say command
protected:
   ConCommand *m_pGameDLLCommand;
   const char * m_pWrap;
   const char * m_pName;
   const char * m_pDescription;
public:
   bool m_bReplaceNextClient;
   int m_nNextCommandClientIndex;


   CCmdWrap(const char *szName, const char *szDescription, const char *szWrap, int nFlags = 0): ConCommand(strdup(szName), dummy, strdup(szDescription), nFlags)
   {
       m_pWrap = (strdup(szWrap));
       m_pDescription = (strdup(szDescription));
       m_pName = (strdup(szName));
       m_pGameDLLCommand = NULL;
       m_bReplaceNextClient = false; 
       m_nNextCommandClientIndex = 0;
       Init();
   }

   // Override Init
   void Init()
   {
      DxMsg(2, __FUNCTION__, __LINE__, "Wrapping command: %s\n",m_pName); 
     if (m_pGameDLLCommand == NULL)
     {
        ConCommandBase *pPtr;
        pPtr = FindCommandEx(m_pWrap, this);
        if (!pPtr)
        {
          ErrMsg(0,"CmdWrap Error! BREAK\n");
          ErrVar("Internal Error");
        }

        m_pGameDLLCommand = (ConCommand *)pPtr;
     }
#ifdef LEFT4DEAD
     AddFlags(m_pGameDLLCommand->GetFlags() & !FCVAR_CHEAT);
#else
     AddFlags(m_pGameDLLCommand->m_nFlags & !FCVAR_CHEAT);
#endif
     DxMsg(2, __FUNCTION__, __LINE__, "Wrapping command: %s, %d, this: %d\n",m_pName, m_pGameDLLCommand, this); 
      // Call base class' init function
      ConCommand::Init();
   }


   void Dispatch(DISPATCH_PARAM)
   {
#ifdef ORANGEBOX
	  g_pCmdArg = &args;
#endif
      DxMsg(7, __FUNCTION__, __LINE__, "Dispatching(): %s\n", m_pName);
#ifdef ALIENSWARM
    ConVar::CVValue_t * cvardata = &(g_Cheats->GetRawValue());
#else
    ConVar *cvardata = g_Cheats;
#endif
      int nOldCheats = cvardata->m_nValue;
      float fOldCheats = cvardata->m_fValue;

      if (m_bReplaceNextClient)
      {
        g_pGameClients->SetCommandClient(m_nNextCommandClientIndex-1);
        cvardata->m_fValue = 1;
        cvardata->m_nValue = 1;
        m_bReplaceNextClient = false;
      }

      if (m_pGameDLLCommand->m_nFlags & FCVAR_CHEAT && cvardata->m_nValue < 1)
      {
          DxMsg(0, __FUNCTION__, __LINE__,"Cannot execute a cheat command while sv_cheats = 0.\n");
      }
      else
      {
        // Forward to gamedll
        m_pGameDLLCommand->Dispatch(DISPATCH_ARGS);
      }
      cvardata->m_fValue = fOldCheats;
      cvardata->m_nValue = nOldCheats;
   }
};



// Special hook for all commands
class CGiveWrap : public CCmdWrap
{

public:

    CGiveWrap(const char *szName, const char *szDescription, const char *szWrap): CCmdWrap(szName, szDescription, szWrap)
	{
	}

   void Dispatch(DISPATCH_PARAM)
   {
#ifdef ORANGEBOX
	  g_pCmdArg = &args;
#endif
#ifdef ALIENSWARM
    ConVar::CVValue_t * cvardata = &(g_Cheats->GetRawValue());
#else
    ConVar *cvardata = g_Cheats;
#endif
      int nOldCheats = cvardata->m_nValue;
      float fOldCheats = cvardata->m_fValue;

      if (m_bReplaceNextClient)
      {
        g_pGameClients->SetCommandClient(m_nNextCommandClientIndex-1);
        cvardata->m_fValue = 1;
        cvardata->m_nValue = 1;
        m_bReplaceNextClient = false;
      }

      if (m_pGameDLLCommand->m_nFlags & FCVAR_CHEAT && cvardata->m_nValue < 1)      {
          DxMsg(0, __FUNCTION__, __LINE__,"Cannot execute a cheat command while sv_cheats = 0.\n");
      }
      else
      {
        for (int i = 0;i<gpGlobals->maxEntities;i++)
        {
            DxMsg(5, __FUNCTION__, __LINE__, "i: %d\n",i);
            edict_t *pEntCur = PEntityOfEntIndex(i);
            if(pEntCur && !pEntCur->IsFree() && pEntCur->GetUnknown())
            {
                DxMsg(5, __FUNCTION__, __LINE__, "i2: %d\n",i);
                if (pEntCur->GetClassName())
                {
                    DxMsg(5, __FUNCTION__, __LINE__, "i2a: %d,%s\n",i,g_pESEntityList[i].szName);
                    DxMsg(5, __FUNCTION__, __LINE__, "i2a2: %d,%s\n",i,pEntCur->GetClassName());
                    strcpy(g_pESEntityList[i].szName, pEntCur->GetClassName());
                }
                else
                {
                    DxMsg(5, __FUNCTION__, __LINE__, "i2b: %d\n",i);
                    strcpy(g_pESEntityList[i].szName, "");
                }
                DxMsg(5, __FUNCTION__, __LINE__, "i3: %d\n",i);
            }
            else
            {
                DxMsg(5, __FUNCTION__, __LINE__, "i4: %d\n",i);
                strcpy(g_pESEntityList[i].szName, "");
                DxMsg(5, __FUNCTION__, __LINE__, "i5: %d\n",i);
            }
        }
        lastgive_cvar.SetValue(0);

        // Forward to gamedll
		m_pGameDLLCommand->Dispatch(DISPATCH_ARGS);

        for (int i = 0;i<gpGlobals->maxEntities;i++)
        {
            DxMsg(5, __FUNCTION__, __LINE__, "i6: %d\n",i);
            edict_t *pEntCur = PEntityOfEntIndex(i);
            if ((pEntCur!=0) && (!pEntCur->IsFree()) && pEntCur->GetUnknown())
            {
                DxMsg(5, __FUNCTION__, __LINE__, "i6a: %d\n",i);
                if (pEntCur->GetClassName())
                {
                    DxMsg(5, __FUNCTION__, __LINE__, "i7: %d\n",i);
                    if(!FStrEq(g_pESEntityList[i].szName, pEntCur->GetClassName()))
                    {
                        DxMsg(5, __FUNCTION__, __LINE__, "i8: %d\n",i);
                        lastgive_cvar.SetValue(i);
                        strcpy(g_pESEntityList[i].szName, pEntCur->GetClassName());
                        break;
                    }
                }
            }
        } 
      }
      cvardata->m_fValue = fOldCheats;
      cvardata->m_nValue = nOldCheats;
   }
};




#include "es_plugin_internal.h"

   int CEventScriptsPlugin::IsDead(int userid)
   {
       edict_t * pEntity = LookupEdictByUserid(userid);
       if (pEntity)
       {
           if (deadflag_cvar.GetInt() > 0)
           {
                int nOffset = 0;
                SendPropType nType;
                if (GetOffset("CBasePlayer.pl.deadflag", &nOffset, &nType))
                {
                    int value = 0;
                    if(GetTypedProp<int>(nOffset, pEntity, &value))
                    {
                        return value;
                    }
                    else
                    {
                        return -1;
                    }
                }
                else
                {
                    return -1;
                }
           }
           else
           {
            if(playerinfomanager)
            {
	            IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEntity );
                if (playerinfo && playerinfo->IsPlayer())
                {
                    return playerinfo->IsDead() ? 1 : 0;
                }
                else
                {
                    DxMsg(1, __FUNCTION__, __LINE__,"Userid isn't a player: %d\n", userid);
                    return -1;
                }
            }
            else
            {
                DxMsg(1, __FUNCTION__, __LINE__,"playerinfomanager isn't ready yet.\n");
                return -1;
            }
           }
       }
       else
       {
           DxMsg(1, __FUNCTION__, __LINE__,"Player doesn't exist: %d\n", userid);
           return -1;
       }
   }

CEventScriptsPlugin g_EventScriptsPlugin;
IServerPluginCallbacks * pPlugin;
// 
// The plugin is a static singleton that is exported as an interface
//

EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CEventScriptsPlugin, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS, g_EventScriptsPlugin );

IGameEvent * pEventScriptsKey;
ConCommand * pCmdExec;
void InitPython();
//---------------------------------------------------------------------------------
// Purpose: constructor/destructor
//---------------------------------------------------------------------------------
CEventScriptsPlugin::CEventScriptsPlugin()
{
	m_iClientCommandIndex = 0;
    pEventScriptsKey = NULL;
    g_pRegistered = NULL;
#ifdef PYTHON_SUPPORT
#endif
}

CEventScriptsPlugin::~CEventScriptsPlugin()
{
    if (g_pRegistered) g_pRegistered->deleteThis();
}



bool doblock_esshell(const char *pTrim, char*szCommandLine = NULL);
void doblock(const char *szBlock, char*szCommandLine = NULL)
{
    DxMsg(4, __FUNCTION__, __LINE__,"es__db %s;\n", szBlock);
    char *pCopy = strdup(szBlock);
    char *pTrim = trim(pCopy);

    if (shellengine_cvar.GetInt() >= 1 || !doblock_esshell(pTrim, szCommandLine))
    {
        if (g_nPythonEnabled)
        {
            if (szCommandLine)
            {
                g_ShellVM.StartArgBlock(szCommandLine);
            }
            g_ShellVM.StartBlock();
            DxMsg(4, __FUNCTION__, __LINE__,"Trying a python block...\n");
            PyObject *pBlockName = PyString_FromString(pTrim);
            PyObject *pbReturn = PyObject_CallFunctionObjArgs(g_pEsAddonsCallBlock, pBlockName, NULL);
            if (PyErr_Occurred())
            {
                PyErr_Print();
            }
            Py_XDECREF(pBlockName);
            Py_XDECREF(pbReturn);
            g_ShellVM.EndBlock();

        }
    }
    if (pCopy) free(pCopy);
    return;
}

int g_nCmdUserid = 0;

// Special hook for all commands
class CChangeLevelWrap : public CCmdWrap
{
public:
   static ConCommand * BuildHook(const char *szName) 
   {
      ConCommandBase *pPtr;
      pPtr = FindCommandEx(szName);
      if (!pPtr)
      {
         ErrMsg(0,"PerformWrap Error! BREAK\n");
         ErrVar("Internal Error");
         return NULL;
      }
#ifdef LEFT4DEAD
      DxMsg(0, __FUNCTION__, __LINE__, "[EventScripts] Reregistering our '%s' as duplicates not allowed.\n", szName);
      cvarmanager->UnregisterConCommand(pPtr);
      CChangeLevelWrap *pWrap = new CChangeLevelWrap(szName, pPtr->GetHelpText(), szName, pPtr->GetFlags());
#else
      CChangeLevelWrap *pWrap = new CChangeLevelWrap(szName, pPtr->GetHelpText(), szName, pPtr->m_nFlags);
#endif
      pWrap->m_pGameDLLCommand = (ConCommand *) pPtr;
      return pWrap;
   }

    CChangeLevelWrap(const char *szName, const char *szDescription, const char *szWrap, int nFlags = 0): CCmdWrap(szName, szDescription, szWrap, nFlags)
	{}
   void Dispatch(DISPATCH_PARAM)
   {
#ifdef ORANGEBOX
	  g_pCmdArg = &args;
#endif
	  char *pNextMap = strdup(nextmap_cvar.GetString());
	  if (!FStrEq(pNextMap,"") && !FStrEq(pNextMap,"0") && ARGC()>1)
	  {
		nextmap_cvar.SetValue("");
		char szCommand[MAX_COMMAND_STRING];
		*szCommand=0;
		sprintf(szCommand,"%s %s\n", m_pWrap, pNextMap); 
		ImmediateExecuteCommand(szCommand);
		DxMsg(0, __FUNCTION__, __LINE__, "[EventScripts] Next map changed from %s to %s.\n", ARGV(1), pNextMap);
	  }
	  else
	  {
		// Forward to gamedll
	    DxMsg(2, __FUNCTION__, __LINE__, "Forwarding to the original changelevel... %s", ARGS());
		m_pGameDLLCommand->Dispatch(DISPATCH_ARGS);
	  }
	  if (pNextMap) free(pNextMap);
   }
};


// Special hook for all commands
class CSayWrap : public CCmdWrap
{
public:
   static ConCommand * BuildHook(const char *szName) 
   {
      ConCommandBase *pPtr;
      pPtr = FindCommandEx(szName);
      if (!pPtr)
      {
         ErrMsg(0,"PerformWrap Error! BREAK\n");
         ErrVar("Internal Error");
         return NULL;
      }
#ifdef LEFT4DEAD
      DxMsg(0, __FUNCTION__, __LINE__, "[EventScripts] Reregistering our '%s' as duplicates not allowed.\n", szName);
      cvarmanager->UnregisterConCommand(pPtr);
      CSayWrap *pWrap = new CSayWrap(szName, pPtr->GetHelpText(), szName, pPtr->GetFlags());
#else
      CSayWrap *pWrap = new CSayWrap(szName, pPtr->GetHelpText(), szName, pPtr->m_nFlags);
#endif
      pWrap->m_pGameDLLCommand = (ConCommand *) pPtr;
      return pWrap;
   }

   CSayWrap(const char *szName, const char *szDescription, const char *szWrap, int nFlags = 0): CCmdWrap(szName, szDescription, szWrap, nFlags)
   {       
   }

   void Dispatch(DISPATCH_PARAM)
   {
#ifdef ORANGEBOX
       g_pCmdArg = &args;
#endif
	   DxMsg(5, __FUNCTION__, __LINE__, "say wrap dispatched.\n");
       bool bEatCommand = false;
       const char *pFullText = ARGS();
       bool bTeamOnly = FStrEq("say_team",ARGV(0));
       int index = g_EventScriptsPlugin.GetCommandIndex()+1;
       edict_t *p = PEntityOfEntIndex(index);
       int userid = engine->GetPlayerUserId(p);

#ifdef PYTHON_SUPPORT
        PyObject *SayText = PyString_FromString(pFullText);
        PyObject *TheUserId = Py_BuildValue("i", userid);
        PyObject *TeamOnly = Py_BuildValue("b", bTeamOnly);
        PyObject *pbReturn = PyObject_CallFunctionObjArgs(g_pEsAddonsSayWrap, TheUserId, SayText, TeamOnly, NULL);
        if (PyErr_Occurred())
        {
            PyErr_Print();
        }
        else
        {
            bEatCommand = !PyInt_AsLong(pbReturn);
        }
        Py_XDECREF(SayText);
        Py_XDECREF(TheUserId);
        Py_XDECREF(TeamOnly);
        Py_XDECREF(pbReturn);
#endif           
        if (!bEatCommand)
        {
            IGameEvent * event = gameeventmanager->CreateEvent( "es_player_chat");
	        if ( event && ARGC()>1)
	        {
                
                if (p) {
		            event->SetInt( "userid", userid );
                    if (bTeamOnly)
                    {
                    event->SetBool("teamonly", true);
                    }
                    else
                    {
                    event->SetBool("teamonly", false);
                    }
                    char szFirstWord[MAX_COMMAND_STRING];
                    char *pTemp = NULL;
                    if (userid>0)
                    {
                        char szText[8000];
                        strcpy(szText, pFullText);
                        int nEnd = strlen(szText) - 1;
                        if (szText[0] == '"' && szText[nEnd] == '"')
                        {
                            // make sure there are only 2 quotes
                            int qcount = 0;
                            int j = 0;
                            for (j = 0; j< nEnd;j++)
                                if (szText[j] == '"') qcount++;
                            if (qcount > 2)
                            {
                                pTemp = szText;
                            }
                            else
                            {
                                pTemp = szText + 1;
                                szText[nEnd]='\0';
                            }
                            
                            for (j = 0; pTemp[j] != 0 && pTemp[j] != ' ';j++)
                            {
                                szFirstWord[j] = pTemp[j];
                            }
                            szFirstWord[j]='\0';
                        }
                        else
                        {
                            pTemp = szText;
                            int j = 0;
                            for (j = 0; pTemp[j] != 0 && pTemp[j] != ' ';j++)
                            {
                                szFirstWord[j] = pTemp[j];
                            }
                            szFirstWord[j]='\0';
                        }
                        event->SetString( "text", pTemp );
                    }
                    else
                    {
                        event->SetString( "text", ARGS() );
                    }
		            gameeventmanager->FireEvent( event );
                    if (pTemp)
                    {
                        const char * blockname = (const char *)g_SayCmdTable.FindElement(strlwr(szFirstWord));
                        if (blockname)
                        {
                            char szCommandLine[MAX_COMMAND_STRING];
                            sprintf(szCommandLine, "%s", pTemp);
                            g_nCmdUserid = userid;
                            doblock(blockname, szCommandLine);
                            bEatCommand = true;
                        }
                    }

                }
	        }
      }

      if (m_pGameDLLCommand->m_nFlags & FCVAR_CHEAT && g_Cheats->GetInt() < 1)
      {
          DxMsg(0, __FUNCTION__, __LINE__,"Cannot execute a cheat command while sv_cheats = 0.\n");
      }
      else if (!bEatCommand)
      {
        // Forward to gamedll
        m_pGameDLLCommand->Dispatch(DISPATCH_ARGS);
      }
      else
      {
          DxMsg(3, __FUNCTION__, __LINE__, "say command eaten.");
      }
   }
};

// pointers to the hooked commands
ConCommand * m_pSay;
ConCommand * m_pSayTeam;
ConCommand * m_pChangeLevel;

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is loaded, load the interfaces we need from the engine
//---------------------------------------------------------------------------------
bool CEventScriptsPlugin::Load(	CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory )
{
#ifdef CSGO
	ConnectInterfaces(&interfaceFactory, 1);
#else
#ifdef ORANGEBOX
	ConnectTier1Libraries( &interfaceFactory, 1 );
	ConnectTier2Libraries( &interfaceFactory, 1 );
#endif
#endif

    pPlugin = this;
	DxMsg(0, __FUNCTION__, __LINE__, "[EventScripts] Loading Mattie's EventScripts %s %s...\n", VER_PRIVATEBUILD_STR, VERSIONBASE); 

    g_LatestInstruction.szLineNum = NULL;
    g_LatestInstruction.szCommand = NULL;
	playerinfomanager = (IPlayerInfoManager *)gameServerFactory(INTERFACEVERSION_PLAYERINFOMANAGER,NULL);
	if ( !playerinfomanager )
	{
		Warning( "Unable to load playerinfomanager, ignoring\n" ); // this isn't fatal, we just won't be able to access specific player data
	}

        voiceserver = (IVoiceServer *)interfaceFactory(INTERFACEVERSION_VOICESERVER,NULL);
        if (!voiceserver)
        {
            Warning ("IVoiceServer not obtained.\n");
        }
#ifdef ORANGEBOX
		cvarmanager = (ICvar *)interfaceFactory(CVAR_INTERFACE_VERSION,NULL);
		if (!cvarmanager) {
            Warning ("ICvar not obtained.\n");
		}
#endif
        // we require all these interfaces to function
		engine = (IVEngineServer*)interfaceFactory(INTERFACEVERSION_VENGINESERVER, NULL);
		if (!engine) 
		{
			// get the left4dead engine
			engine = (IVEngineServer*)interfaceFactory("VEngineServer022", NULL);
			if (!engine) 
			{
				Warning("[EventScripts] Failure in getting the engine interfaces we need.\n");
				Sleep(4000);
				return false; 
			}
			g_nEngineVersion = 22;
		}
#ifdef ORANGEBOX
	ConVar_Register( 0 );

        // Invoke a special EventScripts autoexec since early variables aren't set 
        //   before the plugin loads.
        ExecuteCommandNow("exec eventscripts_autoexec.cfg\n");
        engine->ServerExecute();
#endif
		// we require all these interfaces to function
		gameeventmanager = (IGameEventManager2 *)interfaceFactory(INTERFACEVERSION_GAMEEVENTSMANAGER2,NULL);
		if (!gameeventmanager) 
		{
			Warning("[EventScripts] Failure in getting the gameeventmanager interfaces we need.\n");
			Sleep(4000);
			return false; 
		}
		// we require all these interfaces to function
		filesystem = (IFileSystem*)interfaceFactory(FILESYSTEM_INTERFACE_VERSION, NULL);
		if (!filesystem) 
		{
			Warning("[EventScripts] Failure in getting the filesystem interfaces we need.\n");
			Sleep(4000);
			return false; 
		}
		// we require all these interfaces to function
		helpers = (IServerPluginHelpers*)interfaceFactory(INTERFACEVERSION_ISERVERPLUGINHELPERS, NULL);
		if (!helpers) 
		{
			Warning("[EventScripts] Failure in getting the helpers interfaces we need.\n");
			Sleep(4000);
			return false; 
		}
		// we require all these interfaces to function
		enginetrace = (IEngineTrace *)interfaceFactory(INTERFACEVERSION_ENGINETRACE_SERVER,NULL);
		if (!engine) 
		{
			Warning("[EventScripts] Failure in getting the enginetrace interfaces we need.\n");
			Sleep(4000);
			return false; 
		}
		// we require all these interfaces to function
		esounds = (IEngineSound *)interfaceFactory(IENGINESOUND_SERVER_INTERFACE_VERSION,NULL);
		if (!esounds) 
		{
			Warning("[EventScripts] Failure in getting the esounds interfaces we need.\n");
			Sleep(4000);
			return false; 
		}
		// we require all these interfaces to function
		effects = (IEffects *)gameServerFactory(IEFFECTS_INTERFACE_VERSION,NULL);
		if (!effects) 
		{
			Warning("[EventScripts] Failure in getting the effects interfaces we need.\n");
			Sleep(4000);
			return false; 
		}
		// we require all these interfaces to function
		networkstringtable = (INetworkStringTableContainer *)interfaceFactory(INTERFACENAME_NETWORKSTRINGTABLESERVER,NULL);
		if (!networkstringtable) 
		{
			Warning("[EventScripts] Failure in getting the networkstringtable interfaces we need.\n");
			Sleep(4000);
			return false; 
		}


		// we require all these interfaces to function
		randomStr = (IUniformRandomStream *)interfaceFactory(VENGINE_SERVER_RANDOM_INTERFACE_VERSION, NULL);
		if (!randomStr) 
		{
			Warning("[EventScripts] Failure in getting the randomStr interfaces we need.\n");
			Sleep(4000);
			return false; 
		}
#ifdef ORANGEBOX
	tools = (IServerTools *)gameServerFactory(VSERVERTOOLS_INTERFACE_VERSION, NULL);
	if (!tools) {
		Warning("[EventScripts] Failure in getting the IServerTools interfaces we need.\n");
	}
#endif

    physics = (IPhysics *)interfaceFactory(VPHYSICS_INTERFACE_VERSION,NULL);
    if (!physics)
    {
        Warning ("IPhysics not obtained.\n");
    }

    gameents = (IServerGameEnts *)gameServerFactory(INTERFACEVERSION_SERVERGAMEENTS,NULL);
    if (!gameents)
    {
        Warning ("IServerGameEnts not obtained!\n");
    }
    char pServerGameDLLVer[MAX_COMMAND_STRING];
    if (engine)
    {
        for (int i=3;i<999;i++)
        {
            sprintf(pServerGameDLLVer, "ServerGameDLL%03d", i);
            m_ServerDll = (IServerGameDLL *)gameServerFactory(pServerGameDLLVer, NULL);
            if (m_ServerDll)
            {
                DxMsg(1, __FUNCTION__, __LINE__,"Using %s for game.\n", pServerGameDLLVer);
				serverdll_cvar.SetValue(i);
                break;
            }
        }
        // Find the latest ServerGameClients interface and use that.
        for (int i=3;i<999;i++)
        {
            sprintf(pServerGameDLLVer, "ServerGameClients%03d", i);
            g_pGameClients = (IServerGameClients *)gameServerFactory(pServerGameDLLVer, NULL);
            if (g_pGameClients)
            {
                DxMsg(1, __FUNCTION__, __LINE__,"Using %s for game clients.\n", pServerGameDLLVer);
				serverclients_cvar.SetValue(i);
                break;
            }
        }        
    }

    if ( !m_ServerDll )
    {
        Warning ("[EventScripts] Failure in getting the serverDLL interface.\n");
    }
	if ( playerinfomanager )
	{
		gpGlobals = playerinfomanager->GetGlobalVars();
	}
#ifndef ORANGEBOX
	InitCVars( interfaceFactory ); // register any cvars we have defined
#endif
	MathLib_Init( 2.2f, 2.2f, 0.0f, 2.0f );
    // store the addon path in the variable
    char szTempPath[MAX_COMMAND_STRING];
    engine->GetGameDir(szTempPath, MAX_COMMAND_STRING-1);
#ifdef WIN32
    strcat(szTempPath, "\\addons\\eventscripts");
#else
    strcat(szTempPath, "/addons/eventscripts");
#endif
    addondir_cvar.SetValue(szTempPath);

    // store the addon path in the variable
    engine->GetGameDir(szTempPath, MAX_COMMAND_STRING-1);
    gamedir_cvar.SetValue(szTempPath);


    g_nEifaceLevel = interface_cvar.GetInt();
    g_nDebugLevel = debug_cvar.GetInt();
    g_nDebugLog = debuglog_cvar.GetInt();

    gameeventmanager->LoadEventsFromFile("addons/mattie_eventscripts.res");


    if (g_nESEventRegEnabled !=0)
    {
        DefaultEventRegistration();
    }
	InitKeyGroups();

    m_pSay = NULL;
    m_pSayTeam = NULL;
	m_pEntSetname = new CCmdWrap("es__entsetname", "EventScripts internal command.", "ent_setname");
    m_pEntFire = new CCmdWrap("es__fire", "EventScripts internal command.", "ent_fire");
    m_pDispatchEffect  = new CCmdWrap("es__dispatcheffect", "EventScripts internal command.", "test_dispatcheffect");
    m_pEntityBlocker = new CCmdWrap("es__entblock", "EventScripts internal command.", "test_entity_blocker");
    m_pCreateEntity = new CGiveWrap("es__createentity", "EventScripts internal command.", "Test_CreateEntity");
    m_pGive = new CGiveWrap("es__give", "EventScripts internal command.", "give");
    m_pCreateEntity2 = new CGiveWrap("es__createentity2", "EventScripts internal command.", "ent_create");
    m_pPropPhysics = new CGiveWrap("es__prop_physics_create", "EventScripts internal command.", "prop_physics_create");
    m_pPropDynamic = new CGiveWrap("es__prop_dynamic_create", "EventScripts internal command.", "prop_dynamic_create");
    m_pSetPos = new CCmdWrap("es__setpos", "EventScripts internal command.", "setpos");
    m_pSetAng = new CCmdWrap("es__setang", "EventScripts internal command.", "setang");
    m_pEntRemove = new CCmdWrap("es__remove", "EventScripts internal command.", "ent_remove");
    DxMsg(1, __FUNCTION__, __LINE__,"Wrapped useful commands.\n");

	/* Initialize our command classes. */
	DxMsg(1, __FUNCTION__, __LINE__, "Initializing command classes..\n");
	g_pUserMessages = new CESUserMessages(m_ServerDll, engine, playerinfomanager);

	if( !g_pUserMessages )
	{
		Warning("[EventScripts] Could not initialize command classes.\n");
		return false;
	}

    
    g_Cheats = FindVar("sv_cheats");

    // create a variable that provides the server's command-line
    SetConVarDescription("eventscripts_cmdline", CommandLine()->GetCmdLine(), "Server's command-line information.");

    if (protectrcon_cvar.GetInt() > 0)
    {
        ConVar *pRconPassword = FindVar("rcon_password");
        pRconPassword->m_nFlags |= FCVAR_PROTECTED;
    }
    
    g_pESEntityList = NULL;

#ifdef PYTHON_SUPPORT
    InitPython();
#endif

    // Temporary hack until ServerCommand() or "wait;" is fixed
    if (setipcmdline_cvar.GetInt() > 0)
    {
        const char *pVal = g_pCVar->GetCommandLineValue( "ip" );
        if (pVal)
        {
            DxMsg(0, __FUNCTION__, __LINE__, "[EventScripts] Temporary: Setting the host's IP variable from the command-line.\n");
            SetConVar("ip", pVal);
        }
    }

    DxMsg(1, __FUNCTION__, __LINE__,"   Automatically loading es_corelib.txt...\n");


    InitializeMathParse();

    // automatically loading corelib.
    // TODO: Loading ESC instead of corelib
    if (shellengine_cvar.GetInt() > 0)
    {
      LoadAddonFile("esc");
      ExecuteCommandNow("es_load corelib\n");
    }
    else
    {
        LoadAddonFile("corelib");
        DxMsg(1, __FUNCTION__, __LINE__,"   Done loading corelib.\n");
    }
    DxMsg(0, __FUNCTION__, __LINE__,"[EventScripts] Loaded successfully.\n"); 
    return true;
}

void BeginAllScriptEvents(const char *szEventName);

//---------------------------------------------------------------------------------
// Purpose: called whenever we need to exec a bunch of script files
//---------------------------------------------------------------------------------
void CEventScriptsPlugin::ExecAllRegistered( const char *szEvent )
{
    if (!szEvent || szEvent[0]=='\0')
    {
        DxMsg(0, __FUNCTION__, __LINE__,"[EventScripts] Error, event string nonexistent.\n");
        ErrVar("Internal error");
        return;
    }
	if (g_pRegistered)
    {
        DxMsg(2, __FUNCTION__, __LINE__,"Script pack registration scanning...\n");
        KeyValues *pTemp = g_pRegistered->GetFirstValue();
        while (pTemp) {
            DxMsg(2, __FUNCTION__, __LINE__,"ExecAllRegistered found value: %s with int: %d\n", pTemp->GetName(), pTemp->GetInt());
            if (pTemp->GetInt()>0) {
                DxMsg(3, __FUNCTION__, __LINE__,"ExecAllRegistered2 found value: %s with int: %d\n", pTemp->GetName(), pTemp->GetInt());
                char szCommand[MAX_COMMAND_STRING];
                sprintf(szCommand, "cfg/%s/%s/%s.cfg", scriptdir_cvar.GetString(), pTemp->GetName(), szEvent);
                if (filesystem->FileExists(szCommand))
                {
                    // build the client command
                    sprintf(szCommand, "%s %s/%s/%s.cfg\n", execmd_cvar.GetString(), scriptdir_cvar.GetString(), pTemp->GetName(), szEvent);
                    DxMsg(2, __FUNCTION__, __LINE__,"Sending %s command for %s/%s/%s.\n", execmd_cvar.GetString(), scriptdir_cvar.GetString(), pTemp->GetName(), szEvent);
                    ServerCommand(szCommand); 
                }
                else
                {
                    DxMsg(1, __FUNCTION__, __LINE__,"File doesn't exist: %s\n", szCommand);
                }
            }
            else
            {
                DxMsg(3, __FUNCTION__, __LINE__,"ExecAllRegistered3 found value: %s with int: %d\n", pTemp->GetName(), pTemp->GetInt());
            }
            pTemp = pTemp->GetNextValue();
        }
    }
    else
    {
    }
    DxMsg(2, __FUNCTION__, __LINE__,"Checking all scripts...\n");
    BeginAllScriptEvents(szEvent);
}

bool CEventScriptsPlugin::RegisteredScriptExists( const char *szEvent )
{
    if (!szEvent || szEvent[0]=='\0')
    {
        DxMsg(0, __FUNCTION__, __LINE__,"[EventScripts] Error, event string nonexistent.\n");
        ErrVar("Internal Error");
        return FALSE;
    }
	if (g_pRegistered)
    {
        DxMsg(2, __FUNCTION__, __LINE__,"Script pack registration scanning...\n");
        KeyValues *pTemp = g_pRegistered->GetFirstValue();
        while (pTemp) {
            DxMsg(3, __FUNCTION__, __LINE__,"RegisteredScriptExists found value: %s with int: %d\n", pTemp->GetName(), pTemp->GetInt());
            if (pTemp->GetInt()>0) {
                char szCommand[MAX_COMMAND_STRING];
                sprintf(szCommand, "cfg/%s/%s/%s.cfg", scriptdir_cvar.GetString(), pTemp->GetName(), szEvent);
                if (filesystem->FileExists(szCommand))
                {
                    return TRUE;
                }
            }
            pTemp = pTemp->GetNextValue();
        }
    }
    else
    {
        DxMsg(6, __FUNCTION__, __LINE__," Script Packs: No registration system available.\n");
        return FALSE;
    }
    return FALSE;
}


void CEventScriptsPlugin::PrintAllRegistered(int nUserid)
{
    if (nUserid > 0) 
    {
        engine->ClientCommand(LookupEdictByUserid(nUserid), "echo EventScripts Script packs:");
        engine->ClientCommand(LookupEdictByUserid(nUserid), "echo ------------------------------------------");
    }
    else
    {
        DxMsg(0, __FUNCTION__, __LINE__,"\nEventScripts Script packs:\n------------------------------------------\n");
    }
    int count = 0;
	if (g_pRegistered)
    {
        DxMsg(2, __FUNCTION__, __LINE__,"Script pack registration scanning...\n");
        KeyValues *pTemp = g_pRegistered->GetFirstValue();
        while (pTemp) {
            bool bDisabled = TRUE;
            DxMsg(2, __FUNCTION__, __LINE__,"RegisteredScriptExists found value: %s with int: %d\n", pTemp->GetName(), pTemp->GetInt());
            if (pTemp->GetInt()>0) {
                bDisabled = FALSE;
            }
            if (nUserid > 0)
            {
                engine->ClientCommand(LookupEdictByUserid(nUserid), "echo %02d:   %s   \"%s\"", count, bDisabled ? "[off]" : "[on] ", pTemp->GetName());
            }
            else
            {
                DxMsg(0, __FUNCTION__, __LINE__,"%02d:   %s   \"%s\"\n", count, bDisabled ? "[off]" : "[on] ", pTemp->GetName());
            }
            pTemp = pTemp->GetNextValue();
            count++;
        }
    }
    else
    {
        DxMsg(1, __FUNCTION__, __LINE__," Error: No registration system available because you've never loaded a script pack. (Script pack subsystem is deactivated until you register the first script pack with eventscripts_register.)\n");
    }
    if (nUserid > 0) 
    {
        engine->ClientCommand(LookupEdictByUserid(nUserid), "echo ------------------------------------------");
    }
    else
    {
        DxMsg(0, __FUNCTION__, __LINE__,"------------------------------------------\n");
    }
    
}


//---------------------------------------------------------------------------------
// Purpose: called when the plugin is unloaded (turned off)
//---------------------------------------------------------------------------------
void CEventScriptsPlugin::Unload( void )
{
    DxMsg(0, __FUNCTION__, __LINE__, "[EventScripts] shutting down...\n");
	gameeventmanager->RemoveListener( this ); // make sure we are unloaded from the event system

	// Free memory taken up by g_pUserMessages
	if( g_pUserMessages )
		delete g_pUserMessages;

#ifdef ORANGEBOX
	ConVar_Unregister( );
#ifdef CSGO
	DisconnectInterfaces();
#else
	DisconnectTier2Libraries( );
	DisconnectTier1Libraries( );
#endif
#endif
    DxMsg(0, __FUNCTION__, __LINE__, "[EventScripts] shut down successfully.\n");
#ifdef WIN32
    // you can spare a second for the message, can't you?
    Sleep(1000);
#endif
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is paused (i.e should stop running but isn't unloaded)
//---------------------------------------------------------------------------------
void CEventScriptsPlugin::Pause( void )
{
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is unpaused (i.e should start executing again)
//---------------------------------------------------------------------------------
void CEventScriptsPlugin::UnPause( void )
{

}


//---------------------------------------------------------------------------------
// Purpose: the name of this plugin, returned in "plugin_print" command
//---------------------------------------------------------------------------------
const char *CEventScriptsPlugin::GetPluginDescription( void )
{
    return PLUGIN_VERSION_DESCRIPTION;
}

void CEventScriptsPlugin::DefaultEventRegistration()
{
    DxMsg(1, __FUNCTION__, __LINE__," ** Registering files...\n");
    if (!RegisterForEventFile("resource/modevents.res")) {
        if (!RegisterForEventFile("Resource/modevents.res")) {
			DxMsg(0, __FUNCTION__, __LINE__, "[EventScripts] ERROR: Couldn't load modevents.res.");
		}
    }
	if (!RegisterForEventFile("../hl2/resource/gameevents.res")) {
		if (!RegisterForEventFile("resource/gameevents.res")) {
			DxMsg(0, __FUNCTION__, __LINE__, "[EventScripts] ERROR: Couldn't load gameevents.res.");
		}
	}
	if (!RegisterForEventFile("../hl2/resource/serverevents.res")) {
		if (!RegisterForEventFile("resource/serverevents.res")) {
			DxMsg(0, __FUNCTION__, __LINE__, "[EventScripts] ERROR: Couldn't load serverevents.res.");
		}
	}

    
    if (!RegisterForEventFile("addons/mattie_eventscripts.res"))
    {
        DxMsg(0, __FUNCTION__, __LINE__,"ERROR: COULDN'T LOAD MATTIE CORE EVENTS!\n");
    }
	DxMsg(1, __FUNCTION__, __LINE__," ** Done registering default event files.\n");
}

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CEventScriptsPlugin::LevelInit( char const *pMapName )
{
   // initialize these
    g_nEifaceLevel = interface_cvar.GetInt();
    g_nDebugLevel = debug_cvar.GetInt();
    g_nDebugLog = debuglog_cvar.GetInt();
    g_bFirstLevelLoaded = true;

	DxMsg(0, __FUNCTION__, __LINE__, "[EventScripts] Map Loading Mattie's EventScripts %s %s...\n", VER_PRIVATEBUILD_STR, VERSIONBASE); 

    if (!m_pSay && sayevents_cvar.GetInt() > 0)
    {
        // hook say and say_team
        m_pSay = CSayWrap::BuildHook("say");
        m_pSayTeam = CSayWrap::BuildHook("say_team");
        DxMsg(1, __FUNCTION__, __LINE__, " ** Say commands wrapped.\n");
    }
    if (!m_pChangeLevel && mapcommands_cvar.GetInt() > 0)
    {
        m_pChangeLevel = CChangeLevelWrap::BuildHook("changelevel");
        DxMsg(1, __FUNCTION__, __LINE__, " ** Changelevel command wrapped.\n");
    }
    currentmap_cvar.SetValue(pMapName);

    if (g_nESEventRegEnabled !=0)
    {
        DefaultEventRegistration();
    }
    if (g_nESEnabled!=0) {
        // anyone with a server.cfg in a subdirectory should get a chance to fire.
        DxMsg(1, __FUNCTION__, __LINE__," Executing all registered server.cfgs.\n");
        ExecAllRegistered("server");

        ResetDelayed();// CALLED IN LevelInit() !
    }

    IGameEvent * event = gameeventmanager->CreateEvent( "es_map_start");
	if ( event )
	{
		event->SetString( "mapname", pMapName );
		gameeventmanager->FireEvent( event );
	}
	DxMsg(3, __FUNCTION__, __LINE__, " es_map_start fired.\n");
    if (g_pESEntityList) 
    {
        delete[] g_pESEntityList;
        g_pESEntityList = NULL;
    }
	DxMsg(6, __FUNCTION__, __LINE__, " Deleted old entity list.\n");
    g_pESEntityList = new EntityInfo[gpGlobals->maxEntities];
    for (int i = 0;i<gpGlobals->maxEntities;i++)
    {
        edict_t *pEntCur = PEntityOfEntIndex(i);

        if(pEntCur && !pEntCur->IsFree() && pEntCur->GetUnknown())
        {
            strcpy(g_pESEntityList[i].szName, pEntCur->GetClassName());
        }
        else
        {
            strcpy(g_pESEntityList[i].szName, "");
        }
    }
	DxMsg(3, __FUNCTION__, __LINE__, " Built entity list.\n");

    ParseResetVariables();
	DxMsg(6, __FUNCTION__, __LINE__, " Reset variables for muParser.\n");

#ifdef ORANGEBOX
    if (autorefreshvars_cvar.GetInt() > 0)
    {
        ForEachConCommandBase(RefreshNotifyVars);
	    DxMsg(3, MSGLOC, " Refreshed public variables.\n");
    }
#endif

    DxMsg(0, __FUNCTION__, __LINE__, "[EventScripts] Loaded successfully.\n"); 
}

//---------------------------------------------------------------------------------
// Purpose: called on level start, when the server is ready to accept client connections
//		edictCount is the number of entities in the level, clientMax is the max client count
//---------------------------------------------------------------------------------
void CEventScriptsPlugin::ServerActivate( edict_t *pEdictList, int edictCount, int clientMax )
{
    maxplayers = clientMax;
}


//---------------------------------------------------------------------------------
// Purpose: called once per server frame, do recurring work here (like checking for timeouts)
//---------------------------------------------------------------------------------
void CEventScriptsPlugin::GameFrame( bool simulating )
{
    float nowt = 0.0f;
    if (g_nFrameTimer)
    {
        nowt = GetTime();
    }
    if(g_nESEnabled==1) {
        CheckDelayed(); // CALLED IN GameFrame() !.
#ifdef PYTHON_SUPPORT
        if (g_nPythonEnabled==1 && g_bPythonTried && g_NumTickListenersRegistered > 0) {
            PyObject_CallFunctionObjArgs(g_pEsAddonsTick, NULL);
            if (PyErr_Occurred())
            {
                PyErr_Print();
            }
        }
#endif
    }
    if (g_nFrameTimer)
    {
        float thent = GetTime();
        float diff = thent - nowt;
        if (diff > 0.01)
        {
            DxMsg(0, __FUNCTION__, __LINE__, "[EventScripts] Long frame: %f seconds\n", diff);
        }
    }
}

//---------------------------------------------------------------------------------
// Purpose: called on level end (as the server is shutting down or going to a new map)
//---------------------------------------------------------------------------------
void CEventScriptsPlugin::LevelShutdown( void ) // !!!!this can get called multiple times per map change
{

	gameeventmanager->RemoveListener( this );
}

//---------------------------------------------------------------------------------
// Purpose: called when a client spawns into a server (i.e as they begin to play)
//---------------------------------------------------------------------------------
void CEventScriptsPlugin::ClientActive( edict_t *pEntity )
{
}

//---------------------------------------------------------------------------------
// Purpose: called when a client leaves a server (or is timed out)
//---------------------------------------------------------------------------------
void CEventScriptsPlugin::ClientDisconnect( edict_t *pEntity )
{
}

//---------------------------------------------------------------------------------
// Purpose: called when the client is put in the server
//---------------------------------------------------------------------------------
void CEventScriptsPlugin::ClientPutInServer( edict_t *pEntity, char const *playername )
{
}

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CEventScriptsPlugin::SetCommandClient( int index )
{
	m_iClientCommandIndex = index;
}

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CEventScriptsPlugin::ClientSettingsChanged( edict_t *pEdict )
{
	IGameEvent * event = gameeventmanager->CreateEvent( "es_player_setting");
	if ( event )
	{
		event->SetInt( "userid", engine->GetPlayerUserId(pEdict) );
		gameeventmanager->FireEvent( event );
	}
}

//---------------------------------------------------------------------------------
// Purpose: called when a client joins a server
//---------------------------------------------------------------------------------
PLUGIN_RESULT CEventScriptsPlugin::ClientConnect( bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen )
{
	return PLUGIN_CONTINUE;
}


//---------------------------------------------------------------------------------
// Purpose: called when a client types in a command (only a subset of commands however, not CON_COMMAND's)
//---------------------------------------------------------------------------------
#ifdef ORANGEBOX
PLUGIN_RESULT CEventScriptsPlugin::ClientCommand( edict_t *pEntity, const CCommand &args)
#else
PLUGIN_RESULT CEventScriptsPlugin::ClientCommand( edict_t *pEntity)
#endif
{
#ifdef ORANGEBOX
	g_pCmdArg = &args;
#endif
	const char *pcmd = ARGV(0);
	if ( !pEntity || pEntity->IsFree() ) 
	{
		return PLUGIN_CONTINUE;
	}

#ifdef PYTHON_SUPPORT
    if (g_nESEnabled > 0 && g_nPythonEnabled > 0)
    {
        int nUserId = engine->GetPlayerUserId(pEntity);
        if (nUserId > 0)
        {
            PyObject *TheUserId = Py_BuildValue("i", nUserId);
            PyObject *pbReturn = PyObject_CallFunctionObjArgs(g_pEsAddonsClientCommand, TheUserId, NULL);
            bool bReturner = !!PyInt_AsLong(pbReturn);

            if (PyErr_Occurred())
            {
                DxMsg(0, __FUNCTION__, __LINE__, "error happened.\n");
                PyErr_Print();
            }
            else
            {
                if (!bReturner)
                    return PLUGIN_STOP;
            }
            if (TheUserId) Py_DECREF(TheUserId);
            if (pbReturn) Py_DECREF(pbReturn);
        }
    }
#endif

    // check for the prefix they specified.
    bool bFireClientCommand = false;
    for (char *p=(char *)cmdprefix_cvar.GetString(); *p!='\0'; p++)
    {
       if (*p == pcmd[0]) 
       {
           DxMsg(2, __FUNCTION__, __LINE__,"Client command received, time to fire es_client_command for: %s\n", pcmd);
           bFireClientCommand = true;
           break;
       }
    }
    if (bFireClientCommand || FStrEq(pcmd, "menuselect"))
    {
	    IGameEvent * event = gameeventmanager->CreateEvent( "es_client_command");
	    if ( event )
	    {
		    event->SetInt( "userid", engine->GetPlayerUserId(pEntity) );
		    event->SetString( "command", pcmd );
            event->SetString( "commandstring", ARGS() );
		    gameeventmanager->FireEvent( event );
	    }
        else
        {
            DxMsg(0, __FUNCTION__, __LINE__,"es_client_command not fired! =(\n");
        }
    }

    if (g_nESEnabled > 0)
    {
        char szCommandName[MAX_COMMAND_STRING];
        strcpy(szCommandName, pcmd);
        const char * blockname = (const char *)g_ClientCmdTable.FindElement(strlwr(szCommandName));
        if (blockname)
        {
            char szCommandLine[MAX_COMMAND_STRING];
            if (ARGS())
            {
                sprintf(szCommandLine, "%s %s", pcmd, ARGS());
            }
            else
            {
                sprintf(szCommandLine, "%s", pcmd);
            }
            g_nCmdUserid = engine->GetPlayerUserId(pEntity);
            doblock(blockname, szCommandLine);
            return PLUGIN_STOP;
        }

    }

	return PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: called when a client is authenticated
//---------------------------------------------------------------------------------
PLUGIN_RESULT CEventScriptsPlugin::NetworkIDValidated( const char *pszUserName, const char *pszNetworkID )
{
	IGameEvent * event = gameeventmanager->CreateEvent( "es_player_validated");
	if ( event )
	{
		event->SetString( "name", pszUserName );
        event->SetString( "networkid", pszNetworkID );
		gameeventmanager->FireEvent( event );
	}

	return PLUGIN_CONTINUE;
}


bool AnyScriptEventExists(const char *szEventName);

//---------------------------------------------------------------------------------
// Purpose: called when an event is fired
//---------------------------------------------------------------------------------
void CEventScriptsPlugin::FireGameEvent( IGameEvent *event )
{
	const char * name = event->GetName();

    if(name && g_nESEnabled==1)
    {
        DxMsg(2, __FUNCTION__, __LINE__,"Event fired: %s\n", name);
        if(FStrEq(name, "weapon_reload") || FStrEq(name, "player_footstep") || FStrEq(name, "weapon_reload") || FStrEq(name, "weapon_fire")|| FStrEq(name, "bullet_impact"))
        {
            if (g_nESNoisy!=1)
            {
                return;
            }
        }
        // else we execute the config file

        char szCommand[1028];
        sprintf(szCommand, "cfg/%s/%s.cfg", scriptdir_cvar.GetString(), name);
        bool bRootCfgExists = filesystem->FileExists(szCommand);
        if(bRootCfgExists || RegisteredScriptExists(name) || AnyScriptEventExists(name))
        {
            DxMsg(2, __FUNCTION__, __LINE__,"Event has a script-- building extended variables.\n");
            // add in special event variables for people to use.
            // 
            event->SetString("es_event", name);
			int nUserid = event->GetInt("userid");
            if(nUserid)
            {
                const char * pString = GetPlayerName(nUserid);
                if (pString)
                {
                    event->SetString("es_username", pString);
                    DxMsg(3, __FUNCTION__, __LINE__,"es_username set: %s\n", pString);
                }
                pString = GetPlayerSteamID(nUserid);
                if (pString)
                {
                    event->SetString("es_steamid", pString);
                    DxMsg(3, __FUNCTION__, __LINE__,"es_steamid set: %s\n", pString);
                }
                pString = GetPlayerWeaponName(nUserid);
                if (pString) 
                {
                    event->SetString("es_userweapon", pString);
                    DxMsg(3, __FUNCTION__, __LINE__,"es_userweapon set: %s\n", pString);
                }
                event->SetInt("es_userteam", GetPlayerTeam(nUserid));
                DxMsg(3, __FUNCTION__, __LINE__,"es_userteam set.\n");
                event->SetInt("es_userarmor", GetPlayerArmor(nUserid));
                DxMsg(3, __FUNCTION__, __LINE__,"es_userarmor set.\n");
                event->SetInt("es_userhealth", GetPlayerHealth(nUserid));
                DxMsg(3, __FUNCTION__, __LINE__,"es_userhealth set.\n");
                event->SetInt("es_userdeaths", GetPlayerDeaths(nUserid));
                DxMsg(3, __FUNCTION__, __LINE__,"es_userdeaths set.\n");
                event->SetInt("es_userkills", GetPlayerFrags(nUserid));
                DxMsg(3, __FUNCTION__, __LINE__,"es_userkills set.\n");
                event->SetInt("es_userdead", IsDead(nUserid));
                DxMsg(3, __FUNCTION__, __LINE__,"es_userdead set.\n");
                event->SetInt("es_userindex", g_EventScriptsPlugin.LookupIndexByUserID(nUserid));
                DxMsg(3, __FUNCTION__, __LINE__,"es_userindex set.\n");
            }
			int nAttackerUserid = event->GetInt("attacker");
            if (nAttackerUserid)
            {
                const char * pString = GetPlayerName(nAttackerUserid);
                if (pString)
                {
                    event->SetString("es_attackername", pString);
                    DxMsg(3, __FUNCTION__, __LINE__,"es_attackername set.\n");
                }
                pString = GetPlayerSteamID(nAttackerUserid);
                if (pString)
                {
                    event->SetString("es_attackersteamid", pString);
                    DxMsg(3, __FUNCTION__, __LINE__,"es_attackersteamid set.\n");
                }
                pString = GetPlayerWeaponName(nAttackerUserid);
                if (pString)
                {
                    event->SetString("es_attackerweapon", pString);
                    DxMsg(3, __FUNCTION__, __LINE__,"es_attackerweapon set.\n");
                }
                event->SetInt("es_attackerteam", GetPlayerTeam(nAttackerUserid));
                DxMsg(3, __FUNCTION__, __LINE__,"es_attackerteam set.\n");
                event->SetInt("es_attackerarmor", GetPlayerArmor(nAttackerUserid));
                DxMsg(3, __FUNCTION__, __LINE__,"es_attackerarmor set.\n");
                event->SetInt("es_attackerhealth", GetPlayerHealth(nAttackerUserid));
                DxMsg(3, __FUNCTION__, __LINE__,"es_attackerhealth set.\n");
                event->SetInt("es_attackerdeaths", GetPlayerDeaths(nAttackerUserid));
                DxMsg(3, __FUNCTION__, __LINE__,"es_attackerdeaths set.\n");
                event->SetInt("es_attackerkills", GetPlayerFrags(nAttackerUserid));
                DxMsg(3, __FUNCTION__, __LINE__,"es_attackerkills set.\n");
                event->SetInt("es_attackerdead", IsDead(nAttackerUserid));
                DxMsg(3, __FUNCTION__, __LINE__,"es_attackerdead set.\n");
                event->SetInt("es_attackerindex", g_EventScriptsPlugin.LookupIndexByUserID(nAttackerUserid));
                DxMsg(3, __FUNCTION__, __LINE__,"es_attackerindex set.\n");
            }
            DxMsg(4, __FUNCTION__, __LINE__,"Event has fired, duplicating event and pushing it to stack. Before: %d\n", 
                  g_pEventQueue.Count());
            IGameEvent *pTempEvent = gameeventmanager->DuplicateEvent(event);
            g_pEventQueue.Insert(pTempEvent);
            DxMsg(4, __FUNCTION__, __LINE__,"Duplicated event and pushed it to stack. After: %d\n", 
                  g_pEventQueue.Count());
            // Send out pop event so we know to grab it back out.
            DxMsg(4, __FUNCTION__, __LINE__,"Sending es__ep command.\n");
            char szSrvCommand[MAX_COMMAND_STRING];
            sprintf(szSrvCommand, "es__ep %d\n", (int)pTempEvent);
            ServerCommand(szSrvCommand);

            if (bRootCfgExists)
            {
                // build the client command
                sprintf(szCommand, "%s %s/%s.cfg\n", execmd_cvar.GetString(), scriptdir_cvar.GetString(), name);
                DxMsg(2, __FUNCTION__, __LINE__,"Sending %s command for %s.\n", execmd_cvar.GetString(), name);
                ServerCommand(szCommand); 
            }
            
            // v0.7.5 added support for multiple subdirectories for script packs.
            // e.g. cfg\events\KingOfTheHill\player_say.cfg
            ExecAllRegistered(name);
            
            // get rid of the temporary subkey
        }
        DxMsg(2, __FUNCTION__, __LINE__,"Done with an event: %s\n", name);
    }
}



typedef enum _VarType {
    eNone       = 0,
    eEventVar   = 1,
    eServerVar  = 2
} VarType;

int ParseVar(const CCommand &args, char * szVirtualTokenOut, int i, int nMaxTokens)
{
    VarType vtype = eNone;
    char szTempFloat[1028];
    char szTempToken[MAX_COMMAND_STRING];
    char * pThisToken = (char*)args.Arg(i);
    char * pCurrent = pThisToken;
    IGameEvent *pCurrentKey = pEventScriptsKey;
    DxMsg(5, __FUNCTION__, __LINE__,"ParseVar: Current Token: %s\n", pThisToken);
    if (FStrEq("event_var",pThisToken)) {
        vtype = eEventVar;
    }
    else if (FStrEq("server_var",pThisToken))  {
        vtype = eServerVar;
    }
    if (vtype == eEventVar || vtype == eServerVar) {
        DxMsg(5, __FUNCTION__, __LINE__,"ParseVar: Need to expand variable: %s\n", pThisToken);
        // now we have a valid key, if they're using keys in a bad way, error out 
        if(++i<nMaxTokens && (pCurrent=(char *)args.Arg(i)))                {
            if (pCurrent[0]='(')                    {
                if (++i<nMaxTokens && (pCurrent=(char *)args.Arg(i))) {
                    if (pCurrent) DxMsg(5, __FUNCTION__, __LINE__,"ParseVar: Variable Name '%s'...\n", pCurrent);
                    if (strlen(pCurrent)>=MAX_COMMAND_STRING) return -1;
                    strcpy(szTempToken, pCurrent);
                    i = ParseVar(args, szTempToken, i, nMaxTokens);
                    if (i<0) return i;
                    pCurrent = szTempToken;
                    if (++i<nMaxTokens && (args.Arg(i)[0]==')'))  {
                        // pCurrent now points to the valuename
                        const char * pTest = NULL;
                        if (vtype == eEventVar)  {
                            if (pCurrentKey && pCurrent)
                            {
                                pTest = pCurrentKey->GetString(pCurrent);
                            }
                            else
                            {
                                DxMsg(5, __FUNCTION__, __LINE__,"ParseVar: No event was found when event_var used!\n");
                                pTest = "";
                            }
                        } 
                        else if (vtype == eServerVar) {
                            ConVar * ctest = FindVar(pCurrent);
                            if (ctest)
                            {
                                if (!(ctest->m_nFlags & FCVAR_PROTECTED))
                                {
                                    DxMsg(5, __FUNCTION__, __LINE__,"ParseVar: Name: %s, Value: s%s i%d\n", ctest->GetName(), ctest->GetString(), ctest->GetInt());
                                    // always want int if
                                    //    float and int are the same and non-zero (means the string is a number)
                                    //    float and int are zero and string 0.00000
                                    //
                                    // 0 0.00000 0.00000  -- int
                                    // 1 1.00000 1.00000  -- int
                                    // 2 2.10000 2.10000  -- float
                                    // 0 0.00000 Blah     -- string
                                    // 3 3.00000 3        -- int
                                    if (ctest->GetFloat() - (float)ctest->GetInt() != 0.0f)
                                    {
                                        // we have a float, take the string portion
                                        DxMsg(5, __FUNCTION__, __LINE__,"ParseVar: Looks like a float, use the string version, %s\n", ctest->GetString());
                                        pTest = ctest->GetString();    

                                    }
                                    else
                                    {
                                        char *stopstring = NULL; 
                                        char *stopstring2 = NULL; 
                                        long _n = strtol(ctest->GetString(), &stopstring, 10);
                                        float _f = strtod(ctest->GetString(), &stopstring2);
                                        if (FStrEq("", stopstring) || FStrEq("", stopstring2))
                                        {
                                            // we convert int to string and use that.
                                            sprintf(szTempFloat, "%ld", _n);
                                            pTest = szTempFloat;
                                        }
                                        else
                                        {
                                            DxMsg(5, __FUNCTION__, __LINE__,"ParseVar: Thought it was a #, but got string instead of #, %s\n", ctest->GetString());
                                            pTest = ctest->GetString();    
                                        }

                                    }
                                }
                                else
                                {
                                    DxMsg(1, __FUNCTION__, __LINE__,"WARNING: Server variable is protected: %s\n", pCurrent);
                                    pTest = "(protected)";
                                    strcpy(szVirtualTokenOut, "(protected)");
                                }

                            }
                            else {
                                DxMsg(1, __FUNCTION__, __LINE__,"WARNING: Couldn't find server variable: %s\n", pCurrent);
                                pTest = NULL;
                                strcpy(szVirtualTokenOut, "0");
                            }

                        }
                        else {
                            pTest = NULL;
                            ErrMsg(0,"ERROR: not a valid variable type.\n");
                            strcpy(szVirtualTokenOut, "0");
                        }
                       
                        if (pTest)  { 
                            if(pTest[0] == '\0') {
                                strcpy(szVirtualTokenOut, "0");
                            }
                            else  {
                                // strip all the quotes
                                char pSanitize[MAX_COMMAND_STRING];
                                pSanitize[0]='\0';
                                
                                // try to replace quotes with nothing
                                DxMsg(5, __FUNCTION__, __LINE__,"ParseVar: Replacing variable quotes with `...\n");
                                // BUG: Nested names can truncate.
                                V_StrSubst(pTest, "\"", "`", pSanitize, MAX_COMMAND_STRING);
                                // escape other characters.
                                if(strcspn(pSanitize, escape_cvar.GetString())!=strlen(pSanitize) || strstr(pSanitize, "//"))  {
                                    if (strlen(pSanitize)+2>=MAX_COMMAND_STRING) return -1;
                                    strcpy(szVirtualTokenOut, "\"");
                                    strcat(szVirtualTokenOut, pSanitize);   
                                    strcat(szVirtualTokenOut, "\"");
                                    DxMsg(2, __FUNCTION__, __LINE__,"ParseVar: Quoted variable because it contained eventscripts_escape characters...\n");
                                }
                                else // we can hopefully leave it alone
                                {
                                    strcpy(szVirtualTokenOut, pSanitize);  
                                }
                            }
                            DxMsg(5, __FUNCTION__, __LINE__,"ParseVar: Variable expanded, %s.\n", pTest);
                        }
                        else {
                            DxMsg(5, __FUNCTION__, __LINE__,"ParseVar: Wasn't valid server variable!\n");
                        }
                    }
                    else    {
                        // badly formed
                        return -1;
                    }
                }
                else        {
                    // badly formed
                    return -1;
                }
            }
            else                    {
                // badly formed
                return -1;
            }
        }
        else                {
            //badly formed 
            return -1; 
        }

    }
    DxMsg(5, __FUNCTION__, __LINE__,"ParseVar: Done trying to expand variables.\n");
    return i;
        
}

int ParseCommand(const CCommand &args, char * szDestination, int nToken, int nMaxTokens, char appendChar)
{
    if (nToken<0 || nToken>=nMaxTokens || nMaxTokens>args.ArgC()) return -1;
    char * pCurrent = (char *)args.Arg(nToken);
    char szVirtualToken[MAX_COMMAND_STRING];

    // loop through Tokens
    for (int i=nToken;i<nMaxTokens;i++)
    {
        char *pThisToken = (char *)args.Arg(i);
        DxMsg(5, __FUNCTION__, __LINE__,"ParseCommand: token: %s\n", pThisToken);
        //   loop through KeyValues
        IGameEvent *pCurrentKey = pEventScriptsKey;
        if (pCurrentKey)
        {
            DxMsg(5, __FUNCTION__, __LINE__,"ParseCommand: CurrentKey: %s\n", pCurrentKey->GetName());
        }
        int lasti = i;
        if (strlen(pThisToken)>=MAX_COMMAND_STRING) return -1;
        strcpy(szVirtualToken, pThisToken);
        i = ParseVar(args, szVirtualToken, i, nMaxTokens);
        if (i<0)
        {
            ErrMsg(0,"ParseCommand: Error in string format.\n");
            ErrVar("Error in string format.");
            return -1;
        }
        int nLength = strlen(szVirtualToken);
        // we need to re-add the quotes if they were removed
        bool bQuote = i==lasti && (pThisToken[0]==';' ||
            (nLength!=1 && strcspn(pThisToken, escape_cvar.GetString())!=strlen(pThisToken)));
        if (strlen(szDestination)+nLength+(bQuote ? 3 : 1)>=MAX_COMMAND_STRING)
        {
            ErrMsg(0,"ParseCommand: Command too long.\n");
            ErrVar("Command too long.");
            return -1;
        }
        if (bQuote) strcat(szDestination, "\"");
        strcat(szDestination, szVirtualToken);
        strcat(szDestination, bQuote ? "\" " : " ");
    }
    DxMsg(5, __FUNCTION__, __LINE__, "ParseCommand: command: %s\n", szDestination);
    // commands need a carriage return. Since we have a trailing space, let's replace it.
    szDestination[strlen(szDestination)-1]=appendChar;
    return nMaxTokens;
}



CON_COMMAND(es__ip, "Internal command\n")
{
    g_LatestInstruction.nIP = 0;
    if (g_ShellVM.ExecNextInstruction((void *)atoi(ARGS()), &g_LatestInstruction))
    {
        if (g_LatestInstruction.nIP != 0)
        {
            char szCommand[MAX_COMMAND_STRING];
            sprintf(szCommand, "%s;es__ip %d\n", g_LatestInstruction.szCommand,g_LatestInstruction.nIP);
            ImmediateExecuteCommand(szCommand);
        }
		else
		{
			ImmediateExecuteCommand(g_LatestInstruction.szCommand);
		}
		DxMsg(7, __FUNCTION__, __LINE__,"ip.cmd: %s\nip.line: %s\n", g_LatestInstruction.szCommand, g_LatestInstruction.szLineNum);
		if (g_nTraceLevel == 1 && !(g_LatestInstruction.szCommand[2] == '_' && g_LatestInstruction.szCommand[3] == '_'))
		{
			DxMsg(0, __FUNCTION__, __LINE__, "[%s] %s", g_LatestInstruction.szLineNum, g_LatestInstruction.szCommand);
		}
    }
    else
    {
       g_LatestInstruction.szLineNum = "(console)";
       g_LatestInstruction.szCommand = "(no script)\n";
    }
	DxMsg(7, __FUNCTION__, __LINE__, "es__ip done\n");
    return;
}

CON_COMMAND(es, "Mattie's EventScripts variable parsing\n   usage: es <command string>\nThis will execute <command string> after parsing it for items like event_var() and server_var() and replacing them with the appropriate value.\n" )
{
    int nCount = ARGC();
    if(nCount < 2)
    {
        DxMsg(0, __FUNCTION__, __LINE__,"es: Please provide a command string.\n");
        ErrVar("No arguments provided.");
        return;
    }
    char szExpandCommand[MAX_COMMAND_STRING];
    szExpandCommand[0]='\0';
    if (g_nDebugLevel>4) {
        for (int i = 0;i<nCount;i++)
        {
            DxMsg(5, __FUNCTION__, __LINE__,"   Arg%d: %s\n", i, ARGV(i));
        }
    }

	PARSE_ARGS_FROM_ENGINE();

    if (ParseCommand(args, szExpandCommand, 1, nCount)>0)
    {
        DxMsg(2, __FUNCTION__, __LINE__,"es executing: %s\n", szExpandCommand);

        ExecuteCommandNow(szExpandCommand);
        return;
    }
    else
    {
        DxMsg(0, __FUNCTION__, __LINE__,"es: Error parsing command string: %s\n", ARGS());
        ErrVar("Error while parsing command string.");
        return;
    }
   
}

// es__doblock wrapper
CON_COMMAND(es__db, "Internal command\n")
{
    doblock(ARGS());
}

// es_doevent
CON_COMMAND(es__de, "Internal commandn ")
{
    const char * szBlockT = ARGS();
    unsigned long nBlock = strtoul(szBlockT, NULL, 16);
    DxMsg(4, __FUNCTION__, __LINE__,"es__de %x;\n", nBlock);
    VMInstruction Next;
    if (g_ShellVM.ExecEvent(nBlock, &Next))
    {
        DxMsg(5, __FUNCTION__, __LINE__, "doevent preparing:\n   NextIP: 0x%x\n      Cmd: %s", Next.nIP, Next.szCommand);
        if (Next.nIP != 0)
        {
            char szCommand[MAX_COMMAND_STRING];
              sprintf(szCommand, "%s;es__ip %d\n", Next.szCommand,Next.nIP);
            ImmediateExecuteCommand(szCommand);
        }
        else
        {
            DxMsg(2, __FUNCTION__, __LINE__,"Null instruction pointer provided for block: %u.\n", nBlock);
			ImmediateExecuteCommand(Next.szCommand);
        }
        
    }
    else
    {
        DxMsg(2, __FUNCTION__, __LINE__,"Block was refused: %u.\n", nBlock);
    }
    return;
}

// es__startblock
CON_COMMAND(es__sb, "Internal command\n")
{
    g_ShellVM.StartBlock();
    DxMsg(4, __FUNCTION__, __LINE__,"es__startblock %s\n", ARGS());
    return;
}
// es__endblock
CON_COMMAND(es__eb, "Internal command\n")
{
	DxMsg(7, __FUNCTION__, __LINE__,"es__endblock started.\n");
    g_ShellVM.EndBlock();
    DxMsg(4, __FUNCTION__, __LINE__,"es__endblock\n");
    return;
}


ES_CON_COMMAND2(doblock, 
               "Executes a block.\n",
               2,
               "<script>/<function>")
	DxMsg(4, __FUNCTION__, __LINE__,"es_xdoblock: Executing block: %s\n", ARGS());
	doblock(ARGS());
}


// es__eventpop
CON_COMMAND( es__ep, "Internal command used by EventScripts. Do not use unless you're willing to crash your server.\n" )
{
    int nArgc = ARGC();
    IGameEvent *pEvent = NULL;
    if (nArgc > 1)
    {
        pEvent = (IGameEvent *)atoi(ARGV(1));
    }
    DxMsg(5, __FUNCTION__, __LINE__, "Internal pop event received.\n");
    if (pEventScriptsKey)
    {
        DxMsg(5, __FUNCTION__, __LINE__,"Freeing previous event.\n");
        gameeventmanager->FreeEvent(pEventScriptsKey);
    }
    DxMsg(5, __FUNCTION__, __LINE__,"Popping event pointer.\n");
    if(g_pEventQueue.Count()>0)
    {
        DxMsg(5, __FUNCTION__, __LINE__,"Counted event pointer, grabbing.\n");
        pEventScriptsKey = g_pEventQueue.RemoveAtHead();
        if(pEventScriptsKey) 
        {
            DxMsg(5, __FUNCTION__, __LINE__,"Grabbed event pointer: %s.\n", pEventScriptsKey ? pEventScriptsKey->GetName() : "(NULL!)");
            if (pEvent && pEventScriptsKey != pEvent)
            {
                pEventScriptsKey = pEvent;
                DxMsg(5, __FUNCTION__, __LINE__,"Changed event pointer: %s.\n", pEventScriptsKey ? pEventScriptsKey->GetName() : "(NULL!)");
            }
            return;
        } 
        else
        {
            DxMsg(1, __FUNCTION__, __LINE__,"ERROR: Failed to grab event pointer.\n");
            return;
        }
    }
    else 
    {
        DxMsg(1, __FUNCTION__, __LINE__,"ERROR: Stack empty, couldn't grab event pointer.\n");
        pEventScriptsKey = NULL;
        return;
    }
}

#include "es_client_messages.h"

enum ConVarType {
    Integer,
    Float,
    String
};


// RETURNVAL
ES_CON_COMMAND2(getargv, 
               "Gets the command parameter passed to the current ES console command.\n",
               2,
               "<var> <index>")
    const char * szIndex = bUseServerVariablesForReturn ? args.Arg(2) : args.Arg(1);
    char * val = g_ShellVM.Cmd_Argv(atoi(szIndex));
    RETURNVAL(args.Arg(1), val, "s");
}

// RETURNVAL
ES_CON_COMMAND2(getargc, 
                "Gets the count of parameters passed to the current ES console command.\n",
               1,
               "<var>")
    RETURNVAL(args.Arg(1), g_ShellVM.Cmd_Argc(), "i");
}

// RETURNVAL
ES_CON_COMMAND2(getargs, 
               "Gets the commandstring passed to the current ES console command.\n",
               1,
               "<var>")
    RETURNVAL(args.Arg(1), g_ShellVM.Cmd_Args(), "s");
}

// RETURNVAL
ES_CON_COMMAND3(getcmduserid, 
               "Gets the userid of the user that executed the command.\n",
               1,
               "<var>")
    RETURNVAL(args.Arg(1), g_nCmdUserid, "i");
}

ES_CON_COMMAND2(fire,
               "Fires an entity trigger.\n",
               3,
               "<userid> <target> [action] [value] [delay]\n")
    int nUserid = atoi(args.Arg(1));
    if (nUserid > 0) 
    {
        int i = g_EventScriptsPlugin.LookupIndexByUserID(nUserid);
        g_EventScriptsPlugin.m_pEntFire->m_bReplaceNextClient = true;
        g_EventScriptsPlugin.m_pEntFire->m_nNextCommandClientIndex = i;
        char szMessage[MAX_COMMAND_STRING];
        char szTemp[MAX_COMMAND_STRING];
        strcpy(szMessage, args.ArgS());
        sprintf(szTemp, "es__fire %s\n", strchr(szMessage, ' ') + 1);
        ExecuteCommandNow(szTemp);
    }
}

ES_CON_COMMAND2(trick,
               "Miscellaneous tricky things.\n",
               2,
               "<operation> [parameters]\n")
    const char *op = args.Arg(1);
    if (FStrEq(op, "greenblock"))
    {
        if (nCount > 2) 
        {
            int nUserid = atoi(args.Arg(2));
            if (nUserid > 0)
            {
                int i = g_EventScriptsPlugin.LookupIndexByUserID(nUserid);
                g_EventScriptsPlugin.m_pEntityBlocker->m_bReplaceNextClient = true;
                g_EventScriptsPlugin.m_pEntityBlocker->m_nNextCommandClientIndex = i;
                char szTemp[MAX_COMMAND_STRING];
                sprintf(szTemp, "es__entblock\n");
                ExecuteCommandNow(szTemp);
            }
        }
    } 
    else if (FStrEq(op, "entity"))
    {
            g_EventScriptsPlugin.m_pCreateEntity->m_bReplaceNextClient = true;
            g_EventScriptsPlugin.m_pCreateEntity->m_nNextCommandClientIndex = 0;
            char szTemp[MAX_COMMAND_STRING];
            sprintf(szTemp, "es__createentity %s\n", args.Arg(2));
            ExecuteCommandNow(szTemp);
    }
    else if (FStrEq(op, "dispatcheffect"))
    {
        int nUserid = atoi(args.Arg(2));
        if (nUserid > 0)
        {
            int i = g_EventScriptsPlugin.LookupIndexByUserID(nUserid);
            g_EventScriptsPlugin.m_pDispatchEffect->m_bReplaceNextClient = true;
            g_EventScriptsPlugin.m_pDispatchEffect->m_nNextCommandClientIndex = i;
            char szMessage[MAX_COMMAND_STRING];
            char szTemp[MAX_COMMAND_STRING];
            strcpy(szMessage, args.ArgS());
            int argc = nCount;
            sprintf(szTemp, "es__dispatcheffect %s %s %s %s\n", args.Arg(3), (argc>4 )? args.Arg(4) : "", (argc>5 )? args.Arg(5) : "", (argc>6 )? args.Arg(6) : "");
            ExecuteCommandNow(szTemp);
        }
    }
}

ES_CON_COMMAND2(give,
               "Gives the player a named item.\n",
               2,
               "<userid> <entity>\n")
    int nUserid = atoi(args.Arg(1));
    if (nUserid > 0) 
    {
        int i = g_EventScriptsPlugin.LookupIndexByUserID(nUserid);
        g_EventScriptsPlugin.m_pGive->m_bReplaceNextClient = true;
        g_EventScriptsPlugin.m_pGive->m_nNextCommandClientIndex = i;
        char szMessage[MAX_COMMAND_STRING];
        char szTemp[MAX_COMMAND_STRING];
        strcpy(szMessage, args.ArgS());
        sprintf(szTemp, "es__give %s\n", strchr(szMessage, ' ') + 1);
        ExecuteCommandNow(szTemp);

    }
}
#ifdef ORANGEBOX
ES_CON_COMMAND2(spawnplayer,
               "Spawn a player with the given userid.\n",
               1,
               "<userid>\n")
    int nIndex = g_EventScriptsPlugin.LookupIndexByUserID(atoi(args.Arg(1)));
	if (nIndex) {
		void *j = g_EventScriptsPlugin.GetEntityByIndex(nIndex);
		if (j) {
			tools->DispatchSpawn((CBaseEntity*)j);
		}
		else {
			ErrMsg(0, "Could not spawn entity: %d", nIndex);
		}
	}
	else {
			ErrMsg(0, "Could not spawn userid: %s", args.Arg(1));
	}
}
ES_CON_COMMAND2(spawnentity,
               "Spawn a given entity index.\n",
               1,
               "<index>\n")
    int nIndex = atoi(args.Arg(1));
	void *j = g_EventScriptsPlugin.GetEntityByIndex(nIndex);
	if (j) {
		tools->DispatchSpawn((CBaseEntity*)j);
	}
	else {
		ErrMsg(0, "Could not spawn entity: %d", nIndex);
	}
}

ES_CON_COMMAND2(entitysetvalue,
               "Set a value name for a given entity.\n",
               3,
               "<entity-index> <valuename> <value>\n")
	int nindex = atoi(args.Arg(1));
	const char * valuename = args.Arg(2);
	const char * value = args.Arg(3);
	void *j = g_EventScriptsPlugin.GetEntityByIndex(nindex);
	if (j) {
		tools->SetKeyValue((CBaseEntity*)j, valuename, value);
	}
	else {
		ErrMsg(0, "Entity not found: %d", nindex);
	}
}
ES_CON_COMMAND2(entitygetvalue,
               "Get a value name for a given entity.\n",
               3,
               "<variable-name> <entity-index> <valuename>\n")
    int nFirstArg = bUseServerVariablesForReturn ? 0 : -1;
    nCount+=abs(nFirstArg);

	bool bSuccess = false;
	int nindex = atoi(args.Arg(nFirstArg+2));
	const char * valuename = args.Arg(nFirstArg+3);
	char szValue[2000]; 
	strcpy(szValue, "");
	void *j = g_EventScriptsPlugin.GetEntityByIndex(nindex);
	if (j) {
		bSuccess = tools->GetKeyValue((CBaseEntity*)j, valuename, szValue, 2000);
	}
	if (!bSuccess) {
		strcpy(szValue, "");
	}
	RETURNVAL(args.Arg(1), 	szValue, "s");
}
ES_CON_COMMAND2(createentity,
               "Creates an entity somewhere by name.\n",
               1,
               "<entity-name> [targetname]\n")
	void *j = tools->CreateEntityByName(args.Arg(1));
	IServerUnknown *p = (IServerUnknown *)j;
	IServerNetworkable *n = p->GetNetworkable();
	edict_t * e = n->GetEdict();
	if (nCount > 2) {
		const char * targetname = args.Arg(2);
		tools->SetKeyValue( (CBaseEntity*)j, "targetname", targetname );
	}
	RETURNVAL("eventscripts_lastgive", 	IndexOfEdict(e), "i");
}
ES_CON_COMMAND2(setentityname,
               "Sets the targetname of an entity by index.\n",
               2,
               "<index> <targetname>\n")
    int nIndex = atoi(args.Arg(1));
	void *j = g_EventScriptsPlugin.GetEntityByIndex(nIndex);
	if (j) {
		const char * targetname = args.Arg(2);
		tools->SetKeyValue( (CBaseEntity*)j, "targetname", targetname );
	}
	else {
		ErrMsg(0, "Could not set targetname for entity: %d", nIndex);
	}
}
#endif

ES_CON_COMMAND2(entcreate,
               "Creates an entity where a player is looking.\n",
               2,
               "<userid> <entity>\n")
    int nUserid = atoi(args.Arg(1));
    if (nUserid > 0) 
    {
        int i = g_EventScriptsPlugin.LookupIndexByUserID(nUserid);
        g_EventScriptsPlugin.m_pCreateEntity2->m_bReplaceNextClient = true;
        g_EventScriptsPlugin.m_pCreateEntity2->m_nNextCommandClientIndex = i;
        char szMessage[MAX_COMMAND_STRING];
        char szTemp[MAX_COMMAND_STRING];
        strcpy(szMessage, args.ArgS());
        sprintf(szTemp, "es__createentity2 %s\n", strchr(szMessage, ' ') + 1);
        ExecuteCommandNow(szTemp);
    }
}

ES_CON_COMMAND2(prop_dynamic_create,
               "See prop_dynamic_create for syntax, but requires a userid first\n",
               2,
               "<userid> <model>\n")
    int nUserid = atoi(args.Arg(1));
    if (nUserid > 0) 
    {
        int i = g_EventScriptsPlugin.LookupIndexByUserID(nUserid);
        g_EventScriptsPlugin.m_pPropDynamic->m_bReplaceNextClient = true;
        g_EventScriptsPlugin.m_pPropDynamic->m_nNextCommandClientIndex = i;
        char szMessage[MAX_COMMAND_STRING];
        char szTemp[MAX_COMMAND_STRING];
        strcpy(szMessage, args.ArgS());
        sprintf(szTemp, "es__prop_dynamic_create %s\n", strchr(szMessage, ' ') + 1);
        ExecuteCommandNow(szTemp);
    }
}

ES_CON_COMMAND2(prop_physics_create,
               "See prop_physics_create for syntax, but requires a userid first.\n",
               2,
               "<userid> <model>\n")
    int nUserid = atoi(args.Arg(1));
    if (nUserid > 0) 
    {
        int i = g_EventScriptsPlugin.LookupIndexByUserID(nUserid);
        g_EventScriptsPlugin.m_pPropPhysics->m_bReplaceNextClient = true;
        g_EventScriptsPlugin.m_pPropPhysics->m_nNextCommandClientIndex = i;
        char szMessage[MAX_COMMAND_STRING];
        char szTemp[MAX_COMMAND_STRING];
        strcpy(szMessage, args.ArgS());
        sprintf(szTemp, "es__prop_physics_create %s\n", strchr(szMessage, ' ') + 1);
        ExecuteCommandNow(szTemp);
    }
}


class ScheduledEvent
{
public:
    float m_fTime;
    char * m_szCmd;
    char * m_szName;
    bool m_bMapOnly;
    ScheduledEvent * m_pNext;
    ScheduledEvent(float runtime, char * szCmd, bool bMapOnly = true, char * szName = NULL);
    ~ScheduledEvent();
};
ScheduledEvent::ScheduledEvent(float runtime, char * szCmd, bool bMapOnly, char * szName)
{
    m_fTime = runtime;
    m_szCmd = szCmd ? strdup(szCmd) : NULL;

    m_bMapOnly = bMapOnly;
    m_pNext = NULL;
    m_szName = szName ? strdup(szName) : NULL;

}
ScheduledEvent::~ScheduledEvent()
{
    free(m_szCmd);
    free(m_szName);
}

ScheduledEvent * g_pScheduledHead = NULL;

void CEventScriptsPlugin::ResetDelayed( void )// CALLED IN LevelInit() !
{
    DxMsg(1, __FUNCTION__, __LINE__," Resetting delayed counters.\n");
    ScheduledEvent * pTemp = g_pScheduledHead;
    ScheduledEvent * pParent = NULL;
    while (pTemp)
    {
        if (pTemp->m_bMapOnly)
        {
            if (pParent)
            {
                pParent->m_pNext = pTemp->m_pNext;
                delete pTemp;
                pTemp = pParent;
            }
            else
            {
                g_pScheduledHead = pTemp->m_pNext;
                delete pTemp;
                pTemp = g_pScheduledHead;
                pParent = NULL;
                continue;
            }
        }
        pParent = pTemp;
        pTemp = pTemp->m_pNext;
    }
} 
void CEventScriptsPlugin::CheckDelayed( void ){ // CALLED IN GameFrame() !.
    ScheduledEvent * pTemp = g_pScheduledHead;
    ScheduledEvent * pParent = NULL;
    while (pTemp)
    {
        if (GetTime() >= pTemp->m_fTime){
            char szPreparseCommand[MAX_COMMAND_STRING];
            strcpy(szPreparseCommand, "");
            strcat(szPreparseCommand, pTemp->m_szCmd);
            strcat(szPreparseCommand, "\n");
            DxMsg(2, __FUNCTION__, __LINE__,"es_xdelayed %s, %f seconds late, is executing:\n  %s;\n", pTemp->m_szName, (GetTime() - pTemp->m_fTime), pTemp->m_szCmd);
            ServerCommand(szPreparseCommand);
            // delete it now
            if (pParent)
            {
                pParent->m_pNext = pTemp->m_pNext;
                delete pTemp;
                pTemp = pParent;
            }
            else
            {
                g_pScheduledHead = pTemp->m_pNext;
                delete pTemp;
                pTemp = NULL;
                continue;
            }
            pParent = pTemp;
            pTemp = pTemp->m_pNext;
        }
        else
        {
            // stop searching if the top isn't ready
            pTemp = NULL;
        }
    }
}
ES_CON_COMMAND2(delayed, 
            "Will run <commandstring>, after <seconds> seconds.",
            3,
            "Syntax : es_xdelayed <seconds> <commandstring>")
    float future = (GetTime() + atof(args.Arg(1)));
	char *pCmd = NULL;
	// If it's a single argument let's use it directly to dequote it
	if (args.ArgC() != 3)
	{
		pCmd = strchr((char *)args.ArgS(), ' ')+1;
	}
	else
	{
		pCmd = (char *)args.Arg(2);
	}
    ScheduledEvent * pNew = new ScheduledEvent(future, pCmd);
    ScheduledEvent * pTemp = g_pScheduledHead;
    ScheduledEvent * pParent = NULL;
    while (pTemp)
    {
        if (future >= pTemp->m_fTime){
            pParent = pTemp;
            pTemp = pTemp->m_pNext;
        }
        else
        {
            break;
        }
    }
    if (pParent) {
        pParent->m_pNext = pNew;
    }
    else {
        g_pScheduledHead = pNew;
    }
    pNew->m_pNext = pTemp;
    // stop searching now
    pTemp = NULL;
    DxMsg(2, __FUNCTION__, __LINE__,"es_xdelayed, will run \"%s\" in %f seconds.\n", pNew->m_szCmd, atof(args.Arg(1)));
    return;
}

// TODO: Relocate to es_keygroups.h
ES_CON_COMMAND2(keysetvalue, 
               "Sets a value within a given key (where the key could be free-floating or associated with a group).\n",
               4,
               "[group] <key> <valuename> <value>")
    KeyValues *pGroup = NULL;
    char szKeyName[MAX_FORMATTED];
    char szValueName[MAX_FORMATTED];
    char szValue[MAX_FORMATTED];
    if (!g_pUserKeys)
    {
        ErrMsg(0,"ERROR: Eventscripts key access attempted before keys have been initialized.\n");
        ErrVar("Keys not ready, yet");
        return;
    }
    if (args.ArgC() == 5)
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
        strcpy(szValueName, args.Arg(3));
        strcpy(szValue, args.Arg(4));
    }
    else
    {
        // use the ungrouped group.
        pGroup = g_pUserKeys->FindKey(KEYNAME_UNGROUPED);
        strcpy(szKeyName, args.Arg(1));
        strcpy(szValueName, args.Arg(2));
        strcpy(szValue, args.Arg(3));
    }
    KeyValues *pNewKey = pGroup->FindKey(szKeyName);
    if (!pNewKey)
    {
        ErrMsg(0,"ERROR: Eventscripts cannot find/create the %s key!\n", szKeyName);
        ErrVar("Key could not be found or created");
        return;
    }
    pNewKey->SetString(szValueName, szValue);
    DxMsg(1, __FUNCTION__, __LINE__,"%s key's %s value was set to %s.\n", szKeyName, szValueName, szValue);
    return;
}

// RETURNVAL
ES_CON_COMMAND2(exists, 
               "Checks whether a keygroup, keys, variable, or function exists.\n", 
               3, 
               "<var> <type> [optional] [optional] <name-to-check>")
    int nFirstArg = bUseServerVariablesForReturn ? 0 : -1;
    nCount+=abs(nFirstArg);

    int nExists = 0;
    const char *var = args.Arg(1);
    const char *type = args.Arg(nFirstArg+2);
    const char *name = args.Arg(nFirstArg+3);
    const char *name2 = nCount > 4 ? args.Arg(nFirstArg+4) : NULL;
    const char *name3 = nCount > 5 ? args.Arg(nFirstArg+5) : NULL;
    if (FStrEq(type, "variable"))
    {
        if(NULL!=FindVar(name))
        {
            nExists = 1;
        }
        else
        {
            nExists = 0;
        }
    }
    else if (FStrEq(type, "map"))
    {
        char szMapName[MAX_FORMATTED];
        snprintf(szMapName, MAX_FORMATTED-1, "%s", name);
        nExists = NULL != engine->IsMapValid(szMapName);
    }
    else if (FStrEq(type, "saycommand"))
    {
        nExists = NULL != g_SayCmdTable.FindElement(name);
    }
    else if (FStrEq(type, "clientcommand"))
    {
        nExists = NULL != g_ClientCmdTable.FindElement(name);
    }
    else if(FStrEq(type, "command"))
    {
      ConCommandBase *pPtr = FindCommandEx(name);
      nExists = 0;
      int nCounter = 0;
      if (pPtr)
      {
        if (pPtr->IsCommand())
        {
            nExists = 1;
        }
      }
      DxMsg(1, __FUNCTION__, __LINE__, "Found command %s\n", name);
    }
    else if(FStrEq(type, "keygroup"))
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
        KeyValues *pNewGroup = pUserGroups->FindKey(name, false);
        if (!pNewGroup)
        {
            nExists = 0;
        }
        else
        {
            nExists = 1;
        }
    }
    else if(FStrEq(type, "userid"))
    {
        if(NULL!=g_EventScriptsPlugin.LookupEdictByUserid(atoi(name)))
        {
            nExists = 1;
        }
        else
        {
            nExists = 0;
        }
    }
    else if(FStrEq(type, "key"))
    {
        char szKeyName[MAX_COMMAND_STRING];
        if (!g_pUserKeys)
        {
            ErrMsg(0,"ERROR: Eventscripts key access attempted before keys have been initialized.\n");
            ErrVar("Keys not ready, yet");
            return;
        }
        KeyValues *pGroup = NULL;
        if (name2 != NULL)
        {
            KeyValues *pUserGroups = g_pUserKeys->FindKey(KEYNAME_USERGROUPS);
            pGroup = pUserGroups->FindKey(name);
            if (!pGroup)
            {
                // group doesn't exist so key doesn't exist
                nExists = 0;
            }
            else
            {
                strcpy(szKeyName, name2);
            }
        }
        else
        {
            // use the ungrouped group.
            pGroup = g_pUserKeys->FindKey(KEYNAME_UNGROUPED);
            strcpy(szKeyName, name);
        }
        if (pGroup)
        {
            KeyValues *pNewKey = pGroup->FindKey(szKeyName, false);
            if (!pNewKey)
            {
                nExists = 0;
            }
            else
            {
                nExists = 1;
            }
        }

    }
    else if(FStrEq(type, "keyvalue"))
    {
        char szKeyName[MAX_COMMAND_STRING];
        if (!g_pUserKeys)
        {
            ErrMsg(0,"ERROR: Eventscripts key access attempted before keys have been initialized.\n");
            ErrVar("Keys not ready, yet");
            return;
        }
        KeyValues *pGroup = NULL;
        if (nCount > 5)
        {
            KeyValues *pUserGroups = g_pUserKeys->FindKey(KEYNAME_USERGROUPS);
            pGroup = pUserGroups->FindKey(name);
            if (!pGroup)
            {
                // group doesn't exist so key doesn't exist
                nExists = 0;
            }
            else
            {
                strcpy(szKeyName, name2);
            }
        }
        else
        {
            // use the ungrouped group.
            pGroup = g_pUserKeys->FindKey(KEYNAME_UNGROUPED);
            strcpy(szKeyName, name);
        }
        if (pGroup)
        {
            KeyValues *pNewKey = pGroup->FindKey(szKeyName, false);
            if (!pNewKey)
            {
                nExists = 0;
            }
            else
            {
                KeyValues *pValue = NULL;
                if (nCount > 5)
                    pValue = pNewKey->FindKey(name3, false); 
                else
                    pValue = pNewKey->FindKey(name2, false); 

                if (pValue)
                    nExists = 1;
                else
                    nExists = 0;
            }
        }

    }
    else if(FStrEq(type, "script"))
    {
        if(g_ShellVM.ScriptExists(name))
        {
            nExists = 1;
        }
        else
        {
            nExists = 0;
        }
    }
    else if(FStrEq(type, "block"))
    {
        if(g_ShellVM.BlockExists(name))
        {
            nExists = 1;
        }
        else
        {
            nExists = 0;
        }
    }

    RETURNVAL(var, (int)nExists, "i");
}

ES_CON_COMMAND3(dbgmsg, "Prints a debug message for EventScripts",2,"Please provide something to message.")
    if (nCount > 2)
    {
        int nLevel = atoi(args.Arg(1));
        char szMessage[MAX_COMMAND_STRING];
		const char * nvalue = strchr(args.ArgS(), ' ')+1;
		StripAllQuotes(szMessage, nvalue);
        DxMsgExternal(nLevel, "%s\n", szMessage);
    }
    else
    {
        ErrMsg(0,"Invalid syntax.\n");
        ErrVar("Invalid syntax");
    }
}

CON_COMMAND(printcount, "Prints the current gameframe count\n")
{
    DxMsg(0, __FUNCTION__, __LINE__, "%d\n", gpGlobals->framecount);
}


ES_CON_COMMAND2(dbgmsgv, "Prints a debug message for EventScripts",2,"Please provide something to message.")
    int nLevel = atoi(args.Arg(1));

    ConVar *pVarC = GetVar(args.Arg(2));
    if (pVarC)
    {
        DxMsgExternal(nLevel, "%s\n", pVarC->GetString());
    }
    else
    {
        ErrMsg(0,"ERROR: variable %s does not exist.\n", args.Arg(2));
        ErrVar("Variable does not exist");
        return;
    }

}



ES_CON_COMMAND2(log, "Logs a message to the server log.", 2, "Please provide something to message.")
    char szMessage[MAX_COMMAND_STRING];
    StripAllQuotes(szMessage, args.ArgS());
    strcat(szMessage, "\n");
    engine->LogPrint(szMessage);
}

ES_CON_COMMAND2(msg, 
                "Broadcasts a message to all players. If the first word of the message is '#green', or '#lightgreen' then the message is displayed in that color, supports '#multi' also for embedded #green/#lightgreen in the message.",
                2,
                "Please provide a message to broadcast.")
    char szMessage[MAX_COMMAND_STRING];
    char szTemp[MAX_COMMAND_STRING];
    int color = 1;
    int nColorMessage = COLORMESSAGE;
    int nLargeMessage = LARGEMESSAGE;
    StripAllQuotes(szMessage, args.ArgS());
    if (!strchr(szMessage, '\n'))
    {
        strcat(szMessage, "\n");
    }
    if (FStrEq(args.Arg(1), "#GREEN")) {
        DxMsg(1, __FUNCTION__, __LINE__,"#green text\n");
        color = 4;
        sprintf(szTemp, "%c%s", color, Q_stristr(szMessage, "#GREEN ") + strlen("#GREEN "));
        SendAllMessage(nColorMessage, szTemp);
    }
    else if (FStrEq(args.Arg(1), "#LIGHTGREEN")) {
        DxMsg(1, __FUNCTION__, __LINE__,"#lightgreen text\n");
        color = 3;
        sprintf(szTemp, "%c%s", color, Q_stristr(szMessage, "#LIGHTGREEN ") + + strlen("#LIGHTGREEN "));
        SendAllMessage(nColorMessage, szTemp);
    }
    else if (FStrEq(args.Arg(1), "#DARKGREEN")) {
        DxMsg(1, __FUNCTION__, __LINE__,"#darkgreen text\n");
        color = 5;
        sprintf(szTemp, "%c%s", color, Q_stristr(szMessage, "#DARKGREEN ") + + strlen("#DARKGREEN "));
        SendAllMessage(nColorMessage, szTemp);
    }
    else if (FStrEq(args.Arg(1), "#multi"))
    {
        DxMsg(1, __FUNCTION__, __LINE__,"#multi text\n");
        sprintf(szTemp, "\1%s", Q_stristr(szMessage, "#MULTI ") + + strlen("#MULTI "));
        strcpy(szMessage, szTemp);
        char szColor[10];
        sprintf(szColor, "%c", 4); //green
        if (!V_StrSubst(szMessage, "#green",szColor, szTemp, MAX_COMMAND_STRING))
        {
            ErrMsg(0,"ERROR: resulting replaced string too long. Copied as much as we could.\n");
            ErrVar("Resulting string too long.");
            return;
        }
        strcpy (szMessage, szTemp);
        sprintf(szColor, "%c", 3); //light green
        if (!V_StrSubst(szMessage, "#lightgreen",szColor, szTemp, MAX_COMMAND_STRING))
        {
            ErrMsg(0,"ERROR: resulting replaced string too long. Copied as much as we could.\n");
            ErrVar("Resulting string too long.");
            return;
        }
        strcpy (szMessage, szTemp);
        sprintf(szColor, "%c", 1); //default orange
        if (!V_StrSubst(szMessage, "#default",szColor, szTemp, MAX_COMMAND_STRING))
        {
            ErrMsg(0,"ERROR: resulting replaced string too long. Copied as much as we could.\n");
            ErrVar("Resulting string too long.");
            return;
        }
        strcpy (szMessage, szTemp);
        sprintf(szColor, "%c", 5); // darkgreen
        if (!V_StrSubst(szMessage, "#darkgreen",szColor, szTemp, MAX_COMMAND_STRING))
        {
            ErrMsg(0,"ERROR: resulting replaced string too long. Copied as much as we could.\n");
            ErrVar("Resulting string too long.");
            return;
        }
        SendAllMessage(nColorMessage, szTemp);
    }
    else {
        sprintf(szTemp, "%s", szMessage);
        SendAllMessage(nLargeMessage, szTemp);
    }
    
}

ES_CON_COMMAND2(tell, "Sends HUD message to one player. If the first word of the message is '#green', or '#lightgreen' then the message is displayed in that color. Supports '#multi' also for embedded #green/#lightgreen in the message.\n\n",
                3,
                "<userid> [color] <message>")
    char szMessage[MAX_COMMAND_STRING];
    char szTemp[MAX_COMMAND_STRING];
    int color = 1;
    int nColorMessage = COLORMESSAGE;
    int nLargeMessage = LARGEMESSAGE;

    StripAllQuotes(szMessage, args.ArgS()); 
    if (!strchr(szMessage, '\n'))
    {
        strcat(szMessage, "\n");
    }
    if (FStrEq(args.Arg(2), "#GREEN")) {
        DxMsg(1, __FUNCTION__, __LINE__,"#green text\n");
        color = 4;
        sprintf(szTemp, "%c%s", color, Q_stristr(szMessage, "#GREEN ") + strlen("#GREEN "));
        SendMessage(atoi(args.Arg(1)), nColorMessage, szTemp);
    }
    else if (FStrEq(args.Arg(2), "#LIGHTGREEN")) {
        DxMsg(1, __FUNCTION__, __LINE__,"#lightgreen text\n");
        color = 3;
        sprintf(szTemp, "%c%s", color, Q_stristr(szMessage, "#LIGHTGREEN ") + + strlen("#LIGHTGREEN "));
        SendMessage(atoi(args.Arg(1)), nColorMessage, szTemp);
    }
    else if (FStrEq(args.Arg(2), "#DARKGREEN")) {
        DxMsg(1, __FUNCTION__, __LINE__,"#darkgreen text\n");
        color = 5;
        sprintf(szTemp, "%c%s", color, Q_stristr(szMessage, "#DARKGREEN ") + + strlen("#DARKGREEN "));
        SendMessage(atoi(args.Arg(1)), nColorMessage, szTemp);
    }
   else if (FStrEq(args.Arg(2), "#multi"))
    {
        DxMsg(1, __FUNCTION__, __LINE__,"#multi text\n");
        sprintf(szTemp, "%s", Q_stristr(szMessage, "#MULTI ") + + strlen("#MULTI "));
        strcpy(szMessage, szTemp);
        char szColor[10];
        sprintf(szColor, "%c", 4); //green
        if (!V_StrSubst(szMessage, "#green",szColor, szTemp, MAX_COMMAND_STRING))
        {
            ErrMsg(0,"ERROR: resulting replaced string too long. Copied as much as we could.\n");
            ErrVar("Resulting string too long.");
            return;
        }
        strcpy (szMessage, szTemp);
        sprintf(szColor, "%c", 3); //light green
        if (!V_StrSubst(szMessage, "#lightgreen",szColor, szTemp, MAX_COMMAND_STRING))
        {
            ErrMsg(0,"ERROR: resulting replaced string too long. Copied as much as we could.\n");
            ErrVar("Resulting string too long.");
            return;
        }
        strcpy (szMessage, szTemp);
        sprintf(szColor, "%c", 1); //default orange
        if (!V_StrSubst(szMessage, "#default",szColor, szTemp, MAX_COMMAND_STRING))
        {
            ErrMsg(0,"ERROR: resulting replaced string too long. Copied as much as we could.\n");
            ErrVar("Resulting string too long.");
            return;
        }
        strcpy (szMessage, szTemp);
        sprintf(szColor, "%c", 5); // darkgreen
        if (!V_StrSubst(szMessage, "#darkgreen",szColor, szTemp, MAX_COMMAND_STRING))
        {
            ErrMsg(0,"ERROR: resulting replaced string too long. Copied as much as we could.\n");
            ErrVar("Resulting string too long.");
            return;
        }
        SendMessage(atoi(args.Arg(1)),nColorMessage, szTemp);
    }
    else {
        sprintf(szTemp, "%s", strchr(szMessage, ' ') + 1);
        SendMessage(atoi(args.Arg(1)), nLargeMessage, szTemp);
    }
    


}

CON_COMMAND( es_xlogq, "Logs a message to the server log. Will not expand any EventScripts variables. Allows quotes.\n" )
{
    if(ARGC() < 2)
    {
        DxMsg(0, __FUNCTION__, __LINE__,"es_xlogq: Please provide something to broadcast.\n"); 
        ErrVar("Not enough arguments.");
        return;
    }
    char szMessage[MAX_COMMAND_STRING];
    strcpy(szMessage, ARGS());
    strcat(szMessage, "\n");
    engine->LogPrint(szMessage);
}
CON_COMMAND( es_logq, "Logs a message to the server log. Supports expanding EventScripts variables. Allows quotes.\n" )
{
    if(ARGC() < 2)
    {
        DxMsg(0, __FUNCTION__, __LINE__,"es_log: Please provide something to log.\n"); 
        ErrVar("Not enough arguments.");
        return;
    }

    char szPreparseCommand[MAX_COMMAND_STRING];
    strcpy(szPreparseCommand, "es es_xlogq ");
    strcat(szPreparseCommand, ARGS());
    strcat(szPreparseCommand, "\n");
    DxMsg(2, __FUNCTION__, __LINE__,"es_logq executing: %s\n", szPreparseCommand);

    ImmediateExecuteCommand(szPreparseCommand);
}




CON_COMMAND(profile, "profile begin <var>\nor\nprofile end <var>\n")
{
    if (FStrEq(ARGV(1), "begin"))
    {
        DxMsg(1, __FUNCTION__, __LINE__, "Profiler timer %s started.\n", ARGV(2));
        float nowt = GetTime();
        ConVar* cvar = SetConVar( ARGV(2), nowt, true);
    }
    else
    {
        ConVar* cvar2 = FindVar(ARGV(2));
        if (!cvar2)
        {
            DxMsg(0, __FUNCTION__, __LINE__,"The var \"%s\" could not be found\n", ARGV(2)); 
            return; // If it doesn t find the var
        }
		if (!CanChange(cvar2)){
			return;
		}
        float nowt = GetTime();
        double diff = nowt - cvar2->GetFloat();
        DxMsg(2, __FUNCTION__, __LINE__, "Profiler: %lf - %lf = %lf\n", nowt, cvar2->GetFloat(), diff);
        cvar2->SetValue((float)diff);
        DxMsg(1, __FUNCTION__, __LINE__, "Profiler timer %s ended: %lf\n", cvar2->GetName(), diff);
    }
}

ES_CON_COMMAND2(logv,
                "Logs the text inside of a variable.",
                2,
                "<variable>")
    ConVar *pVarC = GetVar(args.Arg(1));
    char szCommand[MAX_COMMAND_STRING];
    *szCommand = 0;
    if (pVarC)
    {
        sprintf(szCommand, "%s\n", pVarC->GetString());
        engine->LogPrint(szCommand);

    }
    else
    {
        ErrMsg(0,"ERROR: variable %s does not exist.\n", args.Arg(1));
        ErrVar("Variable does not exist");
        return;
    }
}

ES_CON_COMMAND2(commandv,
                "Just runs a command-string inside of the variable.",
                2,
                "<variable>")
    ConVar *pVarC = GetVar(args.Arg(1));
    char szCommand[MAX_COMMAND_STRING];
    *szCommand = 0;
    if (pVarC)
    {
        sprintf(szCommand, "%s\n", pVarC->GetString());
        ImmediateExecuteCommand(szCommand);
    }
    else
    {
        ErrMsg(0,"ERROR: variable %s does not exist.\n", args.Arg(1));
        ErrVar("Variable does not exist");
        return;
    }
}

// RETURNVAL_UNSUPPORTED
ES_CON_COMMAND3(formatv, 
                "Allows you to format a string by filling in a list of variable's strings into a format string.",
                3,
                "<variable> <format-string> [var1] [var2] [...] [varN]")
        //              0           1             2            3       4      ...     N
    const int nTokenStart = 2;          // offset into Cmd_Argv() one ahead of where tokens begin 
    char szFormatted[MAX_FORMATTED];    // to store the formatted string we're building.
    szFormatted[0]='\0';
    char szArgument[MAX_FORMATTED];     // to store the semi-expanded string each time we loop
    strcpy(szArgument, args.Arg(2)); // starts as the full format string

    // Using an algorithm that shro hates because it's so wildly
    // inefficient. I like it because there's so little pointer
    // arithmetic.
    // Loop 1-9 doing a string replace in the format string for %1, %2, etc.
    for (int i = 1;i<=(args.ArgC()-nTokenStart)-1;i++)
    {
        char szParam[5];
        sprintf(szParam, "%%%d", i);
        ConVar *pVarN = GetVar(args.Arg(nTokenStart + i), false);
        if (!pVarN) 
        {
            ErrMsg(0,"ERROR: variable %s does not exist.\n", args.Arg(nTokenStart+i));
            ErrVar("Variable does not exist");
            return;
        }
        const char *pReplaceWith = pVarN->GetString();
        if (pReplaceWith) {
            V_StrSubst(szArgument, szParam, pReplaceWith, szFormatted, MAX_FORMATTED);
        }
        else
        {
            ErrMsg(0,"ERROR: You did not provide enough parameters for the string you specified.\n");
            ErrVar("Not enough arguments.");
        }
        strcpy(szArgument, szFormatted);
    }

    // All double-percents should resolve last into a single %
    V_StrSubst(szArgument, "%%", "%", szFormatted, MAX_FORMATTED);

    SetConVar(args.Arg(1), szFormatted, true);
}

// RETURNVAL_UNSUPPORTED
ES_CON_COMMAND3(format, 
                "Allows you to format a string by filling in a list of strings into a format string.",
                3,
                "<variable> <format-string> [token1] [token2] [...] [tokenN]")
        //              0           1             2            3       4      ...     N
    const int nTokenStart = 2;          // offset into Cmd_Argv() one ahead of where tokens begin 
    char szFormatted[MAX_FORMATTED];    // to store the formatted string we're building.
    szFormatted[0]='\0';
    char szArgument[MAX_FORMATTED];     // to store the semi-expanded string each time we loop
    strcpy(szArgument, args.Arg(2)); // starts as the full format string

    // Using an algorithm that shro hates because it's so wildly
    // inefficient. I like it because there's so little pointer
    // arithmetic.
    // Loop 1-9 doing a string replace in the format string for %1, %2, etc.
    for (int i = 1;i<=(args.ArgC()-nTokenStart)-1;i++)
    {
        char szParam[5];
        sprintf(szParam, "%%%d", i);
        const char *pReplaceWith = args.Arg(nTokenStart + i);
        if (pReplaceWith) {
            V_StrSubst(szArgument, szParam, args.Arg(nTokenStart + i), szFormatted, MAX_FORMATTED);
        }
        else
        {
            ErrMsg(0,"ERROR: You did not provide enough parameters for the string you specified.\n");
            ErrVar("Not enough arguments.");
        }
        strcpy(szArgument, szFormatted);
    }

    // All double-percents should resolve last into a single %
    V_StrSubst(szArgument, "%%", "%", szFormatted, MAX_FORMATTED);

    SetConVar(args.Arg(1), szFormatted, true);
}

// RETURNVAL_UNSUPPORTED
ES_CON_COMMAND3( gettime, 
                "Stores the server time in a variable.",
                2,
                "<varname>")
    if (FStrEq(args.Arg(1),""))
    {
        DxMsg(0, __FUNCTION__, __LINE__,"Specify a variable!\n"); // If i made a findvar with a non specified arg that cause crashs
        ErrVar("Not enough arguments.");
        return;
    }
    ConVar* cvar = SetConVar( args.Arg(1), (int)time(NULL), true);
    if (!cvar)
    {
        DxMsg(0, __FUNCTION__, __LINE__,"The var \"%s\" could not be set.\n", args.Arg(1)); 
        ErrVar("Variable could not be set.");
        return; // If it doesn t find the var
    }
    else
    {
        DxMsg(1, __FUNCTION__, __LINE__,"%s = \"%s\"\n",cvar->GetName(), cvar->GetString());
    }
}

// RETURNVAL_UNSUPPORTED
ES_CON_COMMAND3( gettimestamp, 
                "Stores a timestamp in seconds in a variable. gettimestamp provides a shorter timestamp only for comparing against other es_gettimestamp values.",
                2,
                "<varname>")
    if (FStrEq(args.Arg(1),""))
    {
        DxMsg(0, __FUNCTION__, __LINE__,"Specify a variable!\n"); // If i made a findvar with a non specified arg that cause crashs
        ErrVar("Not enough arguments.");
        return;
    }
    ConVar* cvar = SetConVar( args.Arg(1), (int)(time(NULL)-1136049334L), true);
    if (!cvar)
    {
        DxMsg(0, __FUNCTION__, __LINE__,"The var \"%s\" could not be set.\n", args.Arg(1)); 
        ErrVar("Variable could not be set.");
        return; // If it doesn t find the var
    }
    else
    {
        DxMsg(1, __FUNCTION__, __LINE__,"%s = \"%s\"\n",cvar->GetName(), cvar->GetString());
    }
}

// RETURNVAL_UNSUPPORTED
ES_CON_COMMAND3( getmoney, 
                "Stores the player's money value in a variable. (CS:S only)",
                3,
                "<varname> <userid>")
    char szPreparseCommand[MAX_COMMAND_STRING];
    strcpy(szPreparseCommand, "es_xgetplayerprop ");
    strcat(szPreparseCommand, args.ArgS());
    strcat(szPreparseCommand, " CCSPlayer.m_iAccount\n");
    DxMsg(2, __FUNCTION__, __LINE__,"es_xgetmoney executing: %s\n", szPreparseCommand);
    ImmediateExecuteCommand(szPreparseCommand);
}



// RETURNKEY
ES_CON_COMMAND2(scriptpacklist, 
               "Lists the script packs running on the server. If a userid is provided, will es_tell the list to the user.\n",
               1,
               "[userid]")
    int nUserid = 0;
    if (args.ArgC() > 1)
    {
        nUserid = atoi(args.Arg(1));
    }

    g_EventScriptsPlugin.PrintAllRegistered(nUserid);
    return;
}

CON_COMMAND( eventscripts_register, "Syntax : eventscripts_register [subdirectory]\n  Registers a script pack subdirectory or, with no parameters, lists all registered script packs.\n" )
{
    if (ARGC() < 2)
    {
        g_EventScriptsPlugin.PrintAllRegistered();
        return;
    }
    if (!g_pRegistered)
    {
        g_pRegistered = new KeyValues("ScriptPacks");
    }
    if (g_pRegistered) 
    {

        g_pRegistered->SetInt(ARGV(1), 1);
        char szCommand[MAX_COMMAND_STRING];
        sprintf(szCommand, "setinfo %s_dir \"%s/%s/\"\n", ARGV(1), scriptdir_cvar.GetString(), ARGV(1));

        DxMsg(1, __FUNCTION__, __LINE__,"eventscripts_register executing: %s\n", szCommand);

        ImmediateExecuteCommand(szCommand);

        DxMsg(0, __FUNCTION__, __LINE__,"[EventScripts] Registered script pack: %s\n", ARGS());
	    IGameEvent * event = gameeventmanager->CreateEvent( "es_scriptpack_register");
	    if ( event )
	    {
		    event->SetString( "scriptpack", ARGV(1) );
		    gameeventmanager->FireEvent( event );
	    }
    }
    else
    {
        DxMsg(0, __FUNCTION__, __LINE__,"[EventScripts] Internal Error: Couldn't register-- registration system is unavailable.\n");
        ErrVar("Internal error: registration system is unavailable.");
    }
    return;
}


CON_COMMAND( eventscripts_unregister, "Syntax : eventscripts_unregister [subdirectory]\n  Unregisters/disables a script pack subdirectory, or, with no parameters, lists all registered script packs.\n" )
{
    if (ARGC() < 2)
    {
        g_EventScriptsPlugin.PrintAllRegistered();
        return;
    }
    if (g_pRegistered)
    {
	    IGameEvent * event = gameeventmanager->CreateEvent( "es_scriptpack_unregister");
	    if ( event )
	    {
		    event->SetString( "scriptpack", ARGS() );
		    gameeventmanager->FireEvent( event );
	    }

        g_pRegistered->SetInt(ARGS(), 0);
        DxMsg(0, __FUNCTION__, __LINE__,"[EventScripts] Unregistered script pack: %s\n", ARGS());
    }
    else
    {
        DxMsg(0, __FUNCTION__, __LINE__,"[EventScripts] Internal Error: Couldn't unregister-- registration system is unavailable until you load your first script pack with eventscripts_register.\n");
        ErrVar("Internal error: registration system is unavailable.");
    }
    return;
}

// RETURNVAL
ES_CON_COMMAND2(getuserid, 
               "Looks-up a userid based on the string provided. Checks it against a userid, steamid, exact name, and partial name.",
               2,
               "<variable> [string]")
    int nFirstArg = bUseServerVariablesForReturn ? 0 : -1;
    nCount+=abs(nFirstArg);

    if (nCount > 2) 
    {
        RETURNVAL(args.Arg(1), g_EventScriptsPlugin.FindUserIDByString(args.Arg(nFirstArg+2)), "i");
    }
    else
    {
        RETURNVAL(args.Arg(1), g_EventScriptsPlugin.FindUserIDByString(0), "i");
    }
}


// ***********************************
// ** End Cr3V3TT3's es_delayed Code
// ***********************************







// RETURNVAL
ES_CON_COMMAND2(getclientvar, 
               "Reads a console variable from a given player.\n",
               4,
               "<var> <userid> <varname>")
    int nFirstArg = bUseServerVariablesForReturn ? 0 : -1;

    edict_t *pEdict = NULL;
    pEdict = g_EventScriptsPlugin.LookupEdictByUserid(atoi(args.Arg(nFirstArg+2)));
    if (!pEdict)
    {
        ErrMsg(0,"ERROR: userid not found: %s\n", args.Arg(nFirstArg+2));
        ErrVar("Userid not valid");
        return;
    }
    int nIndex = IndexOfEdict(pEdict); 
    if (nIndex > 0)
    {
        const char *pValue = engine->GetClientConVarValue(nIndex, args.Arg(nFirstArg+3));
        DxMsg(1, __FUNCTION__, __LINE__,"pValue: %d, pEdict: %d, nIndex: %d, UserID: %d\n", pValue, pEdict, nIndex, atoi(args.Arg(nFirstArg+2)));
        if (pValue)
        {
            RETURNVAL(args.Arg(1), pValue, "s");
        }
        else
        {
            RETURNVAL(args.Arg(1), "", "s");
        }
    }
}




// RETURNVAL
ES_CON_COMMAND2(getplayercount, 
               "Stores the count of players on the server into a variable. Optionally a team can be specified. Returns -1 on error.\n",
               2,
               "<var> [team number]")
    int nFirstArg = bUseServerVariablesForReturn ? 0 : -1;
    nCount+=abs(nFirstArg);

    if (nCount < 2 || nCount > 3)
    {
        DxMsg(0, __FUNCTION__, __LINE__,"Syntax: es_xgetplayercount <var> [team number]\n");
        ErrVar("Not enough arguments.");
        return;
    }
    if (nCount == 2)
    {
        RETURNVAL(args.Arg(1), g_EventScriptsPlugin.GetPlayerCount(-1), "i");
    }
    else if (nCount == 3)
    {
        RETURNVAL(args.Arg(1), g_EventScriptsPlugin.GetPlayerCount(atoi(args.Arg(nFirstArg+2))), "i");
    }
}

// RETURNVAL
ES_CON_COMMAND2(getlivingplayercount, 
               "Stores the count of living players on the server into a variable. Optionally a team can be specified. Returns -1 on error.\n",
               2,
               "<var> [team number]")
    int nFirstArg = bUseServerVariablesForReturn ? 0 : -1;
    nCount+=abs(nFirstArg);

    if (nCount < 2 || nCount > 3)
    {
        DxMsg(0, __FUNCTION__, __LINE__,"Syntax: es_xgetplayercount <var> [team number]\n");
        ErrVar("Not enough arguments.");
        return;
    }
    if (nCount == 2)
    {
        RETURNVAL(args.Arg(1), g_EventScriptsPlugin.GetPlayerCount(-1, true), "i");
    }
    else if (nCount == 3)
    {
        RETURNVAL(args.Arg(1), g_EventScriptsPlugin.GetPlayerCount(atoi(args.Arg(nFirstArg+2)), true), "i");
    }
}
// RETURNVAL
ES_CON_COMMAND2(getmaxplayercount, 
               "Stores the maximum number of player slots the server allows.\n",
               1,
               "<var>")
    int nFirstArg = bUseServerVariablesForReturn ? 0 : -1;
    nCount+=abs(nFirstArg);

    RETURNVAL(args.Arg(1), maxplayers, "i");
}

// RETURNVAL_UNSUPPORTED
ES_CON_COMMAND3( gettimestring, 
                "Stores the server time string in a variable.",
                2,
                "<varname>")
    char szTimeString[MAX_COMMAND_STRING];
    time_t now;
    time(&now);
    ConVar* cvar = NULL; 
    if (NULL != strftime(szTimeString, MAX_COMMAND_STRING, timeformat_cvar.GetString(), localtime(&now)))
    {
        cvar = SetConVar( args.Arg(1),szTimeString);
    }
    else
    {
        cvar = SetConVar( args.Arg(1),"#TimeFormatError");
    }
    if (!cvar)
    {
        DxMsg(0, __FUNCTION__, __LINE__,"The var \"%s\" could not be set\n", args.Arg(1)); 
        return; // If it doesn t find the var
    }
    else
    {
        DxMsg(1, __FUNCTION__, __LINE__,"%s = \"%s\"\n",cvar->GetName(), cvar->GetString());
    }
}





// RETURNVAL3
ES_CON_COMMAND2(getplayermovement, 
               "Stores the player's current forward movement value, side movement value, and upward movement value (in 3 different variables or Python 3-tuple).\n",
               5,
               "<var forwardmove> <var sidemove> <var upmove> <userid>")
    int nFirstArg = bUseServerVariablesForReturn ? 0 : -3;
    nCount+=abs(nFirstArg);

    float d[4];
   // dynamically creates a CBotCmd
    CBotCmd *p = g_EventScriptsPlugin.GetPlayerBotCmd(atoi(args.Arg(nFirstArg+4)));
    if (p) 
    {
        d[0]=0;
        d[1]=p->forwardmove;
        d[2]=p->sidemove;
        d[3]=p->upmove;
        RETURNVAL3(args.Arg(1), d[1], "f", args.Arg(2), d[2], "f", args.Arg(3), d[3], "f");
        // free the CBotCmd
        delete p;
    }
}


// RETURNVAL
ES_CON_COMMAND2(getgravityvector,
                "Returns the gravity vector.\n",
                1,
                "<var>")
    IPhysicsEnvironment * ipe = physics->GetActiveEnvironmentByIndex(atoi(args.Arg(1)));
    if (!ipe) 
    {
        DxMsg(0, __FUNCTION__, __LINE__, "No physics environment.\n");
    }
    Vector grav;
#ifdef ORANGEBOX
	ipe->GetGravity(&grav);
#else
	ipe->GetGravity(grav);
#endif

    char *pv = AllocStringFromVector(grav);

    RETURNVAL(args.Arg(1),pv,"s");
    if (pv) free (pv);

    DxMsg(1, __FUNCTION__, __LINE__, "gravity: %f, %f, %f\n", grav.x, grav.y, grav.z);
}

IPhysicsObject * GetActivePhysicsObjectByIndex(int nIndex)
{
    IPhysicsEnvironment * ipe = physics->GetActiveEnvironmentByIndex(0);
	int activeCount = ipe->GetActiveObjectCount();
	IPhysicsObject **pActiveList = NULL;
    IPhysicsObject *pObject = NULL;
    IPhysicsObject *pRetObject = NULL;
	if ( activeCount )
	{
		pActiveList = (IPhysicsObject **)stackalloc( sizeof(IPhysicsObject *)*activeCount );
		ipe->GetActiveObjects( pActiveList );
		for ( int i = 0; i < activeCount; i++ )
		{
			pObject = pActiveList[i];
            edict_t *pObjectEdict = gameents->BaseEntityToEdict((CBaseEntity *)pObject->GetGameData());
            if (IndexOfEdict(pObjectEdict) ==  nIndex)
            {
                pRetObject = pObject;
                break;
            }
		}
	}
    return pRetObject;
}
// TEMPORARY, PROBABLY
class ESObjectHandler : public IPhysicsObjectEvent
{
public:
	// these can be used to optimize out queries on sleeping objects
	// Called when an object is woken after sleeping
	virtual void ObjectWake( IPhysicsObject *pObject )
    {
        void *pObj = pObject->GetGameData();
        if (pObj)
        {
            edict_t *pObjectEdict = gameents->BaseEntityToEdict((CBaseEntity*)pObj);
            if (pObjectEdict)
            {
                DxMsg(3, __FUNCTION__, __LINE__, "Object waking: %d\n", IndexOfEdict(pObjectEdict));
                Vector newv(0,0,100);
                pObject->ApplyForceCenter(newv);
            }
        }
    }
	// called when an object goes to sleep (no longer simulating)
	virtual void ObjectSleep( IPhysicsObject *pObject )
    {
        void *pObj = pObject->GetGameData();
        if (pObj)
        {
            edict_t *pObjectEdict = gameents->BaseEntityToEdict((CBaseEntity*)pObj);
            if (pObjectEdict)
            {
                DxMsg(3, __FUNCTION__, __LINE__, "Object sleeping: %d\n", IndexOfEdict(pObjectEdict));
            }
        }
    }

};



ES_CON_COMMAND2(voicechat,
                "Allows you to control listening players.\n",
                2,
                "<command> [variable] <to_userid> <from_userid>")
    // es_voice islistening
    // es_voice listen
    // es_voice nolisten
    if (!voiceserver)
    {
        ErrMsg(0, "Error: This game did not successfully return the VoiceListener interface for ES to use.\n");
        return;
    }
    const char *cmd = args.Arg(1);
    if (FStrEq(cmd, "islistening"))
    {
        int nFirstArg = bUseServerVariablesForReturn ? 0 : -1;
        int nTo = atoi(args.Arg(nFirstArg+3));
        int nFrom =atoi( args.Arg(nFirstArg+4));
        if (nTo && nFrom)
        {
            nTo = g_EventScriptsPlugin.LookupIndexByUserID(nTo);
            nFrom = g_EventScriptsPlugin.LookupIndexByUserID(nFrom);
            DxMsg(3, __FUNCTION__, __LINE__, "%d, %d -- %d\n", nTo, nFrom, (int)voiceserver->GetClientListening(nTo, nFrom));
            RETURNVAL(args.Arg(2), (int)voiceserver->GetClientListening(nTo, nFrom), "i");
        }
        else
        {
            ErrMsg(0, "Incorrect userids provided.\n");
            ErrVar("Syntax Error");
            return;
        }

    }
    else if (FStrEq(cmd, "listen") || FStrEq(cmd, "nolisten"))
    {
            int nTo = atoi(args.Arg(2));
            int nFrom =atoi( args.Arg(3));
            if (nTo && nFrom)
            {
                nTo = g_EventScriptsPlugin.LookupIndexByUserID(nTo);
                nFrom = g_EventScriptsPlugin.LookupIndexByUserID(nFrom);
                bool bListenUp = false;
                bListenUp = voiceserver->SetClientListening(nTo, nFrom, FStrEq(cmd, "listen"));
                DxMsg(3, __FUNCTION__, __LINE__, "bListenUp = %d for %d/%d\n", bListenUp, nTo, nFrom);
            }
            else
            {
                ErrMsg(0, "Incorrect userids provided: %s and %s\n", args.Arg(2), args.Arg(3));
                ErrVar("Syntax Error");
                return;
            }
    }


}

ESObjectHandler *g_ObjectHandler = NULL;

// RETURNVAL
ES_CON_COMMAND2(physics,
                "Interface with the Source physics engine (physics gravity, object velocity, etc).\n",
                2,
                "<command> [options]")
    if (!physics || !gameents) 
    {
        ErrMsg(0, "Physics engine or game entity interfaces were not obtained when EventScripts was loaded. May not support this mod for physics.\n");
        ErrVar("Internal error");
        return;
    }
    IPhysicsEnvironment * ipe = physics->GetActiveEnvironmentByIndex(0);
    if (!ipe) 
    {
        ErrMsg(0, "ERROR: No physics environment.\n");
        ErrVar("Internal error");
        return;
    }
    const char * command = args.Arg(1);
    if (FStrEq(command, "set"))
    {
        const char *subarea = args.Arg(2);
        if (FStrEq(subarea, "gravity"))
        {
            Vector * grav = AllocVectorFromString(args.Arg(3));
            ipe->SetGravity(*grav);
            DxMsg(1, __FUNCTION__, __LINE__, "gravity: %f, %f, %f\n", grav->x, grav->y, grav->z);
            if (grav) free(grav);
        }
        else if (FStrEq(subarea, "airdensity"))
        {
            double f = atof(args.Arg(3));
            ipe->SetAirDensity(f);
        }
    }
    else if (FStrEq(command, "get"))
    {
        const char *subarea = args.Arg(2);
        if (FStrEq(subarea, "gravity"))
        {
            Vector grav;
#ifdef ORANGEBOX
			ipe->GetGravity(&grav);
#else
			ipe->GetGravity(grav);
#endif
            char *pv = AllocStringFromVector(grav);
            RETURNVAL(args.Arg(3), pv, "s");
            if (pv) free (pv);

            DxMsg(1, __FUNCTION__, __LINE__, "gravity: %f, %f, %f\n", grav.x, grav.y, grav.z);
        }
        else if (FStrEq(subarea, "airdensity"))
        {
            RETURNVAL(args.Arg(3), ipe->GetAirDensity(), "f");
        }
    }
    else if (FStrEq(command, "start"))
    {
        DxMsg(1, __FUNCTION__, __LINE__, "physics start\n");
        if (g_ObjectHandler) delete g_ObjectHandler;
        g_ObjectHandler = new ESObjectHandler();
        ipe->SetObjectEventHandler(g_ObjectHandler);
    }
    else if (FStrEq(command, "active"))
    {
        const char * subcommand = args.Arg(2);
        if (FStrEq(subcommand, "teleport"))
        {
            // es_physics active teleport <index> <neworigin>
            IPhysicsObject *pObject = GetActivePhysicsObjectByIndex(atoi(args.Arg(3)));
            if (pObject)
            {
                DxMsg(3, __FUNCTION__, __LINE__, "pObject: %d\n", pObject);
                Vector v;
                QAngle q;
                pObject->GetPosition(&v, &q);
                DxMsg(3, __FUNCTION__, __LINE__, "Position: %f, %f, %f\n", v.x, v.y, v.z);
                Vector *newv = AllocVectorFromString(args.Arg(4));
                pObject->SetPosition(*newv, q, true); 
                free(newv);
                DxMsg(2, __FUNCTION__, __LINE__, "Teleported %d.\n", pObject->GetGameIndex());
            }
        }
        else if (FStrEq(subcommand, "setvelocity"))
        {
            IPhysicsObject *pObject = GetActivePhysicsObjectByIndex(atoi(args.Arg(3)));
            if (pObject)
            {
                DxMsg(3, __FUNCTION__, __LINE__, "pObject: %d\n", pObject);
                Vector v;
                Vector va;
                pObject->GetVelocity(&v, &va);
                DxMsg(3, __FUNCTION__, __LINE__, "Position: %f, %f, %f\n", v.x, v.y, v.z);
                Vector *newv = AllocVectorFromString(args.Arg(4));
                pObject->SetVelocityInstantaneous(newv, &va); 
                free(newv);
                DxMsg(2, __FUNCTION__, __LINE__, "Set velocity %d.\n", pObject->GetGameIndex());
            }            
        }
        else if (FStrEq(subcommand, "applyforce"))
        {
            IPhysicsObject *pObject = GetActivePhysicsObjectByIndex(atoi(args.Arg(3)));
            if (pObject)
            {
                DxMsg(3, __FUNCTION__, __LINE__, "pObject: %d\n", pObject);
                Vector *newv = AllocVectorFromString(args.Arg(4));
                pObject->ApplyForceCenter(*newv);
                free(newv);
                DxMsg(2, __FUNCTION__, __LINE__, "Set velocity %d.\n", pObject->GetGameIndex());
            }            
        }
        else if (FStrEq(subcommand, "setmass"))
        {
            IPhysicsObject *pObject = GetActivePhysicsObjectByIndex(atoi(args.Arg(3)));
            if (pObject)
            {
                pObject->SetMass(atof(args.Arg(4)));
            }            
        }
    }

}

// RETURNVAL3
ES_CON_COMMAND2(getplayerlocation, 
               "Stores the player's current x, y, and z location (in 3 different variables or a 3-tuple in Python).\n",
               5,
               "<var x> <var y> <var z> <userid>")
    int nFirstArg = bUseServerVariablesForReturn ? 0 : -3;

    float d[4];
    int nUserid = 0;
    memset(d, 0, sizeof(d));
    // dynamically creates a CBotCmd
    nUserid = atoi(args.Arg(nFirstArg+4));
    DxMsg(3, __FUNCTION__, __LINE__,"getloc, nUserid: %d\n", nUserid);
    if (nUserid > 0) 
    {
        g_EventScriptsPlugin.GetPlayerLocation(nUserid, (d+1));
        RETURNVAL3(args.Arg(1), d[1], "f", args.Arg(2), d[2], "f", args.Arg(3), d[3], "f");
    }
    else
    {
        DxMsg(0, __FUNCTION__, __LINE__,"getplayerlocation, invalid userid\n");
        ErrVar("Invalid userid");
    }
}

// RETURNVAL
ES_CON_COMMAND2(isbot, 
               "Checks a userid to see if it's a bot, stores 1 in the variable if so, 0 if not.\n",
               3,
               "<var> <userid>")
    int nFirstArg = bUseServerVariablesForReturn ? 0 : -1;

    edict_t *pPlayer = g_EventScriptsPlugin.LookupEdictByUserid(atoi(args.Arg(nFirstArg+2)));
    if (pPlayer) 
    {
        int n = g_EventScriptsPlugin.EdictIsFakeClient(pPlayer);
        RETURNVAL(args.Arg(1), n, "i");
    }
}

ES_CON_COMMAND2(cexec_all, 
               "Forces all users to execute a command in their console.\n",
               2,
               "<commandstring>")
    char szCommand[MAX_COMMAND_STRING];
    if (args.ArgC() == 2) {
        // exactly two, let's just grab the second one
        strcpy (szCommand, args.Arg(1));
    }
    else
    {
        strcpy(szCommand, args.ArgS());
    }

    for(int i=1;i<=maxplayers;i++) 
    {
        edict_t *pEntity =  PEntityOfEntIndex(i);
        if(!pEntity || pEntity->IsFree()) 
        {
                continue;
        }
        if(playerinfomanager)
        {
            
            IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEntity );

            // exclude bots
            if(playerinfo && playerinfo->IsPlayer())
            {
                if (!playerinfo->IsFakeClient())
                {
                    engine->ClientCommand(pEntity, szCommand);
                }
                else
                {
                    if (botcexec_cvar.GetInt() > 0 && 0==strstr(args.ArgS(), "jointeam"))
                    {
                        helpers->ClientCommand(pEntity, szCommand);
                    }
                }
            }
        }

    }
}

ES_CON_COMMAND2(cexec, 
               "Forces a userid to execute a command in their console.\n",
               3,
               "<userid> <commandstring>")
    char szCommand[MAX_COMMAND_STRING];
    if (args.ArgC() == 3) {
        // exactly three, let's just grab the third one
        strcpy (szCommand, args.Arg(2));
    }
    else
    {
        // find the first argument
        const char * pLoc = strstr(args.ArgS(), args.Arg(1));
        if (pLoc)
        {
            strcpy(szCommand, pLoc+strlen(args.Arg(1)) + 1);
        }
        else {
            Msg ("Error with es_cexec, should never happen!\n");
            ErrVar("Internal error: es_cexec");
            return;
        }
    }
    edict_t * pPlayer = g_EventScriptsPlugin.LookupEdictByUserid(atoi(args.Arg(1)));
    if (pPlayer)
    {
        if (!g_EventScriptsPlugin.EdictIsFakeClient(pPlayer)) 
        {
            engine->ClientCommand(pPlayer, szCommand);
        }
        else
        {
            if (botcexec_cvar.GetInt() > 0 && 0==strstr(args.ArgS(), "jointeam"))
            {
                helpers->ClientCommand(pPlayer, szCommand);
            }

        }
    }
}




ES_CON_COMMAND2(flags, 
               "Adds or removes the cheat flag from a command or variable. (EXPERIMENTAL/UNSUPPORTED)\n",
               4,
               "<add/remove> <flag> <command/var>")
    unsigned int flags = 0;
    const char *flagname = args.Arg(2);
    if (FStrEq(flagname, "cheat"))
        flags = FCVAR_CHEAT;
    else if (FStrEq(flagname, "notify"))
        flags = FCVAR_NOTIFY;
    else if (FStrEq(flagname, "gamedll"))
        flags = FCVAR_GAMEDLL;
    else if (FStrEq(flagname, "replicated"))
        flags = FCVAR_REPLICATED;
    else if (FStrEq(flagname, "protected"))
        flags = FCVAR_PROTECTED;
    else if (FStrEq(flagname, "unlogged"))
        flags = FCVAR_UNLOGGED;
    else if (FStrEq(flagname, "neverstring"))
        flags = FCVAR_NEVER_AS_STRING;
    else if (FStrEq(flagname, "printable"))
        flags = FCVAR_PRINTABLEONLY;
    else if (FStrEq(flagname, "demo"))
        flags = FCVAR_DEMO;
    else if (FStrEq(flagname, "dontrecord"))
        flags = FCVAR_DONTRECORD;
#ifdef FCVAR_DEVELOPMENTONLY
    else if (FStrEq(flagname, "developmentonly"))
        flags = FCVAR_DEVELOPMENTONLY;
#endif
    else
        flags = atoi(flagname);

    

    ConVar* cvar = FindVar(args.Arg(3));
 
    if (!cvar)
    {
        // Try to find the command
        ConCommandBase *pPtr = FindCommandEx(args.Arg(3));
        if (!pPtr)
        {
            DxMsg(0, __FUNCTION__, __LINE__,"es_xflags: Could not find var or command: %s\n", args.Arg(3));
            return;
        }

		// protect from exploiting on listen servers
		if (!CanChange(pPtr)){
			return;
		}

        if (FStrEq(args.Arg(1), "add"))
        {
            pPtr->AddFlags(flags);        
        }
        else if (FStrEq(args.Arg(1), "remove"))
        {
            if (pPtr->m_nFlags & flags)
            {
                pPtr->m_nFlags-=flags;
            }
        }        
    } 
    else
    {
		// protect from exploiting on listen servers
		if (!CanChange(cvar)){
			return;
		}
        if (FStrEq(args.Arg(1), "add"))
        {
            cvar->AddFlags(flags);        
        }
        else if (FStrEq(args.Arg(1), "remove"))
        {
            if (cvar->m_nFlags & flags)
            {
                cvar->m_nFlags-=flags;
            }
        }
    }
    return;
}


ES_CON_COMMAND2(sexec, 
               "Forces a userid to execute a command on the server console (bypassing client console).\n",
               3,
               "<userid> <commandstring>")
    char szCommand[MAX_COMMAND_STRING];
    if (args.ArgC() == 3) {
        // exactly three, let's just grab the third one
        strcpy (szCommand, args.Arg(2));
    }
    else
    {
        // find the first argument
        const char * pLoc = strstr(args.ArgS(), args.Arg(1));
        if (pLoc)
        {
            strcpy(szCommand, pLoc+strlen(args.Arg(1)) + 1);
        }
        else {
            ErrMsg (0,"Error with es_cexec, should never happen!\n");
            ErrVar("Internal error: es_cexec");
            return;
        }
    }
    if (0==strstr(args.ArgS(), "jointeam"))
    {
        edict_t * pPlayer = g_EventScriptsPlugin.LookupEdictByUserid(atoi(args.Arg(1)));
        if (pPlayer)
            helpers->ClientCommand(pPlayer, szCommand);
    }
    else
    {
        DxMsg(0, __FUNCTION__, __LINE__, "jointeam not supported on bots");
    }
}

ES_CON_COMMAND2(sexec_all, 
               "Forces all users to execute a command on the server console.\n",
               2,
               "<commandstring>")
    char szCommand[MAX_COMMAND_STRING];
    if (args.ArgC() == 2) {
        // exactly three, let's just grab the third one
        strcpy (szCommand, args.Arg(2));
    }
    else
    {
        strcpy(szCommand, args.ArgS());
    }

    for(int i=1;i<=maxplayers;i++) 
    {
        edict_t *pEntity =  PEntityOfEntIndex(i);
        if(!pEntity || pEntity->IsFree()) 
        {
                continue;
        }
        if(playerinfomanager)
        {
            
            IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEntity );

            if(playerinfo && playerinfo->IsPlayer() && 0==strstr(args.ArgS(), "jointeam"))
            {
                helpers->ClientCommand(pEntity, szCommand);
            }
        }

    }
}

ES_CON_COMMAND2(loadevents, 
               "Reads an event file and registers EventScripts as a handler for those events.\n",
               2,
               "[declare] <pathtoeventfile>")
    int nPosition = 1;    
    if (nCount > 2)
    {
        nPosition = 2;
        gameeventmanager->LoadEventsFromFile(args.Arg(nPosition));
    }
    g_EventScriptsPlugin.RegisterForEventFile(args.Arg(nPosition));
}




// RETURNVAL
ES_CON_COMMAND2(getplayername, 
               "Stores the player's name in the variable.\n",
               3,
               "<var> <userid>")
    int nFirstArg = bUseServerVariablesForReturn ? 0 : -1;
    int nUserid = 0;
    nUserid = atoi(args.Arg(nFirstArg+2));
    if (nUserid > 0) 
    {
        const char *pName = g_EventScriptsPlugin.GetPlayerName(nUserid);
        if (pName)
        {
            RETURNVAL(args.Arg(1), pName, "s");
        }
        else
        {
            RETURNVAL(args.Arg(1), 0, "i");
        }
    }
    else
    {
        DxMsg(0, __FUNCTION__, __LINE__,"Invalid userid for getplayername.\n");
        ErrVar("Invalid userid");
    }
}

// RETURNVAL
ES_CON_COMMAND2(getplayersteamid, 
               "Stores the player's STEAMID in the variable.\n",
               3,
               "<var> <userid>")
    int nFirstArg = bUseServerVariablesForReturn ? 0 : -1;
    const char * pText = g_EventScriptsPlugin.GetPlayerSteamID(atoi(args.Arg(nFirstArg+2)));
    RETURNVAL(args.Arg(1), pText, "s");
}

// RETURNVAL
ES_CON_COMMAND2(getplayerteam, 
               "Stores the player's team # in the variable.\n",
               3,
               "<var> <userid>")
    int nFirstArg = bUseServerVariablesForReturn ? 0 : -1;
    int nTeam = g_EventScriptsPlugin.GetPlayerTeam(atoi(args.Arg(nFirstArg+2)));
    RETURNVAL(args.Arg(1),nTeam,"i");
}

#define ASSOCIATED_KEYGROUP(x,y) \
    KeyValues *pNewGroup = NULL; \
    if (bUseServerVariablesForReturn) \
    { \
        pNewGroup = GetKeyGroup(x, y); \
        if (!pNewGroup) \
        { \
            ErrMsg(0,"ERROR: EventScripts couldn't create the user group.\n"); \
        } \
    } \
    else \
    { \
            g_pPyReturn = PyDict_New(); \
    } 

#define ID_STRING bUseServerVariablesForReturn ? "%s" : "s"
#define ID_INT bUseServerVariablesForReturn ? "%i" : "i"

#define CREATESUBKEY_ID(parent, childid, pointer, type) \
    void *pointer = NULL; \
    if (bUseServerVariablesForReturn) \
    { \
        char szChildName[256]; \
        sprintf(szChildName, type, childid); \
        KeyValues *pChildN = new KeyValues(szChildName); \
        parent->AddSubKey(pChildN); \
        pointer = (void *)pChildN; \
    } \
    else { \
        pointer = (void *)PyDict_New(); \
        PyObject *ochild = Py_BuildValue(type, childid); \
        PyDict_SetItem(g_pPyReturn, ochild, (PyObject*)pointer); \
        Py_XDECREF(ochild); \
        Py_XDECREF((PyObject*)pointer); \
    } 


#define SETDICTVAL(p, name, value, type) \
    PyObject *ovalue = Py_BuildValue(type, value); \
    PyDict_SetItemString((PyObject *)p, name, ovalue); \
    Py_XDECREF(ovalue);

#define SETKEYVAL_S(pointer,name,value) \
    if (bUseServerVariablesForReturn)     { \
        ((KeyValues*)pointer)->SetString(name, value); \
    } \
    else { \
        SETDICTVAL(pointer,name,value,"s"); \
    }
#define SETKEYVAL_F(pointer,name,value) \
    if (bUseServerVariablesForReturn)     { \
        ((KeyValues*)pointer)->SetFloat(name, value); \
    } \
    else { \
        SETDICTVAL(pointer,name,value,"f"); \
    }
#define SETKEYVAL_I(pointer,name,value) \
    if (bUseServerVariablesForReturn)     { \
        ((KeyValues*)pointer)->SetInt(name, value); \
    } \
    else { \
        SETDICTVAL(pointer,name,value,"i"); \
    }
#define SETKEYVAL_L(pointer,name,value) \
    if (bUseServerVariablesForReturn)     { \
        ((KeyValues*)pointer)->SetInt(name, value); \
    } \
    else { \
        SETDICTVAL(pointer,name,value,"l"); \
    }

// RETURNKEY
ES_CON_COMMAND2(createplayerlist, 
               "Creates a new keygroup containing the current list of players.\n",
               1,
               "<keygroup> [userid]")
    int nFirstArg = bUseServerVariablesForReturn ? 0 : -1;
    nCount+=abs(nFirstArg);

    ASSOCIATED_KEYGROUP(args.Arg(1), true);
    for(int i=1;i<=maxplayers;i++) 
    {
        edict_t *pEntity =  PEntityOfEntIndex(i);
        if(!pEntity || pEntity->IsFree()) 
        {
                continue;
        }
        if(playerinfomanager)
        {
	        IPlayerInfo *pl = playerinfomanager->GetPlayerInfo( pEntity );
            bool bDone = false;
            if (pl && pl->IsPlayer())
            {
                if (nCount > 2) 
                {
                    if (pl->GetUserID() != atoi(args.Arg(nFirstArg+2)))
                    {
                        continue;
                    }
                    else
                    {
                        bDone = true;
                    }
                }
                int nUserId = pl->GetUserID();
                DxMsg(1, __FUNCTION__, __LINE__,"listing: %d\n", nUserId);
                CREATESUBKEY_ID(pNewGroup, nUserId,p, ID_INT);
                SETKEYVAL_S(p,"name", pl->GetName());
                SETKEYVAL_S(p,"steamid", pl->GetNetworkIDString());
                SETKEYVAL_I(p,"index", i);
                SETKEYVAL_I(p,"teamid", pl->GetTeamIndex());
                SETKEYVAL_I(p,"kills", pl->GetFragCount());
                SETKEYVAL_I(p,"deaths", pl->GetDeathCount());
                SETKEYVAL_I(p,"armor", pl->GetArmorValue());
                SETKEYVAL_S(p,"model", pl->GetModelName());
                SETKEYVAL_I(p,"isdead", g_EventScriptsPlugin.IsDead(nUserId) ? 1 : 0);
                SETKEYVAL_I(p,"isbot", pl->IsFakeClient() ? 1 : 0);
                SETKEYVAL_I(p,"ishltv", pl->IsHLTV() ? 1 : 0);
                SETKEYVAL_I(p,"isobserver", pl->IsObserver() ? 1 : 0);
                SETKEYVAL_I(p,"isinavehicle", pl->IsInAVehicle() ? 1 : 0);
                SETKEYVAL_I(p,"health", pl->GetHealth());
                SETKEYVAL_I(p,"serialnumber", pEntity->m_NetworkSerialNumber);
                SETKEYVAL_S(p,"weapon", pl->GetWeaponName());
                SETKEYVAL_I(p,"handle", g_EventScriptsPlugin.LookupHandleByUserid(nUserId));
                int ping = 0;
                int packetloss = 0;
                float connected = 0;
                char szAddress[1024];
                strcpy(szAddress, "0");
                g_EventScriptsPlugin.GetPlayerConnectionInfo(nUserId, ping, packetloss, connected, szAddress);
                SETKEYVAL_I(p,"ping", ping);
                SETKEYVAL_I(p,"packetloss", packetloss);
                SETKEYVAL_F(p,"timeconnected", connected);
                SETKEYVAL_S(p,"address", szAddress);
                DxMsg(1, __FUNCTION__, __LINE__,"listing: %d location\n", nUserId);
                float d[4];
                memset(d, 0, sizeof(d));
                g_EventScriptsPlugin.GetPlayerLocation(nUserId, (d+1));
                SETKEYVAL_F(p,"x", d[1]);
                SETKEYVAL_F(p,"y", d[2]);
                SETKEYVAL_F(p,"z", d[3]); 
                SETKEYVAL_S(p,"language", engine->GetClientConVarValue(i, "cl_language"));
                DxMsg(1, __FUNCTION__, __LINE__,"listing done: %d\n", nUserId);
                if (bDone) return;
            }
        }
        else
        {
            ErrMsg(0,"ERROR: Playerinfomanager couldn't be contacted.\n");
            ErrVar("Playerinfomanager couldn't be accessed.");
            return;
        }
    }
    return;
}

// RETURNVAL
ES_CON_COMMAND2(getentityindex,
               "Gets the index for the first named entity found by that name. Returns -1 if not found.\n",
               2,
               "<variable> <entity-name>")
    int nFirstArg = bUseServerVariablesForReturn ? 0 : -1;

    for (int i = 0;i<engine->GetEntityCount();i++)
    {
        edict_t *p = PEntityOfEntIndex(i);
        if (p)
        {
            IServerNetworkable *pisn = p->GetNetworkable();
            if (p->GetClassName())
            {
                if (FStrEq(p->GetClassName(),args.Arg(nFirstArg+2))){
                    RETURNVAL(args.Arg(1), i, "i");
                    return;
                }
            }
        }
    }
    RETURNVAL(args.Arg(1),-1,"i");
}

CON_COMMAND( _mexecl, "Internal EventScripts Command for memory exec'ing.\n" )
{
    KeyValues * pTemp = (KeyValues*)atoi(ARGS());
    pTemp = pTemp->GetNextValue();
    // we put "eof" at the end of the keygroup to signify it's done
    if (pTemp && !FStrEq(pTemp->GetName(), "eof"))
    {
        char szCommandz[MAX_COMMAND_STRING];
        sprintf (szCommandz, "%s;_mexecl %d\n", pTemp->GetString(),pTemp);
        ImmediateExecuteCommand(szCommandz);
    }    
}


KeyValues * pMexec = NULL;

ES_CON_COMMAND2(old_mexec, 
               "Runs an exec file from memory.\n",
               2,
               "<file>")
    if (!pMexec)
    {
        pMexec = new KeyValues("Mexec");
    }

    char szFile[MAX_COMMAND_STRING];
    sprintf(szFile, "cfg/%s", args.ArgS());
    KeyValues *pNewKey = pMexec->FindKey(szFile);
    if (!pNewKey || pNewKey->GetInt("filetime") < filesystem->GetFileTime(szFile))
    {
        if (pNewKey) pNewKey->deleteThis();
        pNewKey = pMexec->FindKey(szFile, true);
        FileHandle_t fFile = NULL;
        fFile = filesystem->Open(szFile, "r"); 
        if (fFile) 
        {
                
            pNewKey->SetInt("filetime", filesystem->GetFileTime(szFile));
            int i = 0;
                
            while (!filesystem->EndOfFile(fFile))
            {
                    
                i++;
                char szName[199];
                sprintf(szName, "%d", i);
                    
                char szBuffer[8000];
                filesystem->ReadLine(szBuffer, 8000, fFile);
                    
                pNewKey->SetString(szName, szBuffer);
            }
                
            pNewKey->SetString("eof", "eof");
            filesystem->Close(fFile);                
        }

    }
    KeyValues * pTemp = pNewKey->GetFirstValue();
        
    if (pTemp && FStrEq(pTemp->GetName(), "filetime")) pTemp = pTemp->GetNextValue();
        
    if (pTemp && !FStrEq(pTemp->GetName(), "eof"))
    {
            
        char szCommandz[MAX_COMMAND_STRING];
        sprintf (szCommandz, "%s;_mexecl %d\n", pTemp->GetString(), pTemp);
            
        ImmediateExecuteCommand(szCommandz);
    }   
}

// startup
// read from addons/eventscripts/addons.ini
//    loop through list
//        LoadScriptFile(es_PLUGIN.txt)

// when event fires
//    runscriptevent
//  

KeyValues * pScripts = NULL;

void BeginScriptEvent(KeyValues * pKeyAddon, const char *szEventName)
{
    if (!pKeyAddon || !szEventName) return;
    DxMsg(1, __FUNCTION__, __LINE__,"Inside BeginScriptEvent\n");
    KeyValues *pEventKey = pKeyAddon->FindKey(szEventName);
    DxMsg(1, __FUNCTION__, __LINE__,"Inside BeginScriptEvent2\n");
    if (pEventKey)
    {
        DxMsg(1, __FUNCTION__, __LINE__,"Inside BeginScriptEvent3\n");
        KeyValues * pTemp = pEventKey->GetFirstValue();
        DxMsg(1, __FUNCTION__, __LINE__,"Inside BeginScriptEvent4\n");
            
        if (pTemp && FStrEq(pTemp->GetName(), "filetime")) pTemp = pTemp->GetNextValue();
            
        DxMsg(1, __FUNCTION__, __LINE__,"Inside BeginScriptEvent5\n");
        if (pTemp && !FStrEq(pTemp->GetName(), "eof"))
        {
            char szCommandz[MAX_COMMAND_STRING];
            sprintf (szCommandz, "%s;_mexecl %d\n", pTemp->GetString(), pTemp);
            ImmediateExecuteCommand(szCommandz);

            DxMsg(1, __FUNCTION__, __LINE__,"Inside BeginScriptEvent6\n");
        }   
    }
    DxMsg(1, __FUNCTION__, __LINE__,"Inside BeginScriptEvent7\n");
    return;
}

inline void BeginAllPythonEvents(const char *szEventName);

void BeginAllScriptEvents(const char *szEventName)
{
    if (shellengine_cvar.GetInt() == 0) 
    {
        char *pCmdString = NULL;
        DxMsg(7, __FUNCTION__, __LINE__, "BeginAllScriptEvents: %s\n", szEventName);
        pCmdString = g_ShellVM.CreateCmdStringForAllFunctions(szEventName);

        if (pCmdString)
        {
            DxMsg(7, __FUNCTION__, __LINE__, "BeginAllScriptEvents: %s\n+%s", szEventName, pCmdString);
            ServerCommand(pCmdString);
        }
        free(pCmdString);
    }

    BeginAllPythonEvents(szEventName);
}

inline bool AnyPythonRegistered(const char *szEventName);

bool AnyScriptEventExists(const char *szEventName)
{

    if (shellengine_cvar.GetInt() == 0 && g_ShellVM.AnyBlockExistsForEvent(szEventName))
        return true;
    if (AnyPythonRegistered(szEventName))
        return true;
    return false;
}

void RemovePythonAddon(const char *szAddonName);

void RemoveAddonFile(const char *szAddonName)
{
    if (shellengine_cvar.GetInt() == 0 && g_ShellVM.RemoveScript(szAddonName))
    {
        DxMsg(0, __FUNCTION__, __LINE__,"Script removed: %s\n", szAddonName);
    }
    else
    {
        DxMsg(1, __FUNCTION__, __LINE__, "Attempting to remove Python: %s\n", szAddonName);
        RemovePythonAddon(szAddonName);
    }
}

bool LoadPythonAddon(const char *szAddonName);

void LoadAddonFile(const char *szAddon)
{
    DxMsg(2, __FUNCTION__, __LINE__,"LoadAddonFile().\n");
    char szAddonName[1028];
    strcpy(szAddonName, szAddon);
    bool bPythonScriptLoaded = false;
#ifdef PYTHON_SUPPORT
    if (g_nPythonEnabled)
    {
        bPythonScriptLoaded = LoadPythonAddon(szAddonName);
    }
#endif
    if (shellengine_cvar.GetInt() ==0 && !bPythonScriptLoaded)
    {
		DxMsg(2, __FUNCTION__, __LINE__,"LoadAddonFile()2.\n");
        char szScriptFile[1028];
        char *pFilename = strrchr(szAddonName, '\\');
        if (!pFilename)
        {
            pFilename = strrchr(szAddonName, '/');
            if (!pFilename)
            {
                pFilename = (char*)szAddonName;
            }
            else
            {
                pFilename++;
            }
        }
        else
        {
            pFilename++;
        }
        char *pTemp = szAddonName;
        while (*pTemp)
        {
            if (*pTemp=='\\') *pTemp='/';
            pTemp++;
        }

        sprintf(szScriptFile, "addons/eventscripts/%s/es_%s.txt", szAddonName, pFilename);
        DxMsg(1, __FUNCTION__, __LINE__, "%s\n", szScriptFile);
        FileHandle_t fFile = NULL;
        fFile = filesystem->Open(szScriptFile, "r"); 
		DxMsg(2, __FUNCTION__, __LINE__,"LoadAddonFile()c.\n");
        if (fFile) 
        {
            DxMsg(1, __FUNCTION__, __LINE__,"File opened: %s.\n", szAddonName);
            CScript *pNewScript = new CScript(szAddonName);
            if (!pNewScript)
                return;
            int nSize = filesystem->Size(fFile) + 1;
            DxMsg(1, __FUNCTION__, __LINE__,"Script %s created. Size: %d\n", szAddonName, nSize);
            char *szFullBuffer = (char *)malloc(sizeof(char)*nSize+1);
            memset(szFullBuffer, 0, nSize+1);
            int nReturn = filesystem->Read(szFullBuffer, nSize, fFile);
            szFullBuffer[nSize] = 0;
            DxMsg(1, __FUNCTION__, __LINE__,"Read from file: %s return %d.\n", szScriptFile, nReturn);
#ifdef ORANGEBOX
			CUtlBuffer *pFirstBuffer = new CUtlBuffer(szFullBuffer, nSize, CUtlBuffer::CONTAINS_CRLF | CUtlBuffer::TEXT_BUFFER | CUtlBuffer::READ_ONLY);
			CUtlBuffer *pBuffer = new CUtlBuffer(0, 0, CUtlBuffer::TEXT_BUFFER);
			pFirstBuffer->ConvertCRLF(*pBuffer);
#else
			CUtlBuffer *pBuffer = new CUtlBuffer(szFullBuffer, nSize, true);
#endif
            if (pNewScript->LoadFromBuffer(pBuffer))
            {
                DxMsg(1, __FUNCTION__, __LINE__," Loading file... \n");
                if (g_ShellVM.AddScript(pNewScript))
                {
                    int nAnnounceLevel = 0;
                    if (strstr(szAddonName, "corelib") != NULL) {
                        nAnnounceLevel = 1;
                    }
                    DxMsg(nAnnounceLevel, __FUNCTION__, __LINE__,"[EventScripts] Loaded %s\n", szAddonName);
                    char szCommand[MAX_COMMAND_STRING];
                    sprintf(szCommand, "es__db %s/load\n", szAddonName);
                    ImmediateExecuteCommand(szCommand);
                }
                else
                {
                    DxMsg(0, __FUNCTION__, __LINE__,"[EventScripts] Load script failed. It might already be loaded, try to es_unload it first. %s\n", szAddonName);
                    if (pNewScript) delete pNewScript;
                }
            }
            else
            {
                DxMsg(0, __FUNCTION__, __LINE__,"Error parsing script %s\n", szScriptFile);
            }
            pBuffer->Purge();
            delete pBuffer;
#ifdef ORANGEBOX
            pFirstBuffer->Purge();
			delete pFirstBuffer;
#endif
            free(szFullBuffer);
            filesystem->Close(fFile);
        }
        else
        {
            ErrMsg(0,"Could not open script for %s\n", szScriptFile);
        }
    }        
}

// RETURNKEY
ES_CON_COMMAND2(createscriptlist, 
               "Creates a new keygroup containing the current list of players.\n",
               1,
               "<keygroup> [scriptname]")
    int nFirstArg = bUseServerVariablesForReturn ? 0 : -1;
    nCount+=abs(nFirstArg);
    if (nCount > 1) {
        ASSOCIATED_KEYGROUP(args.Arg(1), true);
        CScript *p = (CScript *)g_ShellVM.GetScriptList();
        while (p)
        {
            bool bDone = false;
            if (nCount > 2) 
            {
                if (!FStrEq(p->GetName(),args.Arg(nFirstArg+2)))
                {
                    p = p->m_pNextScript;
                    continue;
                }
                else
                {
                    bDone = true;
                }
            }
            CREATESUBKEY_ID(pNewGroup, p->GetName(),k, ID_STRING);
            SETKEYVAL_S(k,"status", p->IsEnabled() ? "enabled" : "disabled");
            SETKEYVAL_S(k,"type", "txt");
            DxMsg(1, __FUNCTION__, __LINE__,"createscriptlist, listing done: %d\n", p->GetName());
            if (bDone) return;
            p = p->m_pNextScript;
        }
    }
}

void OutputScripts()
{
    DxMsg(0, __FUNCTION__, __LINE__,"\n[EventScripts] Loaded:\n");
    CScript *p = (CScript *)g_ShellVM.GetScriptList();
    while (p)
    {
        DxMsg(0, __FUNCTION__, __LINE__,"[EventScripts]   [%s] %s\n", p->IsEnabled() ? " enabled" : "disabled", p->GetName());
        p = p->m_pNextScript;
    }
#ifdef PYTHON_SUPPORT
    PyRun_SimpleString("es.printScriptList()");
#endif
    DxMsg(0, __FUNCTION__, __LINE__,"[EventScripts]------------------------\n\n");
}

ES_CON_COMMAND2(load, 
               "Loads a script or lists all loaded scripts if no script is provided.\n",
               1,
               "[scriptname]")
   DxMsg(2, __FUNCTION__, __LINE__, "es_load...\n");
    if (ARGC() < 2)
    {
        OutputScripts();
        return;
    }
    else
    {
        LoadAddonFile(ARGV(1));
        return;
    }
}


ES_CON_COMMAND2(_unload,
               "EventScripts internal command.\n",
               2,
               "<scriptname>")
    if (nCount > 1)
    {
        RemoveAddonFile(args.ArgS());
    }
    return;
}

ES_CON_COMMAND2(reload, 
               "Reloads a script that is loaded.\n",
               2,
               "<scriptname>")
    char szCommand[MAX_COMMAND_STRING];
    sprintf(szCommand, "es_xunload %s; es_xload %s\n", args.ArgS(),args.ArgS());
    ImmediateExecuteCommand(szCommand);
    return;
}

ES_CON_COMMAND2(unload, 
               "Unloads a script that has been loaded.\n",
               2,
               "<scriptname>")
    if (nCount > 1)
    {
        char szCommand[MAX_COMMAND_STRING];
        DxMsg(0, __FUNCTION__, __LINE__,"Unloading %s...\n", args.ArgS());
        sprintf(szCommand, "es__db %s/unload;es__unload %s\n", args.ArgS(),args.ArgS());
        ImmediateExecuteCommand(szCommand);
        return;
    }
}

ES_CON_COMMAND2(_disable, 
               "EventScripts internal command.\n",
               2,
               "<scriptname>")
    g_ShellVM.DisableScript(args.ArgS());
    return;
}

ES_CON_COMMAND2(disable, 
               "Disables a script that has been loaded.\n",
               2,
               "<scriptname>")
    char szCommand[MAX_COMMAND_STRING];
    DxMsg(0, __FUNCTION__, __LINE__,"Disabling %s...\n", args.ArgS());
    sprintf(szCommand, "es__db %s/disable;es__disable %s\n", args.ArgS(),args.ArgS());
    ImmediateExecuteCommand(szCommand);

    return;
}

ES_CON_COMMAND2(enable, 
               "Enables a script that has been loaded.\n",
               2,
               "<scriptname>")
    g_ShellVM.EnableScript(args.ArgS());
    DxMsg(0, __FUNCTION__, __LINE__,"Enabling %s...\n", args.ArgS());
    char szCommand[MAX_COMMAND_STRING];
    sprintf(szCommand, "es__db %s/enable\n", args.ArgS());
    ImmediateExecuteCommand(szCommand);
    return;
}


void GetEntityProp(edict_t *pEntity, const char * pReturnConVar, const char * pPropPath, bool bUseServerVariablesForReturn)
{
    int nOffset = 0;
    SendPropType nType = DPT_Vector;

    if (pEntity)
    {
        if (g_EventScriptsPlugin.GetOffset(pPropPath, &nOffset, &nType))
        {
            int value = 0;
            switch (nType)
            {
                case DPT_Array:
                case DPT_DataTable:
                    ErrMsg(0,"ERROR: Property type not yet supported.\n");
                    return;
                case DPT_Int:
                    {
                    int value = 0;
                    if(g_EventScriptsPlugin.GetTypedProp<int>(nOffset, pEntity, &value))
                    {
                        RETURNVAL(pReturnConVar, value, "i");
                    }
                    break;
                    }
                case DPT_Float:
                    {
                    float value = 0;
                    if(g_EventScriptsPlugin.GetTypedProp<float>(nOffset, pEntity, &value))
                    {
                        RETURNVAL(pReturnConVar, value, "f");
                    }
                    break;
                    }
                case DPT_Vector:
                    {
                    Vector value;
                    if(g_EventScriptsPlugin.GetTypedProp<Vector>(nOffset, pEntity, &value))
                    {
                        char *pVecString = AllocStringFromVector(value);
                        RETURNVAL(pReturnConVar, pVecString, "s");
                        free(pVecString);
                    }
                    break;
                    }
                case DPT_String:
                    {
                    char *value;
                    if(g_EventScriptsPlugin.GetTypedPropPtr<char *>(nOffset, pEntity, &value))
                    {
                        RETURNVAL(pReturnConVar, value == NULL ? "" : value, "s");
                    }
                    break;
                    }
                default:
                    ErrMsg(0,"ERROR: Property type unknown.\n");
                    return;
            }
        }
    }
    else
    {
        DxMsg(0, __FUNCTION__, __LINE__,"Entity doesn't exist.\n");
    }
}



// RETURNVAL
ES_CON_COMMAND2(getplayerprop,
               "Gets a server class property for a particular player\n",
               3,
               "<variable> <userid> <propertypath>")
    int nFirstArg = bUseServerVariablesForReturn ? 0 : -1;
    int nUserid = atoi(args.Arg(nFirstArg+2));
    if (nUserid > 0)
    {
        edict_t *pEntity = g_EventScriptsPlugin.LookupEdictByUserid(nUserid);
        GetEntityProp(pEntity, args.Arg(1), args.Arg(nFirstArg+3),bUseServerVariablesForReturn);
    }
    else
    {
        ErrMsg(0,"ERROR: Userid must be greater than 0.\n");
        ErrVar("Invalid arguments");
    }
}

// RETURNVAL
ES_CON_COMMAND2(getpropoffset,
               "Gets a server class property offset for a particular property path\n",
               3,
               "<variable> <propertypath>")
    int nFirstArg = bUseServerVariablesForReturn ? 0 : -1;
	int nOffset = 0;
	SendPropType nType;
	g_EventScriptsPlugin.GetOffset(args.Arg(nFirstArg+2), &nOffset, &nType);
	RETURNVAL(args.Arg(1), nOffset, "i");
}

// RETURNVAL
ES_CON_COMMAND2(getproptype,
               "Gets a server class property type for a particular property path\n",
               3,
               "<variable> <propertypath>")
    int nFirstArg = bUseServerVariablesForReturn ? 0 : -1;
	int nOffset = 0;
	SendPropType nType;
	g_EventScriptsPlugin.GetOffset(args.Arg(nFirstArg+2), &nOffset, &nType);
	RETURNVAL(args.Arg(1), nType, "i");
}

// RETURNVAL
ES_CON_COMMAND2(getentitypropoffset,
               "Gets a server class property for a particular entity index\n",
               3,
               "<variable> <index> <offset> <type>")
    int nFirstArg = bUseServerVariablesForReturn ? 0 : -1;
	int index = atoi(args.Arg(nFirstArg+2));
    edict_t *pEntity = PEntityOfEntIndex(index);
	int nOffset = atoi(args.Arg(nFirstArg+3));
	SendPropType nType = (SendPropType)atoi(args.Arg(nFirstArg+4));
    switch (nType)
    {
        case DPT_String:
        case DPT_Array:
        case DPT_DataTable:
            ErrMsg(0,"ERROR: Property type not yet supported.\n");
            return;
        case DPT_Int:
            {
            int value = 0;
            if(g_EventScriptsPlugin.GetTypedProp<int>(nOffset, pEntity, &value))
            {
                RETURNVAL(args.Arg(1), value, "i");
            }
            break;
            }
        case DPT_Float:
            {
            float value = 0;
            if(g_EventScriptsPlugin.GetTypedProp<float>(nOffset, pEntity, &value))
            {
                RETURNVAL(args.Arg(1), value, "f");
            }
            break;
            }
        case DPT_Vector:
            {
            Vector value;
            if(g_EventScriptsPlugin.GetTypedProp<Vector>(nOffset, pEntity, &value))
            {
                char *pVecString = AllocStringFromVector(value);
                RETURNVAL(args.Arg(1), pVecString, "s");
                free(pVecString);
            }
            break;
            }
        default:
            ErrMsg(0,"ERROR: Property type unknown.\n");
            return;
	}
}


// RETURNVAL
ES_CON_COMMAND2(getindexprop,
               "Gets a server class property for a particular entity index\n",
               3,
               "<variable> <index> <propertypath>")
    int nFirstArg = bUseServerVariablesForReturn ? 0 : -1;
    edict_t *pEntity = PEntityOfEntIndex(atoi(args.Arg(nFirstArg+2)));
    GetEntityProp(pEntity,args.Arg(1), args.Arg(nFirstArg+3),bUseServerVariablesForReturn);
}


ES_CON_COMMAND2(emitsound,
               "Plays a sound from an entity.",
               5,
               "<emitter-type> <id> <sound> <volume> <attenuation> [flags] [pitch]")
    int nIndex = 0;
    const char * szSound = args.Arg(3);
    esounds->PrecacheSound(szSound);
    if (FStrEq(args.Arg(1), "player"))
    {
        int nUserid = atoi(args.Arg(2));
        nIndex = g_EventScriptsPlugin.LookupIndexByUserID(nUserid);
    }
    else if (FStrEq(args.Arg(1), "entity"))
    {
        nIndex = atoi(args.Arg(2));
    }
    if (nIndex)
    {
	    MRecipientFilter filter;
        filter.AddAllPlayers(maxplayers, playerinfomanager);
        float fVolume = 1.0f;
        fVolume = atof(args.Arg(4));
        float fAttenuation = 0.0f;
        fAttenuation = atof(args.Arg(5));
		int nFlags = 0;
		if (nCount > 6) {
			nFlags = atoi(args.Arg(6));
		}
		int iPitch = PITCH_NORM;
		if (nCount > 7) {
			iPitch = atoi(args.Arg(7));
		}
#if defined(CSGO)
		esounds->EmitSound(filter, nIndex, 0, szSound, -1, szSound, fVolume, fAttenuation, 0, nFlags, iPitch);
#else
        esounds->EmitSound(filter, nIndex, 0, szSound, fVolume, fAttenuation, nFlags, iPitch);
#endif
    }
}


ES_CON_COMMAND2(playsound,
               "Plays a sound to a player.",
               4,
               "<userid> <sound> [volume]")
    int nIndex = 0;
    const char * szSound = args.Arg(2);
    esounds->PrecacheSound(szSound);
    int nUserid = atoi(args.Arg(1));
    nIndex = g_EventScriptsPlugin.LookupIndexByUserID(nUserid);
    if (nIndex && nUserid>0)
    {
	    MRecipientFilter filter;
        filter.AddPlayer(nUserid, maxplayers, playerinfomanager);
        float fVolume = 1.0f;
        fVolume = atof(args.Arg(3));
        float fAttenuation = 1.0f;
#if defined(CSGO)
        esounds->EmitSound(filter, nIndex, 0, szSound, -1, szSound, fVolume, fAttenuation, 0);
#else
        esounds->EmitSound(filter, nIndex, 0, szSound, fVolume, fAttenuation);
#endif

    }

}

// RETURNVAL
ES_CON_COMMAND2(precachedecal,
               "Precache a decal and return its index.\n",
               2,
               "[var] <modelpath>")
    int nFirstArg = bUseServerVariablesForReturn ? 0 : -1;
    nCount+=abs(nFirstArg);
    if (nCount > 1)
    {
        int nPosition = 1;
        if (nCount > 2)
        {
            nPosition++;
        }
        int nIndex = engine->PrecacheDecal (args.Arg(nPosition+nFirstArg));
        DxMsg(1, __FUNCTION__, __LINE__,"Precaching: %s\n",args.Arg(nPosition+nFirstArg));
        if (nCount > 2)
        {
            RETURNVAL(args.Arg(1), nIndex, "i");
        }
    }
    else
    {
        ErrMsg(0,"ERROR: Syntax error.\n");
        ErrVar("Syntax error");
    }
}



// RETURNVAL
ES_CON_COMMAND2(precachemodel,
               "Precache a model and return its index.\n",
               2,
               "[var] <modelpath>")
    int nFirstArg = bUseServerVariablesForReturn ? 0 : -1;
    nCount+=abs(nFirstArg);
    if (nCount > 1)
    {
        int nPosition = 1;
        if (nCount > 2)
        {
            nPosition++;
        }
        int nIndex = engine->PrecacheModel (args.Arg(nPosition+nFirstArg));
        DxMsg(1, __FUNCTION__, __LINE__,"Precaching: %s\n",args.Arg(nPosition+nFirstArg));
        if (nCount > 2)
        {
            RETURNVAL(args.Arg(1), nIndex, "i");
        }
    }
    else
    {
        ErrMsg(0,"ERROR: Syntax error.\n");
        ErrVar("Syntax error");
    }
}

ES_CON_COMMAND2(precachesound,
               "Precache sound.\n",
               1,
               "<soundpath>")
    if (nCount > 1)
    {
        esounds->PrecacheSound(args.Arg(1));
        DxMsg(1, __FUNCTION__, __LINE__,"Precaching: %s\n",args.Arg(1));
    }
}

// RETURNVAL
ES_CON_COMMAND2(getplayerhandle,
               "Gets the handle for a player class property using an entity handle (Untested)\n",
               2,
               "<variable> <userid>")
    CHANGE_FIRST_ARG_INDEX_FOR_N_RETURNVALS(1)
    RETURNVAL(args.Arg(1), (int)g_EventScriptsPlugin.LookupHandleByUserid(atoi(args.Arg(nFirstArg+2))) ,"i");
}

void SetEntityProp(edict_t *pEntity, const char *szOffsetName, const char *szOffsetValue)
{
    if (pEntity)
    {
        int nOffset = 0;
        SendPropType nType = DPT_Vector;
        if (g_EventScriptsPlugin.GetOffset(szOffsetName, &nOffset, &nType))
        {
            int value = 0;
            switch (nType)
            {
                case DPT_String:
                case DPT_Array:
                case DPT_DataTable:
                    ErrMsg(0,"ERROR: Property type not yet supported.\n");
                    return;
                case DPT_Int:
                    {
                    int value = 0;
                    if(!g_EventScriptsPlugin.SetTypedProp<int>(nOffset, pEntity, atoi(szOffsetValue)))
                    {
                        ErrMsg(0,"ERROR: Unable to set property. It might be read-only.\n");
                        ErrVar("Invalid parameter");
                    }
                    break;
                    }
                case DPT_Float:
                    {
                    float value = 0;
                    if(!g_EventScriptsPlugin.SetTypedProp<float>(nOffset, pEntity, atof(szOffsetValue)))
                    {
                        ErrMsg(0,"ERROR: Unable to set property. It might be read-only.\n");
                        ErrVar("Invalid parameter");
                    }
                    break;
                    }
                case DPT_Vector:
                    {
                    Vector * value = AllocVectorFromString(szOffsetValue);
                    if(!g_EventScriptsPlugin.SetTypedProp<Vector>(nOffset, pEntity, *value))
                    {
                        ErrMsg(0,"ERROR: Unable to set property. It might be read-only.\n");
                        ErrVar("Invalid parameter");
                    }
                    delete value;
                    break;
                    }
                default:
                    ErrMsg(0,"ERROR: Property type unknown.\n");
                    return;
            }
        }
    }

}

ES_CON_COMMAND2(setplayerprop,
               "Sets a server class property for the given player\n",
               3,
               "<userid> <propertypath> <value>")
    int nUserid = atoi(args.Arg(1));
    if (nUserid > 0) 
    {
        edict_t *pEntity = g_EventScriptsPlugin.LookupEdictByUserid(nUserid);
        SetEntityProp(pEntity, args.Arg(2), args.Arg(3));
    }
}

ES_CON_COMMAND2(entsetname,
               "Names the entity the player is looking at. (DOES NOT SET PLAYER NAME)\n",
               2,
               "<userid> <name>\n")
    int nUserid = atoi(args.Arg(1));
    if (nUserid > 0) 
    {
        int i = g_EventScriptsPlugin.LookupIndexByUserID(nUserid);
        g_EventScriptsPlugin.m_pEntSetname->m_bReplaceNextClient = true;
        g_EventScriptsPlugin.m_pEntSetname->m_nNextCommandClientIndex = i;
        char szMessage[MAX_COMMAND_STRING];
        char szTemp[MAX_COMMAND_STRING];
        strcpy(szMessage, args.ArgS());
        sprintf(szTemp, "es__entsetname %s\n", strchr(szMessage, ' ') + 1);
        ExecuteCommandNow(szTemp);

    }
}

ES_CON_COMMAND2(remove,
               "Removes an entity class\n",
               1,
               "<entity>")
    if (nCount < 2)
    {
        DxMsg(0, __FUNCTION__, __LINE__,"Syntax: es_xremove <entity>\n");
        ErrVar("Not enough arguments.");
        return;
    }

    g_EventScriptsPlugin.m_pEntRemove->m_bReplaceNextClient = true;
    g_EventScriptsPlugin.m_pEntRemove->m_nNextCommandClientIndex = 0;
    char szTemp[MAX_COMMAND_STRING];
    sprintf(szTemp, "es__remove %s\n", args.ArgS());
    ExecuteCommandNow(szTemp);

}


ES_CON_COMMAND2(setpos,
               "Teleports a player.\n",
               3,
               "<userid> <x> <y> [z]\n")
    int nUserid = atoi(args.Arg(1));
    if (nUserid > 0) 
    {
        int i = g_EventScriptsPlugin.LookupIndexByUserID(nUserid);
        g_EventScriptsPlugin.m_pSetPos->m_bReplaceNextClient = true;
        g_EventScriptsPlugin.m_pSetPos->m_nNextCommandClientIndex = i;
        char szMessage[MAX_COMMAND_STRING];
        char szTemp[MAX_COMMAND_STRING];
        strcpy(szMessage, args.ArgS());
        sprintf(szTemp, "es__setpos %s\n", strchr(szMessage, ' ') + 1);
        ExecuteCommandNow(szTemp);
    }
}



ES_CON_COMMAND2(setang,
               "Sets player view angle.\n",
               3,
               "<userid> <pitch> <yaw> [roll]\n")
    int nUserid = atoi(args.Arg(1));
    if (nUserid > 0) 
    {
        int i = g_EventScriptsPlugin.LookupIndexByUserID(nUserid);
        g_EventScriptsPlugin.m_pSetAng->m_bReplaceNextClient = true;
        g_EventScriptsPlugin.m_pSetAng->m_nNextCommandClientIndex = i;
        char szMessage[MAX_COMMAND_STRING];
        char szTemp[MAX_COMMAND_STRING];
        strcpy(szMessage, args.ArgS());
        sprintf(szTemp, "es__setang %s\n", strchr(szMessage, ' ') + 1);
        ExecuteCommandNow(szTemp);
    }
}
ES_CON_COMMAND2(setentitypropoffset,
               "Gets a server class property for a particular entity index\n",
               4,
               "<index> <offset> <type> <value>")
    int nFirstArg = 0;
	int index = atoi(args.Arg(nFirstArg+1));
    edict_t *pEntity = PEntityOfEntIndex(index);
	int nOffset = atoi(args.Arg(nFirstArg+2));
	SendPropType nType = (SendPropType)atoi(args.Arg(nFirstArg+3));
	const char *szOffsetValue = args.Arg(nFirstArg+4);
    int value = 0;
    switch (nType)
    {
        case DPT_String:
        case DPT_Array:
        case DPT_DataTable:
            ErrMsg(0,"ERROR: Property type not yet supported.\n");
            return;
        case DPT_Int:
            {
            int value = 0;
            if(!g_EventScriptsPlugin.SetTypedProp<int>(nOffset, pEntity, atoi(szOffsetValue)))
            {
                ErrMsg(0,"ERROR: Unable to set property. It might be read-only.\n");
                ErrVar("Invalid parameter");
            }
            break;
            }
        case DPT_Float:
            {
            float value = 0;
            if(!g_EventScriptsPlugin.SetTypedProp<float>(nOffset, pEntity, atof(szOffsetValue)))
            {
                ErrMsg(0,"ERROR: Unable to set property. It might be read-only.\n");
                ErrVar("Invalid parameter");
            }
            break;
            }
        case DPT_Vector:
            {
            Vector * value = AllocVectorFromString(szOffsetValue);
            if(!g_EventScriptsPlugin.SetTypedProp<Vector>(nOffset, pEntity, *value))
            {
                ErrMsg(0,"ERROR: Unable to set property. It might be read-only.\n");
                ErrVar("Invalid parameter");
            }
            delete value;
            break;
            }
        default:
            ErrMsg(0,"ERROR: Property type unknown, %d.\n", nType);
            return;
    }
}

ES_CON_COMMAND2(setindexprop,
               "Sets a server class property for the given entity index\n",
               3,
               "<index> <propertypath> <value>")
    edict_t *pEntity = PEntityOfEntIndex(atoi(args.Arg(1)));
    SetEntityProp(pEntity, args.Arg(2), args.Arg(3));
}

// RETURNVAL_UNSUPPORTED
ES_CON_COMMAND3(strcmp,
               "Compares two strings.\n",
               3,
               "<returnvar> <string1> <string2>")
    int nReturn = 0;
    if (args.ArgC() > 2)
    {
        nReturn = strcmp(args.Arg(2), args.Arg(3));
    }
    
    ConVar* cvar = SetConVar( args.Arg(1), nReturn, true);
    if (!cvar)
    {
        DxMsg(0, __FUNCTION__, __LINE__,"The var \"%s\" could not be set.\n", args.Arg(1)); 
        ErrVar("Variable could not be set.");
        return; // If it doesn t find the var
    }
    else
    {
        DxMsg(1, __FUNCTION__, __LINE__,"%s = \"%s\"\n",cvar->GetName(), cvar->GetString());
    }

}

// RETURNVAL_UNSUPPORTED
ES_CON_COMMAND3(strlen,
               "Returns the length of a string.\n",
               1,
               "<returnvar> \"string-to-measure\"\n")
    int nLen = 0;
    if (args.ArgC() > 1)
    {
        nLen = strlen(args.Arg(2));
    }
    
    ConVar* cvar = SetConVar( args.Arg(1), nLen, true);
    if (!cvar)
    {
        DxMsg(0, __FUNCTION__, __LINE__,"The var \"%s\" could not be set.\n", args.Arg(1)); 
        ErrVar("Variable could not be set.");
        return; // If it doesn t find the var
    }
    else
    {
        DxMsg(1, __FUNCTION__, __LINE__,"%s = \"%s\"\n",cvar->GetName(), cvar->GetString());
    }

}


ES_CON_COMMAND2(dumpserverclasses,
               "Dumps to the console all server classes.\n",
               1,
               "\n")
    g_EventScriptsPlugin.DUMP_ServerClasses();
}


void CreateKeyTableInformation(SendTable * pTable, edict_t *p, int TableOffset, const char *szTableName, void *pKey, bool bUseServerVariablesForReturn)
{

    int NumProps = pTable->GetNumProps();
    for (int i=0; i<NumProps; i++)
    {
        char szFullPath[1024];
        sprintf(szFullPath, "%s.%s", szTableName, pTable->GetProp(i)->GetName());
        char *pType = "Unknown";
        char *pValueOutput = "%s = (TODO: %s)\n";
        bool bValueOutput = false;
        switch (pTable->GetProp(i)->GetType())
        {
        case DPT_Int:
            pType = "int";
            pValueOutput = "%s = %d\n";
            bValueOutput = true;
            break;
        case DPT_Float:
            pType = "float";
            pValueOutput = "%s = %f\n";
            bValueOutput = true;
            break;
        case DPT_Vector:
            pType = "vector";
            pValueOutput = "%s = %s\n";
            bValueOutput = true;
            break;
#ifdef LEFT4DEAD
        case DPT_VectorXY:
            pType = "vectorXY";
            break;
#endif
        case DPT_String:
            pType = "string";
            pValueOutput = "%s = %s\n";
            bValueOutput = true;
            break;
        case DPT_Array:
            pType = "array";
            break;
        case DPT_DataTable:
            pType = "datatable";
            break;
        default:
            pType = "Unknown";
            pValueOutput = "%s = (%s)\n";
            break;
        }
        if (DPT_DataTable == pTable->GetProp(i)->GetType())
        {
            CreateKeyTableInformation(pTable->GetProp(i)->GetDataTable(), p, TableOffset+pTable->GetProp(i)->GetOffset(), szFullPath, pKey, bUseServerVariablesForReturn);
        }
        else
        {
            if (bValueOutput)
            {
              int n = pTable->GetProp(i)->GetOffset();
              n+= TableOffset;
              if (DPT_Int == pTable->GetProp(i)->GetType())
              {
                  int iValue = 0;
                  if (g_EventScriptsPlugin.GetTypedProp<int>(n, p, &iValue))
                  {
                    SETKEYVAL_I(pKey,szFullPath, iValue);
                  }
              }
              if (DPT_Float == pTable->GetProp(i)->GetType())
              {
                  float fValue = 0;
                  if (g_EventScriptsPlugin.GetTypedProp<float>(n, p, &fValue))
                  {
                    SETKEYVAL_F(pKey,szFullPath, fValue);
                  }
              }
              if (DPT_Vector == pTable->GetProp(i)->GetType())
              {
                  Vector vValue;
                  if (g_EventScriptsPlugin.GetTypedProp<Vector>(n, p, &vValue))
                  {
                     char *szVector = AllocStringFromVector(vValue);
                    SETKEYVAL_S(pKey,szFullPath, szVector);
                     free(szVector);
                  }
              }
              if (DPT_String == pTable->GetProp(i)->GetType())
              {
                  char * value = NULL;
                  if (g_EventScriptsPlugin.GetTypedPropPtr<char *>(n, p, &value))
                  {
                      SETKEYVAL_S(pKey,szFullPath, value == NULL ? "" : value);
                  }
              }
            }
            else
            {
                SETKEYVAL_S(pKey,szFullPath, pType);
            }
        }
        
    }
}


void DumpTableInformation(SendTable * pTable, edict_t *p, int TableOffset, const char *szTableName)
{
    int NumProps = pTable->GetNumProps();
    if (!pTable || !szTableName)
        return;

    for (int i=0; i<NumProps; i++)
    {
        DxMsg(1, __FUNCTION__, __LINE__,"dumpti: %d\n", i);
        char szFullPath[1024];
        sprintf(szFullPath, "%s.%s", szTableName, pTable->GetProp(i)->GetName());
        DxMsg(1, __FUNCTION__, __LINE__,"dump2ti: %s\n", szFullPath);
        char *pType = "Unknown";
        char *pValueOutput = "%s = (TODO: %s)\n";
        bool bValueOutput = false;
        switch (pTable->GetProp(i)->GetType())
        {
        case DPT_Int:
            pType = "int";
            pValueOutput = "%s = %d\n";
            bValueOutput = true;
            break;
        case DPT_Float:
            pType = "float";
            pValueOutput = "%s = %f\n";
            bValueOutput = true;
            break;
        case DPT_Vector:
            pType = "vector";
            pValueOutput = "%s = %s\n";
            bValueOutput = true;
            break;
        case DPT_String:
            pType = "string";
            break;
        case DPT_Array:
            pType = "array";
            break;
        case DPT_DataTable:
            pType = "datatable";
            break;
        default:
            pType = "Unknown";
            pValueOutput = "%s = (%s)\n";
            break;
        }
        if (DPT_DataTable == pTable->GetProp(i)->GetType())
        {
            DumpTableInformation(pTable->GetProp(i)->GetDataTable(), p, TableOffset+pTable->GetProp(i)->GetOffset(), szFullPath);
        }
        else
        {
            if (bValueOutput)
            {
              int n = pTable->GetProp(i)->GetOffset();
              n+= TableOffset;
              if (DPT_Int == pTable->GetProp(i)->GetType())
              {
                  int iValue = 0;
                  if (g_EventScriptsPlugin.GetTypedProp<int>(n, p, &iValue))
                  {
                     DxMsg(0, __FUNCTION__, __LINE__,pValueOutput, szFullPath, iValue);
                  }
              }
              if (DPT_Float == pTable->GetProp(i)->GetType())
              {
                  float fValue = 0;
                  if (g_EventScriptsPlugin.GetTypedProp<float>(n, p, &fValue))
                  {
                     DxMsg(0, __FUNCTION__, __LINE__,pValueOutput, szFullPath, fValue);
                  }
              }
              if (DPT_Vector == pTable->GetProp(i)->GetType())
              {
                  Vector vValue;
                  if (g_EventScriptsPlugin.GetTypedProp<Vector>(n, p, &vValue))
                  {
                     char *szVector = AllocStringFromVector(vValue);
                     DxMsg(0, __FUNCTION__, __LINE__,pValueOutput, szFullPath, szVector);
                     free(szVector);
                  }
              }
            }
            else
            {
                DxMsg(0, __FUNCTION__, __LINE__,pValueOutput, szFullPath, pType);
            }
        }
        
    }
}

void DumpClassInformation(ServerClass * sc, edict_t *p, const char *szPrefix)
{
    char szFullPath[1028];
    sprintf(szFullPath, "%s: %s", szPrefix, sc->GetName());
    DumpTableInformation(sc->m_pTable, p, 0, szFullPath);
}

void CreateKeyClassInformation(ServerClass * sc, edict_t *p, void * pNewKey, bool bUseServerVariablesForReturn)
{
    char szFullPath[1028];
    sprintf(szFullPath, "%s", sc->GetName());
    CreateKeyTableInformation(sc->m_pTable, p, 0, szFullPath, pNewKey, bUseServerVariablesForReturn);
}


ES_CON_COMMAND2(dumpentities, 
               "Dumps to console all server classes and properties for all entities.\n",
               1,
               "\n")
    for (int i = 0;i<engine->GetEntityCount();i++)
    {
        DxMsg(1, __FUNCTION__, __LINE__,"dump: %d\n", i);
        edict_t *p = PEntityOfEntIndex(i);
        DxMsg(1, __FUNCTION__, __LINE__,"dump2: %d\n", i);
        if (p)
        {
            DxMsg(1, __FUNCTION__, __LINE__,"dump3: %d\n", i);
            IServerNetworkable *pisn = p->GetNetworkable();
            DxMsg(1, __FUNCTION__, __LINE__,"dump4: %d\n", i);
            if (p->GetClassName() && strlen(p->GetClassName())>0)
            {
                DxMsg(1, __FUNCTION__, __LINE__,"dump5: %d, %s\n", i, p->GetClassName());
                if (pisn)
                {
                    DxMsg(1, __FUNCTION__, __LINE__,"dump6: %d\n", i);
                    ServerClass * pclass = pisn->GetServerClass();
                    DxMsg(1, __FUNCTION__, __LINE__,"dump7: %d\n", i);
                    if (pclass && pclass->GetName())
                    {
                        DxMsg(1, __FUNCTION__, __LINE__,"dump8: %d\n", i);
                        char szHeader[1028];
                        DxMsg(1, __FUNCTION__, __LINE__,"dump9: %d\n", i);
                        sprintf(szHeader, "%d[%s]", i, p->GetClassName());
                        DxMsg(1, __FUNCTION__, __LINE__,"dump10: %d\n", i);
                        DumpClassInformation(pclass, p, szHeader);
                        DxMsg(1, __FUNCTION__, __LINE__,"dump11: %d\n", i);
                    }
                    else
                    {
                        DxMsg(0, __FUNCTION__, __LINE__,"%d: %s.\n", i, p->GetClassName());
                    }
                }
                else
                {
                    DxMsg(0, __FUNCTION__, __LINE__,"%d: %s\n", i, p->GetClassName());
                }
            }
        }
    }
}

ES_CON_COMMAND2(menu, 
               "Sends an AMX-Style menu to the users",
               3,
               "<duration> <userid> <menu-text> [options]")
    char szCommand[MAX_COMMAND_STRING];
    unescape(szCommand, args.Arg(3));
    const char *options = NULL;
    if (nCount>4)
         options = args.Arg(4);

    UTIL_SendMenu(atoi(args.Arg(2)), atoi(args.Arg(1)), szCommand, options);
}

// RETURNVAL
ES_CON_COMMAND2(gethandlefromindex, 
               "Gets the handle for an entity from its integer index.\n",
               2,
               "<var> <index>")
    const char * szIndex = bUseServerVariablesForReturn ? args.Arg(2) : args.Arg(1);
    unsigned int val = g_EventScriptsPlugin.LookupHandleByIndex(atoi(szIndex));
    RETURNVAL(args.Arg(1), (int)val, "i");
}

// RETURNVAL
ES_CON_COMMAND2(getindexfromhandle, 
               "Gets the index for an entity from its integer handle.\n",
               2,
               "<var> <handle-int>")
    const char * szIndex = bUseServerVariablesForReturn ? args.Arg(2) : args.Arg(1);
    unsigned int val = g_EventScriptsPlugin.LookupIndexByHandle(atoi(szIndex));
    RETURNVAL(args.Arg(1), (int)val, "i");
}

// RETURNKEY
ES_CON_COMMAND2(createentitylist, 
               "Creates a keygroup (or dictionary) for an entity class or for all entities.\n",
               2,
               "<keygroupname> [entity-class]\n")
    int nFirstArg = bUseServerVariablesForReturn ? 0 : -1;
    nCount+=abs(nFirstArg);

    if (nCount > 1)
    {
        ASSOCIATED_KEYGROUP(args.Arg(1), true);

        for (int i = 0;i<engine->GetEntityCount();i++)
        {
            edict_t *p = PEntityOfEntIndex(i);
            if (p)
            {
                IServerNetworkable *pisn = p->GetNetworkable();
                if (p->GetClassName() && strlen(p->GetClassName())>0)
                {
                    if (nCount > 2)
                    {
                        int nIndex = atoi(args.Arg(nFirstArg+2));
                        if ((nIndex > 0 && nIndex == i) || FStrEq(args.Arg(nFirstArg+2), p->GetClassName()))
                        {
                            if (pisn)
                            {
                                ServerClass * pclass = pisn->GetServerClass();
                                if (pclass && pclass->GetName())
                                {
                                    
                                    char szHeader[1028];
                                    sprintf(szHeader, "%d", i);
                                    CREATESUBKEY_ID(pNewGroup, i,pNewKey, ID_INT);
                                    SETKEYVAL_S(pNewKey,"classname", p->GetClassName());
									SETKEYVAL_I(pNewKey,"handle", g_EventScriptsPlugin.LookupHandleByIndex(i));
                                    CreateKeyClassInformation(pclass, p, pNewKey, bUseServerVariablesForReturn);
                                }
                            }
                            if ((nIndex > 0 && nIndex == i))
                            {
                                return;
                            }
                        }
                    }
                    else // only 1 argument
                    {
                        if (pisn)
                        {
                            ServerClass * pclass = pisn->GetServerClass();
                            if (pclass && pclass->GetName())
                            {
                                
                                char szHeader[1028];
                                sprintf(szHeader, "%d", i);
                                    CREATESUBKEY_ID(pNewGroup, i,pNewKey, ID_INT);
                                    SETKEYVAL_S(pNewKey,"classname", p->GetClassName());
									SETKEYVAL_I(pNewKey,"handle", g_EventScriptsPlugin.LookupHandleByIndex(i));
                                    CreateKeyClassInformation(pclass, p, pNewKey, bUseServerVariablesForReturn);
                            }
                        }
                }
                }
            }
        }
    }
}

// RETURNKEY
ES_CON_COMMAND2(createentityindexlist, 
               "Creates a keygroup (or dictionary) of all indexes for an entity class or for all entities.\n",
               2,
               "<keygroupname> [entity-class]\n")
    int nFirstArg = bUseServerVariablesForReturn ? 0 : -1;
    nCount+=abs(nFirstArg);

    if (nCount > 1)
    {
        ASSOCIATED_KEYGROUP(args.Arg(1), true);

        for (int i = 0;i<engine->GetEntityCount();i++)
        {
            edict_t *p = PEntityOfEntIndex(i);
            if (p)
            {
                const char * classname = p->GetClassName();
                if (classname && strlen(classname)>0)
                {
                    if (nCount > 2)
                    {
                        if (FStrEq(args.Arg(nFirstArg+2), classname))
                        {
                            char szHeader[1028];
                            sprintf(szHeader, "%d", i);
                            CREATESUBKEY_ID(pNewGroup, i,pNewKey, ID_INT);
                            SETKEYVAL_S(pNewKey,"classname", classname);
                        }
                    }
                    else // only 1 argument
                    {
                            char szHeader[1028];
                            sprintf(szHeader, "%d", i);
                            CREATESUBKEY_ID(pNewGroup, i,pNewKey, ID_INT);
                            SETKEYVAL_S(pNewKey,"classname", classname);
					}
                }
            }
        }
    }
}

ES_CON_COMMAND2(soon, 
               "Adds a command to the end of the command queue.\n",
               2,
               "<command>")
    char szCommand[MAX_COMMAND_STRING];
    sprintf(szCommand, "%s\n",args.ArgS());
    ServerCommand(szCommand);
}

ES_CON_COMMAND2(createbot, 
               "Adds a bot to the server.\n",
               2,
               "[var] <botname>")
    int nFirstArg = bUseServerVariablesForReturn ? 0 : -1;
    nCount+=abs(nFirstArg);
    const char *pname = NULL;
    const char *vname = NULL;
    if (nCount>2)
    {
        pname = args.Arg(2);
        vname = args.Arg(1);
    }
    else
    {
        pname = args.Arg(1);
        vname = NULL;
    }
    edict_t *pPlayer = engine->CreateFakeClient(pname);
    if (vname && pPlayer)
    {
            RETURNVAL(vname, engine->GetPlayerUserId(pPlayer), "i");
    }
}

void FadeClientVolume(int nUserid, float percent, float fadetime, float holdtime, float fadeintime )
{
    edict_t * pPlayer = g_EventScriptsPlugin.LookupEdictByUserid(nUserid);
    if (pPlayer)
    {
        engine->FadeClientVolume(pPlayer, percent, fadetime, holdtime, fadeintime);
        DxMsg(1, __FUNCTION__, __LINE__, "Fading client volume: %d\n");
    }
    else
    {
        DxMsg(0, __FUNCTION__, __LINE__, "FadeClientVolume: Unable to find player\n");
        ErrVar("Invalid userid");
    }
}

ES_CON_COMMAND2(fadevolume, 
               "Fades the volume for a client.\n",
               5,
               "<userid> <percent> <fadetime> <holdtime> <fadeintime>")
    FadeClientVolume(atoi(args.Arg(1)), atof(args.Arg(2)), atof(args.Arg(3)), atof(args.Arg(4)),atof(args.Arg(5)));
}

void LightStyle(int nStyle, const char *val)
{
    engine->LightStyle(nStyle, val);
    DxMsg(1, __FUNCTION__, __LINE__, "Setting lightstyle: %d, %s\n", nStyle, val);
}

ES_CON_COMMAND2(lightstyle, 
               "Set light style.\n",
               3,
               "<stylenum> <stylestring>")
    LightStyle(atoi(args.Arg(1)), args.Arg(2));
}

void SetFakeClientConvarValue(int nUserid, const char *cvarname, const char *cvarval)
{

    edict_t * pPlayer = g_EventScriptsPlugin.LookupEdictByUserid(nUserid);
    if (pPlayer)
    {
        engine->SetFakeClientConVarValue(pPlayer, cvarname, cvarval);
        DxMsg(1, __FUNCTION__, __LINE__, "Set client var: %d, %s = %s\n", nUserid, cvarname, cvarval);
    }
    else
    {
        DxMsg(0, __FUNCTION__, __LINE__, "SetFakeClientConvarValue: Unable to find player\n");
        ErrVar("Invalid userid");
    }
}
ES_CON_COMMAND2(botsetvalue, 
               "Sets a convar value for a fake client.\n",
               4,
               "<userid> <convar-name> \"<convar-value>\"")

    SetFakeClientConvarValue(atoi(args.Arg(1)), args.Arg(2), args.Arg(3));
}



void StopSound(int nUserid, const char *pSample)
{
    int nIndex = g_EventScriptsPlugin.LookupIndexByUserID(nUserid);
    if (nIndex > 0)
    {
#if defined(CSGO)
        esounds->StopSound(nIndex, 0, pSample, -1);
#else
        esounds->StopSound(nIndex, 0, pSample);
#endif
    }
    else
    {
        DxMsg(0, __FUNCTION__, __LINE__, "StopSound: Unable to find player %d\n", nUserid);
        ErrVar("Invalid userid");
    }
}

ES_CON_COMMAND2(stopsound, 
               "Stops a specific sound for a player.\n",
               3,
               "<userid> <soundname>")
    StopSound(atoi(args.Arg(1)), args.Arg(2));
}

const char * CEventScriptsPlugin::GetGameDescription()
{
    return m_ServerDll ? m_ServerDll->GetGameDescription() : NULL;
}

// RETURNVAL
ES_CON_COMMAND2(getgame, 
               "Returns the name of the Source game being played.\n",
               1,
               "<variable>")
    RETURNVAL(args.Arg(1), g_EventScriptsPlugin.GetGameDescription(), "s");
}

// RETURNVAL
ES_CON_COMMAND2(isdedicated, 
               "Returns 1 in the variable if the server a dedicated server.\n",
               1,
               "<variable>")
    RETURNVAL(args.Arg(1), static_cast<int>(engine->IsDedicatedServer()), "i");
}

void ChangeTeam( int userid, int iTeamNum )
{
    edict_t * pEntity = g_EventScriptsPlugin.LookupEdictByUserid(userid);
    if (pEntity)
    {
        if(playerinfomanager)
        {
	        IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEntity );
            if (playerinfo && playerinfo->IsPlayer())
            {
                
                playerinfo->ChangeTeam(iTeamNum);
                return;
                
            }
            else
            {
                DxMsg(0, __FUNCTION__, __LINE__,"Userid isn't a player: %d\n", userid);
                return;
            }
        }
        else
        {
            DxMsg(0, __FUNCTION__, __LINE__,"playerinfomanager isn't ready yet.\n");
            return;
        }
    }
    else
    {
        DxMsg(0, __FUNCTION__, __LINE__,"Player doesn't exist: %d\n", userid);
        return;
    }
    
}


ES_CON_COMMAND2(changeteam, 
               "Changes the team of the player.\n",
               2,
               "<userid> <teamnum>")
    ChangeTeam(atoi(args.Arg(1)), atoi(args.Arg(2)));
}

void CEventScriptsPlugin::DumpUserMessages()
{
    if(m_ServerDll)
	    {
		    char sz_mname[256] = "";
		    int sizereturn = 0;
		    for(int x=0; x<maxmessages_cvar.GetInt(); x++)
		    {
			    if(m_ServerDll->GetUserMessageInfo(x, sz_mname, 255, sizereturn))
			    {
                    DxMsg(0, __FUNCTION__, __LINE__, " Id: %02d, Size: %d, Message: %s\n", x, sizereturn, sz_mname);
			    }
		    }
	    }
}

CON_COMMAND(es_dumpusermessages, "Dump UserMessage list for the Source mod.")
{
    g_EventScriptsPlugin.DumpUserMessages();
}


CON_COMMAND( es__clientcmdproxy, "Internal ES Command" )
{
#ifdef ORANGEBOX
	g_pCmdArg = &args;
#endif
    DxMsg(1, __FUNCTION__, __LINE__, "CmdProxy: \"%s %s\"\n", ARGV(0), ARGS());
    const char * blockname = (const char *)g_ClientCmdTable.FindElement(ARGV(0));
    if (NULL == blockname)
    {
        DxMsg(0, __FUNCTION__, __LINE__, "CmdProxy: Internal Error. No such command registered: %s\n", ARGV(0));
        ErrVar("Internal Error");
    }
    else
    {
        char szCommandLine[MAX_COMMAND_STRING];
        if (ARGS())
        {
            sprintf(szCommandLine, "%s %s", ARGV(0), ARGS());
        }
        else
        {
            sprintf(szCommandLine, "%s", ARGV(0));
        }
        doblock(blockname, szCommandLine);
    }
}

CON_COMMAND( es__cmdproxy, "Internal ES Command" )
{
#ifdef ORANGEBOX
	g_pCmdArg = &args;
#endif
    DxMsg(1, __FUNCTION__, __LINE__, "CmdProxy: \"%s %s\"\n", ARGV(0), ARGS());
    const char * blockname = (const char *)g_CmdTable.FindElement(ARGV(0));
    if (NULL == blockname)
    {
        DxMsg(0, __FUNCTION__, __LINE__, "CmdProxy: Internal Error. No such command registered: %s\n", ARGV(0));
        ErrVar("Internal Error");
    }
    else
    {
        char szCommandLine[MAX_COMMAND_STRING];
        if (ARGS())
        {
            sprintf(szCommandLine, "%s %s", ARGV(0), ARGS());
        }
        else
        {
            sprintf(szCommandLine, "%s", ARGV(0));
        }
        doblock(blockname, szCommandLine);
    }
}


ES_CON_COMMAND2(setview,
               "Changes a players view to share that of a particular entity index.",
               1,
               "<userid> [entity-index]")

    int nUserid = atoi(args.Arg(1));
    edict_t * pPlayer = g_EventScriptsPlugin.LookupEdictByUserid(nUserid);
    if (pPlayer)
    {
        edict_t *pEntity = NULL;
        if (args.ArgC()>1)
        {
            int nEntityIndex = atoi(args.Arg(2));
            pEntity = PEntityOfEntIndex(nEntityIndex);
        }
        else
        {
            pEntity = pPlayer;
        }
        if (pEntity)
        {
            engine->SetView(pPlayer, pEntity);
        }
        else
        {
            DxMsg(0, __FUNCTION__, __LINE__, "es_xsetview: Couldn't find an entity at that index.\n");
            ErrVar("No such entity");
        }
    }
    else
    {
        DxMsg(0, __FUNCTION__, __LINE__, "es_xsetview: Couldn't find a player entity at that index.\n");
        ErrVar("No such entity");
    }
}


ES_CON_COMMAND2(regcmd,
               "Adds a console command that refers to a particular block.",
               3,
               "<command-name> <block-name> <description>")
   const char *pszName = args.Arg(1);
   if (NULL != g_CmdTable.FindElement(pszName))
   {
       DxMsg(0, __FUNCTION__, __LINE__, "es_xregcmd: ERROR. Command %s already exists.\n",pszName);
       return;
   }
#ifdef LEFT4DEAD
    ConCommandBase *pPtr;
    pPtr = FindCommandEx(pszName);
    if (pPtr)
    {
      DxMsg(0, __FUNCTION__, __LINE__, "[EventScripts] Reregistering our '%s' as duplicates not allowed.\n", pszName);
      cvarmanager->UnregisterConCommand(pPtr);
    }
#endif
   ConCommand * tempcmd = new ConCommand( strdup(pszName), es__cmdproxy, strdup(args.Arg(3)));
   if (tempcmd)
   {	
		HashData_t *pNew = new HashData_t;
		pNew->name = strdup(args.Arg(1));
		pNew->value = strdup(args.Arg(2));
		g_CmdTable.InsertElement(pNew);
   }
   else
   {
       DxMsg(0, __FUNCTION__, __LINE__, "es_xregcmd: Out of memory.\n");
       return;
   }
   
}

ES_CON_COMMAND2(queryregcmd,
               "Queries which block a console command refers to.",
               2,
               "<varname> <cmdname>")
   int nFirstArg = bUseServerVariablesForReturn ? 0 : -1;
   const char *varname = args.Arg(nFirstArg+1);
   const char *cmdname = args.Arg(nFirstArg+2);
   const char * blockname = (const char *)g_CmdTable.FindElement(cmdname);
   if (NULL == blockname)
   {
       DxMsg(2, __FUNCTION__, __LINE__, "es_xqueryregcmd: ERROR. Command %s wasn't registered.\n",cmdname);
       blockname = "";
   }
   RETURNVAL(varname, blockname, "s");
}

// RETURNVAL
ES_CON_COMMAND2(regex,
               "Various regular expression commands.",
               4,
               "<command-name> <output-var> <expression> <string> [start] [range]")
    int nFirstArg = bUseServerVariablesForReturn ? 0 : -1;
    nCount += abs(nFirstArg);
    const char *command = args.Arg(1);
    if (FStrEq(command, "match") || FStrEq(command, "search") || FStrEq(command, "matchformat"))
    {
        EsRegex regex;
        if (!regex.compile(args.Arg(nFirstArg+3)))
        {
            DxMsg(0, __FUNCTION__, __LINE__, "Could not compile regular expression.\n");
            return;
        }
        const char *text = args.Arg(nFirstArg+4);
        if (FStrEq(command, "match"))
        {
            RETURNVAL(args.Arg(2), regex.match(text, NULL, 0), "i");
        }
        else if (FStrEq(command, "search"))
        {
            int start = 0, range = 10;
            if (nCount > 6) { start = atoi(args.Arg(nFirstArg+5)); range = atoi(args.Arg(nFirstArg+6)); }
            int result = regex.search(text, start, range);
            if (result == -2) DxMsg(0, __FUNCTION__, __LINE__, "Regular expression search failed or uses an unsupported backreference.\n");
            RETURNVAL(args.Arg(2), result, "i");
        }
        else
        {
            regmatch_t groups[10];
            if (regex.match(text, groups, 10) >= 0)
            {
                const char *format = args.Arg(nFirstArg+5);
                std::string result;
                for (size_t i = 0; format[i]; ++i)
                {
                    if (format[i] == '%' && format[i+1])
                    {
                        char c = format[++i];
                        if (c >= '1' && c <= '9')
                        {
                            int n = c - '0';
                            if (groups[n].rm_so >= 0 && groups[n].rm_eo >= groups[n].rm_so)
                                result.append(text + groups[n].rm_so, groups[n].rm_eo - groups[n].rm_so);
                        }
                        else result += c;
                    }
                    else result += format[i];
                }
                RETURNVAL(args.Arg(2), result.c_str(), "s");
            }
        }
    }
}


ES_CON_COMMAND2(unregsaycmd,
               "Removes a say command that refers to a particular block.",
               1,
               "<command-name>")
    if (!g_SayCmdTable.RemoveElement(args.Arg(1)))
    {
        DxMsg(0, __FUNCTION__, __LINE__, "unregsaycmd: Did not find command: %s\n", args.Arg(1));
    }
}

ES_CON_COMMAND2(unregclientcmd,
               "Removes a client command that refers to a particular block.",
               1,
               "<command-name>")

    if (!g_ClientCmdTable.RemoveElement(args.Arg(1)))
    {
        DxMsg(0, __FUNCTION__, __LINE__, "unregclientcmd: Did not find command: %s\n", args.Arg(1));
    }
}


ES_CON_COMMAND2(regsaycmd,
               "Adds a say command that refers to a particular block.",
               3,
               "<command-name> <block-name> <description>")
   HashData_t *pNew = new HashData_t();
   if (pNew)
   {
		pNew->name = strlwr(strdup(args.Arg(1)));
		pNew->value = strdup(args.Arg(2));
		g_SayCmdTable.InsertElement(pNew);
   }
   else
   {
       DxMsg(0, __FUNCTION__, __LINE__, "es_xregcmd: Out of memory.\n");
       return;
   }
}
ES_CON_COMMAND2(queryregsaycmd,
               "Queries which block a particular say cmd is pointed to.",
               2,
               "<varname> <cmdname>")
   int nFirstArg = bUseServerVariablesForReturn ? 0 : -1;
   const char *varname = args.Arg(nFirstArg+1);
   const char *cmdname = args.Arg(nFirstArg+2);
   const char * blockname = (const char *)g_SayCmdTable.FindElement(cmdname);
   if (NULL == blockname)
   {
       DxMsg(2, __FUNCTION__, __LINE__, "es_xqueryregsaycmd: ERROR. Command %s doesn't exist.\n",cmdname);
       blockname = "";
   }
   RETURNVAL(varname, blockname, "s");
}

ES_CON_COMMAND2(regclientcmd,
               "Adds a client command that refers to a particular block.",
               3,
               "<command-name> <block-name> <description>")
   HashData_t *pNew = new HashData_t;
   if (pNew)
   {
		pNew->name = strlwr(strdup(args.Arg(1)));
	    pNew->value = strdup(args.Arg(2));
		g_ClientCmdTable.InsertElement(pNew);
   }
   else
   {
       DxMsg(0, __FUNCTION__, __LINE__, "es_xregcmd: Out of memory.\n");
       return;
   }
}
ES_CON_COMMAND2(queryregclientcmd,
               "Queries which block a particular client cmd is pointed to.",
               2,
               "<varname> <cmdname>")
   int nFirstArg = bUseServerVariablesForReturn ? 0 : -1;
   const char *varname = args.Arg(nFirstArg+1);
   const char *cmdname = args.Arg(nFirstArg+2);
   const char * blockname = (const char *)g_ClientCmdTable.FindElement(cmdname);
   if (NULL == blockname)
   {
       DxMsg(2, __FUNCTION__, __LINE__, "es_xqueryregclientcmd: ERROR. Command %s doesn't exist.\n",cmdname);
       blockname = "";
   }
   RETURNVAL(varname, blockname, "s");
}
ES_CON_COMMAND2(effect,
                "Performs a particular effect.",
                2,
                "<mode> [options]")
    const char * command = args.Arg(1);
    if (FStrEq(command, "sparks"))
    {
        Vector *porig = AllocVectorFromString(args.Arg(2));
        const Vector origin = *porig;
        delete porig;
        int nMagnitude = atoi(args.Arg(3));
        int nTrailLength = atoi(args.Arg(4));
        const Vector *Direction = NULL;
        if (nCount > 4) 
        {
            Direction = AllocVectorFromString(args.Arg(5));
        }
        effects->Sparks(origin, nMagnitude, nTrailLength, Direction);
        if (Direction) delete Direction;
    }
    else if (FStrEq(command, "smoke"))
    {
        Vector *porig = AllocVectorFromString(args.Arg(2));
        const Vector origin = *porig;
        delete porig;
        int nModel = atoi(args.Arg(3));
        float scale = atof(args.Arg(4));
        float framerate = atof(args.Arg(5));
        effects->Smoke(origin, nModel, scale, framerate);
    }
    else if (FStrEq(command, "beam"))
    {
        Vector *porig = AllocVectorFromString(args.Arg(2));
        const Vector origin = *porig;
        delete porig;
        Vector *pend = AllocVectorFromString(args.Arg(3));
        const Vector end = *pend;
        delete pend;
        if (nCount > 17)
        {
            int nModel = atoi(args.Arg(4));
            int nHaloIndex = atoi(args.Arg(5));
            int nFrameStart = atoi(args.Arg(6));
            int nFrameRate = atoi(args.Arg(7));
            float flLife = atof(args.Arg(8));
            int width = atoi(args.Arg(9));
            int endWidth = atoi(args.Arg(10));
            int fadeLength = atoi(args.Arg(11));
            int noise = atoi(args.Arg(12));
            int red = atoi(args.Arg(13));
            int green = atoi(args.Arg(14));
            int blue = atoi(args.Arg(15));
            int brightness = atoi(args.Arg(16));
            int speed = atoi(args.Arg(17));
            effects->Beam(origin, end, nModel, nHaloIndex, nFrameStart, nFrameRate, flLife, width, endWidth, fadeLength, noise, red, green, blue, brightness, speed);
            DxMsg(1, __FUNCTION__, __LINE__, "Beam sent!\n");
        }
        else
        {
            DxMsg(1, __FUNCTION__, __LINE__, "Incorrect syntax for es_effect beam.\n");
        }
    }
    else if (FStrEq(command, "dust"))
    {
        Vector *porig = AllocVectorFromString(args.Arg(2));
        const Vector origin = *porig;
        delete porig;
        Vector *pend = AllocVectorFromString(args.Arg(3));
        const Vector end = *pend;
        delete pend;
        float size = atof(args.Arg(4));
        float speed = atof(args.Arg(5));
        effects->Dust(origin, end, size, speed);
    }
    else if (FStrEq(command, "energysplash"))
    {
        Vector *porig = AllocVectorFromString(args.Arg(2));
        const Vector origin = *porig;
        delete porig;
        Vector *pend = AllocVectorFromString(args.Arg(3));
        const Vector end = *pend;
        delete pend;
        bool bExplosive = atoi(args.Arg(4)) ? true : false;
        effects->EnergySplash(origin, end, bExplosive);
    }
}
ES_CON_COMMAND2(queryclientvar,
                "Sends a request to query a client's console variable.",
                3,
                "<userid> <variable-name>")
    int nUserid = atoi(args.Arg(1));
    if (nUserid > 0)
    {
        edict_t * pPlayer = g_EventScriptsPlugin.LookupEdictByUserid(nUserid);
        if (pPlayer)
        {
            helpers->StartQueryCvarValue(pPlayer, args.Arg(2));
        }
        else
        {
            DxMsg(0, __FUNCTION__, __LINE__, "Userid does not exist.\n");
            ErrVar("Invalid userid");
        }
    }
    else
    {
        DxMsg(0, __FUNCTION__, __LINE__, "Userid does not exist.\n");
        ErrVar("Invalid userid");
    }
}
ES_CON_COMMAND2(event,
                "Create and fire events to signal to plugins that an event has happened. It must be an event loaded via es_loadevents.",
                3,
                "<command> <event-name> [value-name] [value]")
    const char * command = args.Arg(1);
    const char * cmdeventname = args.Arg(2);
    const char * valuename = nCount>3 ? args.Arg(3) : NULL;
    const char * value = nCount>4 ? args.Arg(4) : NULL;
    char * stopstring = NULL;
    static IGameEvent *pNewEvent = NULL;
    if (pNewEvent && pNewEvent->GetName() && !FStrEq(cmdeventname,  pNewEvent->GetName()))
    {
        DxMsg(0, __FUNCTION__, __LINE__, "WARNING: A script is calling 'es_event %s' for %s when the existing event %s has not been cancelled or fired. Trying to continue anyway...\n", command, cmdeventname,  pNewEvent->GetName());
    }
    if (FStrEq(command, "initialize"))
    {
        pNewEvent = gameeventmanager->CreateEvent(cmdeventname, true);
        if (!pNewEvent)
        {
            DxMsg(0, __FUNCTION__, __LINE__, "%s: Event %s not registered or no listeners.\n", args.Arg(0), cmdeventname);
            ErrVar("Event not registered or no listeners.");
        }
        else
        {
            DxMsg(1, __FUNCTION__, __LINE__, "Event %s created successfully.\n", pNewEvent->GetName());
        }
    }
    else if (FStrEq(command, "cancel"))
    {
        if (pNewEvent)
        {
            gameeventmanager->FreeEvent(pNewEvent);
            pNewEvent = NULL;
        }
    }
    else if (FStrEq(command, "fire"))
    {
        if (pNewEvent)
        {
            if (gameeventmanager->FireEvent(pNewEvent))
            {
                DxMsg(1, __FUNCTION__, __LINE__, "Event %s fired successfully.\n", cmdeventname);
            }
            else
            {
                DxMsg(1, __FUNCTION__, __LINE__, "Event %s NOT fired successfully.\n", cmdeventname);
            }
            pNewEvent = NULL;
        }
        else
        {
            DxMsg(0, __FUNCTION__, __LINE__, "ERROR: Event lost.\n");
        }
    }
    else if (nCount > 4 && pNewEvent && FStrEq(command, "setint"))
    {
        pNewEvent->SetInt(valuename, strtol(value, &stopstring, 10));
    }
    else if (nCount > 4 && pNewEvent && FStrEq(command, "setfloat"))
    {
        pNewEvent->SetFloat(valuename, strtod(value, &stopstring));
    }
    else if (nCount > 4 && pNewEvent && FStrEq(command, "setstring"))
    {
        pNewEvent->SetString(valuename, value);
    }
    else
    {
        DxMsg(0, __FUNCTION__, __LINE__, "No valid command situation for es_event! %d, 0x%x\n", nCount, pNewEvent);
    }
}

CON_COMMAND( es_xnq, "Remove quotation marks.\n   usage: es_xnq <command string>\nThis will execute <command string> after removing quotation marks. WARNING: Removes all quotation marks.\n" )
{
    if(ARGC() < 2)
    {
        DxMsg(0, __FUNCTION__, __LINE__,"es_xnq: Please provide a command string.\n");
        ErrVar("No command string provided.");
        return;
    }
    char szUnquotedCommand[MAX_COMMAND_STRING];
    char szPreparseCommand[MAX_COMMAND_STRING];
    strcpy(szPreparseCommand, ARGS());
    strcat(szPreparseCommand, "\n");
    if (!StripAllQuotes(szUnquotedCommand, szPreparseCommand))
    {
            strcpy(szUnquotedCommand, szPreparseCommand);
    }
    ImmediateExecuteCommand(szUnquotedCommand);
    return;
}

CON_COMMAND( esnq, "Mattie's EventScripts variable parsing, without quotation marks.\n   usage: esnq <command string>\nThis will execute <command string> after parsing it for items like event_var() and server_var() and replacing them with the appropriate value. WARNING: Removes all quotation marks.\n" )
{
    if(ARGC() < 2)
    {
        DxMsg(0, __FUNCTION__, __LINE__,"esnq: Please provide a command string.\n");
        ErrVar("No command string provided.");
        return;
    }
    char szExpandCommand[MAX_COMMAND_STRING];
    char szUnquotedCommand[MAX_COMMAND_STRING];
    char szPreparseCommand[MAX_COMMAND_STRING];
    strcpy(szPreparseCommand, ARGS());
    strcat(szPreparseCommand, "\n");
    szExpandCommand[0]='\0';
    if (g_nDebugLevel>4) {
        for (int i = 0;i<ARGC();i++)
        {
            DxMsg(5, __FUNCTION__, __LINE__,"   Arg%d: %s\n", i, ARGV(i));
        }
    }
#ifndef ORANGEBOX
	PARSE_ARGS_FROM_ENGINE();
#endif
    if (ParseCommand(args, szExpandCommand, 1, ARGC())>0)
    {
        DxMsg(2, __FUNCTION__, __LINE__,"esnq executing: %s", szExpandCommand);
        if (strstr(szExpandCommand, ";") == 0) 
        {
            StripAllQuotes(szUnquotedCommand, szExpandCommand);
        }
        else
        {
            DxMsg(0, __FUNCTION__, __LINE__,"WARNING: esnq: Quotes not stripped because command-line contained a semicolon.\n");
            strcpy(szUnquotedCommand, szExpandCommand);
        }
        ImmediateExecuteCommand(szUnquotedCommand);
        return;
    }
    else
    {
        DxMsg(0, __FUNCTION__, __LINE__,"esnq: Error parsing command string: %s\n", ARGS());
        ErrVar("Error while parsing command string.");
        return;
    }
   
}

// RETURNVAL_UNSUPPORTED
CON_COMMAND(stack, "Allows you to do various stack operations with a named stack. Supports create, delete, pop, push, getlength.\n")
{
    const char *cmd = ARGV(1);
    CUtlStack<const char *> * pTheStack = NULL;
    if (FStrEq(cmd, "create"))
    {
        const char *name = ARGV(2);
        void *p = g_StackTable.FindElement(name);
        if (p)
        {
            ErrMsg(0, "The stack %s already exists.\n", name);
            return;
        }
        else
        {
            HashData_t *pNew = new HashData_t;
            pNew->name = strdup(name);
            pNew->value = new CUtlStack<const char *>();
            g_StackTable.InsertElement(pNew);
        }
    }
    else if (FStrEq(cmd, "delete"))
    {
        const char *name = ARGV(2);
		if (name) {
			g_StackTable.RemoveElement(name);
		}
    }
    else if (FStrEq(cmd, "save"))
    {
        const char *name = ARGV(2);
        const char *var = ARGV(3);
        ConVar *pvar = GetVar(var, false);
        if (pvar)
        {
            if (name)
            {
                pTheStack = (CUtlStack<const char *> *)g_StackTable.FindElement(name);
                if (!pTheStack)
                {
                    ErrMsg(0, "The stack %s couldn't be found.\n", name);
                }
                else
                {
                    const char *value = pvar->GetString();
                    pTheStack->Push(strdup(value));
                }
            }
        }
        else
        {
            ErrMsg(0,"The variable %s does not exist.\n", var);
        }

    }
    else if (FStrEq(cmd, "restore"))
    {
        const char *name = ARGV(2);
        const char *var = ARGV(3);
        if (name)
        {
            pTheStack = (CUtlStack<const char *> *)g_StackTable.FindElement(name);
            if (!pTheStack)
            {
                ErrMsg(0, "The stack %s couldn't be found.\n", name);
            }
            else
            {
                if (pTheStack->Count() > 0)
                {
                    const char *value = NULL;
                    pTheStack->Pop(value);
                    if (value)
                    {
                        ConVar * cvar = NULL;
                        cvar = SetConVar( var, value);
                        free((void *)value);
                        if (!cvar){
                            ErrMsg(0,"The var \"%s\" could not be set\n", ARGV(2)); 
                            return; // If it doesn t find the var
                        }

                    }
                }
                else
                {
                    ErrMsg(0, "stack: ERROR: Pop attempt when stack empty: %s\n", name);
                }
            }
        }
    }
    else if (FStrEq(cmd, "pop"))
    {
        const char *name = ARGV(3);
        if (name)
        {
            pTheStack = (CUtlStack<const char *> *)g_StackTable.FindElement(name);
            if (!pTheStack)
            {
                DxMsg(0, __FUNCTION__, __LINE__, "The stack %s couldn't be found.\n", name);
            }
            else
            {
                if (pTheStack->Count() > 0)
                {
                    const char *value = NULL;
                    pTheStack->Pop(value);
                    if (value)
                    {
                        ConVar * cvar = NULL;
                        cvar = SetConVar( ARGV(2), value);
                        free((void *)value);
                        if (!cvar){
                            DxMsg(0, __FUNCTION__, __LINE__,"The var \"%s\" could not be set\n", ARGV(2)); 
                            return; // If it doesn t find the var
                        }

                    }
                }
                else
                {
                    DxMsg(0, __FUNCTION__, __LINE__, "stack: ERROR: Pop attempt when stack empty.\n");
                }
            }
        }
    }
    else if (FStrEq(cmd, "push"))
    {
        const char *name = ARGV(2);
        if (name)
        {
            pTheStack = (CUtlStack<const char *> *)g_StackTable.FindElement(name);
            if (!pTheStack)
            {
                DxMsg(0, __FUNCTION__, __LINE__, "The stack %s couldn't be found.\n", name);
            }
            else
            {
                const char *value = ARGV(3);
                pTheStack->Push(strdup(value));
            }
        }
    }
    else if (FStrEq(cmd, "getlength"))
    {
       const char *name = ARGV(3);
        if (name)
        {
            pTheStack = (CUtlStack<const char *> *)g_StackTable.FindElement(name);
            if (!pTheStack)
            {
                DxMsg(0, __FUNCTION__, __LINE__, "The stack %s couldn't be found.\n", name);
            }
            else
            {
                ConVar * cvar = NULL;
                cvar = SetConVar( ARGV(2), pTheStack->Count());
                if (!cvar){
                    DxMsg(0, __FUNCTION__, __LINE__,"The var \"%s\" could not be set\n", ARGV(2)); 
                    return; // If it doesn t find the var
                }
            }
        }
   }
}

ES_CON_COMMAND2(centermsg,
                "Broadcasts a centered HUD message to all players.",
                2,
                "<string>")
    UTIL_ClientPrintAll(HUD_PRINTCENTER, args.ArgS());
}

ES_CON_COMMAND2(centertell,
                "Sends a centered HUD message to all players.",
                3,
                "<userid> <string>")
    char szMessage[MAX_COMMAND_STRING];
    char * szTemp;

    StripAllQuotes(szMessage, args.ArgS()); 
    if (!strchr(szMessage, '\n'))
    {
        strcat(szMessage, "\n");
    }
    szTemp = strchr(szMessage, ' ') + 1;
    UTIL_ClientPrint(atoi(args.Arg(1)), HUD_PRINTCENTER, szTemp);
}

// es_function myfunc events
// es_getarg myvar myfunc 1
// es_getarg myvar myfunc 2
// es_getarg myvar myfunc 3
// es_getarg myvar myfunc 4



inline bool AnyPythonRegistered(const char *szEventName)
{
#ifdef PYTHON_SUPPORT
    if (g_nPythonEnabled)
    {
        DxMsg(5, __FUNCTION__, __LINE__, "AnyPythonRegistered: %s\n", szEventName);
        // for some random reason, Python isn't initialized in very early situations when we fire events, INIT it.
        InitPython();
        PyObject *pEventName = PyString_FromString(szEventName);
        PyObject *pbReturn = PyObject_CallFunctionObjArgs(g_pEsAddonsHasEvent, pEventName, NULL);
        bool bRet = true;
        if (PyErr_Occurred())
        {
            PyErr_Print();
            bRet = false;
        }
        else
        {
            bRet = !!PyInt_AsLong(pbReturn);
        }
        Py_XDECREF(pEventName);
        Py_XDECREF(pbReturn);
        return bRet;
    }
#endif
    return false;
}
inline void BeginAllPythonEvents(const char *szEventName)
{
#ifdef PYTHON_SUPPORT
    DxMsg(5, __FUNCTION__, __LINE__, "BeginAllPythonEvents: %s\n", szEventName);
    if (AnyPythonRegistered(szEventName))
    {
        char szCommand[MAX_COMMAND_STRING];
        sprintf(szCommand, "es__pyevent %s\n", szEventName);
        ServerCommand(szCommand);
    }
    DxMsg(5, __FUNCTION__, __LINE__, "BeginAllPythonEvents: %s (END)\n", szEventName);

#endif
}

bool LoadPythonAddon(const char *szAddonName)
{
#ifdef PYTHON_SUPPORT
    PyObject *pAddonName = PyString_FromString(szAddonName);
    PyObject *pbReturn = PyObject_CallFunctionObjArgs(g_pEsAddonsLoadModuleAddon, pAddonName, NULL);
    bool bRet = true;
    if (PyErr_Occurred())
    {
        PyErr_Print();
        bRet = false;
    }
    else
    {
        bRet = !!PyInt_AsLong(pbReturn);
    }
    Py_XDECREF(pAddonName);
    Py_XDECREF(pbReturn);
    return bRet;
#endif
}

void RemovePythonAddon(const char *szAddonName)
{
#ifdef PYTHON_SUPPORT
    char szCommand[MAX_COMMAND_STRING];
    sprintf(szCommand, "es.unloadModuleAddon('%s')",szAddonName);
    PyRun_SimpleString(szCommand);
#endif
}

#ifdef PYTHON_SUPPORT

static PyObject*
espy_setnumregistered(PyObject *self, PyObject *args)
{
    DxMsg(4, __FUNCTION__, __LINE__,"setnumregistered, %d\n",g_NumTickListenersRegistered);
    if(!PyArg_ParseTuple(args, "i", &g_NumTickListenersRegistered))
        return NULL;
    DxMsg(4, __FUNCTION__, __LINE__,"setnumregistered:done %d\n",g_NumTickListenersRegistered);
    Py_INCREF(g_pPyNone);
    return g_pPyNone;
}


static PyObject*
espy_getuseridlist(PyObject *self, PyObject *args)
{
    PyObject * plist = PyList_New(0);
    for(int i=1;i<=maxplayers;i++) 
    {
        edict_t *pEntity =  PEntityOfEntIndex(i);
        if(!pEntity || pEntity->IsFree()) 
        {
                continue;
        }
        if(playerinfomanager)
        {
	        IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEntity );
            if (playerinfo && playerinfo->IsPlayer())
            {
                PyObject *pPlayer = Py_BuildValue("i", engine->GetPlayerUserId(pEntity));
                PyList_Append(plist,pPlayer);
                Py_XDECREF(pPlayer);
            }
        }
    }

    return plist;
}
static PyObject*
espy_cmdargv(PyObject *self, PyObject *args)
{
    int nArgNum = 0;
    if(!PyArg_ParseTuple(args, "i", &nArgNum))
        return NULL;
#ifdef ORANGEBOX
	if (g_pCmdArg)
	{
	    return Py_BuildValue("s", g_pCmdArg->Arg(nArgNum));
	}
	else
	{
		return g_pPyNone;
	}
#else
    return Py_BuildValue("s", engine->Cmd_Argv(nArgNum));
#endif
}

static PyObject*
espy_cmdargs(PyObject *self, PyObject *args)
{
#ifdef ORANGEBOX
	if (g_pCmdArg)
	{
	    return Py_BuildValue("s", g_pCmdArg->ArgS());
	}
	else
	{
		return g_pPyNone;
	}
#else
	return Py_BuildValue("s", engine->Cmd_Args());
#endif

}

static PyObject*
espy_cmdargc(PyObject *self, PyObject *args)
{
#ifdef ORANGEBOX
	if (g_pCmdArg)
	{
	    return Py_BuildValue("i", g_pCmdArg->ArgC());
	}
	else
	{
		return g_pPyNone;
	}
#else
	return Py_BuildValue("i", engine->Cmd_Argc());
#endif
}

static PyObject*
espy_getcmduserid(PyObject *self, PyObject *args)
{
    return Py_BuildValue("i", g_nCmdUserid);
}

static PyObject*
espy_cmd(PyObject *self, PyObject *args)
{
    char * szString;
    if(!PyArg_ParseTuple(args, "s", &szString))
    {
        return NULL;
    }
#ifdef ORANGEBOX
    if (managed_servercmd_cvar.GetBool())
    {
        if (EsCommandNeedsEngineBuffer(szString))
        {
            std::string commandBuffer(szString);
            // Source rejects inserted buffers without a terminating newline.
            if (commandBuffer[commandBuffer.size() - 1] != '\n')
                commandBuffer += '\n';
            engine->InsertServerCommand(commandBuffer.c_str());
        }
        else if (g_nManagedServerCmdDepth >= 8)
        {
            bool bPreviousManagedExecution = g_bManagedServerCmdExecution;
            Msg("[ESMANAGED] depth limit reached; using legacy deferred block execution for: %s\n", szString);
            g_bManagedServerCmdExecution = false;
            ExecuteCommandNow(szString);
            g_bManagedServerCmdExecution = bPreviousManagedExecution;
        }
        else
        {
            bool bPreviousManagedExecution = g_bManagedServerCmdExecution;
            g_nManagedServerCmdDepth++;
            g_bManagedServerCmdExecution = true;
            Msg("[ESMANAGED] enter depth=%d command=%s\n", g_nManagedServerCmdDepth, szString);
            ExecuteCommandNow(szString);
            Msg("[ESMANAGED] leave depth=%d command=%s\n", g_nManagedServerCmdDepth, szString);
            g_bManagedServerCmdExecution = bPreviousManagedExecution;
            g_nManagedServerCmdDepth--;
        }
    }
    else
    {
        ExecuteCommandNow(szString);
    }
#else
    char szCommand[MAX_COMMAND_STRING];
    sprintf(szCommand, "%s;wait;", szString);
    engine->InsertServerCommand(szCommand);
    engine->ServerExecute();
#endif
    return Py_BuildValue("i", 1);
}

static PyObject*
espy_insertcmd(PyObject *self, PyObject *args)
{
    char * szString;
    if(!PyArg_ParseTuple(args, "s", &szString))
    {
        return NULL;
    }
    char szCommand[MAX_COMMAND_STRING];
    sprintf(szCommand, "%s;", szString);
    engine->InsertServerCommand(szCommand);
    return Py_BuildValue("i", 1);
}

static PyObject*
espy_scmd(PyObject *self, PyObject *args)
{
    char * szString;
    if(!PyArg_ParseTuple(args, "s", &szString))
        return NULL;
    char szCommand[MAX_COMMAND_STRING];
    sprintf(szCommand, "%s\n", szString);
    ServerCommand(szCommand);
    return Py_BuildValue("i", 1);
}

static PyObject*
espy_print(PyObject *self, PyObject *args)
{
    char * szString;
    if(!PyArg_ParseTuple(args, "s", &szString))
        return NULL;
    Msg("%s", szString);
    return Py_BuildValue("i", 1);
}


static PyObject*
espy_dbgmsg(PyObject *self, PyObject *args)
{
    char * szString;
    int nLevel = 0;
    PyObject *pObj = NULL;
    // get the debug level and the 2nd param as a raw obj
    if(!PyArg_ParseTuple(args, "iO", &nLevel, &pObj))
        return NULL;
    // since there is more work ahead, let's do this check for speed
    // purposes
    if (g_nDebugLevel>=nLevel)
    {
        // let's see if that object can be coerced to a string
        PyObject *pPyString = PyObject_Str(pObj);
        if (pPyString)
        {
            int nSize = 0;
            // get the C form and then send that onto Valve
            PyString_AsStringAndSize(pPyString, &szString, &nSize);
            if(szString)
            {
                // for messages longer than 1000 bytes, send chunks again and again until done
                char *pStart = szString;
                while(nSize > 0){
                    tchar szCommand[1024];
                    // only append a \n if it's the last message waiting
                    sprintf(szCommand, "%0.1000s%s", pStart, nSize <= 1000 ? "\n" : "");
                    DebugMessageOut(szCommand);
                    // move to the next 1000 sized chunk
                    nSize = nSize - 1000;
                    if (nSize > 0) pStart = pStart + 1000;
                }
                return Py_BuildValue("i", 1);
            }
            DxMsg(1, __FILE__, __LINE__, "Can't convert dbgmsg parameter to a string.\n");
        }
        DxMsg(1, __FILE__, __LINE__, "Can't convert dbgmsg parameter.\n");
        return NULL;
    }
    return Py_BuildValue("i", 1);
}





static PyObject*
espy_geteventinfo(PyObject *self, PyObject *args)
{
    char * szVar;
    if(!PyArg_ParseTuple(args, "s", &szVar))
        return NULL;
    if (!pEventScriptsKey) return NULL;
    const char * pString = pEventScriptsKey->GetString(szVar);
    if (pString)
        return Py_BuildValue("s", pString);
    else
        return NULL;
}

PY_COMMAND_VARARGS(getCurrentEventVarString, "Returns the value of a named event variable in string form.")
{
    return espy_geteventinfo(self, args);
}

PY_COMMAND_VARARGS(getCurrentEventVarInt, "Returns the value of a named event variable in integer form.")
{
    char * szVar;
    if(!PyArg_ParseTuple(args, "s", &szVar))
        return NULL;
    if (!pEventScriptsKey) return NULL;
    int nVal = pEventScriptsKey->GetInt(szVar);
    return Py_BuildValue("i", nVal);
}
PY_COMMAND_VARARGS(getCurrentEventVarFloat, "Returns the value of a named event variable in integer form.")
{
    char * szVar;
    if(!PyArg_ParseTuple(args, "s", &szVar))
        return NULL;
    if (!pEventScriptsKey) return NULL;
    float fVal = pEventScriptsKey->GetFloat(szVar);
    return Py_BuildValue("f", fVal);
}


PY_COMMAND_VARARGS(getCurrentEventVarIsEmpty, "Returns 1 if the named event variable doesn't exist.")
{
    char * szVar;
    if(!PyArg_ParseTuple(args, "s", &szVar))
        return NULL;
    if (!pEventScriptsKey) return NULL;
    bool bVal = pEventScriptsKey->IsEmpty(szVar);
    return Py_BuildValue("i", bVal ? 1 : 0);
}


static PyObject*
espy_showmenu(PyObject *self, PyObject *args)
{
    char * szMenuText = NULL;
    int nDuration = 0;
    long nUserid = 0;
    char * szOptions = NULL;
    PyObject *pObj = NULL;
    if(!PyArg_ParseTuple(args, "iOs|s", &nDuration, &pObj, &szMenuText, &szOptions))
    {
        return NULL;
    }
    // Regression fix: We have to coerce the userid to int for ticket #38
    PyObject *pInt = PyNumber_Int(pObj);
    if (pInt)
    {
        nUserid = PyInt_AsLong(pInt);
    }
    if (pInt && nUserid > 0)
    {
        DxMsg(4, MSGLOC, "showmenu: %d, %d\n", nDuration, nUserid); 
        UTIL_SendMenu(nUserid, nDuration, szMenuText, szOptions);
    }
    else
    {
        DxMsg(1, MSGLOC, "showmenu: Userid couldn't be forced to a valid userid\n"); 
    }
    return g_pPyNone;
}

PY_COMMAND_VARARGS(getEntityIndexes, "Returns list of all entity indexes on the server, optionally filtered by a classname")
{
    char * szClassname = NULL;
    PyArg_ParseTuple(args, "|s", &szClassname);

    PyObject *pointer = PyList_New(0); 
    int count = engine->GetEntityCount();
    for (int i = 0;i<count;i++)
    {
        edict_t *p = PEntityOfEntIndex(i);
        if (p)
        {
            const char* classname = p->GetClassName();
            if (classname && classname[0] != '\0')
            {
                if (szClassname == NULL || FStruEq(szClassname, classname))
                {
                    PyObject *pVal = PyInt_FromLong(i);
                    PyList_Append(pointer, pVal);
                    Py_XDECREF(pVal);
                }
            }
        }
    }
    return pointer;
}

CON_COMMAND( es__pyevent, "Executes a python line of code." )
{
    if (g_nPythonEnabled)
    {
		DxMsg(4, __FUNCTION__, __LINE__,"Calling the Python event function: %s\n", ARGS());

        PyObject *pEventName = PyString_FromString(ARGS());
        PyObject *pbReturn = PyObject_CallFunctionObjArgs(g_pEsAddonsTriggerEvent, pEventName, NULL);
        if (PyErr_Occurred())
        {
            PyErr_Print();
        }
        Py_XDECREF(pEventName);
        Py_XDECREF(pbReturn);
		DxMsg(4, __FUNCTION__, __LINE__,"Calling the Python event function: %s (done)\n", ARGS());
    }
}


void pygo(DISPATCH_PARAM)
{
    if (g_nPythonEnabled)
    {
        PyRun_SimpleString(ARGS());
    }
}

CON_COMMAND(pycmd_register, "Registers a Python interpreter access command. Please use a name no hacker would guess.")
{
    static bool s_bRegistered = false;
    static ConCommand * pSystem = NULL;
    DxMsg(2, __FUNCTION__, __LINE__, "pycmd_register: starting...\n");
    if (ARGC() == 2)
    {
        DxMsg(2, __FUNCTION__, __LINE__, "pycmd_register: proper arguments found.\n");
        if (!s_bRegistered && !g_bFirstLevelLoaded)
        {
            DxMsg(2, __FUNCTION__, __LINE__, "pycmd_register: duplicating command name string\n");
            char *pTemp = strdup(ARGV(1));
            DxMsg(2, __FUNCTION__, __LINE__, "system_register: creating console command\n");
            pSystem = new ConCommand(pTemp, pygo, "" );
            s_bRegistered = true;
            DxMsg(0, __FUNCTION__, __LINE__, "Python command registered.\n");
        }
        else
        {
            DxMsg(0, __FUNCTION__, __LINE__,"Sorry! You can only register the Python console command before a server loads (e.g. in autoexec.cfg).\n");
        }
    }
    else
    {
        DxMsg(0, __FUNCTION__, __LINE__, "EventScripts\n\nsyntax:\npycmd_register <obscure-name-for-python-command>\n");
    }

}



// Internal Python commands
static PyCommand espy_insertcmd_register("InsertServerCommand", espy_insertcmd, METH_VARARGS,
                                   "Inserts a command to the server queue at the beginning.");
static PyCommand espy_cmd_register("ForceServerCommand", espy_cmd, METH_VARARGS,
                                   "Inserts a command to the server queue at the beginning and forces execution.");
static PyCommand espy_scmd_register("ServerCommand", espy_scmd, METH_VARARGS,
                                    "Adds a command to the end of the server queue.");
static PyCommand espy_dbgmsg_register("dbgmsg", espy_dbgmsg, METH_VARARGS,
                                    "Outputs a message to the console.");
static PyCommand espy_print_register("printmsg", espy_print, METH_VARARGS,
                                    "Outputs a message to the console.");
static PyCommand espy_setnumregistered_register("setNumRegistered", espy_setnumregistered, METH_VARARGS,
                                    "Internal command for setting number of ticklisteners registered.");
static PyCommand espy_getuseridlist_register("getUseridList", espy_getuseridlist, METH_VARARGS,
                                    "Returns a list of the userids of all players on the server.");
static PyCommand espy_geteventinfo_register("getEventInfo", espy_geteventinfo, METH_VARARGS,
                                    "Gets the value of a particular event variable.");
static PyCommand espy_cmdargv_register("cmdargv", espy_cmdargv, METH_VARARGS,
                                   "Gets the command parameter passed to the current Valve console command.");
static PyCommand espy_cmdargc_register("cmdargc", espy_cmdargc, METH_VARARGS,
                                   "Gets the command parameter count passed to the current Valve console command.");
static PyCommand espy_cmdargs_register("cmdargs", espy_cmdargs, METH_VARARGS,
                                   "Gets the commandstring passed to the current Valve console command.");
static PyCommand espy_getcmduserid_register("getcmduserid", espy_getcmduserid, METH_VARARGS,
                                   "Gets the commandstring passed to the current Valve console command.");
static PyCommand espy_showmenu_register("showMenu", espy_showmenu, METH_VARARGS, 
                                    "Sends an AMX-Style menu to the users");

PyObject * g_pEsModule = NULL;
PyObject * g_pEsAddons = NULL;
void InitPython()
{
    if (g_nPythonEnabled && !g_bPythonTried)
    {
        g_bPythonTried = true;
        DxMsg(1, __FUNCTION__, __LINE__, "Initializing Python...\n");

        char szPythonDir[MAX_COMMAND_STRING];
        char szAppendAddonDir[MAX_COMMAND_STRING];
        char szLibDir[MAX_COMMAND_STRING];

#ifdef WIN32
        sprintf(szPythonDir, "%s\\_engines\\python", addondir_cvar.GetString());

        // We need to preload two DLLs on Windows just to be sure we have them properly for delayload.
        char szAddonDir[MAX_COMMAND_STRING];
        sprintf(szAddonDir, "%s\\Lib\\plat-win\\msvcr71.dll", szPythonDir);
        HANDLE hd = LoadLibrary(szAddonDir);
        sprintf(szAddonDir, "%s\\Lib\\plat-win\\python25.dll", szPythonDir);
        HANDLE hd2 = LoadLibrary(szAddonDir);
        
        // Let's be sure our library directory is on the sys.path.
        sprintf(szLibDir, "sys.path.insert(1,r'%s\\_libs\\python')", addondir_cvar.GetString());
#else
        // NOT WIN32
        sprintf(szPythonDir, "%s/_engines/python", addondir_cvar.GetString());
        // let's preload libpython just in case such that its symbols are global for 
        //   modules to access.
	    void * handle = 0;
	    handle = dlopen("libpython2.5.so.1.0", RTLD_GLOBAL | RTLD_NOW);
	    DxMsg(1, __FUNCTION__, __LINE__, "Handle: %d\n%s\n", handle, dlerror());
        sprintf(szLibDir, "sys.path.insert(1,r'%s/_libs/python')", addondir_cvar.GetString());
#endif
        Py_SetPythonHome(szPythonDir);
   	    /* Initialize the Python interpreter.  Required. */
        Py_SetProgramName(szPythonDir);

        Py_Initialize();
		// -tt enforces tab stuff
		// -v tells the interp to be verbose
        char *no_args[] = {"-E", "-i", "-tt", "-v", 0}; // update next line, too, if you change count
        PySys_SetArgv(4, no_args);
        PyObject * pmain = PyImport_AddModule("__builtins__");
        g_pPyNone = PyRun_String("None", Py_single_input, pmain, NULL);
        Py_InitModule("es_C", g_pEsMethods);

        // Let's do some hacky replacement for the system path.
        PyRun_SimpleString("import sys");
        sprintf(szAppendAddonDir, "eventscripts_addondir = r\"%s\"", addondir_cvar.GetString());
        PyRun_SimpleString(szAppendAddonDir);
        sprintf(szAppendAddonDir, "sys.path.append(eventscripts_addondir)");
        PyRun_SimpleString(szAppendAddonDir);
        PyRun_SimpleString(szLibDir);

        // HACK moved from es.py.
        PyRun_SimpleString("import es_C");
        PyRun_SimpleString("sys.path.append('%s/addons/eventscripts/_engines/python/Lib' % es_C.getString('eventscripts_gamedir'))");
        PyRun_SimpleString("sys.path.append('%s/addons/eventscripts/_engines/python/Lib.zip' % es_C.getString('eventscripts_gamedir'))");
#ifdef WIN32
        PyRun_SimpleString("sys.path.append('%s/addons/eventscripts/_engines/python/Lib/plat-win' % es_C.getString('eventscripts_gamedir'))");
#else
        PyRun_SimpleString("sys.path.append('%s/addons/eventscripts/_engines/python/Lib/plat-linux2' % es_C.getString('eventscripts_gamedir'))");
#endif
        PyRun_SimpleString("sys.path.append('%s/addons/eventscripts/_engines/python/Lib/site-packages' % es_C.getString('eventscripts_gamedir'))");
        PyRun_SimpleString("sys.path.append('%s/addons/eventscripts/_libs/python' % es_C.getString('eventscripts_gamedir'))");

        PyRun_SimpleString("import es");
        PyRun_SimpleString("from es import addons");
		if (g_nDebugLevel>0) {
    		PyRun_SimpleString("print 'Python sys.path=',sys.path");
		}

        // get some handy globals for fast Python access
        g_pEsModule = PyImport_ImportModule("es");
        g_pEsAddons = PyObject_GetAttrString(g_pEsModule, "addons");
        g_pEsAddonsLoadModuleAddon = PyObject_GetAttrString(g_pEsModule, "loadModuleAddon");
        g_pEsAddonsTriggerEvent = PyObject_GetAttrString(g_pEsAddons, "triggerEvent");
        g_pEsAddonsCallBlock = PyObject_GetAttrString(g_pEsAddons, "callBlock");
        g_pEsAddonsTick = PyObject_GetAttrString(g_pEsAddons, "tick");
        g_pEsAddonsHasEvent = PyObject_GetAttrString(g_pEsAddons, "hasEvent");
        g_pEsAddonsSayWrap = PyObject_GetAttrString(g_pEsAddons, "sayFilter");
        g_pEsAddonsClientCommand = PyObject_GetAttrString(g_pEsAddons, "clientCommand");

        DxMsg(1, __FUNCTION__, __LINE__, "Python initialized.\n");
#ifdef WIN32
        CloseHandle(hd);
#endif
    }

}

#endif


