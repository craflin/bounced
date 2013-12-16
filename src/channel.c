/*
         file: channel.c
   desciption: channel handlers
        begin: 12/09/03
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bounced.h"

PPROFILECHANNEL ProfileChannelCreate(PPROFILE pProfile, const char* strName, const char* strKey)
{
	PPROFILECHANNEL pProfileChannel;

	ASSERT(pProfile);

	if( !(pProfileChannel = CALLOC(PPROFILECHANNEL,1,sizeof(PROFILECHANNEL))) ||
		!(pProfileChannel->pProfile = pProfile) ||
		!(pProfileChannel->strName = STRDUP(char*,strName)) ||
		!(pProfileChannel->strLogNick = STRDUP(char*,pProfile->strNick)) ||
		(strKey && !(pProfileChannel->strKey = STRDUP(char*,strKey))) )
	{
		OUTOFMEMORY;
		if(pProfileChannel)
			ProfileChannelFree(pProfileChannel);
		return 0;
	}

	HASHLIST_ADD(PROFILE_PROFILECHANNEL_HASHLIST,pProfile->hashlistProfileChannels,pProfileChannel);

	if(!ConfigFindVar(pProfile->c_strChannels,strName))
	{
		if(strKey)
		{
			strformat(g_strOutBuffer,sizeof(g_strOutBuffer),"%s %s",strName,strKey);
			ProfileConfigAddVar(pProfile,"Channels",g_strOutBuffer,0);
		}
		else
			ProfileConfigAddVar(pProfile,"Channels",strName,0);
	}

	if(*pProfile->c_strLogChannels == '*' || ConfigFindVar(pProfile->c_strLogChannels,strName))
		pProfileChannel->bLog = 1;

	return pProfileChannel;
}

void ProfileChannelFree(PPROFILECHANNEL pProfileChannel)
{
	ProfileConfigRemoveVar(pProfileChannel->pProfile,pProfileChannel->pProfile->c_strChannels,pProfileChannel->strName);

	LIST_REMOVE_ALL(PROFILECHANNEL_PROFILELOGMSG_LIST,pProfileChannel->listProfileLogMsgs);

	HASHLIST_REMOVE_ALL(PROFILECHANNEL_PROFILECHANNELUSER_HASHLIST,pProfileChannel->hashlistProfileChannelUsers);
	if(pProfileChannel->strMode)
		FREE(char*,pProfileChannel->strMode);
	if(pProfileChannel->strTopic)
		FREE(char*,pProfileChannel->strTopic);
	if(pProfileChannel->strTopicSetBy)
		FREE(char*,pProfileChannel->strTopicSetBy);

	HASHLIST_REMOVE_ALL(PROFILECHANNEL_PROFILECHANNELUSER_HASHLIST,pProfileChannel->hashlistLogProfileChannelUsers);
	if(pProfileChannel->strLogMode)
		FREE(char*,pProfileChannel->strLogMode);
	if(pProfileChannel->strLogTopic)
		FREE(char*,pProfileChannel->strLogTopic);
	if(pProfileChannel->strLogTopicSetBy)
		FREE(char*,pProfileChannel->strLogTopicSetBy);
	if(pProfileChannel->strLogNick)
		FREE(char*,pProfileChannel->strLogNick);

	if(pProfileChannel->strName)
		FREE(char*,pProfileChannel->strName);
	if(pProfileChannel->strKey)
		FREE(char*,pProfileChannel->strKey);
	FREE(PPROFILECHANNEL,pProfileChannel);
}

void ProfileChannelClose(PPROFILECHANNEL pProfileChannel)
{
	PPROFILE pProfile = pProfileChannel->pProfile;
	HASHLIST_REMOVE_ITEM(PROFILE_PROFILECHANNEL_HASHLIST,pProfile->hashlistProfileChannels,pProfileChannel);
}

PPROFILECHANNELUSER ProfileChannelUserCreate(PPROFILECHANNEL pProfileChannel, char* strNick, char cPrefix, char bLog)
{
	PPROFILECHANNELUSER pProfileChannelUser;

	if( !(pProfileChannelUser = CALLOC(PPROFILECHANNELUSER,1,sizeof(PROFILECHANNELUSER))) ||
		!(pProfileChannelUser->strNick = STRDUP(char*,strNick)) )
	{
		OUTOFMEMORY;
		if(pProfileChannelUser)
			ProfileChannelUserFree(pProfileChannelUser);
		return 0;
	}

	pProfileChannelUser->pProfileChannel = pProfileChannel;
	pProfileChannelUser->cPrefix = cPrefix;
#ifdef DEBUG
	pProfileChannelUser->bLog = bLog;
#endif

	if(bLog)
		HASHLIST_ADD(PROFILECHANNEL_PROFILECHANNELUSER_HASHLIST,pProfileChannel->hashlistLogProfileChannelUsers,pProfileChannelUser)
	else
		HASHLIST_ADD(PROFILECHANNEL_PROFILECHANNELUSER_HASHLIST,pProfileChannel->hashlistProfileChannelUsers,pProfileChannelUser);

	return pProfileChannelUser;
}

void ProfileChannelUserFree(PPROFILECHANNELUSER pProfileChannelUser)
{
	if(pProfileChannelUser->strNick)
		FREE(char*,pProfileChannelUser->strNick);
		
	FREE(PPROFILECHANNELUSER,pProfileChannelUser);
}

void ProfileChannelUserClose(PPROFILECHANNELUSER pProfileChannelUser, char bLog)
{
	PPROFILECHANNEL pProfileChannel = pProfileChannelUser->pProfileChannel;

	ASSERT(pProfileChannelUser->bLog == bLog);
	if(bLog)
		HASHLIST_REMOVE_ITEM(PROFILECHANNEL_PROFILECHANNELUSER_HASHLIST,pProfileChannel->hashlistLogProfileChannelUsers,pProfileChannelUser)
	else
		HASHLIST_REMOVE_ITEM(PROFILECHANNEL_PROFILECHANNELUSER_HASHLIST,pProfileChannel->hashlistProfileChannelUsers,pProfileChannelUser);
}

int ProfileChannelUserUpdatePrefix(PPROFILECHANNELUSER pProfileChannelUser, char cPrefix, char cAction)
{
	if(cAction == '+')
	{
		if(pProfileChannelUser->cPrefix)
		{
			PPROFILE pProfile = pProfileChannelUser->pProfileChannel->pProfile;
			if(strchr(pProfile->strPrefix,cPrefix) > strchr(pProfile->strPrefix,pProfileChannelUser->cPrefix))
				return 0;
		}
		pProfileChannelUser->cPrefix = cPrefix;
	}
	else
	{
		if(pProfileChannelUser->cPrefix == cPrefix)
			pProfileChannelUser->cPrefix = 0;
	}
	return 1;
}

/* server handlers */

