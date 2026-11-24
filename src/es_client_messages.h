#ifndef ES_CLIENT_MESSAGES_H
#define ES_CLIENT_MESSAGES_H

#ifdef _WIN32
#pragma once
#endif

#ifdef WIN32
#include <windows.h>
#endif

#include <stddef.h>

// obsolete
#define LARGEMESSAGE 3
#define COLORMESSAGE 5

void UTIL_ClientPrint(int userid, int location, const char *msg_name);
void UTIL_ClientPrintAll(int location, const char *msg_name);
void UTIL_ClientSayAll(int color, const char *msg_name);
short UTIL_GetMenuOptions(const char *keys);
void UTIL_SendMenu(int userid, int seconds, const char *msg, const char *keys = NULL);
void SendMessage(int nUserid, int nType, const char *szMsg);
void SendAllMessage(int nType, const char *szMsg);

#endif // ES_CLIENT_MESSAGES_H
