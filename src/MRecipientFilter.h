
#ifndef _MRECIIENT_FILTER_H
#define _MRECIPIENT_FILTER_H
#include "irecipientfilter.h"
#include "bitvec.h"
#include "tier1/utlvector.h"
#include "interface.h"
#include "filesystem.h"
#include "engine/iserverplugin.h"
#ifndef ORANGEBOX
#include "dlls/iplayerinfo.h"
#else
#include "game/server/iplayerinfo.h"
#endif

class MRecipientFilter :
   public IRecipientFilter
{
public:
   MRecipientFilter(void);
   ~MRecipientFilter(void);

   virtual bool IsReliable( void ) const;
   virtual bool IsInitMessage( void ) const;

   virtual int GetRecipientCount( void ) const;
   virtual int GetRecipientIndex( int slot ) const;
   void AddAllPlayers( int maxClients, IPlayerInfoManager *playerinfomanager );
   void MRecipientFilter::AddPlayer(int nUserid, int maxClients, IPlayerInfoManager *playerinfomanager);

private:
   bool m_bReliable;
   bool m_bInitMessage;
   CUtlVector< int > m_Recipients;
};

#endif

