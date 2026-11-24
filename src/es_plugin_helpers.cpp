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

#ifdef WIN32
#include <windows.h>
#endif
#include "es_plugin_internal.h"
#include "basehandle.h"
#include "iservernetworkable.h"
#include "shareddefs.h"
#include "es_strutils.h"
#include "inetchannelinfo.h"
#include "filesystem.h"
#ifdef ORANGEBOX
#include "toolframework/itoolentity.h"
#endif

extern IFileSystem *filesystem;
extern IGameEventManager2 *gameeventmanager;
extern IPlayerInfoManager *playerinfomanager;
extern int maxplayers;
#ifdef ORANGEBOX
extern IServerTools *tools;
#endif

    void CEventScriptsPlugin::OnQueryCvarValueFinished( QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue )
    {
        //eQueryCvarValueStatus_ValueIntact=0,	// It got the value fine.
        //eQueryCvarValueStatus_CvarNotFound=1,
        //eQueryCvarValueStatus_NotACvar=2,		// There's a ConCommand, but it's not a ConVar.
        //eQueryCvarValueStatus_CvarProtected=3	// The cvar was marked with FCVAR_SERVER_CAN_NOT_QUERY, so the server is not allowed to have its value.
        char *pStatus=NULL;
        switch (eStatus)
        {
        case eQueryCvarValueStatus_ValueIntact:
            pStatus = "success";
            break;
        case eQueryCvarValueStatus_CvarNotFound:
            pStatus = "not found";
            break;
        case eQueryCvarValueStatus_NotACvar:
            pStatus = "not variable";
            break;
        case eQueryCvarValueStatus_CvarProtected:
            pStatus = "protected";
            break;
        }
        // nothing now
        if (pPlayerEntity)
        {
            int nUserid = engine->GetPlayerUserId(pPlayerEntity);
            IGameEvent * event = gameeventmanager->CreateEvent( "es_player_variable");
	        if ( event )
	        {
		        event->SetInt( "userid", nUserid );
                event->SetString( "status", pStatus);
                event->SetString("variable", pCvarName ? pCvarName : "");
                event->SetString("value", pCvarValue ? pCvarValue : "");
		        gameeventmanager->FireEvent( event );
	        }
        }
        else
        {
            DxMsg(1, __FUNCTION__, __LINE__, "ES: OnQueryCvarValueFinished called but values did not exist.\n");
        }
    }

    unsigned int CEventScriptsPlugin::LookupIndexByHandle(int nHandleInIntForm)
    {
		CBaseHandle htest(nHandleInIntForm);
		unsigned int nIndex = htest.GetEntryIndex();
		edict_t *pEntity = PEntityOfEntIndex(nIndex);
        if (pEntity && !pEntity->IsFree())
        {
            IServerNetworkable *pNet = pEntity->GetNetworkable();
            IHandleEntity *pIE = pNet->GetEntityHandle();
            if (!pIE)  {
                DxMsg(0, __FUNCTION__, __LINE__,"Couldn't GetEntityHandle\n");
                return 0;
            }
            const CBaseHandle newb = pIE->GetRefEHandle();
            if (!newb.IsValid()) {
                DxMsg(0, __FUNCTION__, __LINE__,"Couldn't get RefEHandle2\n");
                return 0;
            }
			if (htest.GetSerialNumber() != newb.GetSerialNumber()) {
				DxMsg(1, __FUNCTION__, __LINE__,"Serials don't match for index %d: %d != %d\n", nIndex, htest.GetSerialNumber(), newb.GetSerialNumber());
				return 0;				
			}
            return nIndex;
        }
        return 0;
    }

    unsigned int CEventScriptsPlugin::LookupHandleForEntity(edict_t * pEntity)
    {
		DxMsg(3, __FUNCTION__, __LINE__, "LookupHandleForEntity(%d) called.\n", pEntity);
        if (pEntity && !pEntity->IsFree())
        {
            IServerNetworkable *pNet = pEntity->GetNetworkable();
			if (!pNet)
			{
				DxMsg(1, __FUNCTION__, __LINE__,"Couldn't GetNetworkable\n");
				return 0;
			}
            DxMsg(4, __FUNCTION__, __LINE__, "LookupHandleForEntity(%d) getting IHandleEntity.\n", pEntity);
			IHandleEntity *pIE = pNet->GetEntityHandle();
            if (!pIE)
            {
                    DxMsg(1, __FUNCTION__, __LINE__,"Couldn't GetEntityHandle\n");
                    return 0;
            }
            DxMsg(4, __FUNCTION__, __LINE__, "LookupHandleForEntity(%d) getting CBaseHandle.\n", pEntity);
            const CBaseHandle newb = pIE->GetRefEHandle();
            if (!newb.IsValid())
            {
                    DxMsg(1, __FUNCTION__, __LINE__,"Couldn't get RefEHandle2\n");
                    return 0;
            }
			DxMsg(3, __FUNCTION__, __LINE__, "LookupHandleForEntity(%d) returning %d.\n", pEntity, newb.ToInt());
            return newb.ToInt();
        }
        return 0;
    }

    int CEventScriptsPlugin::LookupUseridByHandle(unsigned int handle)
    {
       for(int i=1;i<=maxplayers;i++) 
        {
            edict_t *pEntity =  PEntityOfEntIndex(i);
            if(!pEntity || pEntity->IsFree()) 
            {
                    continue;
            }
            int userid = engine->GetPlayerUserId(pEntity);
            if(handle == LookupHandleByUserid(userid))
            {
                return userid;
            }
        }
        return 0;
    }

    //-----------------------------------------------------------------------------
    // Purpose: 
    // Input  : playerIndex - 
    //			ping - 
    //			packetloss - 
    //-----------------------------------------------------------------------------
    void CEventScriptsPlugin::GetPlayerConnectionInfo( int userid, int& ping, int &packetloss, float &fConnectedTime, char * szAddress)
    {
        int playerIndex = LookupIndexByUserID(userid);
        if (playerIndex < 1)
        {
            ping = 0;
            packetloss = 0;
            return;
        }
        edict_t * pEntity = PEntityOfEntIndex(playerIndex);
        if (!pEntity)
        {
            ping = 0;
            packetloss = 0;
            return;
        }
	    IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEntity );
        if (playerinfo && playerinfo->IsPlayer() && !playerinfo->IsFakeClient())
        {

	        INetChannelInfo *nci = engine->GetPlayerNetInfo(playerIndex);

	        if ( nci )
	        {
		        float latency = nci->GetAvgLatency( FLOW_OUTGOING ); // in seconds
        		
		        // that should be the correct latency, we assume that cmdrate is higher 
		        // then updaterate, what is the case for default settings
		        const char * szCmdRate = engine->GetClientConVarValue( playerIndex, "cl_cmdrate" );
#ifndef _WINDOWS
#define max(a1,a2) a1 > a2 ? a1 : a2
#define min(a1,a2) a1 < a2 ? a1 : a2
#endif
		        int nCmdRate = max( 1, atoi( szCmdRate ) );
		        latency -= (0.5f/nCmdRate) + TICKS_TO_TIME( 1.0f ); // correct latency

		        // in GoldSrc we had a different, not fixed tickrate. so we have to adjust
		        // Source pings by half a tick to match the old GoldSrc pings.
		        latency -= TICKS_TO_TIME( 0.5f );

		        ping = latency * 1000.0f; // as msecs
		        ping = clamp( ping, 5, 1000 ); // set bounds, dont show pings under 5 msecs
        		
		        packetloss = 100.0f * nci->GetAvgLoss( FLOW_INCOMING ); // loss in percentage
		        packetloss = clamp( packetloss, 0, 100 );

                // get other things, too:
                fConnectedTime = nci->GetTimeConnected();
                strcpy(szAddress, nci->GetAddress());
	        }
	        else
	        {
		        ping = 0;
		        packetloss = 0;
	        }
        }
        else
        {
            ping = 0;
            packetloss = 0;
        }
    }

