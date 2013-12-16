/*
         file: server.c
   desciption: handle server commands
        begin: 11/25/03
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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "bounced.h"

SERVERHANDLERUNREGISTERED_TABLE_START

  SERVERHANDLER("001",ServerHandlerWelcomeMsg) /* RPL_WELCOME */
  SERVERHANDLER("002",ServerHandlerWelcomeMsg) /* RPL_YOURHOST */
  SERVERHANDLER("003",ServerHandlerWelcomeMsg) /* RPL_CREATED */
  SERVERHANDLER("004",ServerHandlerWelcomeMsg) /* RPL_MYINFO */
    SERVERHANDLER("005",ServerHandlerNumISupport) /* RPL_ISUPPORT */

  SERVERHANDLER("375",ServerHandlerMotd) /* RPL_MOTDSTART */
  SERVERHANDLER("422",ServerHandlerMotd) /* ERR_NOMOTD */

  SERVERHANDLER("431",ServerHandlerUnregisteredNickError) /* ERR_NONICKNAMEGIVEN */ /* nick.c */
  SERVERHANDLER("432",ServerHandlerUnregisteredNickError) /* ERR_ERRONEUSNICKNAME */ /* nick.c */
  SERVERHANDLER("433",ServerHandlerUnregisteredNickError) /* ERR_NICKNAMEINUSE */ /* nick.c */
  SERVERHANDLER("436",ServerHandlerUnregisteredNickError) /* ERR_NICKCOLLISION */ /* nick.c */
  SERVERHANDLER("437",ServerHandlerUnregisteredNickError) /* ERR_UNAVAILRESOURCE */ /* nick.c */

  SERVERHANDLER("PING",ServerHandlerUnregisteredPing) /* ping.c */

SERVERHANDLERUNREGISTERED_TABLE_END

SERVERHANDLER_TABLE_START

  SERVERHANDLER("001",ServerHandlerWelcomeMsg) /* RPL_WELCOME */
  SERVERHANDLER("002",ServerHandlerWelcomeMsg) /* RPL_YOURHOST */
  SERVERHANDLER("003",ServerHandlerWelcomeMsg) /* RPL_CREATED */
  SERVERHANDLER("004",ServerHandlerWelcomeMsg) /* RPL_MYINFO */
    SERVERHANDLER("005",ServerHandlerNumISupport) /* RPL_ISUPPORT */

  SERVERHANDLER("375",ServerHandlerMotd) /* RPL_MOTDSTART */
  SERVERHANDLER("422",ServerHandlerMotd) /* ERR_NOMOTD */

  SERVERHANDLER("403",ServerHandlerJoinError) /* ERR_NOSUCHCHANNEL */ /* channel.c */
  SERVERHANDLER("405",ServerHandlerJoinError) /* ERR_TOOMANYCHANNELS */ /* channel.c */
  SERVERHANDLER("407",ServerHandlerJoinError) /* ERR_TOOMANYTARGETS */ /* channel.c */
  SERVERHANDLER("437",ServerHandlerJoinError) /* ERR_UNAVAILRESOURCE */ /* channel.c */
  SERVERHANDLER("471",ServerHandlerJoinError) /* ERR_CHANNELISFULL */ /* channel.c */
  SERVERHANDLER("473",ServerHandlerJoinError) /* ERR_INVITEONLYCHAN */ /* channel.c */
  SERVERHANDLER("474",ServerHandlerJoinError) /* ERR_BANNEDFROMCHAN */ /* channel.c */
  SERVERHANDLER("475",ServerHandlerJoinError) /* ERR_BADCHANNELKEY */ /* channel.c */
  SERVERHANDLER("476",ServerHandlerJoinError) /* ERR_BADCHANMASK */ /* channel.c */
  SERVERHANDLER("477",ServerHandlerJoinError) /* ERR_NOCHANMODES */ /* channel.c */

  SERVERHANDLER("331",ServerHandlerNumNoTopic) /* RPL_NOTOPIC */ /* topic.c */
  SERVERHANDLER("332",ServerHandlerNumTopic) /* RPL_TOPIC */ /* topic.c */
  SERVERHANDLER("333",ServerHandlerNumTopicSetBy) /* topic.c */
  SERVERHANDLER("324",ServerHandlerNumChannelModeIs) /* RPL_CHANNELMODEIS */ /* mode.c */
  SERVERHANDLER("353",ServerHandlerNumNamReply) /* RPL_NAMREPLY */ /* names.c */
  SERVERHANDLER("366",ServerHandlerNumEndOfNames) /* RPL_ENDOFNAMES */ /* names.c */
  SERVERHANDLER("329",ServerHandlerNumChannelCreateTime) /* ? */ /* mode.c */

  SERVERHANDLER("PRIVMSG",ServerHandlerPrivmsg) /* privmsg.c */
  SERVERHANDLER("NOTICE",ServerHandlerNotice) /* privmsg.c */
  SERVERHANDLER("TOPIC",ServerHandlerTopic) /* topic.c */
  SERVERHANDLER("NICK",ServerHandlerNick) /* nick.c */
  SERVERHANDLER("JOIN",ServerHandlerJoin) /* channel.c */
  SERVERHANDLER("PART",ServerHandlerPart) /* channel.c */
  SERVERHANDLER("KICK",ServerHandlerKick) /* channel.c */
  SERVERHANDLER("PING",ServerHandlerPing) /* ping.c */
  SERVERHANDLER("MODE",ServerHandlerMode) /* mode.c */
  SERVERHANDLER("QUIT",ServerHandlerQuit) /* log.c */
  
