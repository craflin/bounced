/*
         file: privmsg.c
   desciption: handle privmsgs and notices
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
#include <ctype.h>

#include "bounced.h"

/* server handlers */

int ServerHandlerPrivmsg(PSERVER pServer, char* strCommand, char* strParams)
{
  char* strArg[2];

  if( SplitIRCParams(strParams,strArg,sizeof(strArg)/sizeof(*strArg),1) == sizeof(strArg)/sizeof(*strArg))
  {
    PPROFILE pProfile = pServer->pProfile;

    if(*strArg[1] == '\001' && strncmp(&strArg[1][1],"ACTION ",7) ) /* ctcp request */
    {
      if(pProfile->listClients.pFirst)
        return 0; /* don't log ctcp requests */

      /* handle ctcp request */
      if(!strncmp(&strArg[1][1],"VERSION",7))
      {
        char* strNick;
        if(SplitIRCPrefixNick(strCommand,&strNick))
          ConnectionSendFormat(pServer->pConnection,"NOTICE %s :\001VERSION %s\001\r\n",strNick,g_strVersion);
      }
      else if(!strncmp(&strArg[1][1],"PING",4))
      {
        char* strNick;

        char* strCtcpArg = &strArg[1][5];
        char* str;
        if(*strCtcpArg == ' ')
          strCtcpArg++;
        str = strCtcpArg+strlen(strCtcpArg)-1;
        if(*str == '\001')
          *str = '\0';

        if(SplitIRCPrefixNick(strCommand,&strNick))
          ConnectionSendFormat(pServer->pConnection,"NOTICE %s :\001PING %s\001\r\n",strNick,strCtcpArg);
      }
      else if(!strncmp(&strArg[1][1],"CLIENTINFO",10))
      {
        char* strNick;
        if(SplitIRCPrefixNick(strCommand,&strNick))
          ConnectionSendFormat(pServer->pConnection,"NOTICE %s :\001CLIENTINFO VERSION PING CLIENTINFO\001\r\n",strNick);
      }

      return 1;
    }

    /* log message */
    {      
      PPROFILECHANNEL pProfileChannel;
      
      HASHLIST_LOOKUP(PROFILE_PROFILECHANNEL_HASHLIST,pProfile->hashlistProfileChannels,strArg[0],pProfileChannel);  
      if(pProfileChannel)
        ProfileLogMsgCreate(0,pProfileChannel,++pProfile->nLogID,g_strCurrentCommand,g_nCurrentCommandLength,PLMF_TIMESTAMP|PLMF_ADJUSTNICKLEN);
      else if(!pProfile->listClients.pFirst)
        ProfileLogMsgCreate(pProfile,0,++pProfile->nLogID,g_strCurrentCommand,g_nCurrentCommandLength,PLMF_TIMESTAMP);
    }
  }

  return 0;
  strCommand = 0;
}

int ServerHandlerNotice(PSERVER pServer, char* strCommand, char* strParams)
{
  char* strArg[2];

  if( SplitIRCParams(strParams,strArg,sizeof(strArg)/sizeof(*strArg),1) == sizeof(strArg)/sizeof(*strArg))
  {
    PPROFILE pProfile = pServer->pProfile;

    if(*strArg[1] == '\001' ) /* ctcp answer */
      return 0; /* don't log ctcp answer (forward them (when a client is online) or drop them) */

    /* log message */
    {
      PPROFILECHANNEL pProfileChannel;

      HASHLIST_LOOKUP(PROFILE_PROFILECHANNEL_HASHLIST,pProfile->hashlistProfileChannels,strArg[0],pProfileChannel);
      if(pProfileChannel)
        ProfileLogMsgCreate(0,pProfileChannel,++pProfile->nLogID,g_strCurrentCommand,g_nCurrentCommandLength,PLMF_TIMESTAMP|PLMF_ADJUSTNICKLEN);
      else if(!pProfile->listClients.pFirst)
        ProfileLogMsgCreate(pProfile,0,++pProfile->nLogID,g_strCurrentCommand,g_nCurrentCommandLength,PLMF_TIMESTAMP);
    }

  }

  return 0;
  strCommand = 0;
}

/* client handlers */