#ifdef ORANGEBOX
	void * CEventScriptsPlugin::GetEntityByIndex(int index) {
		void *j = tools->FirstEntity();
		int i = -1;
		while (j) {
			IServerUnknown *p = (IServerUnknown *)j;
			IServerNetworkable *n = p->GetNetworkable();
			edict_t * e = n->GetEdict();
			i = IndexOfEdict(e);
			if (index == i) {
				return j;
			}
			j = tools->NextEntity((CBaseEntity*)j);
		}
		return NULL;
	}
#endif

    BOOL CEventScriptsPlugin::RegisterForEventFile(const char* szFileName)
    {
        KeyValues * pEvents = new KeyValues("events");
        KeyValues *kv;
        if (!pEvents->LoadFromFile(filesystem, szFileName))
        {
            ErrVar("Couldn't load events file.");
            return FALSE;
        }
        kv = pEvents->GetFirstSubKey();
        while(kv)
        {
            DxMsg(1, __FUNCTION__, __LINE__,"Added: %s\n", kv->GetName());
            gameeventmanager->AddListener( this, kv->GetName(),true );
            kv = kv->GetNextKey();
        }
        DxMsg(1, __FUNCTION__, __LINE__,"Done loading events.\n");
        pEvents->deleteThis();
        return TRUE;
    }

    int CEventScriptsPlugin::GetPlayerCount(const int team, bool bOnlyAlive)
    {
        int nCount = 0;
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
                    if (bOnlyAlive && IsDead(engine->GetPlayerUserId(pEntity)))
                    {
                        DxMsg(1, __FUNCTION__, __LINE__,"Player is dead, not counting him: %d\n", playerinfo->GetName());
                        continue;
                    }
                    if (team < 0)
                    {
                        nCount++;
                        continue;
                    }
                    else
                    {
                        if (playerinfo->GetTeamIndex()==team)
                        {
                            nCount++;
                            continue;
                        }
                    }

                }
            }
            else
            {
                ErrMsg(0,"ERROR: Playerinfomanager couldn't be contacted.\n");
                ErrVar("Playerinfomanager couldn't be accessed.");
                return -1;
            }
        }
        return nCount;
    }

    CBotCmd *CEventScriptsPlugin::GetPlayerBotCmd(const int userid)
    {
       edict_t * pEntity = LookupEdictByUserid(userid);
       if (pEntity)
       {
            if(playerinfomanager)
            {
	            IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEntity );
                if (playerinfo && playerinfo->IsPlayer())
                {
                    CBotCmd *p = new CBotCmd();
                    *p = playerinfo->GetLastUserCommand();
                    return p;
                }
                else
                {
                    DxMsg(3, __FUNCTION__, __LINE__,"Userid isn't a player: %d\n", userid);
                    return NULL;
                }
            }
            else
            {
                DxMsg(1, __FUNCTION__, __LINE__,"playerinfomanager isn't ready yet.\n");
                return  NULL;
            }
       }
       else
       {
           DxMsg(3, __FUNCTION__, __LINE__,"Player doesn't exist: %d\n", userid);
           return NULL;
       }
    }

    void CEventScriptsPlugin::GetPlayerLocation(const int userid, float *pf)
    {
       edict_t * pEntity = LookupEdictByUserid(userid);
       if (pEntity)
       {
            if(playerinfomanager)
            {
	            IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEntity );
                if (playerinfo && playerinfo->IsPlayer())
                {
                    
                    playerinfo->GetAbsOrigin().CopyToArray(pf);
                    return;
                    
                }
                else
                {
                    DxMsg(2, __FUNCTION__, __LINE__,"Userid isn't a player: %d\n", userid);
                    return;
                }
            }
            else
            {
                DxMsg(1, __FUNCTION__, __LINE__,"playerinfomanager isn't ready yet.\n");
                return;
            }
       }
       else
       {
           DxMsg(2, __FUNCTION__, __LINE__,"Player doesn't exist: %d\n", userid);
           return;
       }
    }

    // used for bot detection. Won't work until client is activated
    int CEventScriptsPlugin::EdictIsFakeClient( edict_t *pEntity )
    {
        if (pEntity) 
        {
            if(playerinfomanager)
            {
	            IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEntity );
                if (playerinfo && playerinfo->IsFakeClient())
                {
                    return 1;
                }
                else
                {
                    return 0;
                }
            }
        }
        return -1;
    }

