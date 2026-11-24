#ifndef ES_DEBUG_H
#define ES_DEBUG_H

typedef char tchar;
void ErrMsg(int nLevel,  const tchar *pszFormat, ...);
void DxMsg(int nLevel, const char* szFunction, int nLine, const tchar *pszFormat, ...);
void DebugMessageOut(const char *pszMsg);
void DxMsgExternal(int nLevel, const tchar *pszFormat, ...);
void ErrVar(const tchar *pszFormat, ...);

#define MSGLOC __FUNCTION__, __LINE__
#endif // ES_DEBUG_H