SERVERHANDLER_TABLE_END

int ServerHandlersInit(void)
{
  unsigned int i;

  HASH_INIT(SERVERHANDLER_HASH,g_hashServerHandlers);
  for(i = 0; i < SERVERHANDLER_COUNT; i++)
    HASH_ADD(SERVERHANDLER_HASH,g_hashServerHandlers,&g_pServerHandlers[i]);

  HASH_INIT(SERVERHANDLERUNREGISTERED_HASH,g_hashServerHandlersUnregistered);
  for(i = 0; i < SERVERHANDLERUNREGISTERED_COUNT; i++)
    HASH_ADD(SERVERHANDLERUNREGISTERED_HASH,g_hashServerHandlersUnregistered,&g_pServerHandlersUnregistered[i]);

  return 1;
}

PSERVER ServerCreate(PPROFILE pProfile, const char* strServer, unsigned short sPort, const char* strNick, const char* strPassword)
{
  PSERVER pServer;

  if( !(pServer = CALLOC(PSERVER,1,sizeof(SERVER))) ||
    !(pServer->strServer = STRDUP(char*,strServer)) ||
    !(pServer->strNick = STRDUP(char*,strNick)) ||
    (strPassword && !(pServer->strPassword = STRDUP(char*,strPassword))) )
  {
    OUTOFMEMORY;
    if(pServer)
      ServerFree(pServer);
    return 0;
  }

  pServer->nPort = sPort;

  pServer->pProfile = pProfile;
  pProfile->pServer = pServer;

  LIST_ADD(SERVER_LIST,g_listServers,pServer);

  return pServer;
}