int ClientHandlerPrivmsg(PCLIENT pClient, char* strCommand, char* strParams)
{
  if(CompareIRCAddressNick(strParams,c_strBouncerName))
  {
    char *strAction,*str;

    pClient->cMessageMode = CMM_PRIVMSG;

    strAction = strParams+strlen(c_strBouncerName)+1;
    if(*strAction == ':')
      strAction++;

    str = strAction;
    do
    {
      if(*str == ' ')
      {
        *(str++) = '\0';
        break;
      }
      else if(*str == '\r' || *str == '\n')
      {
        *str = '\0';
        break;
      }
      *str = toupper(*str);
      ++str;
    } while(*str);
    
    {
      struct tagCLIENTHANDLER* pHandler;
      HASH_LOOKUP(CLIENTHANDLER_HASH,g_hashClientHandlers,strAction,pHandler);
      if(pHandler && ( 
        !(pHandler->nFlags & CHF_BOUNCER) ||
        (!(pHandler->nFlags & CHF_ADMIN) && (!pClient->pUser->strAllowedCommands || !ConfigFindVar(pClient->pUser->strAllowedCommands,pHandler->strAction))) ||
        (pHandler->nFlags & CHF_ADMIN && !pClient->bAdmin) ) )
        pHandler = 0;
      if(pHandler)
      {
        sprintf(strCommand,"msg %s %s",c_strBouncerName,pHandler->strAction);
        pHandler->pProc(pClient,strCommand,str);
      }
      else
        ClientMessage(pClient,"Unknown command \"%s\"\r\n",strAction);
    }

    pClient->cMessageMode = CMM_DEFAULT;
    return 1;
  }

  {
    char* strArg[2];
    if(SplitIRCParams(strParams,strArg,sizeof(strArg)/sizeof(*strArg),1) == sizeof(strArg)/sizeof(*strArg))
    {
      if(pClient->pProfile)
      {
        PPROFILE pProfile = pClient->pProfile;
        PPROFILECHANNEL pProfileChannel;

        HASHLIST_LOOKUP(PROFILE_PROFILECHANNEL_HASHLIST,pProfile->hashlistProfileChannels,strArg[0],pProfileChannel);
        if(pProfileChannel)
        {
          PCLIENT pCurClient;
          unsigned short sLength;

          sLength = strformat(g_strOutBuffer,sizeof(g_strOutBuffer),":%s!~%s@%s PRIVMSG %s :%s\r\n",pClient->strNick,pClient->pUser->strName,iptoa(pClient->pConnection->nIP),strArg[0],strArg[1]);

          for(pCurClient = pProfile->listClients.pFirst; pCurClient; pCurClient = pCurClient->llProfile.pNext)
            if(pCurClient != pClient)
              ConnectionSend(pCurClient->pConnection,g_strOutBuffer,sLength);

          ProfileLogMsgCreate(0,pProfileChannel,++pProfile->nLogID,g_strOutBuffer,sLength,PLMF_TIMESTAMP|PLMF_ADJUSTNICKLEN);
        }
      }
    }
  }

  return 0;
}

int ClientHandlerNotice(PCLIENT pClient, char* strCommand, char* strParams)
{
  if(CompareIRCAddressNick(strParams,c_strBouncerName))
  {
    char *strAction,*str;

    pClient->cMessageMode = CMM_NOTICE;

    strAction = strParams+strlen(c_strBouncerName)+1;
    if(*strAction == ':')
      strAction++;

    str = strAction;
    do
    {
      if(*str == ' ')
      {
        *(str++) = '\0';
        break;
      }
      else if(*str == '\r' || *str == '\n')
      {
        *str = '\0';
        break;
      }
      *str = toupper(*str);
      ++str;
    } while(*str);
    
    {
      struct tagCLIENTHANDLER* pHandler;
      HASH_LOOKUP(CLIENTHANDLER_HASH,g_hashClientHandlers,strAction,pHandler);
      if(pHandler && ( 
        !(pHandler->nFlags & CHF_BOUNCER) ||
        (!(pHandler->nFlags & CHF_ADMIN) && (!pClient->pUser->strAllowedCommands || !ConfigFindVar(pClient->pUser->strAllowedCommands,pHandler->strAction))) ||
        (pHandler->nFlags & CHF_ADMIN && !pClient->bAdmin) ) )
        pHandler = 0;
      if(pHandler)
      {
        sprintf(strCommand,"notice %s %s",c_strBouncerName,pHandler->strAction);
        pHandler->pProc(pClient,strCommand,str);
      }
      else
        ClientMessage(pClient,"Unknown command \"%s\"\r\n",strAction);
    }

    pClient->cMessageMode = CMM_DEFAULT;
    return 1;
  }

  {
    char* strArg[2];
    if(SplitIRCParams(strParams,strArg,sizeof(strArg)/sizeof(*strArg),1) == sizeof(strArg)/sizeof(*strArg))
    {
      if(*strArg[1] == '\001' && strncmp(&strArg[1][1],"ACTION ",7) ) /* ctcp answer */
      {
        if(!strncmp(&strArg[1][1],"VERSION",7) ) /* hook version answer */
        {
          if(pClient->pProfile && pClient->pProfile->pServer && pClient->pProfile->pServer->bRegistered)
          {
            char* strCtcpArg = &strArg[1][8];
            char* str;
            if(*strCtcpArg == ' ')
              strCtcpArg++;
            str = strCtcpArg+strlen(strCtcpArg)-1;
            if(*str == '\001')
              *str = '\0';

            ConnectionSendFormat(pClient->pProfile->pServer->pConnection,"NOTICE %s :\001VERSION %s - %s\001\r\n",strArg[0],g_strVersion,strCtcpArg);
          }
          return 1;
        }

        return 0; /* don't log ctcp answers */
      }

      if(pClient->pProfile)
      {
        PPROFILE pProfile = pClient->pProfile;
        PPROFILECHANNEL pProfileChannel;

        HASHLIST_LOOKUP(PROFILE_PROFILECHANNEL_HASHLIST,pProfile->hashlistProfileChannels,strArg[0],pProfileChannel);
        if(pProfileChannel)
        {
          PCLIENT pCurClient;

          ProfileLogMsgCreate(0,pProfileChannel,++pProfile->nLogID,g_strCurrentCommand,g_nCurrentCommandLength,PLMF_TIMESTAMP|PLMF_ADJUSTNICKLEN);

          for(pCurClient = pProfile->listClients.pFirst; pCurClient; pCurClient = pCurClient->llProfile.pNext)
            if(pCurClient != pClient)
              ConnectionSendFormat(pCurClient->pConnection,":%s!~%s@%s NOTICE %s :%s\r\n",pClient->strNick,pClient->pUser->strName,iptoa(pClient->pConnection->nIP),strArg[0],strArg[1]);
        }
      }
    }
  }

  return 0;
}

