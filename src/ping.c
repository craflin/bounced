/*
         file: ping.c
   desciption: ping handlers
        begin: 11/09/03
    copyright: (C) 2003 by Colin Graf
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

/* unregistered server handlers */

int ServerHandlerUnregisteredPing(PSERVER pServer, char* strCommand, char* strParams)
{
	char* strArg[1];

	if(SplitIRCParams(strParams,strArg,sizeof(strArg)/sizeof(*strArg),0) != sizeof(strArg)/sizeof(*strArg))
		return 1;

	ConnectionSendFormat(pServer->pConnection,"PONG :%s\r\n",strArg[0]);
	return 1;
	strCommand = 0;
}

/* server handlers */

int ServerHandlerPing(PSERVER pServer, char* strCommand, char* strParams)
{
	char* strArg[1];
	PCLIENT pClient;

	if(SplitIRCParams(strParams,strArg,sizeof(strArg)/sizeof(*strArg),0) != sizeof(strArg)/sizeof(*strArg))
		return 1;

	{
		PCLIENT pNextClient;
		for(pClient = pServer->pProfile->listClients.pFirst; pClient; pClient = pNextClient)
		{
			pNextClient = pClient->llProfile.pNext;
			if(pClient->bWantPong)
			{
				if(ConnectionSendFormat(pClient->pConnection,"ERROR :Closing Link: %s (Ping timeout)\r\n",iptoa(pClient->pConnection->nIP)))
					ConnectionCloseAsync(pClient->pConnection);
				ASSERT(pClient->pProfile == pServer->pProfile);
				ProfileDetach(pServer->pProfile,pClient,"Ping timeout");	
			}
		}
	}

	if(!ConnectionSendFormat(pServer->pConnection,"PONG :%s\r\n",strArg[0]))
		return 1;

	for(pClient = pServer->pProfile->listClients.pFirst; pClient; pClient = pClient->llProfile.pNext)
	{
		if(!ConnectionSendFormat(pClient->pConnection,"PING :%s\r\n",iptoa(pClient->pConnection->nIP)))
			continue;
		pClient->bWantPong = 1;
	}

	return 1;
	strCommand = 0;
}

/* client handlers */

int ClientHandlerPing(PCLIENT pClient, char* strCommand, char* strParams)
{
	char* strArg[2];
	unsigned int nArgc;
	PPROFILE pProfile;

	if((nArgc = SplitIRCParams(strParams,strArg,sizeof(strArg)/sizeof(*strArg),0)) < 1)
	{
		ConnectionSendFormat(pClient->pConnection,":%s 409 %s :No origin specified\r\n",c_strBouncerName,pClient->strNick);
		return 1;
	}

	pProfile = pClient->pProfile;
	if( !pProfile || !pProfile->pServer || !pProfile->pServer->bRegistered)
	{
		if(nArgc > 1)
			ConnectionSendFormat(pClient->pConnection,"PONG %s :%s\r\n",strArg[0],strArg[1]);
		else
			ConnectionSendFormat(pClient->pConnection,"PONG :%s\r\n",strArg[0]);
		return 1;
	}

	return 0;
	strCommand = 0;
}

int ClientHandlerPong(PCLIENT pClient, char* strCommand, char* strParams)
{
	char* strArg[1];

	if(SplitIRCParams(strParams,strArg,sizeof(strArg)/sizeof(*strArg),0) != sizeof(strArg)/sizeof(*strArg))
	{
		ConnectionSendFormat(pClient->pConnection,":%s 409 %s :No origin specified\r\n",c_strBouncerName,pClient->strNick);
		return 1;
	}

	if(!strcasecmp(strArg[0],iptoa(pClient->pConnection->nIP)))
	{
		pClient->bWantPong = 0;
		return 1;
	}

	return 0;
	strCommand = 0;
}