void ServerFree(PSERVER pServer)
{
  if(pServer->pProfile)
  {
    PPROFILE pProfile = pServer->pProfile;

    pProfile->pServer = 0;

    /* "part" channels */
    {
      PPROFILECHANNEL pProfileChannel;
      for(pProfileChannel = pProfile->hashlistProfileChannels.pFirst; pProfileChannel; pProfileChannel = pProfileChannel->hllProfile.pNext)
      {
        if(pProfileChannel->bSynced)
        {
          pProfileChannel->bSynced = 0;
          ProfileSendFormatAndLog(pProfile,pProfileChannel,PLMF_TIMESTAMP|PLMF_ADJUSTNICKLEN,":%s PRIVMSG %s :* Sync lost (disconnect)\r\n",c_strBouncerName,pProfileChannel->strName);
        }
        else if(pProfileChannel->bSyncing)
        {
          pProfileChannel->bSyncing = 0;
          ProfileSendFormatAndLog(pProfile,pProfileChannel,PLMF_TIMESTAMP|PLMF_ADJUSTNICKLEN,":%s PRIVMSG %s :* Sync failed (disconnect)\r\n",c_strBouncerName,pProfileChannel->strName);
        }
      }
    }

    /* send disconnect message */
    if(pProfile->listClients.pFirst)
    {
      if(pServer->bConnected)
        ProfileMessage(pProfile,"Disconnected from \"%s\"",pServer->strServer);
      else
        ProfileMessage(pProfile,"Connection to \"%s\" failed",pServer->strServer);
    }

    /* auto reconnect */
    if(!g_bShutdown)
    {
      if(pProfile->nConnectTry < c_nConnectMaxTries)
      {
        ASSERT(pProfile->pConnectTimer == 0);
        pProfile->pConnectTimer = TimerAdd(g_timeNow,pProfile->nConnectTry == 0 ? 0 : c_nConnectTimer,1,(PTIMERPROC)ProfileConnectTimer,pProfile);
      }
    }
  }

  LIST_REMOVE_ALL(SERVER_SERVERWELCOMEMSG_LIST,pServer->listServerWelcomeMsgs);

  if(pServer->strServer)
    FREE(char*,pServer->strServer);
  if(pServer->strNick)
    FREE(char*,pServer->strNick);
  if(pServer->strMode)
    FREE(char*,pServer->strMode);
  if(pServer->strPassword)
    FREE(char*,pServer->strPassword);

  FREE(PSERVER,pServer);
}

void ServerClose(PSERVER pServer)
{
  ASSERT(0);
  return;
  pServer = 0;
}

int ServerConnect(PSERVER pServer, PCONNECTION pConnection)
{
  unsigned int nIP;
  unsigned int nInterfaceIP;
  PPROFILE pProfile = pServer->pProfile;
  PUSER pUser = pProfile->pUser;

  ASSERT(pUser);

  pConnection->cType = CT_SERVER;
  pConnection->pData = pServer;
  pServer->pConnection = pConnection;

  pProfile->nConnectTry++;
  ProfileMessage(pProfile,pProfile->nConnectTry > 1 ? "Connecting to \"%s:%hu\" (%u)..." : "Connecting to \"%s:%hu\"...",pServer->strServer,pServer->nPort,pProfile->nConnectTry);

  nIP = SolveHostname(pServer->strServer);
  if(nIP == INADDR_NONE)
  {
    ConnectionCloseAsync(pConnection);
    return 0;
  }

  if(!pUser->strConnectInterface || !*pUser->strConnectInterface || (nInterfaceIP = SolveIP(pUser->strConnectInterface)) == INADDR_NONE)
    nInterfaceIP = SolveIP(c_strConnectInterface);

  if(!ConnectionConnect(pConnection,nIP,pServer->nPort,nInterfaceIP))
    return 0;

  return 1;
}

int ServerDisconnect(PSERVER pServer)
{
  ASSERT(pServer->pConnection);

  if(pServer->pProfile)
  {
    PPROFILE pProfile = pServer->pProfile;

    pServer->pProfile = 0;
    pProfile->pServer = 0;

    /* "part" channels */
    if(!g_bShutdown)
    {
      PPROFILECHANNEL pProfileChannel;
      for(pProfileChannel = pProfile->hashlistProfileChannels.pFirst; pProfileChannel; pProfileChannel = pProfileChannel->hllProfile.pNext)
      {
        if(pProfileChannel->bSynced)
        {
          pProfileChannel->bSynced = 0;
          ProfileSendFormatAndLog(pProfile,pProfileChannel,PLMF_TIMESTAMP|PLMF_ADJUSTNICKLEN,":%s PRIVMSG %s :* Sync lost (disconnect)\r\n",c_strBouncerName,pProfileChannel->strName);
        }
        else if(pProfileChannel->bSyncing)
        {
          pProfileChannel->bSyncing = 0;
          ProfileSendFormatAndLog(pProfile,pProfileChannel,PLMF_TIMESTAMP|PLMF_ADJUSTNICKLEN,":%s PRIVMSG %s :* Sync failed (disconnect)\r\n",c_strBouncerName,pProfileChannel->strName);
        }
      }
    }

    /* send disconnect message */
    if(!g_bShutdown && pProfile->listClients.pFirst)
    {
      if(pServer->bConnected)
        ProfileMessage(pProfile,"Disconnected from \"%s\"",pServer->strServer);
      else
        ProfileMessage(pProfile,"Connection to \"%s\" failed",pServer->strServer);
    }
  }

  ConnectionCloseAsync(pServer->pConnection);
  pServer->pConnection = 0;

  return 1;
}