int ServerHandlerJoin(PSERVER pServer, char* strCommand, char* strParams)
{
	if( CompareIRCPrefixNick(strCommand,pServer->strNick) )
	{
		char* strArg[1];
		PPROFILE pProfile = pServer->pProfile;
		PPROFILECHANNEL pProfileChannel;

		if(SplitIRCParams(strParams,strArg,sizeof(strArg)/sizeof(*strArg),0) != sizeof(strArg)/sizeof(*strArg))
			return 1;

		HASHLIST_LOOKUP(PROFILE_PROFILECHANNEL_HASHLIST,pProfile->hashlistProfileChannels,strArg[0],pProfileChannel);
		if(!pProfileChannel)
			return 1;

		pProfileChannel->bSyncing = 0;
		pProfileChannel->bSynced = 1;
		ProfileSendFormatAndLog(pProfile,pProfileChannel,PLMF_TIMESTAMP|PLMF_ADJUSTNICKLEN,":%s PRIVMSG %s :* Sync completed\r\n",c_strBouncerName,pProfileChannel->strName);

		if(!ConnectionSendFormat(pServer->pConnection,"MODE %s\r\n",pProfileChannel->strName))
			return 1;

		return 1;
	}

	/* hook for log message */
	{
		char* strArg[1];
		char* strNick;

		if( SplitIRCParams(strParams,strArg,sizeof(strArg)/sizeof(*strArg),0) == sizeof(strArg)/sizeof(*strArg) &&
			SplitIRCPrefixNick(strCommand,&strNick) )
		{
			PPROFILECHANNEL pProfileChannel;
			PPROFILE pProfile;

			pProfile = pServer->pProfile;
			HASHLIST_LOOKUP(PROFILE_PROFILECHANNEL_HASHLIST,pProfile->hashlistProfileChannels,strArg[0],pProfileChannel);
			if(pProfileChannel)
			{
				ProfileChannelUserCreate(pProfileChannel,strNick,0,0);
/*				ProfileLogMsgCreate(0,pProfileChannel,++pProfile->nLogID,g_strCurrentCommand,g_nCurrentCommandLength,PLMF_TIMESTAMP); */
				{
					char* str = strNick+strlen(strNick);
					if(g_strCurrentCommand[str-strCommand] == '!')
					{
						*str = '!';
						str = strchr(++str,' ');
						ASSERT(str);
						*str = '\0';
					}

					ProfileLogMsgCreateFormat(0,pProfileChannel,++pProfile->nLogID,PLMF_TIMESTAMP,":%s JOIN %s\r\n",strNick,strArg[0]); /* to make sure that the timestamp will be placed rightly */
				}
			}
		}
	}

	return 0;
}

