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

#ifndef ES_PLUGIN_INTERNAL_H
#define ES_PLUGIN_INTERNAL_H

#ifdef _WIN32
#pragma once
#endif

#include "es_util.h"
#include "engine/iserverplugin.h"
#ifndef ORANGEBOX
#include "dlls/iplayerinfo.h"
#else
#include "game/server/iplayerinfo.h"
#endif
#include "server_class.h"
#include "es_debug.h"
#include "es_playerutil.h"

class CCmdWrap;
class CGiveWrap;

//---------------------------------------------------------------------------------
// Purpose: The EventScripts plugin class
//---------------------------------------------------------------------------------
class CEventScriptsPlugin: public IServerPluginCallbacks, public IGameEventListener2
{
public:
	CEventScriptsPlugin();
	~CEventScriptsPlugin();

    // IServerPluginCallbacks methods
	virtual bool			Load(	CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory );
	virtual void			Unload( void );
	virtual void			Pause( void );
	virtual void			UnPause( void );
	virtual const char     *GetPluginDescription( void );      
	virtual void			LevelInit( char const *pMapName );
	virtual void			ServerActivate( edict_t *pEdictList, int edictCount, int clientMax );
	virtual void			GameFrame( bool simulating );
	virtual void			LevelShutdown( void );
	virtual void			ClientActive( edict_t *pEntity );
	virtual void			ClientDisconnect( edict_t *pEntity );
	virtual void			ClientPutInServer( edict_t *pEntity, char const *playername );
	virtual void			SetCommandClient( int index );
	virtual void			ClientSettingsChanged( edict_t *pEdict );
	virtual PLUGIN_RESULT	ClientConnect( bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen );
#ifdef ALIENSWARM
    	// Client is fully connected ( has received initial baseline of entities )
    virtual void			ClientFullyConnect( edict_t *pEntity ){};
	// added with version 3 of the interface.
    virtual void			OnEdictAllocated( edict_t *edict ){
        DxMsg(8, __FILE__,__LINE__,"edict created %s\n", edict->GetClassName());}
	virtual void			OnEdictFreed( const edict_t *edict  ) {
        DxMsg(8, __FILE__,__LINE__,"edict freed %s\n", edict->GetClassName());}	
#endif
#ifdef LEFT4DEAD
    // other values beyond 0 were giving:
    // GameEventListener2 callback in list that should NOT be - pounce_end!
    int GetEventDebugID(void) { return EVENT_DEBUG_ID_INIT; }
#endif
#ifdef ORANGEBOX
	virtual PLUGIN_RESULT	ClientCommand( edict_t *pEntity, const CCommand &args);
#else
	virtual PLUGIN_RESULT	ClientCommand( edict_t *pEntity);
#endif
	virtual PLUGIN_RESULT	NetworkIDValidated( const char *pszUserName, const char *pszNetworkID );
    virtual void OnQueryCvarValueFinished( QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue );

	// IGameEventListener Interface
	//virtual void FireGameEvent( KeyValues * event );
    virtual void FireGameEvent(IGameEvent *event);
    
	virtual int GetCommandIndex() { return m_iClientCommandIndex; }

    int FindUserIDByString(const char *target_string);
    void PrintAllRegistered(int nUserid = 0);
    void DumpUserMessages();
    const char * GetGameDescription();
    int CEventScriptsPlugin::LookupIndexByUserID(const int userid)
    {
        return ::LookupIndexByUserID(userid);
    }
    void DefaultEventRegistration();

    edict_t* CEventScriptsPlugin::LookupEdictByUserid(const int userid)
    {
        return ::LookupEdictByUserid(userid);
    }

    unsigned int CEventScriptsPlugin::LookupIndexByHandle(int nHandleInIntForm);

    unsigned int CEventScriptsPlugin::LookupHandleForEntity(edict_t * pEntity);

	unsigned int CEventScriptsPlugin::LookupHandleByUserid(int nUserid)
    {
        edict_t * pPlayer = LookupEdictByUserid(nUserid);
		return LookupHandleForEntity(pPlayer);
    }

    unsigned int CEventScriptsPlugin::LookupHandleByIndex(int nIndex)
    {
		DxMsg(1, __FUNCTION__, __LINE__, "LookupHandleByIndex(%d) called.\n", nIndex);
        edict_t * pEntity = PEntityOfEntIndex(nIndex);
		return LookupHandleForEntity(pEntity);
    }

    int CEventScriptsPlugin::LookupUseridByHandle(unsigned int handle);

    void CEventScriptsPlugin::GetPlayerConnectionInfo( int userid, int& ping, int &packetloss, float &fConnectedTime, char * szAddress);
#ifdef ORANGEBOX
	void * CEventScriptsPlugin::GetEntityByIndex(int index);
#endif
    BOOL RegisterForEventFile(const char* szFileName);

    int CEventScriptsPlugin::GetPlayerCount(const int team, bool bOnlyAlive = false);


    CBotCmd *CEventScriptsPlugin::GetPlayerBotCmd(const int userid);