int ServerRegister(PSERVER pServer)
{
  PPROFILE pProfile = pServer->pProfile;

  ProfileMessage(pServer->pProfile,"Connected to \"%s\"",pServer->strServer);
  pServer->bConnected = 1;

//  am besten hier erst pServer->strNick bestimmen (online oder offline nick? )
//    vorher pServer->strNick garnicht bestimmen?


  if( (pServer->strPassword && !ConnectionSendFormat(pServer->pConnection,"PASS :%s\r\n",pServer->strPassword)) ||
    !ConnectionSendFormat(pServer->pConnection,"NICK :%s\r\n",pServer->strNick) ||
    !ConnectionSendFormat(pServer->pConnection,"USER %s %s %s :%s\r\n",pProfile->pUser->strName,c_strBouncerName,pServer->strServer,pProfile->c_strRealName) )
    return 0;

  return 1;
}

int ServerRegistered(PSERVER pServer, char* strCommand)
{
  PCLIENT pClient;
  PPROFILE pProfile = pServer->pProfile;
  
  ASSERT(!pServer->bRegistered);
  if(pServer->bRegistered)
    return 1;

  pServer->bRegistered = 1;
  pProfile->nConnectTry = 0;

  /* get nick from command */
  memcpy(strCommand,g_strCurrentCommand,g_nCurrentCommandLength+1);
  {
    char* strNick = strCommand;
    if(*strNick == ':') /* skip prefix */
    {
      strNick = strchr(strNick,' ');
      if(strNick)
        strNick++;
    }
    if(strNick) 
    {
      strNick = strchr(strNick,' '); /* skip num */
      if(strNick)
      {
        char* strNickEnd = strchr(++strNick,' ');
        char* strNewNick;
        if(strNickEnd)
          *strNickEnd = '\0';  

        if(!(strNewNick = STRDUP(char*,strNick)))
        {
          OUTOFMEMORY;
          ConnectionCloseAsync(pServer->pConnection);
          return 0;
        }
        if(pServer->strNick)
          FREE(char*,pServer->strNick);
        pServer->strNick = strNewNick;
      }
    }
  }

  /* attach nick */
  for(pClient = pServer->pProfile->listClients.pFirst; pClient; pClient = pClient->llProfile.pNext)
  {
    if(pClient->strMode && *pClient->strMode)
    {
      ConnectionSendFormat(pClient->pConnection,":%s MODE %s :-%s\r\n",pClient->strNick,pClient->strNick,pClient->strMode);
      FREE(char*,pClient->strMode);
      pClient->strMode = 0;
    }

    if(strcmp(pClient->strNick,pServer->strNick))
    {
      char* strNewNick;
  
      if(!ConnectionSendFormat(pClient->pConnection,":%s!~%s@%s NICK :%s\r\n",pClient->strNick,pClient->strName,iptoa(pClient->pConnection->nIP),pServer->strNick))
        continue;

      if(!(strNewNick = STRDUP(char*,pServer->strNick)))
      {
        OUTOFMEMORY;
        ConnectionCloseAsync(pClient->pConnection);
        continue;
      }
      if(pClient->strNick)
        FREE(char*,pClient->strNick);
      pClient->strNick = strNewNick;
    }
  }

  /* add pseudo nick change command to channel logs */
  if(strcmp(pProfile->strNick,pServer->strNick)) 
  {
    PPROFILECHANNEL pProfileChannel;
    pProfile->nLogID++;
    for(pProfileChannel = pProfile->hashlistProfileChannels.pFirst; pProfileChannel; pProfileChannel = pProfileChannel->hllProfile.pNext)
      ProfileLogMsgCreateFormat(0,pProfileChannel,pProfile->nLogID,0,":%s NICK :%s\r\n",pProfile->strNick,pServer->strNick);
  }

  /* attach nick to PROFILE struct */
  if(strcmp(pProfile->strNick,pServer->strNick))
  {
    char* strNewNick;  
    if(!(strNewNick = STRDUP(char*,pServer->strNick)))
    {
      OUTOFMEMORY;
      ConnectionCloseAsync(pServer->pConnection);
      return 0;
    }
    if(pProfile->strNick)
      FREE(char*,pProfile->strNick);
    pProfile->strNick = strNewNick;
  }

  /* set user mode */
  if(pServer->pProfile->c_strMode)
    ConnectionSendFormat(pServer->pConnection,"MODE %s :+%s\r\n",pServer->strNick,pServer->pProfile->c_strMode);

  /* mark as away? */
  if(!pServer->pProfile->listClients.pFirst && pProfile->c_bAway)
    ConnectionSendFormat(pServer->pConnection,"AWAY :%s\r\n",pProfile->strAwayReason ? pProfile->strAwayReason : pProfile->c_strAwayDefaultReason);

  /* join channels */
  {
    PPROFILECHANNEL pProfileChannel;
    for(pProfileChannel = pProfile->hashlistProfileChannels.pFirst; pProfileChannel; pProfileChannel = pProfileChannel->hllProfile.pNext)
    {
      ASSERT(!pProfileChannel->bSynced);
      pProfileChannel->bSyncing = 1;
      ProfileSendFormatAndLog(pProfile,pProfileChannel,PLMF_TIMESTAMP|PLMF_ADJUSTNICKLEN,":%s PRIVMSG %s :* Syncing...\r\n",c_strBouncerName,pProfileChannel->strName);
      ConnectionSendFormat(pServer->pConnection,pProfileChannel->strKey ? "JOIN %s :%s\r\n" : "JOIN %s\r\n",pProfileChannel->strName,pProfileChannel->strKey);
    }
  }

  /* send perform */
  if(*pProfile->c_strPerform)
  {
    char* strPerform = pProfile->c_strPerform;
    char* strPerformEnd;
    do
    {  
      strPerformEnd = strchr(strPerform,';');
      if(strPerformEnd)
        *strPerformEnd = '\0';
      if(*strPerform)
        ConnectionSendFormat(pServer->pConnection,"%s\r\n",strPerform);
      if(strPerformEnd)
      {
        *strPerformEnd = ';';          
        strPerform = strPerformEnd+1;
      }          
    } while(strPerformEnd);
  }

  return 1;
}