void CEventScriptsPlugin::SplitPath(const char *szFullPath, char * ClassName, char *szTempPath)
{
    if (!szFullPath || !ClassName || !szTempPath)
    {
        ErrMsg(0,"ERROR: invalid SplitPath parameters.\n");
        ErrVar("Invalid parameters");
        return;
    }
    int i = 0;
    while(szFullPath[i] != '\0' && szFullPath[i] != '.')
    {
        ClassName[i] = szFullPath[i];
        i++;
    }
    ClassName[i]='\0';
    int j = 0;
    while(szFullPath[i] != '\0')
    {
        i++;
        szTempPath[j] = szFullPath[i];
        j++;
    }
    szTempPath[j]='\0';
}

bool CEventScriptsPlugin::GetOffsetInTable(SendTable * pTable, char *szRemainPath, int *nOffset, SendPropType *pType)
{
    DxMsg(3, __FUNCTION__, __LINE__,"Searching %s...\n", szRemainPath);
    char szTempPath[2056];
    strcpy(szTempPath, szRemainPath);
    char ValueName[1024];
    SplitPath(szRemainPath, ValueName, szTempPath);
    int NumProps = pTable->GetNumProps();
    for (int i=0; i<NumProps; i++)
    {
        SendProp * prp = pTable->GetProp(i);
        if (FStrEq(ValueName, prp->GetName()))
        {
            bool bValueOutput = false;
            *nOffset = *nOffset+prp->GetOffset();
            *pType = prp->GetType();
            switch (prp->GetType())
            {
            case DPT_Vector:
            case DPT_String:
            case DPT_Array:
            case DPT_Int:
            case DPT_Float:
                DxMsg(3, __FUNCTION__, __LINE__,"Found %s\n", ValueName);
                return true;
            case DPT_DataTable:
                return GetOffsetInTable(prp->GetDataTable(), szTempPath, nOffset, pType);
                break;
            default:
                *nOffset = 0;
                return false; 
            }
        }
    }
    return false;
}

