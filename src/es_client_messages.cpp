//=============================================================================//
//
// Mattie's EventScripts
//
// The MIT License
//
// Copyright © 2010, Mattie Casper
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// Except as contained in this notice, the name(s) of the above copyright holders 
// shall not be used in advertising or otherwise to promote the sale, use or other 
// dealings in this Software without prior written authorization.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
//
//=============================================================================//

#include "es_client_messages.h"
#include "es_util.h"
#include "bitbuf.h"
#include "shareddefs.h"
#include "es_debug.h"
#include "es_usermsg.h"
#include "MRecipientFilter.h"

extern IPlayerInfoManager *playerinfomanager;
extern int maxplayers;

void UTIL_ClientPrint(int userid, int location, const char *msg_name)
{
	MRecipientFilter filter;
    if (userid>0)
    {
    	filter.AddPlayer(userid, maxplayers, playerinfomanager);
    }
    else
    {
        filter.AddAllPlayers(maxplayers, playerinfomanager);
    }
    bf_write *pWrite;
	pWrite = g_pUserMessages->UserMessageBegin( (IRecipientFilter *)&filter, 5, NULL);//"TextMsg" );
	pWrite->WriteByte( location );
    pWrite->WriteString(msg_name);

    pWrite->WriteString( "" );

	pWrite->WriteString( "" );
	pWrite->WriteString( "" );
	pWrite->WriteString( "" );

	engine->MessageEnd();
}

void UTIL_ClientPrintAll(int location, const char *msg_name)
{
    UTIL_ClientPrint(0, location, msg_name);
}

void UTIL_ClientSayAll(int color, const char *msg_name)
{
	MRecipientFilter filter;
    filter.AddAllPlayers(maxplayers, playerinfomanager);
    bf_write *pWrite;
	pWrite = g_pUserMessages->UserMessageBegin( (IRecipientFilter *)&filter, 4, NULL);//"SayMsg" );
    //sprintf
	pWrite->WriteByte( 0 );
    pWrite->WriteString(msg_name);

    pWrite->WriteString( "" );

	pWrite->WriteString( "" );
	pWrite->WriteString( "" );
	pWrite->WriteString( "" );

	engine->MessageEnd();

    // %c%s 3, 4 are light green and dark green
}

short UTIL_GetMenuOptions(const char *keys)
{
    if (!keys) return 1023;
    short nRetVal = 0;
    nRetVal+= strstr(keys, "0") ? 1<<9 : 0;
    nRetVal+= strstr(keys, "1") ? 1<<0 : 0;
    nRetVal+= strstr(keys, "2") ? 1<<1 : 0;
    nRetVal+= strstr(keys, "3") ? 1<<2 : 0;
    nRetVal+= strstr(keys, "4") ? 1<<3 : 0;
    nRetVal+= strstr(keys, "5") ? 1<<4 : 0;
    nRetVal+= strstr(keys, "6") ? 1<<5 : 0;
    nRetVal+= strstr(keys, "7") ? 1<<6 : 0;
    nRetVal+= strstr(keys, "8") ? 1<<7 : 0;
    nRetVal+= strstr(keys, "9") ? 1<<8 : 0;
    return nRetVal;
}

#define MENU_MSG_TEXTCHUNK_SIZE 250
void UTIL_SendMenu(int userid, int seconds, const char *msg, const char *keys)
{
		MRecipientFilter filter;
        int nDuration = seconds == 0 ? -1 : seconds;
        const char *msg_portion = msg;
        int nMenuMsg = g_pUserMessages->GetMessageType("ShowMenu");
        if (userid>0)
        {
            short nOptions = 1023;
            if (keys) nOptions = UTIL_GetMenuOptions(keys);
    	    filter.AddPlayer(userid, maxplayers, playerinfomanager);
	        while ( strlen(msg_portion) >= MENU_MSG_TEXTCHUNK_SIZE )
	        {
		        // split the string
		        char sbuf[MENU_MSG_TEXTCHUNK_SIZE+1];
		        Q_strncpy( sbuf, msg_portion, MENU_MSG_TEXTCHUNK_SIZE+1 );
		        msg_portion += MENU_MSG_TEXTCHUNK_SIZE;


		        // send the string portion
		        bf_write *pBuffer = g_pUserMessages->UserMessageBegin( &filter, nMenuMsg, NULL );
			        pBuffer->WriteShort( nOptions );
			        pBuffer->WriteChar( nDuration );		// display time (-1 means unlimited)
			        pBuffer->WriteByte( TRUE );				// there is more message to come
			        pBuffer->WriteString( sbuf );
		        engine->MessageEnd();
	        }

	        // send the remaining string
		    bf_write *pBuffer = g_pUserMessages->UserMessageBegin( &filter, nMenuMsg, NULL );
			    pBuffer->WriteShort( nOptions );
			    pBuffer->WriteChar( nDuration );		// display time (-1 means unlimited)
			    pBuffer->WriteByte( FALSE );				// there is no more message to come
			    pBuffer->WriteString( msg_portion );
		    engine->MessageEnd();
        }
}

// Send a HUD message to a single player.
void SendMessage(int nUserid, int nType, const char *szMsg)
{
    // MRecipientFilter from hl2coding tutorial
    MRecipientFilter mrf;
    mrf.AddPlayer(nUserid, maxplayers, playerinfomanager);
    bf_write *pWrite;
    if (nType == COLORMESSAGE)
    {
        nType = g_pUserMessages->GetMessageType("TextMsg");
    }
    else if (nType == LARGEMESSAGE)
    {
        nType = g_pUserMessages->GetMessageType("SayText");
    }
    pWrite = g_pUserMessages->UserMessageBegin((IRecipientFilter *) &mrf,nType, NULL);
    pWrite->WriteByte(HUD_PRINTTALK); 
    pWrite->WriteString(szMsg); 
    engine->MessageEnd();
}

// Send a HUD message to every non-bot
void SendAllMessage(int nType, const char *szMsg) 
{
    if (nType == COLORMESSAGE)
    {
        nType = g_pUserMessages->GetMessageType("TextMsg");
    }
    else if (nType == LARGEMESSAGE)
    {
        nType = g_pUserMessages->GetMessageType("SayText");
    }

    // MRecipientFilter from hl2coding tutorial
    MRecipientFilter mrf;
    mrf.AddAllPlayers(maxplayers, playerinfomanager);
    bf_write *pWrite;
    pWrite = g_pUserMessages->UserMessageBegin((IRecipientFilter *) &mrf,nType, NULL);
    pWrite->WriteByte(HUD_PRINTTALK); 
    pWrite->WriteString(szMsg); 
    engine->MessageEnd();
    // broadcast to console, too.
    DxMsg(0, __FUNCTION__, __LINE__,szMsg);
}
