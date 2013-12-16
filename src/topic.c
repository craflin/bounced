/*
         file: topic.c
   desciption: handle topic replays
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "bounced.h"

/* server handlers */

int ServerHandlerTopic(PSERVER pServer, char* strCommand, char* strParams)
{
	char* strArg[2];
	unsigned int nArgs;

	if((nArgs = SplitIRCParams(strParams,strArg,sizeof(strArg)/sizeof(*strArg),1)) >= 1)
	{
		PPROFILECHANNEL pProfileChannel;
		PPROFILE pProfile;

		pProfile = pServer->pProfile;
		HASHLIST_LOOKUP(PROFILE_PROFILECHANNEL_HASHLIST,pProfile->hashlistProfileChannels,strArg[0],pProfileChannel);
		if(pProfileChannel)
		{
			ProfileLogMsgCreate(0,pProfileChannel,++pProfile->nLogID,g_strCurrentCommand,g_nCurrentCommandLength,0);

			if(pProfileChannel->strTopic)
			{
				FREE(char*,pProfileChannel->strTopic);
				pProfileChannel->strTopic = 0;
			}
			if(pProfileChannel->strTopicSetBy)
			{
				FREE(char*,pProfileChannel->strTopicSetBy);
				pProfileChannel->strTopicSetBy = 0;
			}
			pProfileChannel->timeTopicSetTime = 0;

			if( nArgs > 1 && *strArg[1] && 
				(pProfileChannel->strTopic = STRDUP(char*,strArg[1])) )
			{
				char* str;

				if( SplitIRCPrefix(strCommand,&str) &&
					(pProfileChannel->strTopicSetBy = STRDUP(char*,str)) )
					pProfileChannel->timeTopicSetTime = g_timeNow;
			}
		}
	}

	return 0;
}

int ServerHandlerNumNoTopic(PSERVER pServer, char* strCommand, char* strParams)
{
	char* strArg[1];

	if(SplitIRCParams(strParams,strArg,sizeof(strArg)/sizeof(*strArg),0) == sizeof(strArg)/sizeof(*strArg))
	{
		PPROFILECHANNEL pProfileChannel;
		PPROFILE pProfile;

		pProfile = pServer->pProfile;
		HASHLIST_LOOKUP(PROFILE_PROFILECHANNEL_HASHLIST,pProfile->hashlistProfileChannels,strArg[0],pProfileChannel);
		if(pProfileChannel)
		{
			ProfileLogMsgCreate(0,pProfileChannel,++pProfile->nLogID,g_strCurrentCommand,g_nCurrentCommandLength,0);

			if(pProfileChannel->strTopic)
			{
				FREE(char*,pProfileChannel->strTopic);
				pProfileChannel->strTopic = 0;
			}
			if(pProfileChannel->strTopicSetBy)
			{
				FREE(char*,pProfileChannel->strTopicSetBy);
				pProfileChannel->strTopicSetBy = 0;
			}
			pProfileChannel->timeTopicSetTime = 0;
		}
	}

	return 0;
	strCommand = 0;
}

int ServerHandlerNumTopic(PSERVER pServer, char* strCommand, char* strParams)
{
	char* strArg[2];

	if(SplitIRCParams(strParams,strArg,sizeof(strArg)/sizeof(*strArg),1) == sizeof(strArg)/sizeof(*strArg))
	{
		PPROFILECHANNEL pProfileChannel;
		PPROFILE pProfile;

		pProfile = pServer->pProfile;
		HASHLIST_LOOKUP(PROFILE_PROFILECHANNEL_HASHLIST,pProfile->hashlistProfileChannels,strArg[0],pProfileChannel);
		if(pProfileChannel)
		{
			ProfileLogMsgCreate(0,pProfileChannel,++pProfile->nLogID,g_strCurrentCommand,g_nCurrentCommandLength,0);

			if(pProfileChannel->strTopic)
			{
				FREE(char*,pProfileChannel->strTopic);
				pProfileChannel->strTopic = 0;
			}
			if(pProfileChannel->strTopicSetBy)
			{
				FREE(char*,pProfileChannel->strTopicSetBy);
				pProfileChannel->strTopicSetBy = 0;
			}
			pProfileChannel->timeTopicSetTime = 0;
			pProfileChannel->strTopic = STRDUP(char*,strArg[1]);
		}
	}

	return 0;
	strCommand = 0;
}

int ServerHandlerNumTopicSetBy(PSERVER pServer, char* strCommand, char* strParams)
{
	char* strArg[3];

	if(SplitIRCParams(strParams,strArg,sizeof(strArg)/sizeof(*strArg),0) == sizeof(strArg)/sizeof(*strArg))
	{
		PPROFILECHANNEL pProfileChannel;
		PPROFILE pProfile;

		pProfile = pServer->pProfile;
		HASHLIST_LOOKUP(PROFILE_PROFILECHANNEL_HASHLIST,pProfile->hashlistProfileChannels,strArg[0],pProfileChannel);
		if(pProfileChannel)
		{
			ProfileLogMsgCreate(0,pProfileChannel,++pProfile->nLogID,g_strCurrentCommand,g_nCurrentCommandLength,0);

			if(pProfileChannel->strTopicSetBy)
			{
				FREE(char*,pProfileChannel->strTopicSetBy);
				pProfileChannel->strTopicSetBy = 0;
			}
			pProfileChannel->timeTopicSetTime = 0;

			if( (pProfileChannel->strTopicSetBy = STRDUP(char*,strArg[1])) )
				pProfileChannel->timeTopicSetTime = atoul(strArg[2]);
		}
	}

	return 0;
	strCommand = 0;
}
