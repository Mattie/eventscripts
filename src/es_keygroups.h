#ifndef ES_KEYGROUPS_H
#define ES_KEYGROUPS_H
#include "KeyValues.h"

void InitKeyGroups();
KeyValues * GetKeyGroup(const char *szName, bool bCreate);

// for es_key* functions
#define KEYNAME_USERKEYS "UserKeys"
#define KEYNAME_USERGROUPS "UserGroups"
#define KEYNAME_UNGROUPED "Ungrouped"

extern KeyValues *g_pUserKeys;


#endif

