
// Build numbers
// these are usually passed in by the automated build.
#ifndef BUILDNUM
#define BUILDNUM 0
#endif
#ifndef REVISION
#define REVISION "DEVELOPER: Don't release without using #define REVISION and #define VER_PRIVATEBUILD_STR"
#pragma message(" *** DEVELOPER: Your private build doesn't specify a revision.")
#pragma message(" *** DEVELOPER: Don't release without using #define REVISION and #define VER_PRIVATEBUILD_STR")
#endif

#if BUILDNUM<1
// reset it to numeric 0
#undef  BUILDNUM 
#define BUILDNUM 0
#define VER_PRIVATEBUILD_STR "Unofficial build"
#endif


#ifdef WIN32
#define STRINGIZE2(s) #s
#define STRINGIZE(s) STRINGIZE2(s)
#define STR_BUILDNUM STRINGIZE(BUILDNUM)
#define STR_REVISION STRINGIZE(REVISION)
#else
#define STRINGIZE2(s) #s
#define STRINGIZE(s) STRINGIZE2(s)
#define STR_BUILDNUM STRINGIZE(BUILDNUM)
#define STR_REVISION STRINGIZE(REVISION)
#endif


// ************************
// * VERSION INFORMATION  *
// ************************
#define VERSIONBASE "2.1.1." STR_BUILDNUM
#define PLUGIN_VERSION_DESCRIPTION "Mattie's EventScripts, http://www.eventscripts.com, Version:" VERSIONBASE
#define VER_FILEVERSION             2,1,1,BUILDNUM
#define VER_PRODUCTVERSION          2,1,1,BUILDNUM
#ifndef VER_PRIVATEBUILD_STR
#define VER_PRIVATEBUILD_STR		"Beta"
#endif 


#define VER_COMPANYNAME_STR "http://mattie.net/cs"
#define VER_FILEDESCRIPTION_STR "EventScripts Plugin for Source games"
#define VER_INTERNALNAME_STR "OysterBox"
#define VER_LEGALCOPYRIGHT_STR "Copyright 2010, Mattie Casper"
#define VER_ORIGINALFILENAME_STR "mattie_eventscripts.dll"
#define VER_PRODUCTNAME_STR PLUGIN_VERSION_DESCRIPTION
#define VER_PATCHLEVEL_STR STR_REVISION
#define VER_ADVICE_STR "You only get one shot at each moment. Only a single try. No redos. Make this moment count."

