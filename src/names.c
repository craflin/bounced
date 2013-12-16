/*
         file: names.c
   desciption: handle names replays
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

int ServerHandlerNumNamReply(PSERVER pServer, char* strCommand, char* strParams)
{
	char* strArg[3];

	if(SplitIRCParams(strParams,strArg,sizeof(strArg)/sizeof(*strArg),1) == sizeof(strArg)/sizeof(*strArg))
	{
		PPROFILECHANNEL pProfileChannel;
		PPROFILE pProfile;

		pProfile = pServer->pProfile;
		HASHLIST_LOOKUP(PROFILE_PROFILECHANNEL_HASHLIST,pProfile->hashlistProfileChannels,strArg[1],pProfileChannel);
		if(pProfileChannel)
		{
			char* str,*strEnd;
			char cPrefix;

			ProfileLogMsgCreate(0,pProfileChannel,++pProfile->nLogID,g_strCurrentCommand,g_nCurrentCommandLength,PLMF_LISTITEM);

			if(!pServer->cInList)
				HASHLIST_REMOVE_ALL(PROFILECHANNEL_PROFILECHANNELUSER_HASHLIST,pProfileChannel->hashlistProfileChannelUsers);
			pServer->cInList++;

			str = strArg[2];
			do
			{
				strEnd = strchr(str,' ');
				if(strEnd)
					*(strEnd++) = '\0';
				if(*str)
				{
					if(IsIRCNickPrefix(*str,pProfile->strPrefix))
					{
						cPrefix = *str;
						str++;
					}
					else
						cPrefix = 0;
					if(*str)
						ProfileChannelUserCreate(pProfileChannel,str,cPrefix,0);
				}
				str = strEnd;
			} while(str);
		}
	}

	return 0;
	strCommand = 0;
}

int ServerHandlerNumEndOfNames(PSERVER pServer, char* strCommand, char* strParams)
{
	char* strArg[1];

	if(SplitIRCParams(strParams,strArg,sizeof(strArg)/sizeof(*strArg),0) == sizeof(strArg)/sizeof(*strArg))
	{
		PPROFILECHANNEL pProfileChannel;
		PPROFILE pProfile;

		pProfile = pServer->pProfile;
		HASHLIST_LOOKUP(PROFILE_PROFILECHANNEL_HASHLIST,pProfile->hashlistProfileChannels,strArg[0],pProfileChannel);
		if(pProfileChannel)
			ProfileLogMsgCreate(0,pProfileChannel,++pProfile->nLogID,g_strCurrentCommand,g_nCurrentCommandLength,0);
	}

	pServer->cInList = 0;

	return 0;
	strCommand = 0;
}
