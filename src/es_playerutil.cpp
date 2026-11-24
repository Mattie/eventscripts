#include "es_util.h"
#include "es_playerutil.h"
//#include "es_strutils.h"
//#include "es_playerutil.h"
//#include "es_cmddef.h"
#include "es_debug.h"
//#include "utlhash.h"
#include "eiface.h"
#include "KeyValues.h"

extern IVEngineServer * engine;
extern int maxplayers;

edict_t* LookupEdictByUserid(const int userid)
{
   if (userid >= 1)
   {
       for(int i=1;i<=maxplayers;i++) 
        {
            DxMsg(6, __FUNCTION__, __LINE__, "LookupEdictByUserid: Looking for %d at %d entity index.\n", userid, i);
            edict_t *pEntity =  PEntityOfEntIndex(i);
            if(!pEntity || pEntity->IsFree())
            {
                    continue;
            }
            if(userid == engine->GetPlayerUserId(pEntity))
            {
                DxMsg(6, __FUNCTION__, __LINE__, "LookupEdictByUserid: Found userid %d at 0x%08x and entity index %d.\n", userid, pEntity, i);
                return pEntity;
            }
        }
   }
   DxMsg(3, __FUNCTION__, __LINE__,"Userid not found. %d\n", userid);
   return NULL;
}

int LookupIndexByUserID(const int userid)
{
   if (userid >= 1)
   {
       for(int i=1;i<=maxplayers;i++) 
        {
            edict_t *pEntity =  PEntityOfEntIndex(i);
            if(!pEntity || pEntity->IsFree()) 
            {
                    continue;
            }
            if(userid == engine->GetPlayerUserId(pEntity))
            {
                return i;
            }
        }
   }
   DxMsg(3, __FUNCTION__, __LINE__,"Userid not found. %d\n", userid);
   return 0;
}