bool CEventScriptsPlugin::GetOffset(const char *szFullPath, int *nOffset, SendPropType *pType)
{
    char szTempPath[2056];
    strcpy(szTempPath, szFullPath);
    char ClassName[1024];
    SplitPath(szFullPath, ClassName, szTempPath);
    *nOffset = 0;

    DxMsg(3, __FUNCTION__, __LINE__,"Searching %s\n", szFullPath);
    ServerClass *sc = m_ServerDll->GetAllServerClasses();
    while (sc)
    {
        DxMsg(3, __FUNCTION__, __LINE__,"ServerClass: %s\n", sc->GetName());
        if (FStrEq(sc->GetName(), ClassName))
        {
            return GetOffsetInTable(sc->m_pTable, szTempPath, nOffset, pType);
        }
        sc = sc->m_pNext;
    }
    return false;
}

    void CEventScriptsPlugin::DUMP_ServerClasses()
    {
        SendTable *m_DataTable = NULL;
        ServerClass *sc = m_ServerDll->GetAllServerClasses();
        while (sc)
        {
            int NumProps = sc->m_pTable->GetNumProps();
            DxMsg(0, __FUNCTION__, __LINE__,"%s %d (%d properties)\n", sc->GetName(), sc->m_InstanceBaselineIndex,  NumProps);
            for (int i=0; i<NumProps; i++)
            {
                char *pType = "Unknown";
                switch (sc->m_pTable->GetProp(i)->GetType())
                {
                case DPT_Int:
                    pType = "int";
                    break;
                case DPT_Float:
                    pType = "float";
                    break;
                case DPT_Vector:
                    pType = "vector";
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
                    break;
                }
                DxMsg(0, __FUNCTION__, __LINE__,"---------%s : %s\n", pType, sc->m_pTable->GetProp(i)->GetName());
                if (DPT_DataTable == sc->m_pTable->GetProp(i)->GetType())
                {
                    if (!FStrEq(sc->m_pTable->GetProp(i)->GetName(), "baseclassx"))
                    {
                        m_DataTable = sc->m_pTable->GetProp(i)->GetDataTable();
                        int nProps = m_DataTable->GetNumProps();
                        int ni;
                        ////DxMsg(1, __FUNCTION__, __LINE__,"Using data table3. %d\n", Props);
                        for (ni=0; ni<nProps; ni++)
                        {
                            switch (m_DataTable->GetProp(ni)->GetType())
                            {
                            case DPT_Int:
                                pType = "int";
                                break;
                            case DPT_Float:
                                pType = "float";
                                break;
                            case DPT_Vector:
                                pType = "vector";
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
                                break;
                            }
                            DxMsg(0, __FUNCTION__, __LINE__,"------------------%s : %s\n", pType, m_DataTable->GetProp(ni)->GetName());
                        }

                    }
                }
            }
            sc = sc->m_pNext;
        }
    }

   const char * CEventScriptsPlugin::GetPlayerName(int userid)
   {
       edict_t * pEntity = LookupEdictByUserid(userid);
       if (pEntity)
       {
            if(playerinfomanager)
            {
	            IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEntity );
                if (playerinfo && playerinfo->IsPlayer())
                {
                    return playerinfo->GetName();
                }
                else
                {
                    DxMsg(1, __FUNCTION__, __LINE__,"Userid isn't a player: %d\n", userid);
                    return NULL;
                }
            }
            else
            {
                DxMsg(1, __FUNCTION__, __LINE__,"playerinfomanager isn't ready yet.\n");
                return NULL;
            }
       }
       else
       {
           DxMsg(1, __FUNCTION__, __LINE__,"Player doesn't exist: %d\n", userid);
           return NULL;
       }
   }

   const char * CEventScriptsPlugin::GetPlayerSteamID(int userid)
   {
       edict_t * pEntity = LookupEdictByUserid(userid);
       if (pEntity)
       {
            if(playerinfomanager)
            {
	            IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEntity );
                if (playerinfo && playerinfo->IsPlayer())
                {
                    return playerinfo->GetNetworkIDString();
                }
                else
                {
                    DxMsg(1, __FUNCTION__, __LINE__,"Userid isn't a player: %d\n", userid);
                    return NULL;
                }
            }
            else
            {
                DxMsg(1, __FUNCTION__, __LINE__,"playerinfomanager isn't ready yet.\n");
                return NULL;
            }
       }
       else
       {
           DxMsg(1, __FUNCTION__, __LINE__,"Player doesn't exist: %d\n", userid);
           return NULL;
       }
   }

   int CEventScriptsPlugin::GetPlayerTeam(int userid)
   {
       edict_t * pEntity = LookupEdictByUserid(userid);
       if (pEntity)
       {
            if(playerinfomanager)
            {
	            IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEntity );
                if (playerinfo && playerinfo->IsPlayer())
                {
                    return playerinfo->GetTeamIndex();
                }
                else
                {
                    DxMsg(1, __FUNCTION__, __LINE__,"Userid isn't a player: %d\n", userid);
                    return NULL;
                }
            }
            else
            {
                DxMsg(1, __FUNCTION__, __LINE__,"playerinfomanager isn't ready yet.\n");
                return NULL;
            }
       }
       else
       {
           DxMsg(1, __FUNCTION__, __LINE__,"Player doesn't exist: %d\n", userid);
           return NULL;
       }
   }

    int CEventScriptsPlugin::LookupUserIDBySteamID(const char *target_string)
    {
       for(int i=1;i<=maxplayers;i++) 
        {
            edict_t *pEntity =  PEntityOfEntIndex(i);
            if(!pEntity || pEntity->IsFree()) 
            {
                    continue;
            }
            if(FStrEq(target_string, engine->GetPlayerNetworkIDString(pEntity)))
            {
                return engine->GetPlayerUserId(pEntity);
            }
        }
        DxMsg(1, __FUNCTION__, __LINE__,"STEAMID not found. %d\n", target_string);
        return 0;
    }

   int CEventScriptsPlugin::GetPlayerFrags(int userid)
   {
       edict_t * pEntity = LookupEdictByUserid(userid);
       if (pEntity)
       {
            if(playerinfomanager)
            {
	            IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEntity );
                if (playerinfo && playerinfo->IsPlayer())
                {
                    return playerinfo->GetFragCount();
                }
                else
                {
                    DxMsg(1, __FUNCTION__, __LINE__,"Userid isn't a player: %d\n", userid);
                    return NULL;
                }
            }
            else
            {
                DxMsg(1, __FUNCTION__, __LINE__,"playerinfomanager isn't ready yet.\n");
                return NULL;
            }
       }
       else
       {
           DxMsg(1, __FUNCTION__, __LINE__,"Player doesn't exist: %d\n", userid);
           return NULL;
       }
   }

   int CEventScriptsPlugin::GetPlayerDeaths(int userid)
   {
       edict_t * pEntity = LookupEdictByUserid(userid);
       if (pEntity)
       {
            if(playerinfomanager)
            {
	            IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEntity );
                if (playerinfo && playerinfo->IsPlayer())
                {
                    return playerinfo->GetDeathCount();
                }
                else
                {
                    DxMsg(1, __FUNCTION__, __LINE__,"Userid isn't a player: %d\n", userid);
                    return NULL;
                }
            }
            else
            {
                DxMsg(1, __FUNCTION__, __LINE__,"playerinfomanager isn't ready yet.\n");
                return NULL;
            }
       }
       else
       {
           DxMsg(1, __FUNCTION__, __LINE__,"Player doesn't exist: %d\n", userid);
           return NULL;
       }
   }

   int CEventScriptsPlugin::GetPlayerArmor(int userid)
   {
       edict_t * pEntity = LookupEdictByUserid(userid);
       if (pEntity)
       {
            if(playerinfomanager)
            {
	            IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEntity );
                if (playerinfo && playerinfo->IsPlayer())
                {
                    return playerinfo->GetArmorValue();
                }
                else
                {
                    DxMsg(1, __FUNCTION__, __LINE__,"Userid isn't a player: %d\n", userid);
                    return NULL;
                }
            }
            else
            {
                DxMsg(1, __FUNCTION__, __LINE__,"playerinfomanager isn't ready yet.\n");
                return NULL;
            }
       }
       else
       {
           DxMsg(1, __FUNCTION__, __LINE__,"Player doesn't exist: %d\n", userid);
           return NULL;
       }
   }

   int CEventScriptsPlugin::GetPlayerHealth(int userid)
   {
       edict_t * pEntity = LookupEdictByUserid(userid);
       if (pEntity)
       {
            if(playerinfomanager)
            {
	            IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEntity );
                if (playerinfo && playerinfo->IsPlayer())
                {
                    return playerinfo->GetHealth();
                }
                else
                {
                    DxMsg(1, __FUNCTION__, __LINE__,"Userid isn't a player: %d\n", userid);
                    return NULL;
                }
            }
            else
            {
                DxMsg(1, __FUNCTION__, __LINE__,"playerinfomanager isn't ready yet.\n");
                return NULL;
            }
       }
       else
       {
           DxMsg(1, __FUNCTION__, __LINE__,"Player doesn't exist: %d\n", userid);
           return NULL;
       }
   }

   const char * CEventScriptsPlugin::GetPlayerWeaponName(int userid)
   {
       edict_t * pEntity = LookupEdictByUserid(userid);
       if (pEntity)
       {
            if(playerinfomanager)
            {
	            IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEntity );
                if (playerinfo && playerinfo->IsPlayer())
                {
                    return playerinfo->GetWeaponName();
                }
                else
                {
                    DxMsg(1, __FUNCTION__, __LINE__,"Userid isn't a player: %d\n", userid);
                    return NULL;
                }
            }
            else
            {
                DxMsg(1, __FUNCTION__, __LINE__,"playerinfomanager isn't ready yet.\n");
                return NULL;
            }
       }
       else
       {
           DxMsg(1, __FUNCTION__, __LINE__,"Player doesn't exist: %d\n", userid);
           return NULL;
       }
   }