PSERVERWELCOMEMSG ServerWelcomeMsgCreate(PSERVER pServer, unsigned int nCode, const char* strMsg)
{
  PSERVERWELCOMEMSG pServerWelcomeMsg;

  if( !(pServerWelcomeMsg = CALLOC(PSERVERWELCOMEMSG,1,sizeof(SERVERWELCOMEMSG))) ||
    !(pServerWelcomeMsg->strMsg = STRDUP(char*,strMsg)) )
  {
    OUTOFMEMORY;
    if(pServerWelcomeMsg)
      ServerWelcomeMsgFree(pServerWelcomeMsg);
    return 0;
  }

  pServerWelcomeMsg->nCode = nCode;

  if(pServer->pLastServerWelcomeMsg)
    LIST_INSERT_AFTER(SERVER_SERVERWELCOMEMSG_LIST,pServer->listServerWelcomeMsgs,pServer->pLastServerWelcomeMsg,pServerWelcomeMsg)
  else
    LIST_ADD(SERVER_SERVERWELCOMEMSG_LIST,pServer->listServerWelcomeMsgs,pServerWelcomeMsg);
  pServer->pLastServerWelcomeMsg = pServerWelcomeMsg;

  return pServerWelcomeMsg;
}

void ServerWelcomeMsgFree(PSERVERWELCOMEMSG pServerWelcomeMsg)
{
  if(pServerWelcomeMsg->strMsg)
    FREE(char*,pServerWelcomeMsg->strMsg);
  FREE(PSERVERWELCOMEMSG,pServerWelcomeMsg);
}