    void CEventScriptsPlugin::GetPlayerLocation(const int userid, float *pf);

    int CEventScriptsPlugin::EdictIsFakeClient( edict_t *pEntity );

void SplitPath(const char *szFullPath, char * ClassName, char *szTempPath);

bool GetOffsetInTable(SendTable * pTable, char *szRemainPath, int *nOffset, SendPropType *pType);

bool GetOffset(const char *szFullPath, int *nOffset, SendPropType *pType);

    void DUMP_ServerClasses();

	// Thanks to coldfyr for recommending this method for tracking down properties
    template <class Type> bool GetTypedProp( int nOffset, edict_t *pEntity, Type *value )
    {
        if ( !pEntity || pEntity->IsFree() || nOffset <= 0)
            return false;

		void *pUnknown = pEntity->GetUnknown();
        if (pUnknown)
        {
			Type *ppropval;
            ppropval = (Type *)((unsigned int)pUnknown + nOffset);
            *value = *ppropval;
            return true;
        }

		return false;
    }

	// Thanks to coldfyr for recommending this method for tracking down properties
    template <class Type> bool GetTypedPropPtr( int nOffset, edict_t *pEntity, Type *value )
    {
        if ( !pEntity || pEntity->IsFree() || nOffset <= 0)
            return false;

		void *pUnknown = pEntity->GetUnknown();
        if (pUnknown)
        {
			Type ppropval;
            ppropval = (Type)((unsigned int)pUnknown + nOffset);
            *value = ppropval ? ppropval : NULL;
            return true;
        }

		return false;
    }

	// Thanks to coldfyr for recommending this method for tracking down properties
    template <class Type> bool SetTypedProp( int nOffset, edict_t *pEntity, Type value )
    {
        if ( !pEntity || pEntity->IsFree() || nOffset <= 0)
            return false;

        void *pUnknown = pEntity->GetUnknown();
        if (pUnknown)
        {
			Type *pval;
            pval = (Type *)((unsigned int)pUnknown + nOffset);
            *pval = value;
			// update that the entity has changed values
            pEntity->m_fStateFlags |= FL_EDICT_CHANGED;
            return true;
        }

		return false;
    }

   const char * GetPlayerName(int userid);

   const char * GetPlayerSteamID(int userid);

   int GetPlayerTeam(int userid);

    IServerGameDLL * m_ServerDll;
	int m_iClientCommandIndex;

#pragma region CommandWrapperDeclarations
    CCmdWrap * m_pEntSetname;
    CCmdWrap * m_pEntFire;
    CCmdWrap * m_pEntityBlocker;
    CCmdWrap * m_pDispatchEffect;
    CGiveWrap * m_pCreateEntity;
    CGiveWrap * m_pCreateEntity2;
    CGiveWrap * m_pPropPhysics;
    CGiveWrap * m_pPropDynamic;
    CGiveWrap * m_pGive;
    CCmdWrap * m_pSetPos;
    CCmdWrap * m_pSetAng;
    CCmdWrap * m_pEntRemove;
#pragma endregion CCmdWrap and CGiveWrap reference declarations


private:
    bool RegisteredScriptExists( const char *szEvent );
    void ExecAllRegistered(const char *szEvent);
    
    int LookupUserIDBySteamID(const char *target_string);

    int GetEdictValue(edict_t* pEdict, int index)
    {
        if(pEdict)
        {
            try{
                int i = *((int *)pEdict->GetUnknown() + index);
                DxMsg(1, __FUNCTION__, __LINE__,"GotEdictValue: %d\n", i);
                return i;
            }
            catch(void *){
                DxMsg(0, __FUNCTION__, __LINE__,"Error reading EdictValue()\n");
                ErrVar("Could not read private EdictValue for entity.");
            }
            return 0;
        }
        else
        {
            ErrMsg(0,"ERROR: Invalid edict, GetEdictValue()\n");
            ErrVar("Invalid edict/entity found.");
            return 0;
        }
    }

    int SetEdictValue(edict_t* pEdict, const int index, const int value)
    {
        if(pEdict)
        {
            *((int *)pEdict->GetUnknown() + index) = value;
            return 1;
            //DxMsg(0, __FUNCTION__, __LINE__,"Edict %d\n", pEdict);
            //DxMsg(0, __FUNCTION__, __LINE__,"Set %d's %d to %d\n", GetUserIDByEdict(pEdict), index, value);
        }
        else
        {
            return -1;
        }
    }

   int GetPlayerFrags(int userid);
   int GetPlayerDeaths(int userid);
   int GetPlayerArmor(int userid);
   int GetPlayerHealth(int userid);

   const char * GetPlayerWeaponName(int userid);

void CEventScriptsPlugin::ResetDelayed( void );// CALLED IN LevelInit() !
void CEventScriptsPlugin::CheckDelayed( void ); // CALLED IN GameFrame() !.

public:
   int IsDead(int userid);
};

#endif // ES_PLUGIN_INTERNAL_H