int ServerHandlerPart(PSERVER pServer, char* strCommand, char* strParams)
{
	if( CompareIRCPrefixNick(strCommand,pServer->strNick) )
	{
		char* strArg[1];
		PPROFILE pProfile = pServer->pProfile;
		PPROFILECHANNEL pProfileChannel;

		if(SplitIRCParams(strParams,strArg,sizeof(strArg)/sizeof(*strArg),0) != sizeof(strArg)/sizeof(*strArg))
			return 1;

		HASHLIST_LOOKUP(PROFILE_PROFILECHANNEL_HASHLIST,pProfile->hashlistProfileChannels,strArg[0],pProfileChannel);
		if(!pProfileChannel)
			return 1;

		pProfileChannel->bSynced = 0;
		ProfileSendFormatAndLog(pProfile,pProfileChannel,PLMF_TIMESTAMP|PLMF_ADJUSTNICKLEN,":%s PRIVMSG %s :* Sync lost\r\n",c_strBouncerName,pProfileChannel->strName);
		return 1;
	}


	/* hook for log message */
	{
		char* strArg[1];
		char* strNick;

		if( SplitIRCParams(strParams,strArg,sizeof(strArg)/sizeof(*strArg),0) == sizeof(strArg)/sizeof(*strArg) &&
			SplitIRCPrefixNick(strCommand,&strNick) )
		{
			PPROFILECHANNEL pProfileChannel;
			PPROFILE pProfile;
			
			pProfile = pServer->pProfile;
			HASHLIST_LOOKUP(PROFILE_PROFILECHANNEL_HASHLIST,pProfile->hashlistProfileChannels,strArg[0],pProfileChannel);
			if(pProfileChannel)
			{
				PPROFILECHANNELUSER pProfileChannelUser;

				HASHLIST_LOOKUP(PROFILECHANNEL_PROFILECHANNELUSER_HASHLIST,pProfileChannel->hashlistProfileChannelUsers,strNick,pProfileChannelUser);
				if(pProfileChannelUser)
				{
					ProfileLogMsgCreate(0,pProfileChannel,++pProfile->nLogID,g_strCurrentCommand,g_nCurrentCommandLength,PLMF_TIMESTAMP);
					ProfileChannelUserClose(pProfileChannelUser,0);
				}
			}
		}
	}

	return 0;
}

