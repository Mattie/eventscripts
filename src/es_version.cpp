#include "es_version.h"
#include "es_util.h"
#include "es_debug.h"
#include "convar.h"


static ConVar version_cvar("eventscripts_ver", VERSIONBASE, FCVAR_REPLICATED|FCVAR_NOTIFY, "The version number of EventScripts.");
static ConVar buildno_cvar("eventscripts_ver_build", STR_BUILDNUM, 0, "The build number of EventScripts.");
static ConVar revision_cvar("eventscripts_ver_revision", STR_REVISION, 0, "The revision number of EventScripts");
static ConVar sourcesdk_cvar("eventscripts_sourcesdk_level", SOURCESDK_VERSION, 0, "The Source SDK level that EventScripts was compiled with (rough numbering scheme)");


CON_COMMAND( eventscripts_version, "prints the version of Mattie's EventScripts plugin" )
{
 
	DxMsg(0, __FUNCTION__, __LINE__, PLUGIN_VERSION_DESCRIPTION ); 
    DxMsg(0, __FUNCTION__, __LINE__,"\n");
}

CON_COMMAND( eventscripts_log, "logs the version of Mattie's EventScripts plugin" )
{
	engine->LogPrint( PLUGIN_VERSION_DESCRIPTION );
    engine->LogPrint( "\n" );
}