int ServerHandleCommand(PSERVER pServer, char* strCommand, unsigned int nLength)
{
  char *strAction,*strActionEnd = 0,*str;
  char cActionEnd = 0;

  if(*strCommand == ':')
  {
    strAction = strchr(strCommand,' ');
    if(!strAction)
      return 1;
    strAction++;
  }
  else
    strAction = strCommand;

  if(!*strAction)
    return 1;

  str = strAction;
  do
  {
    if(*str == ' ')
    {
      strActionEnd = str++;
      cActionEnd = *strActionEnd;
      *strActionEnd = '\0';
      break;
    }
    else if(*str == '\r' || *str == '\n')
    {
      strActionEnd = str;
      cActionEnd = *strActionEnd;
      *strActionEnd = '\0';
      break;
    }
    *(str++) = toupper(*str);
  } while(*str);

  if(isdigit(*strAction))
  {
    char* strNumEnd;

    if(!*str)
      return 1;

    strNumEnd = strchr(str,' ');
    if(strNumEnd)
      str = strNumEnd+1;
    else
      str += strlen(str);
  }

  {
    struct tagSERVERHANDLER* pHandler;
    if(pServer->bRegistered)
      HASH_LOOKUP(SERVERHANDLER_HASH,g_hashServerHandlers,strAction,pHandler)
    else
      HASH_LOOKUP(SERVERHANDLERUNREGISTERED_HASH,g_hashServerHandlersUnregistered,strAction,pHandler);    

    if(pHandler)
    {
      char cInList = pServer->cInList;

      memcpy(g_strInBufferCopy,strCommand,nLength+1);

      if(strActionEnd)
        *strActionEnd = cActionEnd;
      
      g_strCurrentCommand = strCommand;
      g_nCurrentCommandLength = nLength;

      if(!pHandler->pProc(pServer,g_strInBufferCopy,g_strInBufferCopy+(str-strCommand)))
        ProfileSend(pServer->pProfile,strCommand,nLength);

      if(cInList == pServer->cInList)
      {
        pServer->pProfile->pFirstListProfileLogMsg = 0;
        pServer->cInList = 0;
      }
    }
    else
    {
      if(strActionEnd)
        *strActionEnd = cActionEnd;

      ProfileSend(pServer->pProfile,strCommand,nLength);
    }
  }

  return 1;
}

/* server handlers */

int ServerHandlerWelcomeMsg(PSERVER pServer, char* strCommand, char* strParams)
{
  char* strNum;

  if(*strCommand == ':')
  {
    strNum = strchr(strCommand,' ');
    if(strNum)
      strNum++;
  }
  else
    strNum = strCommand;

  if(strNum && !ServerWelcomeMsgCreate(pServer,atol(strNum),strParams))
  {
    ConnectionCloseAsync(pServer->pConnection);
    return 0;
  }
  if(!pServer->bRegistered)
    ServerRegistered(pServer,strCommand);

  return 0;
}

