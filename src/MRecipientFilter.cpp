// Initially used from hl2coding.com's forum tutorial on sounds.

#include "MRecipientFilter.h"
#include "eiface.h"
#include "igameevents.h"
#include "convar.h"
#include "Color.h"
#include "es_util.h"

#include "shake.h"
#include "IEffects.h"
#include "engine/IEngineSound.h"

extern IVEngineServer   *engine;
//extern IPlayerInfoManager *playerinfomanager;
extern IServerPluginHelpers *helpers;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

MRecipientFilter::MRecipientFilter(void)
{
}

MRecipientFilter::~MRecipientFilter(void)
{
}

int MRecipientFilter::GetRecipientCount() const
{
#ifndef ALIENSWARM
   return m_Recipients.Size();
#else
   return m_Recipients.Count();
#endif
}

int MRecipientFilter::GetRecipientIndex(int slot) const
{
   if ( slot < 0 || slot >= GetRecipientCount() )
      return -1;

   return m_Recipients[ slot ];
}

bool MRecipientFilter::IsInitMessage() const
{
   return false;
}

bool MRecipientFilter::IsReliable() const
{
   return false;
}

void MRecipientFilter::AddPlayer(int nUserid, int maxClients, IPlayerInfoManager *playerinfomanager)
{
    m_Recipients.RemoveAll();

    for(int i=1;i<=maxClients;i++) 
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
            if(playerinfo)// && playerinfo->IsPlayer() && !playerinfo->IsFakeClient())
            {
                if(nUserid == playerinfo->GetUserID())
                {
                   m_Recipients.AddToTail(i);
                   return;
                }
            }
        }

    }    
}

void MRecipientFilter::AddAllPlayers(int maxClients, IPlayerInfoManager *playerinfomanager)
{
    m_Recipients.RemoveAll();

    for(int i=1;i<=maxClients;i++) 
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
            if(playerinfo && playerinfo->IsPlayer() && !playerinfo->IsFakeClient())
            {
                m_Recipients.AddToTail(i);
            }
        }

    }
} 