int ServerHandlerKick(PSERVER pServer, char* strCommand, char* strParams)
{
	if(CompareIRCAddressNick(strParams,pServer->strNick))
	{
		char* strArg[3];
		unsigned int nArgs;
		PPROFILE pProfile = pServer->pProfile;
		PPROFILECHANNEL pProfileChannel;
		char* strNick;

		if((nArgs = SplitIRCParams(strParams,strArg,sizeof(strArg)/sizeof(*strArg),1)) < 1)
			return 1;

		HASHLIST_LOOKUP(PROFILE_PROFILECHANNEL_HASHLIST,pProfile->hashlistProfileChannels,strArg[0],pProfileChannel);
		if(!pProfileChannel)
			return 1;

		pProfileChannel->bSynced = 0;
		if(SplitIRCPrefix(strCommand,&strNick))
			ProfileSendFormatAndLog(pProfile,pProfileChannel,PLMF_TIMESTAMP|PLMF_ADJUSTNICKLEN,":%s PRIVMSG %s :* Sync lost (kicked by %s%s%s)\r\n",c_strBouncerName,pProfileChannel->strName,strNick,nArgs > 2 ? ": " : "",nArgs > 2 ? strArg[2] : "");
		else
			ProfileSendFormatAndLog(pProfile,pProfileChannel,PLMF_TIMESTAMP|PLMF_ADJUSTNICKLEN,":%s PRIVMSG %s :* Sync lost (kicked%s%s)\r\n",c_strBouncerName,pProfileChannel->strName,nArgs > 2 ? ": " : "",nArgs > 2 ? strArg[2] : "");

		if(pProfile->c_bChannelRejoin)
		{
			pProfileChannel->bSyncing = 1;
			ProfileSendFormatAndLog(pProfile,pProfileChannel,PLMF_TIMESTAMP|PLMF_ADJUSTNICKLEN,":%s PRIVMSG %s :* Syncing...\r\n",c_strBouncerName,pProfileChannel->strName);
			ConnectionSendFormat(pServer->pConnection,pProfileChannel->strKey ? "JOIN %s :%s\r\n" : "JOIN %s\r\n",pProfileChannel->strName,pProfileChannel->strKey);
		}

		return 1;
	}

	/* hook for log message */
	{
		char* strArg[2];

		if( SplitIRCParams(strParams,strArg,sizeof(strArg)/sizeof(*strArg),0) == sizeof(strArg)/sizeof(*strArg) )
		{
			PPROFILECHANNEL pProfileChannel;
			PPROFILE pProfile;

			pProfile = pServer->pProfile;
			HASHLIST_LOOKUP(PROFILE_PROFILECHANNEL_HASHLIST,pProfile->hashlistProfileChannels,strArg[0],pProfileChannel);
			if(pProfileChannel)
			{
				PPROFILECHANNELUSER pProfileChannelUser;

				HASHLIST_LOOKUP(PROFILECHANNEL_PROFILECHANNELUSER_HASHLIST,pProfileChannel->hashlistProfileChannelUsers,strArg[1],pProfileChannelUser);
				if(pProfileChannelUser)
				{
					ProfileLogMsgCreate(0,pProfileChannel,++pProfile->nLogID,g_strCurrentCommand,g_nCurrentCommandLength,PLMF_TIMESTAMP);
					ProfileChannelUserClose(pProfileChannelUser,0);
				}
			}
		}
	}

	return 0;
}

int ServerHandlerJoinError(PSERVER pServer, char* strCommand, char* strParams)
{
	char* strArg[2];
	unsigned int nArgs;
	PPROFILE pProfile;
	PPROFILECHANNEL pProfileChannel;

	if((nArgs = SplitIRCParams(strParams,strArg,sizeof(strArg)/sizeof(*strArg),1)) < 1)
		return 1;

	pProfile = pServer->pProfile;
	HASHLIST_LOOKUP(PROFILE_PROFILECHANNEL_HASHLIST,pProfile->hashlistProfileChannels,strArg[0],pProfileChannel);

	if(!pProfileChannel || pProfileChannel->bSynced)
		return 0;

	pProfileChannel->bSyncing = 0;
	ProfileSendFormatAndLog(pProfile,pProfileChannel,PLMF_TIMESTAMP|PLMF_ADJUSTNICKLEN,":%s PRIVMSG %s :* Sync failed (%s)\r\n",c_strBouncerName,pProfileChannel->strName,nArgs > 1 ? strArg[1] : "");

	return 1;
	strCommand = 0;
}

