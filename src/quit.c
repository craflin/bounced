/*
         file: quit.c
   desciption: handle quit messages
        begin: 02/06/04
    copyright: (C) 2004 by Colin Graf
        email: addition@users.sourceforge.net

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <string.h>

#include "bounced.h"

/* server handlers */

int ServerHandlerQuit(PSERVER pServer, char* strCommand, char* strParams)
{
	char* strNick;

	if( SplitIRCPrefixNick(strCommand,&strNick) )
	{
		PPROFILE pProfile = pServer->pProfile;
		PPROFILECHANNEL pProfileChannel;
		PPROFILECHANNELUSER pProfileChannelUser;

		pProfile->nLogID++;

		for(pProfileChannel = pProfile->hashlistProfileChannels.pFirst; pProfileChannel; pProfileChannel = pProfileChannel->hllProfile.pNext)
		{
			HASHLIST_LOOKUP(PROFILECHANNEL_PROFILECHANNELUSER_HASHLIST,pProfileChannel->hashlistProfileChannelUsers,strNick,pProfileChannelUser);
			if(pProfileChannelUser)
			{
				ProfileLogMsgCreate(0,pProfileChannel,pProfile->nLogID,g_strCurrentCommand,g_nCurrentCommandLength,PLMF_TIMESTAMP);
				ProfileChannelUserClose(pProfileChannelUser,0);
			}
		}
	}

	/* try to reuse the prefered nick */
	{
		char* strPreferedNick;
		if(!pServer->pProfile->listClients.pFirst && pServer->pProfile->c_strDetachNick && *pServer->pProfile->c_strDetachNick)
			strPreferedNick = pServer->pProfile->c_strDetachNick;
		else
			strPreferedNick = pServer->pProfile->c_strNick;
		if(!strcasecmp(strNick,strPreferedNick) && strcasecmp(pServer->strNick,strPreferedNick))
			ConnectionSendFormat(pServer->pConnection,"NICK :%s\r\n",strPreferedNick);
	}

	return 0;
	strParams = 0;
}

/* client handlers */

int ClientHandlerQuit(PCLIENT pClient, char* strCommand, char* strParams)
{
	char* strArg[1];
	unsigned int nArgs;

	nArgs = SplitIRCParams(strParams,strArg,sizeof(strArg)/sizeof(*strArg),1);

	ConnectionCloseAsync(pClient->pConnection);

	if(pClient->pProfile)
	{
		ProfileDetach(pClient->pProfile,pClient,nArgs > 0 ? strArg[0] : 0);
		ASSERT(pClient->pProfile == 0);
	}

	return 1;
	strCommand = 0;
}
