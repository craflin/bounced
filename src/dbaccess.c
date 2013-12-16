/*
         file: dbaccess.c
   desciption: handle db access commands
        begin: 01/05/04
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
#include <stdlib.h>
#if !defined(_WIN32) || defined(__CYGWIN__)
#include <unistd.h>
#include <arpa/inet.h>
#endif /* !defined(_WIN32) || defined(__CYGWIN__) */

#include "bounced.h"

/* unregistered client handlers */

int ClientHandlerUnregisteredDBAccess(PCLIENT pClient, char* strCommand, char* strParams)
{
	char* strArg[10];
	unsigned int nArgs;

	if(*strParams == ':')
		strParams++;

	if( !*c_strDBAccessPassword )
	{
		ConnectionSendFormat(pClient->pConnection,":%s 464 * :Password incorrect\r\n",c_strBouncerName);
		return 1;
	}

	nArgs = SplitLine(strParams,strArg,sizeof(strArg)/sizeof(*strArg),0);

	if( strcmp(strArg[0],c_strDBAccessPassword) )
	{
		ConnectionSendFormat(pClient->pConnection,":%s 464 * :Password incorrect\r\n",c_strBouncerName);
		return 1;
	}

	if(nArgs > 1)
	{
		if(!strcasecmp(strArg[1],"admin"))
		{
			if(nArgs > 2)
			{
				if(!strcasecmp(strArg[2],"user"))
				{
					if(nArgs > 3)
					{
						/* admin user get [<name>] */
						if(!strcasecmp(strArg[3],"get"))
						{
							PUSER pUser;
							char strPass[33];

							if(nArgs > 4)
							{
								HASHLIST_LOOKUP(USER_HASHLIST,g_hashlistUsers,strArg[4],pUser);
								if(!pUser)
								{
									ConnectionSend(pClient->pConnection,"DBFAIL :admin user get \"Unknown user\"\r\n",-1);
									return 1;
								}

								if(!ConnectionSend(pClient->pConnection,"DBLIST :user password email flags commands interface registered lastseen profiles clients\r\n",-1))
									return 1;

								PasswordToStr(pUser->pcMD5Pass,strPass,sizeof(strPass));
								if(!ConnectionSendFormat(pClient->pConnection,"DBITEM :%s %s \"%s\" \"%s\" \"%s\" \"%s\" %u %u %u %u\r\n",pUser->strName,strPass,pUser->strEmail ? pUser->strEmail : "",pUser->strFlags,pUser->strAllowedCommands ? pUser->strAllowedCommands : "",pUser->strConnectInterface ? pUser->strConnectInterface : "",(unsigned int)pUser->timeRegistered,(unsigned int)pUser->timeLastSeen,pUser->hashlistProfiles.nCount,pUser->listClients.nCount))
									return 1;

								if(!ConnectionSend(pClient->pConnection,"DBOK :admin user get\r\n",-1))
									return 1;
								
							}
							else
							{
								if(!ConnectionSend(pClient->pConnection,"DBLIST :user password email flags commands interface registered lastseen profiles clients\r\n",-1))
									return 1;

								for(pUser = g_hashlistUsers.pFirst; pUser; pUser = pUser->hll.pNext)
								{
									PasswordToStr(pUser->pcMD5Pass,strPass,sizeof(strPass));
									if(!ConnectionSendFormat(pClient->pConnection,"DBITEM :%s %s \"%s\" \"%s\" \"%s\" \"%s\" %u %u %u %u\r\n",pUser->strName,strPass,pUser->strEmail ? pUser->strEmail : "",pUser->strFlags,pUser->strAllowedCommands ? pUser->strAllowedCommands : "",pUser->strConnectInterface ? pUser->strConnectInterface : "",(unsigned int)pUser->timeRegistered,(unsigned int)pUser->timeLastSeen,pUser->hashlistProfiles.nCount,pUser->listClients.nCount))
										return 1;
								}

								if(!ConnectionSend(pClient->pConnection,"DBOK :admin user get\r\n",-1))
									return 1;
							}

							return 1;
						}
						/* admin user count */
						else if(!strcasecmp(strArg[3],"count"))
						{				
							if(!ConnectionSend(pClient->pConnection,"DBLIST :count\r\n",-1))
								return 1;

							if(!ConnectionSendFormat(pClient->pConnection,"DBITEM :%u\r\n",g_hashlistUsers.nCount))
								return 1;

							if(!ConnectionSend(pClient->pConnection,"DBOK :admin user count\r\n",-1))
								return 1;

							return 1;
						}
						/* admin user add <name> <password> [<email> [<flags> [<allowed commands> [<connect interface>]]]] */
						else if(!strcasecmp(strArg[3],"add"))
						{
							PUSER pUser;
							char pcMD5Pass[16];

							if(nArgs <= 5)
							{
								ConnectionSend(pClient->pConnection,"DBFAIL :admin user add \"Invalid params\"\r\n",-1);
								return 1;
							}
							
							HASHLIST_LOOKUP(USER_HASHLIST,g_hashlistUsers,strArg[4],pUser);
							if(pUser)
							{
								ConnectionSend(pClient->pConnection,"DBFAIL :admin user add \"Already exists\"\r\n",-1);
								return 1;
							}

							if(!UserIsNameValid(strArg[4]))
							{
								ConnectionSend(pClient->pConnection,"DBFAIL :admin user add \"Invalid username\"\r\n",-1);
								return 1;
							}

							if(!PasswordIsValid(strArg[5]) || !PasswordCreate(strArg[5],pcMD5Pass))
							{
								ConnectionSend(pClient->pConnection,"DBFAIL :admin user add \"Invalid password\"\r\n",-1);
								return 1;
							}

							if(!UserCreate(strArg[4],pcMD5Pass,nArgs > 6 ? strArg[6] : 0,nArgs > 7 ? strArg[7] : 0,nArgs > 8 ? strArg[8] : c_strDefaultAllowedCommands,nArgs > 9 ? strArg[9] : 0,g_timeNow,0))
							{
								ConnectionSend(pClient->pConnection,"DBFAIL :admin user add \"Out of memory\"\r\n",-1);
								return 1;
							}

							if(!ConnectionSend(pClient->pConnection,"DBOK :admin user add\r\n",-1))
								return 1;

							return 1;
						}
						/* admin user remove <name> */
						else if(!strcasecmp(strArg[3],"remove"))
						{
							PUSER pUser;

							if(nArgs <= 4)
							{
								ConnectionSend(pClient->pConnection,"DBFAIL :admin user remove \"Invalid params\"\r\n",-1);
								return 1;
							}

							HASHLIST_LOOKUP(USER_HASHLIST,g_hashlistUsers,strArg[4],pUser);
							if(!pUser)
							{
								ConnectionSend(pClient->pConnection,"DBFAIL :admin user remove \"Unknown user\"\r\n",-1);
								return 1;
							}

							if((pUser->strAllowedCommands && ConfigFindVar(pUser->strAllowedCommands,"ADMIN")) || pUser == pClient->pUser)
							{
								ConnectionSend(pClient->pConnection,"DBFAIL :admin user remove \"Can't remove admin user\"\r\n",-1);
								return 1;
							}

							UserClose(pUser);

							if(!ConnectionSend(pClient->pConnection,"DBOK :admin user remove\r\n",-1))
								return 1;

							return 1;
						}
						/* admin user flags <name> <newflags> */
						else if(!strcasecmp(strArg[3],"flags"))
						{
							PUSER pUser;

							if(nArgs <= 5)
							{
								ConnectionSend(pClient->pConnection,"DBFAIL :admin user flags \"Invalid params\"\r\n",-1);
								return 1;
							}

							HASHLIST_LOOKUP(USER_HASHLIST,g_hashlistUsers,strArg[4],pUser);
							if(!pUser)
							{
								ConnectionSend(pClient->pConnection,"DBFAIL :admin user flags \"Unknown user\"\r\n",-1);
								return 1;
							}

							UserSetFlags(pUser,strArg[5]);
							g_bUsersChanged = 1;

							if(!ConnectionSend(pClient->pConnection,"DBOK :admin user flags\r\n",-1))
								return 1;

							return 1;
						}
						/* admin user commands <name> <commands> */
						else if(!strcasecmp(strArg[3],"commands"))
						{
							PUSER pUser;

							if(nArgs <= 5)
							{
								ConnectionSend(pClient->pConnection,"DBFAIL :admin user commands \"Invalid params\"\r\n",-1);
								return 1;
							}

							HASHLIST_LOOKUP(USER_HASHLIST,g_hashlistUsers,strArg[4],pUser);
							if(!pUser)
							{
								ConnectionSend(pClient->pConnection,"DBFAIL :admin user commands \"Unknown user\"\r\n",-1);
								return 1;
							}

							if(!UserSetAllowedCommands(pUser,strArg[5]))
							{
								ConnectionSend(pClient->pConnection,"DBFAIL :admin user commands \"Out of memory\"\r\n",-1);
								return 1;
							}
							g_bUsersChanged = 1;

							if(!ConnectionSend(pClient->pConnection,"DBOK :admin user commands\r\n",-1))
								return 1;

							return 1;
						}
						/* admin user interface <name> <interface> */
						else if(!strcasecmp(strArg[3],"interface"))
						{
							PUSER pUser;
							char* strInterface;

							if(nArgs <= 5)
							{
								ConnectionSend(pClient->pConnection,"DBFAIL :admin user interface \"Invalid params\"\r\n",-1);
								return 1;
							}

							HASHLIST_LOOKUP(USER_HASHLIST,g_hashlistUsers,strArg[4],pUser);
							if(!pUser)
							{
								ConnectionSend(pClient->pConnection,"DBFAIL :admin user interface \"Unknown user\"\r\n",-1);
								return 1;
							}

							if(*strArg[5] && SolveIP(strArg[5]) == INADDR_NONE)
							{
								ConnectionSend(pClient->pConnection,"DBFAIL :admin user interface \"Invalid interface\"\r\n",-1);
								return 1;
							}

							if(!(strInterface = STRDUP(char*,strArg[5])))
							{
								ConnectionSend(pClient->pConnection,"DBFAIL :admin user interface \"Out of memory\"\r\n",-1);
								return 1;
							}
							if(pUser->strConnectInterface)
								FREE(char*,pUser->strConnectInterface);
							pUser->strConnectInterface = strInterface;
							g_bUsersChanged = 1;

							if(!ConnectionSend(pClient->pConnection,"DBOK :admin user interface\r\n",-1))
								return 1;

							return 1;
						}
						/* admin user password <name> <newpassword> */
						else if(!strcasecmp(strArg[3],"password"))
						{
							PUSER pUser;
							char pcMD5Pass[16];

							if(nArgs <= 5)
							{
								ConnectionSend(pClient->pConnection,"DBFAIL :admin user password \"Invalid params\"\r\n",-1);
								return 1;
							}

							HASHLIST_LOOKUP(USER_HASHLIST,g_hashlistUsers,strArg[4],pUser);
							if(!pUser)
							{
								ConnectionSend(pClient->pConnection,"DBFAIL :admin user password \"Unknown user\"\r\n",-1);
								return 1;
							}

							if(!PasswordIsValid(strArg[5]) || !PasswordCreate(strArg[5],pcMD5Pass))
							{
								ConnectionSend(pClient->pConnection,"DBFAIL :admin user password \"Invalid password\"\r\n",-1);
								return 1;
							}
							memcpy(pUser->pcMD5Pass,pcMD5Pass,sizeof(pUser->pcMD5Pass));
							g_bUsersChanged = 1;

							if(!ConnectionSend(pClient->pConnection,"DBOK :admin user password\r\n",-1))
								return 1;

							return 1;
						}
						/* admin user email <name> <newemail> */
						else if(!strcasecmp(strArg[3],"email"))
						{
							PUSER pUser;
							char* strEmail;

							if(nArgs <= 5)
							{
								ConnectionSend(pClient->pConnection,"DBFAIL :admin user email \"Invalid params\"\r\n",-1);
								return 1;
							}

							HASHLIST_LOOKUP(USER_HASHLIST,g_hashlistUsers,strArg[4],pUser);
							if(!pUser)
							{
								ConnectionSend(pClient->pConnection,"DBFAIL :admin user email \"Unknown user\"\r\n",-1);
								return 1;
							}

							if(!(strEmail = STRDUP(char*,strArg[5])))
							{
								ConnectionSend(pClient->pConnection,"DBFAIL :admin user email \"Out of memory\"\r\n",-1);
								return 1;
							}

							if(pUser->strEmail)
								FREE(char*,pUser->strEmail);
							pUser->strEmail = strEmail;
							g_bUsersChanged = 1;

							if(!ConnectionSend(pClient->pConnection,"DBOK :admin user email\r\n",-1))
								return 1;

							return 1;
						}
					}
				}
				else if(!strcasecmp(strArg[2],"config"))
				{
					if(nArgs > 3)
					{
						/* admin config get [<directive>] */
						if(!strcasecmp(strArg[3],"get"))
						{
							if(nArgs > 4)
							{
								PCONFIGVAR pConfigVar;
								
								pConfigVar = ConfigGetVar(strArg[4]);
								if(!pConfigVar)
								{
									ConnectionSend(pClient->pConnection,"DBFAIL :admin config get \"Unknown directive\"\r\n",-1);
									return 1;
								}

								if(!ConnectionSend(pClient->pConnection,"DBLIST :directive value default type\r\n",-1))
									return 1;

								switch(pConfigVar->cType & CV_TYPEMASK)
								{
								case CV_STR:
									if(!ConnectionSendFormat(pClient->pConnection,"DBITEM :%s \"%s\" \"%s\" %sstr\r\n",pConfigVar->strVarName,*(char**)pConfigVar->pVar,(char*)pConfigVar->pDefault,pConfigVar->cType & CV_ONCE ? "readonly" : ""))
										return 1;
									break;
								case CV_UINT:
									if(!ConnectionSendFormat(pClient->pConnection,"DBITEM :%s %u %u %suint\r\n",pConfigVar->strVarName,*(unsigned int*)pConfigVar->pVar,(unsigned int)pConfigVar->pDefault,pConfigVar->cType & CV_ONCE ? "readonly" : ""))
										return 1;
									break;
								case CV_BOOL:
									if(!ConnectionSendFormat(pClient->pConnection,"DBITEM :%s %s %s %sbool\r\n",pConfigVar->strVarName,*(char*)pConfigVar->pVar ? "1" : "0",(int)pConfigVar->pDefault ? "1" : "0",pConfigVar->cType & CV_ONCE ? "readonly" : ""))
										return 1;
									break;
								}

								if(!ConnectionSend(pClient->pConnection,"DBOK :admin config get\r\n",-1))
									return 1;
							}
							else
							{
								PCONFIGVAR pConfigVar;
								
								if(!ConnectionSend(pClient->pConnection,"DBLIST :directive value default type\r\n",-1))
									return 1;

								pConfigVar = ConfigGetFirstVar();
								do
								{
									switch(pConfigVar->cType & CV_TYPEMASK)
									{
									case CV_STR:
										if(!ConnectionSendFormat(pClient->pConnection,"DBITEM :%s \"%s\" \"%s\" %sstr\r\n",pConfigVar->strVarName,*(char**)pConfigVar->pVar,(char*)pConfigVar->pDefault,pConfigVar->cType & CV_ONCE ? "readonly" : ""))
											return 1;
										break;
									case CV_UINT:
										if(!ConnectionSendFormat(pClient->pConnection,"DBITEM :%s %u %u %suint\r\n",pConfigVar->strVarName,*(unsigned int*)pConfigVar->pVar,(unsigned int)pConfigVar->pDefault,pConfigVar->cType & CV_ONCE ? "readonly" : ""))
											return 1;
										break;
									case CV_BOOL:
										if(!ConnectionSendFormat(pClient->pConnection,"DBITEM :%s %s %s %sbool\r\n",pConfigVar->strVarName,*(char*)pConfigVar->pVar ? "1" : "0",(int)pConfigVar->pDefault ? "1" : "0",pConfigVar->cType & CV_ONCE ? "readonly" : ""))
											return 1;
										break;
									}
								} while((pConfigVar = ConfigGetNextVar(pConfigVar)));

								if(!ConnectionSend(pClient->pConnection,"DBOK :admin config get\r\n",-1))
									return 1;
							}

							return 1;
						}
						/* admin config set <directive> <value> */
						else if(!strcasecmp(strArg[3],"set"))
						{
							PCONFIGVAR pConfigVar = 0;

							if(nArgs <= 5)
							{
								ConnectionSend(pClient->pConnection,"DBFAIL :admin config set \"Invalid params\"\r\n",-1);
								return 1;
							}

							switch(ConfigSetVar(0,strArg[4],strArg[5],&pConfigVar))
							{
							case 0: /* out of memory */
								ConnectionSend(pClient->pConnection,"DBFAIL :admin config set \"Out of memory\"\r\n",-1);
								return 1;
							case -1: /* invalid var */
								ConnectionSend(pClient->pConnection,"DBFAIL :admin config set \"Unknown directive\"\r\n",-1);
								return 1;
							case -2:
								ASSERT(pConfigVar);
								ConnectionSend(pClient->pConnection,"DBFAIL :admin config set \"Not editable directive\"\r\n",-1);
								return 1;
							case -3:
							case -4:
								ASSERT(pConfigVar);
								ConnectionSend(pClient->pConnection,"DBFAIL :admin config set \"Invalid value\"\r\n",-1);
								return 1;
							}
							ConfigOnLoad(0);

							if(!ConnectionSend(pClient->pConnection,"DBOK :admin config set\r\n",-1))
								return 1;

							return 1;
						}
					}
				}
				else if(!strcasecmp(strArg[2],"client"))
				{
					if(nArgs > 3)
					{
						/* admin client get [<client>] */
						if(!strcasecmp(strArg[3],"get"))
						{
							if(nArgs > 4)
							{
								PCLIENT pCurClient;
								unsigned int nIP;
								unsigned short nPort = 0;

								{
									char* str = strchr(strArg[4],':');
									if(str)
									{
										*(str++) = '\0';
										nPort = (unsigned short)atol(str);
									}
									nIP = SolveIP(strArg[4]);
								}

								for(pCurClient = g_listClients.pFirst; pCurClient; pCurClient = pCurClient->ll.pNext)
									if(pCurClient->pConnection->nIP == nIP && pCurClient->pConnection->nPort == nPort)
										break;

								if(!pCurClient)
								{
									ConnectionSend(pClient->pConnection,"DBFAIL :admin client get \"Unknown client\"\r\n",-1);
									return 1;
								}
								
								if( !ConnectionSend(pClient->pConnection,"DBLIST :client nick realname user profile connected\r\n",-1) ||
									!ConnectionSendFormat(pClient->pConnection,"DBITEM :%s:%hu %s \"%s\" %s \"%s\" %u\r\n",iptoa(pCurClient->pConnection->nIP),pCurClient->pConnection->nPort,pCurClient->strNick ? pCurClient->strNick : "",pCurClient->strRealName ? pCurClient->strRealName : "",pCurClient->pUser ? pCurClient->pUser->strName : "",pCurClient->pProfile ? pCurClient->pProfile->strName : "",pCurClient->pConnection->timeConnected))
									return 1;

								if(!ConnectionSend(pClient->pConnection,"DBOK :admin client get\r\n",-1))
									return 1;
							}
							else
							{
								PCLIENT pCurClient;
								
								if(!ConnectionSend(pClient->pConnection,"DBLIST :client nick realname user profile connected\r\n",-1))
									return 1;

								for(pCurClient = g_listClients.pFirst; pCurClient; pCurClient = pCurClient->ll.pNext)
									if(pCurClient != pClient)
										if(!ConnectionSendFormat(pClient->pConnection,"DBITEM :%s:%hu %s \"%s\" %s \"%s\" %u\r\n",iptoa(pCurClient->pConnection->nIP),pCurClient->pConnection->nPort,pCurClient->strNick ? pCurClient->strNick : "",pCurClient->strRealName ? pCurClient->strRealName : "",pCurClient->pUser ? pCurClient->pUser->strName : "",pCurClient->pProfile ? pCurClient->pProfile->strName : "",pCurClient->pConnection->timeConnected))
											return 1;

								if(!ConnectionSend(pClient->pConnection,"DBOK :admin client get\r\n",-1))
									return 1;
							}

							return 1;
						}
						/* admin client disconnect <client> [<reason>] */
						else if(!strcasecmp(strArg[3],"disconnect"))
						{
							PCLIENT pCurClient;
							unsigned int nIP;
							unsigned short nPort = 0;

							if(nArgs <= 4)
							{
								ConnectionSend(pClient->pConnection,"DBFAIL :admin client disconnect \"Invalid params\"\r\n",-1);
								return 1;
							}

							{
								char* str = strchr(strArg[4],':');
								if(str)
								{
									*(str++) = '\0';
									nPort = (unsigned short)atol(str);
								}
								nIP = SolveIP(strArg[4]);
							}

							for(pCurClient = g_listClients.pFirst; pCurClient; pCurClient = pCurClient->ll.pNext)
								if(pCurClient->pConnection->nIP == nIP && pCurClient->pConnection->nPort == nPort)
									break;

							if(!pCurClient)
							{
								ConnectionSend(pClient->pConnection,"DBFAIL :admin client disconnect \"Unknown client\"\r\n",-1);
								return 1;
							}

							if(nArgs > 5)
								ClientMessage(pCurClient,"Disconnect: %s",strArg[5]);
							ConnectionCloseAsync(pCurClient->pConnection);

							if(!ConnectionSend(pClient->pConnection,"DBOK :admin client disconnect\r\n",-1))
								return 1;

							return 1;
						}
					}
				}
			}
		}
		else if(!strcasecmp(strArg[1],"profile"))
		{
			if(nArgs > 2)
			{
				/* profile get [<user>] */
				if(!strcasecmp(strArg[2],"get"))
				{
					if(nArgs > 3)
					{
						PUSER pUser;
						PPROFILE pProfile;

						HASHLIST_LOOKUP(USER_HASHLIST,g_hashlistUsers,strArg[3],pUser);
						if(!pUser)
						{
							ConnectionSend(pClient->pConnection,"DBFAIL :profile get \"Unknown user\"\r\n",-1);
							return 1;
						}

						if(!ConnectionSend(pClient->pConnection,"DBLIST :profile user server state clients\r\n",-1))
							return 1;

						for(pProfile = pUser->hashlistProfiles.pFirst; pProfile; pProfile = pProfile->hllUser.pNext)
							if(!ConnectionSendFormat(pClient->pConnection,"DBITEM :\"%s\" %s \"%s\" %s %u\r\n",pProfile->strName,pProfile->pUser->strName,pProfile->pServer ? pProfile->pServer->strServer : pProfile->c_strServers,pProfile->pServer ? (pProfile->pServer->bRegistered ? "connected" : "connecting") : "disconnected",pProfile->listClients.nCount))
								return 1;

						if(!ConnectionSend(pClient->pConnection,"DBOK :profile get\r\n",-1))
							return 1;
					}
					else
					{
						PUSER pUser;
						PPROFILE pProfile;

						if(!ConnectionSend(pClient->pConnection,"DBLIST :profile user server state clients\r\n",-1))
							return 1;

						for(pUser = g_hashlistUsers.pFirst; pUser; pUser = pUser->hll.pNext)
							for(pProfile = pUser->hashlistProfiles.pFirst; pProfile; pProfile = pProfile->hllUser.pNext)
								if(!ConnectionSendFormat(pClient->pConnection,"DBITEM :\"%s\" %s \"%s\" %s %u\r\n",pProfile->strName,pProfile->pUser->strName,pProfile->pServer ? pProfile->pServer->strServer : pProfile->c_strServers,pProfile->pServer ? (pProfile->pServer->bRegistered ? "connected" : "connecting") : "disconnected",pProfile->listClients.nCount))
									return 1;

						if(!ConnectionSend(pClient->pConnection,"DBOK :profile get\r\n",-1))
							return 1;
					}

					return 1;
				}
				/* profile add <user> <name> */
				else if(!strcasecmp(strArg[2],"add"))
				{
					PUSER pUser;
					PPROFILE pProfile;

					if(nArgs <= 4)
					{
						ConnectionSend(pClient->pConnection,"DBFAIL :profile add \"Invalid params\"\r\n",-1);
						return 1;
					}

					HASHLIST_LOOKUP(USER_HASHLIST,g_hashlistUsers,strArg[3],pUser);
					if(!pUser)
					{
						ConnectionSend(pClient->pConnection,"DBFAIL :profile add \"Unknown user\"\r\n",-1);
						return 1;
					}

					if( !strchr(pUser->strFlags,'a') &&
						pUser->hashlistProfiles.nCount >= c_nUserMaxProfiles)
					{
						ConnectionSend(pClient->pConnection,"DBFAIL :profile add \"Profile limit\"\r\n",-1);
						return 1;
					}

					HASHLIST_LOOKUP(USER_PROFILE_HASHLIST,pUser->hashlistProfiles,strArg[4],pProfile);
					if(pProfile)
					{
						ConnectionSend(pClient->pConnection,"DBFAIL :profile add \"Already exists\"\r\n",-1);
						return 1;
					}

					if( !ProfileIsNameValid(strArg[4]) )
					{
						ConnectionSend(pClient->pConnection,"DBFAIL :profile add \"Invalid profile name\"\r\n",-1);
						return 1;
					}

					if( !ProfileCreate(pUser,strArg[4],pUser->strName,pUser->strName,c_strDefaultMode) )
					{
						ConnectionSend(pClient->pConnection,"DBFAIL :profile add \"Out of memory\"\r\n",-1);
						return 1;
					}

					if(!ConnectionSend(pClient->pConnection,"DBOK :profile add\r\n",-1))
						return 1;

					return 1;
				}
				/* profile remove <user> <name> */
				else if(!strcasecmp(strArg[2],"remove"))
				{
					PUSER pUser;
					PPROFILE pProfile;

					if(nArgs <= 4)
					{
						ConnectionSend(pClient->pConnection,"DBFAIL :profile remove \"Invalid params\"\r\n",-1);
						return 1;
					}

					HASHLIST_LOOKUP(USER_HASHLIST,g_hashlistUsers,strArg[3],pUser);
					if(!pUser)
					{
						ConnectionSend(pClient->pConnection,"DBFAIL :profile remove \"Unknown user\"\r\n",-1);
						return 1;
					}


					HASHLIST_LOOKUP(USER_PROFILE_HASHLIST,pUser->hashlistProfiles,strArg[4],pProfile);
					if(!pProfile)
					{
						ConnectionSend(pClient->pConnection,"DBFAIL :profile remove \"Unknown profile\"\r\n",-1);
						return 1;
					}

					ProfileClose(pProfile,0);

					if(!ConnectionSend(pClient->pConnection,"DBOK :profile remove\r\n",-1))
						return 1;

					return 1;
				}
				/* profile connect <user> <name> */
				else if(!strcasecmp(strArg[2],"connect"))
				{
					PUSER pUser;
					PPROFILE pProfile;

					if(nArgs <= 4)
					{
						ConnectionSend(pClient->pConnection,"DBFAIL :profile connect \"Invalid params\"\r\n",-1);
						return 1;
					}

					HASHLIST_LOOKUP(USER_HASHLIST,g_hashlistUsers,strArg[3],pUser);
					if(!pUser)
					{
						ConnectionSend(pClient->pConnection,"DBFAIL :profile connect \"Unknown user\"\r\n",-1);
						return 1;
					}


					HASHLIST_LOOKUP(USER_PROFILE_HASHLIST,pUser->hashlistProfiles,strArg[4],pProfile);
					if(!pProfile)
					{
						ConnectionSend(pClient->pConnection,"DBFAIL :profile connect \"Unknown profile\"\r\n",-1);
						return 1;
					}

					if(pProfile->pServer)
					{
						ConnectionSend(pClient->pConnection,"DBFAIL :profile connect \"Already connected\"\r\n",-1);
						return 1;
					}
					ProfileConnect(pProfile,0);

					if(!ConnectionSend(pClient->pConnection,"DBOK :profile connect\r\n",-1))
						return 1;

					return 1;
				}
				/* profile disconnect <user> <name> ["<message>"]*/
				else if(!strcasecmp(strArg[2],"disconnect"))
				{
					PUSER pUser;
					PPROFILE pProfile;

					if(nArgs <= 4)
					{
						ConnectionSend(pClient->pConnection,"DBFAIL :profile disconnect \"Invalid params\"\r\n",-1);
						return 1;
					}

					HASHLIST_LOOKUP(USER_HASHLIST,g_hashlistUsers,strArg[3],pUser);
					if(!pUser)
					{
						ConnectionSend(pClient->pConnection,"DBFAIL :profile disconnect \"Unknown user\"\r\n",-1);
						return 1;
					}


					HASHLIST_LOOKUP(USER_PROFILE_HASHLIST,pUser->hashlistProfiles,strArg[4],pProfile);
					if(!pProfile)
					{
						ConnectionSend(pClient->pConnection,"DBFAIL :profile disconnect \"Unknown profile\"\r\n",-1);
						return 1;
					}

					if(!pProfile->pServer)
					{
						ConnectionSend(pClient->pConnection,"DBFAIL :profile connect \"Not connected\"\r\n",-1);
						return 1;
					}
					ProfileDisconnect(pProfile,nArgs > 5 ? strArg[5] : 0);

					if(!ConnectionSend(pClient->pConnection,"DBOK :profile disconnect\r\n",-1))
						return 1;

					return 1;
				}
			}
		}
		else if(!strcasecmp(strArg[1],"config"))
		{
			if(nArgs > 2)
			{
				/* config get <user> <profile> [<directive>] */
				if(!strcasecmp(strArg[2],"get"))
				{
					PUSER pUser;
					PPROFILE pProfile;
					PPROFILECONFIGVAR pProfileConfigVar;

					if(nArgs <= 4)
					{
						ConnectionSend(pClient->pConnection,"DBFAIL :config get \"Invalid params\"\r\n",-1);
						return 1;
					}

					HASHLIST_LOOKUP(USER_HASHLIST,g_hashlistUsers,strArg[3],pUser);
					if(!pUser)
					{
						ConnectionSend(pClient->pConnection,"DBFAIL :config get \"Unknown user\"\r\n",-1);
						return 1;
					}

					HASHLIST_LOOKUP(USER_PROFILE_HASHLIST,pUser->hashlistProfiles,strArg[4],pProfile);
					if(!pProfile)
					{
						ConnectionSend(pClient->pConnection,"DBFAIL :config get \"Unknown profile\"\r\n",-1);
						return 1;
					}

					if(nArgs > 5)
					{
						pProfileConfigVar = ProfileConfigGetVar(pProfile,strArg[5]);
						if(!pProfileConfigVar)
						{
							ConnectionSend(pClient->pConnection,"DBFAIL :config get \"Unknown directive\"\r\n",-1);
							return 1;
						}

						if(!ConnectionSend(pClient->pConnection,"DBLIST :directive value default type\r\n",-1))
							return 1;

						switch(pProfileConfigVar->cType & CV_TYPEMASK)
						{
						case CV_STR:
							if(!ConnectionSendFormat(pClient->pConnection,"DBITEM :%s \"%s\" \"%s\" %sstr\r\n",pProfileConfigVar->strVarName,*(char**)pProfileConfigVar->pVar,pProfileConfigVar->cType & CV_DYNDEFAULT ? *(char**)pProfileConfigVar->pDefault : (char*)pProfileConfigVar->pDefault,pProfileConfigVar->cType & CV_READONLY ? "readonly" : ""))
								return 1;
							break;
						case CV_UINT:
							if(!ConnectionSendFormat(pClient->pConnection,"DBITEM :%s %u %u %suint\r\n",pProfileConfigVar->strVarName,*(unsigned int*)pProfileConfigVar->pVar,pProfileConfigVar->cType & CV_DYNDEFAULT ? *(unsigned int*)pProfileConfigVar->pDefault : (unsigned int)pProfileConfigVar->pDefault,pProfileConfigVar->cType & CV_READONLY ? "readonly" : ""))
								return 1;
							break;
						case CV_BOOL:
							if(!ConnectionSendFormat(pClient->pConnection,"DBITEM :%s %s %s %sbool\r\n",pProfileConfigVar->strVarName,*(char*)pProfileConfigVar->pVar ? "1" : "0",pProfileConfigVar->cType & CV_DYNDEFAULT ? (*(char*)pProfileConfigVar->pDefault ? "1" : "0") : ((int)pProfileConfigVar->pDefault ? "1" : "0"),pProfileConfigVar->cType & CV_READONLY ? "readonly" : ""))
								return 1;
							break;
						}
					}
					else
					{
						if(!ConnectionSend(pClient->pConnection,"DBLIST :directive value default type\r\n",-1))
							return 1;

						pProfileConfigVar = ProfileConfigGetFirstVar(pProfile);
						do
						{
							switch(pProfileConfigVar->cType & CV_TYPEMASK)
							{
							case CV_STR:
								if(!ConnectionSendFormat(pClient->pConnection,"DBITEM :%s \"%s\" \"%s\" %sstr\r\n",pProfileConfigVar->strVarName,*(char**)pProfileConfigVar->pVar,pProfileConfigVar->cType & CV_DYNDEFAULT ? *(char**)pProfileConfigVar->pDefault : (char*)pProfileConfigVar->pDefault,pProfileConfigVar->cType & CV_READONLY ? "readonly" : ""))
									return 1;
								break;
							case CV_UINT:
								if(!ConnectionSendFormat(pClient->pConnection,"DBITEM :%s %u %u %suint\r\n",pProfileConfigVar->strVarName,*(unsigned int*)pProfileConfigVar->pVar,pProfileConfigVar->cType & CV_DYNDEFAULT ? *(unsigned int*)pProfileConfigVar->pDefault : (unsigned int)pProfileConfigVar->pDefault,pProfileConfigVar->cType & CV_READONLY ? "readonly" : ""))
									return 1;
								break;
							case CV_BOOL:
								if(!ConnectionSendFormat(pClient->pConnection,"DBITEM :%s %s %s %sbool\r\n",pProfileConfigVar->strVarName,*(char*)pProfileConfigVar->pVar ? "1" : "0",pProfileConfigVar->cType & CV_DYNDEFAULT ? (*(char*)pProfileConfigVar->pDefault ? "1" : "0") : ((int)pProfileConfigVar->pDefault ? "1" : "0"),pProfileConfigVar->cType & CV_READONLY ? "readonly" : ""))
									return 1;
								break;
							}

						} while((pProfileConfigVar = ProfileConfigGetNextVar(pProfile,pProfileConfigVar)));

					}

					if(!ConnectionSend(pClient->pConnection,"DBOK :config get\r\n",-1))
						return 1;

					return 1;
				}
				/* config set <user> <profile> <directive> <value> */
				else if(!strcasecmp(strArg[2],"set"))
				{
					PUSER pUser;
					PPROFILE pProfile;
					PPROFILECONFIGVAR pProfileConfigVar;

					if(nArgs <= 6)
					{
						ConnectionSend(pClient->pConnection,"DBFAIL :config set \"Invalid params\"\r\n",-1);
						return 1;
					}

					HASHLIST_LOOKUP(USER_HASHLIST,g_hashlistUsers,strArg[3],pUser);
					if(!pUser)
					{
						ConnectionSend(pClient->pConnection,"DBFAIL :config set \"Unknown user\"\r\n",-1);
						return 1;
					}

					HASHLIST_LOOKUP(USER_PROFILE_HASHLIST,pUser->hashlistProfiles,strArg[4],pProfile);
					if(!pProfile)
					{
						ConnectionSend(pClient->pConnection,"DBFAIL :config set \"Unknown profile\"\r\n",-1);
						return 1;
					}

					switch(ProfileConfigSetVar(0,pProfile,strArg[5],strArg[6],&pProfileConfigVar))
					{
					case 0: /* out of memory */
						ConnectionSend(pClient->pConnection,"DBFAIL :config set \"Out of memory\"\r\n",-1);
						return 1;
					case -1: /* invalid var (strArg[1]) */
						ConnectionSend(pClient->pConnection,"DBFAIL :config set \"Unknown directive\"\r\n",-1);
						return 1;
					case -2:
						ASSERT(pProfileConfigVar);
						ConnectionSend(pClient->pConnection,"DBFAIL :config set \"Not editable directive\"\r\n",-1);
						return 1;
					case -3:
					case -4:
						ASSERT(pProfileConfigVar);
						ConnectionSend(pClient->pConnection,"DBFAIL :config set \"Invalid value\"\r\n",-1);
						return 1;
					}
					ProfileOnLoad(pProfile,0);

					if(!ConnectionSend(pClient->pConnection,"DBOK :config set\r\n",-1))
						return 1;

					return 1;
				}
			}
		}
		else if(!strcasecmp(strArg[1],"server"))
		{
			if(nArgs > 2)
			{
				/* server get <user> <profile> */
				if(!strcasecmp(strArg[2],"get"))
				{
					PUSER pUser;
					PPROFILE pProfile;

					if(nArgs <= 4)
					{
						ConnectionSend(pClient->pConnection,"DBFAIL :server get \"Invalid params\"\r\n",-1);
						return 1;
					}

					HASHLIST_LOOKUP(USER_HASHLIST,g_hashlistUsers,strArg[3],pUser);
					if(!pUser)
					{
						ConnectionSend(pClient->pConnection,"DBFAIL :server get \"Unknown user\"\r\n",-1);
						return 1;
					}

					HASHLIST_LOOKUP(USER_PROFILE_HASHLIST,pUser->hashlistProfiles,strArg[4],pProfile);
					if(!pProfile)
					{
						ConnectionSend(pClient->pConnection,"DBFAIL :server get \"Unknown profile\"\r\n",-1);
						return 1;
					}

					if(!ConnectionSend(pClient->pConnection,"DBLIST :server\r\n",-1))
						return 1;

					if(pProfile->c_strServers && *pProfile->c_strServers)
					{
						char* str = pProfile->c_strServers;
						char* strEnd;
						do
						{	
							strEnd = strchr(str,';');
							if(strEnd)
								*strEnd = '\0';
							if(*str && !ConnectionSendFormat(pClient->pConnection,"DBITEM :\"%s\"\r\n",str))
							{
								if(strEnd)
									*strEnd = ';';
								return 1;
							}
							if(strEnd)
							{
								*strEnd = ';';					
								str = strEnd+1;
							}					
						} while(strEnd);
					}

					if(!ConnectionSend(pClient->pConnection,"DBOK :server get\r\n",-1))
						return 1;

					return 1;
				}
				/* server set <user> <profile> "<servers>" */
				else if(!strcasecmp(strArg[2],"set"))
				{
					PUSER pUser;
					PPROFILE pProfile;

					if(nArgs <= 5)
					{
						ConnectionSend(pClient->pConnection,"DBFAIL :server set \"Invalid params\"\r\n",-1);
						return 1;
					}

					HASHLIST_LOOKUP(USER_HASHLIST,g_hashlistUsers,strArg[3],pUser);
					if(!pUser)
					{
						ConnectionSend(pClient->pConnection,"DBFAIL :server set \"Unknown user\"\r\n",-1);
						return 1;
					}

					HASHLIST_LOOKUP(USER_PROFILE_HASHLIST,pUser->hashlistProfiles,strArg[4],pProfile);
					if(!pProfile)
					{
						ConnectionSend(pClient->pConnection,"DBFAIL :server set \"Unknown profile\"\r\n",-1);
						return 1;
					}

					switch(ProfileConfigSetVar(1,pProfile,"Servers",strArg[5],0))
					{
					case 0: /* out of memory */
						ConnectionSend(pClient->pConnection,"DBFAIL :server set \"Out of memory\"\r\n",-1);
						return 1;
	#ifdef DEBUG
					case -1:
					case -2:
					case -3:
					case -4:
						ASSERT(0);
						break;
	#endif /* DEBUG */
					}
					pProfile->iLastConnectServer = -1;
					
					if(!ConnectionSend(pClient->pConnection,"DBOK :server set\r\n",-1))
						return 1;

					return 1;
				}
				/* server add <user> <profile> "<server>" */
				else if(!strcasecmp(strArg[2],"add"))
				{
					PUSER pUser;
					PPROFILE pProfile;

					if(nArgs <= 5)
					{
						ConnectionSend(pClient->pConnection,"DBFAIL :server add \"Invalid params\"\r\n",-1);
						return 1;
					}

					HASHLIST_LOOKUP(USER_HASHLIST,g_hashlistUsers,strArg[3],pUser);
					if(!pUser)
					{
						ConnectionSend(pClient->pConnection,"DBFAIL :server add \"Unknown user\"\r\n",-1);
						return 1;
					}

					HASHLIST_LOOKUP(USER_PROFILE_HASHLIST,pUser->hashlistProfiles,strArg[4],pProfile);
					if(!pProfile)
					{
						ConnectionSend(pClient->pConnection,"DBFAIL :server add \"Unknown profile\"\r\n",-1);
						return 1;
					}

					if(strchr(strArg[5],';'))
					{
						ConnectionSend(pClient->pConnection,"DBFAIL :server add \"Invalid server\"\r\n",-1);
						return 1;
					}

					switch(ProfileConfigAddVar(pProfile,"Servers",strArg[5],0))
					{
					case 0:
						ConnectionSend(pClient->pConnection,"DBFAIL :server add \"Out of memory\"\r\n",-1);
						return 1;
#ifdef DEBUG
					case -1:
					case -2:
						ASSERT(0);
						break;
#endif /* DEBUG */
					}

					if(!ConnectionSend(pClient->pConnection,"DBOK :server add\r\n",-1))
						return 1;

					return 1;
				}
				/* server remove <user> <profie> "<server>" */
				else if(!strcasecmp(strArg[2],"remove"))
				{
					PUSER pUser;
					PPROFILE pProfile;

					if(nArgs <= 5)
					{
						ConnectionSend(pClient->pConnection,"DBFAIL :server remove \"Invalid params\"\r\n",-1);
						return 1;
					}

					HASHLIST_LOOKUP(USER_HASHLIST,g_hashlistUsers,strArg[3],pUser);
					if(!pUser)
					{
						ConnectionSend(pClient->pConnection,"DBFAIL :server remove \"Unknown user\"\r\n",-1);
						return 1;
					}

					HASHLIST_LOOKUP(USER_PROFILE_HASHLIST,pUser->hashlistProfiles,strArg[4],pProfile);
					if(!pProfile)
					{
						ConnectionSend(pClient->pConnection,"DBFAIL :server remove \"Unknown profile\"\r\n",-1);
						return 1;
					}

					if(ProfileConfigRemoveVar(pProfile,pProfile->c_strServers,strArg[5]) <= 0)
					{
						ConnectionSend(pClient->pConnection,"DBFAIL :server remove \"Unknown server\"\r\n",-1);
						return 1;
					}

					if(!ConnectionSend(pClient->pConnection,"DBOK :server remove\r\n",-1))
						return 1;

					return 1;
				}
				/* server connect <user> <profile> ["<server>"] */
				else if(!strcasecmp(strArg[2],"connect"))
				{
					PUSER pUser;
					PPROFILE pProfile;

					if(nArgs <= 4)
					{
						ConnectionSend(pClient->pConnection,"DBFAIL :server connect \"Invalid params\"\r\n",-1);
						return 1;
					}

					HASHLIST_LOOKUP(USER_HASHLIST,g_hashlistUsers,strArg[3],pUser);
					if(!pUser)
					{
						ConnectionSend(pClient->pConnection,"DBFAIL :server connect \"Unknown user\"\r\n",-1);
						return 1;
					}

					HASHLIST_LOOKUP(USER_PROFILE_HASHLIST,pUser->hashlistProfiles,strArg[4],pProfile);
					if(!pProfile)
					{
						ConnectionSend(pClient->pConnection,"DBFAIL :server connect \"Unknown profile\"\r\n",-1);
						return 1;
					}

					if(pProfile->pServer)
					{
						ConnectionSend(pClient->pConnection,"DBFAIL :server connect \"Already connected\"\r\n",-1);
						return 1;
					}

					if(nArgs > 5)
					{
						char* str,*strEnd;

						str = ConfigFindVar(pProfile->c_strServers,strArg[5]);
						if( !str )
						{
							ConnectionSend(pClient->pConnection,"DBFAIL :server connect \"Unknown server\"\r\n",-1);
							return 1;
						}

						strEnd = strchr(str,';');
						if(strEnd)
							*strEnd = '\0';
						
						ProfileConnect(pProfile,str);

						if(strEnd)
							*strEnd = ';';
					}
					else
					{
						if(!pProfile->c_strServers || !*pProfile->c_strServers)
						{
							ConnectionSend(pClient->pConnection,"DBFAIL :server connect \"No server found\"\r\n",-1);
							return 1;
						}

						ProfileConnect(pProfile,0);
					}

					if(!ConnectionSend(pClient->pConnection,"DBOK :server connect\r\n",-1))
						return 1;

					return 1;
				}
				/* server disconnect <user> <profile> ["<message>"] */
				else if(!strcasecmp(strArg[2],"disconnect"))
				{
					PUSER pUser;
					PPROFILE pProfile;

					if(nArgs <= 4)
					{
						ConnectionSend(pClient->pConnection,"DBFAIL :server disconnect \"Invalid params\"\r\n",-1);
						return 1;
					}

					HASHLIST_LOOKUP(USER_HASHLIST,g_hashlistUsers,strArg[3],pUser);
					if(!pUser)
					{
						ConnectionSend(pClient->pConnection,"DBFAIL :server disconnect \"Unknown user\"\r\n",-1);
						return 1;
					}

					HASHLIST_LOOKUP(USER_PROFILE_HASHLIST,pUser->hashlistProfiles,strArg[4],pProfile);
					if(!pProfile)
					{
						ConnectionSend(pClient->pConnection,"DBFAIL :server disconnect \"Unknown profile\"\r\n",-1);
						return 1;
					}

					if(pProfile->pServer)
						ProfileDisconnect(pProfile,nArgs > 5 ? strArg[5] : 0);
					else
					{
						if(pProfile->pConnectTimer)
						{
							TimerFree(pProfile->pConnectTimer);
							pProfile->pConnectTimer = 0;
						}
						else
						{
							ConnectionSend(pClient->pConnection,"DBFAIL :server disconnect \"Not connected\"\r\n",-1);
							return 1;
						}
					}
					
					if(!ConnectionSend(pClient->pConnection,"DBOK :server disconnect\r\n",-1))
						return 1;

					return 1;
				}
			}
		}
		else if(!strcasecmp(strArg[1],"statistic"))
		{
			if(nArgs > 2)
			{
				if(!strcasecmp(strArg[2],"get"))
				{

					if(!ConnectionSend(pClient->pConnection,"DBLIST :name version system time uptime totalconnections totallogins totalincoming totaloutgoing incoming outgoing users connections clients servers\r\n",-1))
						return 1;

					if(!ConnectionSendFormat(pClient->pConnection,"DBITEM :\"%s\" \"%s\" \"%s\" %u %u %.0f %.0f %.0f %.0f %u %u %u %u %u %u\r\n",
						c_strBouncerName,g_strVersion,g_strSystem,g_timeNow,g_timeNow-g_timeStart,g_dConnections,g_dLogins,g_dTotalBytesIn,g_dTotalBytesOut,g_nLastIncomingTraffic,g_nLastOutgoingTraffic,g_hashlistUsers.nCount,g_listConnections.nCount,g_listClients.nCount,g_listServers.nCount))
						return 1;

					if(!ConnectionSend(pClient->pConnection,"DBOK :statistic get\r\n",-1))
						return 1;

					return 1;
				}
			}
		}
	}

	if(!ConnectionSendFormat(pClient->pConnection,"DBFAIL :%s %s \"Unknown command\"\r\n",strArg[1],strArg[2]))
		return 1;

	return 1;
	strCommand = 0;
}