/* client handlers */

int ClientHandlerJoin(PCLIENT pClient, char* strCommand, char* strParams)
{
	char* strArg[2];
	unsigned int nArgs;
	PPROFILE pProfile;
	PPROFILECHANNEL pProfileChannel;
	PCLIENT pCurClient;

	if((nArgs = SplitIRCParams(strParams,strArg,sizeof(strArg)/sizeof(*strArg),0)) < 1)
	{
		ConnectionSendFormat(pClient->pConnection,":%s 461 %s JOIN :Not enough parameters\r\n",c_strBouncerName,pClient->strNick);
		return 1;
	}

	pProfile = pClient->pProfile;

	{
		char* strChannel = strArg[0],* strKey = nArgs > 1 ? strArg[1] : 0;
		char* str,* strK = 0;

		while(strChannel)
		{
			str = strchr(strChannel,',');
			if(str)
				*(str++) =  '\0';
			if(strKey)
			{
				strK = strchr(strKey,',');
				if(strK)
					*(strK++) = '\0';
				if(!*strKey)
					strKey = 0;
			}

			if(*strChannel)
			{
				if( !pProfile )
				{
					ConnectionSendFormat(pClient->pConnection,":%s 437 %s %s :Nick/channel is temporarily unavailable\r\n",c_strBouncerName,pClient->strNick,strChannel);
				}
				else
				{
					HASHLIST_LOOKUP(PROFILE_PROFILECHANNEL_HASHLIST,pProfile->hashlistProfileChannels,strChannel,pProfileChannel);

					/* received "/join <channel>" altough <channel> is already joined */
					if( pProfileChannel )
					{
						/* try to resync */
						if(!pProfileChannel->bSynced && !pProfileChannel->bSyncing)
						{							
							/* update key */
							if(strKey && *strKey)
							{
								if(pProfileChannel->strKey)
								{
									FREE(char*,pProfileChannel->strKey);
									pProfileChannel->strKey = 0;
								}
								ProfileConfigRemoveVar(pProfile,pProfile->c_strChannels,pProfileChannel->strName);

								strformat(g_strOutBuffer,sizeof(g_strOutBuffer),"%s %s",pProfileChannel->strName,strKey);
								ProfileConfigAddVar(pProfile,"Channels",g_strOutBuffer,0);
								pProfileChannel->strKey = STRDUP(char*,strKey);
							}

							/* send join command to server */
							if(pProfile->pServer && pProfile->pServer->bRegistered)
							{
								PSERVER pServer = pProfile->pServer;
								pProfileChannel->bSyncing = 1;
								ProfileSendFormatAndLog(pProfile,pProfileChannel,PLMF_TIMESTAMP|PLMF_ADJUSTNICKLEN,":%s PRIVMSG %s :* Syncing...\r\n",c_strBouncerName,pProfileChannel->strName);
								ConnectionSendFormat(pServer->pConnection,pProfileChannel->strKey ? "JOIN %s :%s\r\n" : "JOIN %s\r\n",pProfileChannel->strName,pProfileChannel->strKey);
							}
						}

					}
					else
					{
						if( !(pProfileChannel = ProfileChannelCreate(pProfile,strChannel,strKey)) )
						{
							ConnectionCloseAsync(pClient->pConnection);
							return 1;
						}
						
						for(pCurClient = pProfile->listClients.pFirst; pCurClient; pCurClient = pCurClient->llProfile.pNext)
							ConnectionSendFormat(pCurClient->pConnection,":%s!~%s@%s JOIN %s\r\n",pCurClient->strNick,pCurClient->strName,iptoa(pCurClient->pConnection->nIP),pProfileChannel->strName);

						/* send join command to server */
						if(pProfile->pServer && pProfile->pServer->bRegistered)
						{
							PSERVER pServer = pProfile->pServer;
							pProfileChannel->bSyncing = 1;
							ProfileSendFormatAndLog(pProfile,pProfileChannel,PLMF_TIMESTAMP|PLMF_ADJUSTNICKLEN,":%s PRIVMSG %s :* Syncing...\r\n",c_strBouncerName,pProfileChannel->strName);
							ConnectionSendFormat(pServer->pConnection,pProfileChannel->strKey ? "JOIN %s :%s\r\n" : "JOIN %s\r\n",pProfileChannel->strName,pProfileChannel->strKey);
						}
					}
				}
			}

			if(str)
				str[-1] = ',';
			if(strK)
				strK[-1] = ',';

			strChannel = str;
			strKey = strK;
		}
	}

	return 1;
	strCommand = 0;
}

