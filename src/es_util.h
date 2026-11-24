#ifndef ES_UTIL_H
#define ES_UTIL_H

#ifdef _WIN32
#pragma once
#endif 

#include "edict.h"
#include "eiface.h"
#include "igameevents.h"
#include "es_defines.h"

extern CGlobalVars *gpGlobals;
extern int g_nEngineVersion;
extern IVEngineServer *engine;

// HACK: The L4D SDK doesn't seem to define INTERFACEVERSION_SERVERGAMETAGS. 
//       We use this to tell if it's L4D versus ORANGEBOX
// 
#ifdef ALIENSWARM // passed in from build
#define LEFT4DEAD 1
#ifndef  SOURCESDK_VERSION
#define SOURCESDK_VERSION "4"
#endif
#endif

#ifdef LEFT4DEAD
#define ORANGEBOX 1
#ifndef  SOURCESDK_VERSION
#define SOURCESDK_VERSION "3"
#endif
#endif

#ifdef ORANGEBOX
#ifndef  SOURCESDK_VERSION
#define SOURCESDK_VERSION "2"
#endif
#endif

#ifndef  SOURCESDK_VERSION
#define SOURCESDK_VERSION "1"
#endif


inline int IndexOfEdict(const edict_t *pEdict)
{
#ifndef LEFT4DEAD
	return engine->IndexOfEdict(pEdict);
#else
	return (int)(pEdict - gpGlobals->pEdicts);
#endif
}
inline edict_t *PEntityOfEntIndex(int iEntIndex)
{
#ifndef LEFT4DEAD
	return engine->PEntityOfEntIndex(iEntIndex);
#else
	if (iEntIndex >= 0 && iEntIndex < gpGlobals->maxEntities)
	{
		return (edict_t *)(gpGlobals->pEdicts + iEntIndex);
	}
	return NULL;
#endif
}

#endif // #ifdef


#ifndef max
#define max(a1,a2) a1 > a2 ? a1 : a2
#endif

#ifndef min
#define min(a1,a2) a1 < a2 ? a1 : a2
#endif