int ServerHandlerNumISupport(PSERVER pServer, char* strCommand, char* strParams)
{
  char* strNum;

  if(*strCommand == ':')
  {
    strNum = strchr(strCommand,' ');
    if(strNum)
      strNum++;
  }
  else
    strNum = strCommand;

  if(strNum && !ServerWelcomeMsgCreate(pServer,atol(strNum),strParams))
  {
    ConnectionCloseAsync(pServer->pConnection);
    return 0;
  }

  {
    PPROFILE pProfile = pServer->pProfile;
    char* str,*strEnd;
    str = strstr(strParams,"PREFIX=");
    if(str)
    {
      str += 7;
      strEnd = strchr(str,' ');
      if(strEnd)
        *strEnd = '\0';
      str = STRDUP(char*,str);
      if(strEnd)
        *strEnd = ' ';
      if(!str)
      {
        ConnectionCloseAsync(pServer->pConnection);
        return 0;
      }
      if(pProfile->strPrefix && pProfile->strPrefix != g_strDefaultPrefix)
        FREE(char*,pProfile->strPrefix);
      pProfile->strPrefix = str;
    }
    str = strstr(strParams,"CHANMODES=");
    if(str)
    {
      str += 10;
      strEnd = strchr(str,' ');
      if(strEnd)
        *strEnd = '\0';
      str = STRDUP(char*,str);
      if(strEnd)
        *strEnd = ' ';
      if(!str)
      {
        ConnectionCloseAsync(pServer->pConnection);
        return 0;
      }
      if(pProfile->strChanModes && pProfile->strChanModes != g_strDefaultChanModes)
        FREE(char*,pProfile->strChanModes);
      pProfile->strChanModes = str;
    }
    str = strstr(strParams,"NICKLEN=");
    if(str)
    {
      str += 8;
      pProfile->nNickLen = atoul(str);
    }
  }

  if(!pServer->bRegistered)
    ServerRegistered(pServer,strCommand);

  return 0;
}

int ServerHandlerMotd(PSERVER pServer, char* strCommand, char* strParams)
{
  if(!pServer->bRegistered)
    ServerRegistered(pServer,strCommand);

  return 0;
  strParams = 0;
}

/* client handlers */