int ClientHandlerPart(PCLIENT pClient, char* strCommand, char* strParams)
{
	char* strArg[1];
	PPROFILE pProfile;
	PPROFILECHANNEL pProfileChannel;
	PCLIENT pCurClient;
	
	if(SplitIRCParams(strParams,strArg,sizeof(strArg)/sizeof(*strArg),0) != sizeof(strArg)/sizeof(*strArg))
	{
		ConnectionSendFormat(pClient->pConnection,":%s 461 %s PART :Not enough parameters\r\n",c_strBouncerName,pClient->strNick);
		return 1;
	}

	pProfile = pClient->pProfile;

	{
		char* strChannel = strArg[0];
		char* str;

		while(strChannel)
		{
			str = strchr(strChannel,',');
			if(str)
				*(str++) =  '\0';

			if(*strChannel)
			{
				if( !pClient->pProfile )
				{
					ConnectionSendFormat(pClient->pConnection,":%s 442 %s %s :You're not on that channel\r\n",c_strBouncerName,pClient->strNick,strChannel);
				}
				else
				{
					HASHLIST_LOOKUP(PROFILE_PROFILECHANNEL_HASHLIST,pProfile->hashlistProfileChannels,strChannel,pProfileChannel);

					if( !pProfileChannel )
					{
						ConnectionSendFormat(pClient->pConnection,":%s 442 %s %s :You're not on that channel\r\n",c_strBouncerName,pClient->strNick,strChannel);	
					}
					else
					{
						for(pCurClient = pProfile->listClients.pFirst; pCurClient; pCurClient = pCurClient->llProfile.pNext)
							ConnectionSendFormat(pCurClient->pConnection,":%s!~%s@%s PART %s\r\n",pCurClient->strNick,pCurClient->strName,iptoa(pCurClient->pConnection->nIP),pProfileChannel->strName);

						if(pProfileChannel->bSynced && pProfile->pServer && pProfile->pServer->bRegistered)
							ConnectionSendFormat(pProfile->pServer->pConnection,"PART %s\r\n",pProfileChannel->strName);

						ProfileChannelClose(pProfileChannel);
					}
				}
			}

			if(str)
				str[-1] = ',';

			strChannel = str;
		}
	}

	return 1;
	strCommand = 0;
}

/* ??? */

