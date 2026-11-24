#ifndef ES_USERMSG_H
#define ES_USERMSG_H

#ifdef _WIN32
#pragma once
#endif

//=========================================================================
// Includes
//=========================================================================
#include "KeyValues.h"
#include "eiface.h"
#include "es_util.h"
#ifndef ORANGEBOX
#include "dlls/iplayerinfo.h"
#include "VirtualMachine/BlockArgs.h"
#else
#include "game/server/iplayerinfo.h"
#endif

/************************************************************************/
/* GUIDE:																*/
/*	Basically, all the user messages that are registered by the mod 	*/
/*	have keys created for them in one massive KeyValues instance		*/
/*	called m_pUserMessages. When a scripter wants to create a		    */
/*  a usermessage for use, a subkey is created inside m_pUserMessages	*/
/*	under the _ACTUAL_ usermessage name.								*/
/*																		*/
/*	Structure:															*/
/*		|-- m_pUserMessage												*/
/*			|-- Actual Usermsg name (i.e. SayText2)						*/
/*				| -- Script created usermessage instance.				*/
/************************************************************************/

//=========================================================================
// Usermessage class. Provides usermessage functionality across mods.
//=========================================================================
class CESUserMessages
{
	private:
		KeyValues			*m_pUserMessages;
		IServerGameDLL		*m_pGameDLL;
		IVEngineServer		*m_pEngine;
		IPlayerInfoManager	*m_pPlayerInfo;

	public:
		CESUserMessages( IServerGameDLL *pGameDLL, IVEngineServer *pEngine, IPlayerInfoManager *pInfoManager );
        ~CESUserMessages( void );

		int  GetMessageType( const char *msgname );
		void WriteUserMessage( KeyValues *pKey, bf_write *pWrite );
		void SendUserMessageData( const char *key, int nUserid );
		void WriteUserMessageData( const char *key, const char* type, const char *value );
		void UserMessageCommand( const CCommand &args,int nCount );
		
		// This function is to abstract across Source games
		bf_write *UserMessageBegin(IRecipientFilter * filter, int msg_type, const char * name);
};

// The singleton instance
extern CESUserMessages *g_pUserMessages;

#endif // ES_USERMSG_H