int ClientHandlerServer(PCLIENT pClient, char* strCommand, char* strParams)
{
  char* str;

  if(!pClient->pProfile)
  {
    ClientMessage(pClient,"Attach profile first");
    return 1;
  }

  if(*strParams == ':')
    strParams++;
  str = NextArg(&strParams);

  if(*str)
  {
    if(!strcasecmp(str,"add") ||
      !strcasecmp(str,"create"))
    {
      char* strArg[2];
      unsigned int nArgs;
      nArgs = SplitLine(strParams,strArg,sizeof(strArg)/sizeof(*strArg),0);
      if(nArgs > 0)
      {  
        PPROFILE pProfile = pClient->pProfile;

        if(strpbrk(strArg[0],"; "))
        {
          ClientMessage(pClient,"Invalid server \"%s\"",strArg[0]);
          return 1;
        }

        strformat(g_strOutBuffer,sizeof(g_strOutBuffer),"%s%s%s",strArg[0],nArgs > 1 ? " " : "",nArgs > 1 ? strArg[1] : "");

        switch(ProfileConfigAddVar(pProfile,"Servers",g_strOutBuffer,0))
        {
        case 0:
          ConnectionCloseAsync(pClient->pConnection);
          return 1;
#ifdef DEBUG
        case -1:
        case -2:
          ASSERT(0);
          break;
#endif /* DEBUG */
        }
        
        if(!ClientMessage(pClient,"Server \"%s\" added",strArg[0]))
          return 1;

        /* send help message */
        /* if(!strchr(pProfile->c_strServers,';') && !pProfile->pServer && !pProfile->pConnectTimer)
        {
          if(!ClientMessage(pClient,"Use \"/%s connect\" to connect to this server.",strCommand) )
            return 1;
        } .. crap? autoconnect? */

        if(!pProfile->pServer && !pProfile->pConnectTimer)
          ProfileConnect(pProfile,strArg[0]);

        return 1;
      }
    }
    else if(!strcasecmp(str,"remove") ||
      !strcasecmp(str,"delete") )
    {
      char* strArg[1];
      unsigned int nArgs;
      nArgs = SplitLine(strParams,strArg,sizeof(strArg)/sizeof(*strArg),0);
      if(nArgs > 0)
      {
        PPROFILE pProfile = pClient->pProfile;

        if(ProfileConfigRemoveVar(pProfile,pProfile->c_strServers,strArg[0]) <= 0)
        {
          ClientMessage(pClient,"Server \"%s\" not found",strArg[0]);
          return 1;
        }

        ClientMessage(pClient,"Server \"%s\" removed",strArg[0]);
        return 1;
      }
    }
    else if( !strcasecmp(str,"get") ||
           !strcasecmp(str,"show") ||
         !strcasecmp(str,"list") )
    {
      PPROFILE pProfile = pClient->pProfile;

      if(!ClientMessage(pClient,"Server list:"))
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
          if(*str && !ClientMessage(pClient,"%s",str))
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
      return 1;
    }
    else if(!strcasecmp(str,"set") )
    {
      char* strArg[1];
      unsigned int nArgs;
      nArgs = SplitLine(strParams,strArg,sizeof(strArg)/sizeof(*strArg),1);
      if(nArgs > 0)
      {  
        PPROFILE pProfile = pClient->pProfile;

        if(strchr(strArg[0],'"'))
        {
          ClientMessage(pClient,"Invalid parameter \"%s\"",strArg[0]);
          return 1;
        }

        switch(ProfileConfigSetVar(1,pProfile,"Servers",strArg[0],0))
        {
        case 0: /* out of memory */
          ConnectionCloseAsync(pClient->pConnection);
          /* ClientMessage(pClient,"Can't set Servers to \"%s\"",strArg[0]); */
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
        
        if(!ClientMessage(pClient,"Servers = %s",strArg[0]))
          return 1;

        /* send help message */
        /* if(!strchr(pProfile->c_strServers,';') && !pProfile->pServer && !pProfile->pConnectTimer)
        {
          if(!ClientMessage(pClient,"Use \"/%s connect\" to connect to this server.",strCommand) )
            return 1;
        } .. crap? autoconnect? */

        if(!pProfile->pServer && !pProfile->pConnectTimer)
          ProfileConnect(pProfile,0);

        return 1;
      }
    }
    
    else if(!strcasecmp(str,"connect"))
    {
      PPROFILE pProfile = pClient->pProfile;

      if(pProfile->pServer)
      {
        ClientMessage(pClient,"Already connected");
        return 1;
      }
      else
      {
        char* strArg[1];
        unsigned int nArgs;
        nArgs = SplitLine(strParams,strArg,sizeof(strArg)/sizeof(*strArg),0);
        if(nArgs > 0)
        {
          char* str,*strEnd;

          str = ConfigFindVar(pProfile->c_strServers,strArg[0]);
          if( !str )
          {
            ClientMessage(pClient,"Server \"%s\" not found",strArg[0]);
            return 1;
          }

          strEnd = strchr(str,';');
          if(strEnd)
            *strEnd = '\0';
          
          ProfileConnect(pProfile,str);

          if(strEnd)
            *strEnd = ';';

          return 1;
          
        }
        else
        {
          if(!pProfile->c_strServers || !*pProfile->c_strServers)
          {
            ClientMessage(pClient,"No server found");
            return 1;
          }

          ProfileConnect(pProfile,0);
          return 1;
        }
      }
    }
    else if(!strcasecmp(str,"disconnect"))
    {
      PPROFILE pProfile = pClient->pProfile;
      char* strArg[1];
      unsigned int nArgs;
      nArgs = SplitLine(strParams,strArg,sizeof(strArg)/sizeof(*strArg),1);

      if(!pProfile->pServer)
      {
        if(pProfile->pConnectTimer)
        {
          TimerFree(pProfile->pConnectTimer);
          pProfile->pConnectTimer = 0;

          ClientMessage(pClient,"Stopped connecting");
          return 1;
        }

        ClientMessage(pClient,"Not connected");
        return 1;
      }

      ProfileDisconnect(pProfile,nArgs > 0 ? strArg[0] : 0);
      return 1;
    }
  }

  if( !ClientMessage(pClient,"Usage is /%s add <server>[:<port>] [<password>]",strCommand) ||
    !ClientMessage(pClient,"         /%s remove <server>[:<port>]",strCommand) ||
    !ClientMessage(pClient,"         /%s get",strCommand) ||
    !ClientMessage(pClient,"         /%s set <server>[:<port>] [<password>][;<server>[:<port>] [<password>][;...]]",strCommand) ||
    !ClientMessage(pClient,"         /%s connect [<server>[:<port>]]",strCommand) ||
    !ClientMessage(pClient,"         /%s disconnect [<message>]",strCommand) )
    return 1;

  return 1;
  strCommand = 0;
}