// returns -1 if 
int CEventScriptsPlugin::FindUserIDByString(const char *target_string)
{
    const char *display_target = target_string != NULL ? target_string : "<null>";
    edict_t *entity;
    IPlayerInfo *player;
    int index;
    int userid;

    DxMsg(1, __FUNCTION__, __LINE__,
          "FindUserIDByString: resolving '%s'\n", display_target);

    if (target_string == NULL || target_string[0] == '\0')
    {
        for (index = 1; index <= maxplayers; ++index)
        {
            entity = PEntityOfEntIndex(index);
            if (entity == NULL || entity->IsFree())
                continue;

            player = playerinfomanager->GetPlayerInfo(entity);
            if (player != NULL && player->IsConnected())
            {
                userid = player->GetUserID();
                DxMsg(1, __FUNCTION__, __LINE__,
                      "FindUserIDByString: first connected player is userid %d\n",
                      userid);
                return userid;
            }
        }

        DxMsg(1, __FUNCTION__, __LINE__,
              "FindUserIDByString: no connected player was found\n");
        return 0;
    }

    userid = atoi(target_string);
    if (userid != 0)
    {
        if (LookupEdictByUserid(userid))
        {
            DxMsg(1, __FUNCTION__, __LINE__,
                  "FindUserIDByString: matched userid %d\n", userid);
            return userid;
        }

        userid = LookupUseridByHandle(userid);
        if (userid > 0)
        {
            DxMsg(1, __FUNCTION__, __LINE__,
                  "FindUserIDByString: handle resolved to userid %d\n", userid);
            return userid;
        }
    }

    if (strlen(target_string) > 6 &&
        Q_strncasecmp(target_string, "STEAM_", 6) == 0)
    {
        userid = LookupUserIDBySteamID(target_string);
        if (userid != 0)
        {
            DxMsg(1, __FUNCTION__, __LINE__,
                  "FindUserIDByString: network ID resolved to userid %d\n",
                  userid);
            return userid;
        }
    }

    for (index = 1; index <= maxplayers; ++index)
    {
        entity = PEntityOfEntIndex(index);
        if (entity == NULL || entity->IsFree())
            continue;

        player = playerinfomanager->GetPlayerInfo(entity);
        if (player == NULL || !player->IsConnected() || player->GetName() == NULL)
            continue;

        if (FStrEq(player->GetName(), target_string))
        {
            userid = player->GetUserID();
            DxMsg(1, __FUNCTION__, __LINE__,
                  "FindUserIDByString: exact name resolved to userid %d\n",
                  userid);
            return userid;
        }
    }

    if (FStrEq(target_string, "BOT"))
    {
        userid = LookupUserIDBySteamID(target_string);
        if (userid != 0)
        {
            DxMsg(1, __FUNCTION__, __LINE__,
                  "FindUserIDByString: BOT network ID resolved to userid %d\n",
                  userid);
            return userid;
        }
    }

    for (index = 1; index <= maxplayers; ++index)
    {
        entity = PEntityOfEntIndex(index);
        if (entity == NULL || entity->IsFree())
            continue;

        player = playerinfomanager->GetPlayerInfo(entity);
        if (player == NULL || !player->IsConnected() || player->GetName() == NULL)
            continue;

        if (Q_stristr(player->GetName(), target_string) != NULL)
        {
            userid = player->GetUserID();
            DxMsg(1, __FUNCTION__, __LINE__,
                  "FindUserIDByString: partial name resolved to userid %d\n",
                  userid);
            return userid;
        }
    }

    DxMsg(1, __FUNCTION__, __LINE__,
          "FindUserIDByString: no match for '%s'\n", display_target);
    return 0;
}
