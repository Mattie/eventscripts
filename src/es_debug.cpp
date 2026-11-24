#include "es_debug.h"
#include "string.h"
#include "eiface.h"
// only used for ErrMsg() output
#include "VirtualMachine/ExecutionManager.h"

int g_nDebugLevel = 0;
int g_nDebugLog = 0;
int g_nDebugShowFunctions = 0;

extern IVEngineServer *engine;
extern VMInstruction g_LatestInstruction;

void ErrMsg(int nLevel,  const tchar *pszFormat, ...)
{
    if (g_nDebugLevel>=nLevel) {
        tchar m_szBuf[2048];
		va_list arg_ptr;

		va_start(arg_ptr, pszFormat);
		_vsntprintf(m_szBuf, sizeof(m_szBuf)-1, pszFormat, arg_ptr);
		va_end(arg_ptr);

		m_szBuf[sizeof(m_szBuf)-1] = 0;
        if (g_nDebugLog==1) {
            if (engine)
            {
                engine->LogPrint(m_szBuf);
                char szLocMessage[MAX_COMMAND_STRING];
                sprintf(szLocMessage, "  Error location %s: %s", g_LatestInstruction.szLineNum, g_LatestInstruction.szCommand);
                engine->LogPrint(szLocMessage);
            }
        }
        else {
            Msg("%s  Error source %s: %s", m_szBuf, g_LatestInstruction.szLineNum, g_LatestInstruction.szCommand);
        }
    }
}

void DebugMessageOut(const char *pszMsg)
{
        if (g_nDebugLog==1) {
            if (engine)
            {
                engine->LogPrint(pszMsg);
            }
        }
        else {
            Msg("%s", pszMsg);
        }
}

void DebugMessagePrint(int nLevel, const char *prefix, const char *pszFormat, va_list arg_ptr)
{
    if (g_nDebugLevel>=nLevel) {
        tchar szFormat[2048];
        strcpy(szFormat, prefix);
        strcat(szFormat, pszFormat);
        tchar m_szBuf[2048];
		_vsntprintf(m_szBuf, sizeof(m_szBuf)-1, szFormat, arg_ptr);
		m_szBuf[sizeof(m_szBuf)-1] = 0;
        DebugMessageOut(m_szBuf);
    }
}
void DxMsg(int nLevel, const char* szFunction, int nLine, const tchar *pszFormat, ...)
{
    if (g_nDebugLevel>=nLevel) {
        tchar szPrefix[2048];
        if (nLevel>0 ) {
            if (g_nDebugShowFunctions>0) {
                sprintf(szPrefix, "ES-plugin-%02d:\t%s(%d)\n  ", nLevel, szFunction, nLine);
            } else{
                sprintf(szPrefix, "ES-plugin-%02d: ", nLevel);                
            }
        }
        else {
            *szPrefix = 0;
        }
        va_list arg_ptr;
        va_start(arg_ptr, pszFormat);
        DebugMessagePrint(nLevel, szPrefix, pszFormat, arg_ptr);
        va_end(arg_ptr);
    }
}
void DxMsgExternal(int nLevel, const tchar *pszFormat, ...)
{
    if (g_nDebugLevel>=nLevel) {
        tchar szPrefix[20];
        if (nLevel>0) {
            sprintf(szPrefix, "ES-script-%02d: ", nLevel);
        }
        else {
            *szPrefix = 0;
        }
        va_list arg_ptr;
        va_start(arg_ptr, pszFormat);
        DebugMessagePrint(nLevel, szPrefix, pszFormat, arg_ptr);
        va_end(arg_ptr);
    }
}


static ConVar error_cvar("eventscripts_lasterror", "", 0, "The last error message returned by an EventScripts command.");static ConVar errornum_cvar("eventscripts_lasterrornum", "0", 0, "The last error number returned by an EventScripts command.");
void ErrVar(const tchar *pszFormat, ...)
{
        tchar m_szBuf[2048];
		va_list arg_ptr;

		va_start(arg_ptr, pszFormat);
		_vsntprintf(m_szBuf, sizeof(m_szBuf)-1, pszFormat, arg_ptr);
		va_end(arg_ptr);

		m_szBuf[sizeof(m_szBuf)-1] = 0;
        error_cvar.SetValue(m_szBuf);
}