int ProfileChannelAttach(PPROFILECHANNEL pProfileChannel, PCLIENT pClient)
{
	if(strcmp(pClient->strNick,pProfileChannel->strLogNick))
	{
		char* strNick;

		if(!(strNick = STRDUP(char*,pProfileChannel->strLogNick)))
		{
			OUTOFMEMORY;
			ConnectionCloseAsync(pClient->pConnection);
			return 0;
		}

		if(!ConnectionSendFormat(pClient->pConnection,":%s!~%s@%s NICK :%s\r\n",pClient->strNick,pClient->strName,iptoa(pClient->pConnection->nIP),strNick))
		{
			FREE(char*,strNick);
			return 0;
		}

		if(pClient->strNick)
			FREE(char*,pClient->strNick);
		pClient->strNick = strNick;
	}

	if(!ConnectionSendFormat(pClient->pConnection,":%s!~%s@%s JOIN %s\r\n",pProfileChannel->strLogNick,pClient->strName,iptoa(pClient->pConnection->nIP),pProfileChannel->strName))
		return 0;

	if(pProfileChannel->hashlistLogProfileChannelUsers.pFirst)
	{
		char c = '=';
		char *str,*strStart;
		PPROFILECHANNELUSER pProfileChannelUser;
		int i;

		if(pProfileChannel->strLogMode)
		{
			if(strchr(pProfileChannel->strLogMode,'s'))
				c = '@';
			else if(strchr(pProfileChannel->strLogMode,'p'))
				c = '*';
		}

		strStart = g_strOutBuffer+strformat(g_strOutBuffer,sizeof(g_strOutBuffer)-2,":%s 353 %s %c %s :",c_strBouncerName,pClient->strNick,c,pProfileChannel->strName);
		str = strStart;
		
		for(pProfileChannelUser = pProfileChannel->hashlistLogProfileChannelUsers.pFirst; pProfileChannelUser; pProfileChannelUser = pProfileChannelUser->hllProfileChannel.pNext)
		{
			ASSERT(pProfileChannelUser->bLog);
			i = strlen(pProfileChannelUser->strNick);
			if(str-g_strOutBuffer+i+(pProfileChannelUser->cPrefix ? 1 : 0)+(str != strStart ? 1 : 0) >= 255-2)
			{
				if(str == strStart)
					break;
				*(str++) = '\r';
				*(str++) = '\n';
				*str = '\0';

				if(!ConnectionSend(pClient->pConnection,g_strOutBuffer,str-g_strOutBuffer))
					return 0;

				str = strStart;

				if(str-g_strOutBuffer+i+(pProfileChannelUser->cPrefix ? 1 : 0) >= 255-2)
				{
					ASSERT(0);
					break;
				}
			}

			if(str != strStart)
				*(str++) = ' ';
			if(pProfileChannelUser->cPrefix)
				*(str++) = pProfileChannelUser->cPrefix;
			memcpy(str,pProfileChannelUser->strNick,i);
			str += i;
		}
		ASSERT(!pProfileChannelUser);
		if(str != strStart)
		{
			*(str++) = '\r';
			*(str++) = '\n';
			*str = '\0';
			if(!ConnectionSend(pClient->pConnection,g_strOutBuffer,str-g_strOutBuffer))
				return 0;
		}

		if( !ConnectionSendFormat(pClient->pConnection,":%s 366 %s %s :End of NAMES list\r\n",c_strBouncerName,pClient->strNick,pProfileChannel->strName) )
			return 0;
	}

#ifdef DEBUG
	{
		PPROFILECHANNELUSER pProfileChannelUser;
		for(pProfileChannelUser = pProfileChannel->hashlistProfileChannelUsers.pFirst; pProfileChannelUser; pProfileChannelUser = pProfileChannelUser->hllProfileChannel.pNext)
			ASSERT(!pProfileChannelUser->bLog);
		for(pProfileChannelUser = pProfileChannel->hashlistLogProfileChannelUsers.pFirst; pProfileChannelUser; pProfileChannelUser = pProfileChannelUser->hllProfileChannel.pNext)
			ASSERT(pProfileChannelUser->bLog);
	}
#endif

	if(pProfileChannel->strLogTopic)
	{
		if( !ConnectionSendFormat(pClient->pConnection,":%s 332 %s %s :%s\r\n",c_strBouncerName,pClient->strNick,pProfileChannel->strName,pProfileChannel->strLogTopic) ||
			(pProfileChannel->strLogTopicSetBy && !ConnectionSendFormat(pClient->pConnection,":%s 333 %s %s %s %u\r\n",c_strBouncerName,pClient->strNick,pProfileChannel->strName,pProfileChannel->strLogTopicSetBy,pProfileChannel->timeLogTopicSetTime)) )
			return 0;
	}

	if(pProfileChannel->strLogMode)
	{
		if( !ConnectionSendFormat(pClient->pConnection,":%s 324 %s %s +%s\r\n",c_strBouncerName,pClient->strNick,pProfileChannel->strName,pProfileChannel->strLogMode) ||
			(pProfileChannel->timeLogCreate && !ConnectionSendFormat(pClient->pConnection,":%s 329 %s %s %u\r\n",c_strBouncerName,pClient->strNick,pProfileChannel->strName,pProfileChannel->timeLogCreate)) )
			return 0;
	}

	return 1;
}
