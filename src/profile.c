/*
         file: profile.c
   desciption: profiling functions
        begin: 10/26/03
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
#include <stdarg.h>
#include <ctype.h>

#include "bounced.h"

size_t vstrformat(char* str, size_t maxsize, const char* format, va_list ap);


int ProfileIsNameValid(const char* strName)
{
  ASSERT(strName);
  if(!strName)
    return 0;
  return UserIsNameValid(strName);
}

PPROFILE ProfileCreate(PUSER pUser, const char* strName, const char* strNick, const char* strRealName, const char* strMode)
{
  PPROFILE pProfile;

  ASSERT(strName);
  ASSERT((strNick && strRealName) ||
       (!strNick && !strRealName) );

  if( !(pProfile = CALLOC(PPROFILE,1,sizeof(PROFILE))) ||
    (!(pProfile->strName = STRDUP(char*,strName))) ||
    (strNick && !(pProfile->c_strNick = STRDUP(char*,strNick))) ||
    (strNick && !(pProfile->strNick = STRDUP(char*,strNick))) ||
    (strRealName && !(pProfile->c_strRealName = STRDUP(char*,strRealName))) ||
    (strMode && !(pProfile->c_strMode = STRDUP(char*,strMode))) )
  
  {
    OUTOFMEMORY;
    if(pProfile)
      ProfileFree(pProfile);
    return 0;
  }

  pProfile->strPrefix = g_strDefaultPrefix;
  pProfile->strChanModes = g_strDefaultChanModes;
  pProfile->nNickLen = DEFAULT_NICKLEN;
  pProfile->pUser = pUser;
  pProfile->iLastConnectServer = -1;

  /* load default config values */
  {
    PPROFILECONFIGVAR pProfileConfigVar;
    pProfileConfigVar = ProfileConfigGetFirstVar(pProfile);
    do
    {
      switch(pProfileConfigVar->cType & CV_TYPEMASK)
      {
      case CV_STR:
        if(!*(char**)pProfileConfigVar->pVar)
        {
          if(pProfileConfigVar->cType & CV_DYNDEFAULT)
          {
            *(char**)pProfileConfigVar->pVar = STRDUP(char*,*(char**)pProfileConfigVar->pDefault);
            if(!*(char**)pProfileConfigVar->pVar)
            {
              OUTOFMEMORY;
              if(pProfile)
                ProfileFree(pProfile);
              return 0;
            }
          }
          else
            *(char**)pProfileConfigVar->pVar = (char*)pProfileConfigVar->pDefault;
        }
        break;
      case CV_UINT:
        *(unsigned int*)pProfileConfigVar->pVar = pProfileConfigVar->cType & CV_DYNDEFAULT ? *(unsigned int*)pProfileConfigVar->pDefault : (unsigned int)pProfileConfigVar->pDefault;
        break;
      case CV_BOOL:
        *(char*)pProfileConfigVar->pVar = pProfileConfigVar->cType & CV_DYNDEFAULT ? (int)*(char*)pProfileConfigVar->pDefault : (int)pProfileConfigVar->pDefault;
        break;
      }
    } while((pProfileConfigVar = ProfileConfigGetNextVar(pProfile,pProfileConfigVar)));
    
  }
  pProfile->pUser->bConfigChanged = 1;

  HASHLIST_ADD(USER_PROFILE_HASHLIST,pUser->hashlistProfiles,pProfile);

  return pProfile;
}

void ProfileFree(PPROFILE pProfile)
{
  /* close server connection */
  if(pProfile->pServer)
  {
    ProfileDisconnect(pProfile,0);
    ASSERT(pProfile->pServer == 0);
  }

  /* detach all clients */
  while(pProfile->listClients.pFirst)
  {
    PCLIENT pClient = pProfile->listClients.pFirst;
    ProfileDetach(pProfile,pClient,0);
    ASSERT(pClient->pProfile == 0);
  }

  HASHLIST_REMOVE_ALL(PROFILE_PROFILECHANNEL_HASHLIST,pProfile->hashlistProfileChannels);  

  LIST_REMOVE_ALL(PROFILE_MSGPROFILELOGMSG_LIST,pProfile->listMsgProfileLogMsgs);
  ASSERT(!pProfile->listProfileLogMsgs.pFirst);

  if(pProfile->strName)
    FREE(char*,pProfile->strName);
  if(pProfile->pConnectTimer)
    TimerFree(pProfile->pConnectTimer);
  if(pProfile->strNick)
    FREE(char*,pProfile->strNick);
  if(pProfile->strAwayReason)
    FREE(char*,pProfile->strAwayReason);

  if(pProfile->strPrefix && pProfile->strPrefix != g_strDefaultPrefix)
    FREE(char*,pProfile->strPrefix);
  if(pProfile->strChanModes && pProfile->strChanModes != g_strDefaultChanModes)
    FREE(char*,pProfile->strChanModes);
  
  /* free config vars */
  {
    PPROFILECONFIGVAR pProfileConfigVar;
    pProfileConfigVar = ProfileConfigGetFirstVar(pProfile);
    do
    {
      if( (pProfileConfigVar->cType & CV_TYPEMASK) == CV_STR && 
        *(char**)pProfileConfigVar->pVar && (pProfileConfigVar->cType & CV_DYNDEFAULT || *(char**)pProfileConfigVar->pVar != (char*)pProfileConfigVar->pDefault))
        FREE(char*,*(char**)pProfileConfigVar->pVar);
    } while((pProfileConfigVar = ProfileConfigGetNextVar(pProfile,pProfileConfigVar)));
    
  }
  pProfile->pUser->bConfigChanged = 1;

  FREE(PPROFILE,pProfile);
}

void ProfileClose(PPROFILE pProfile, const char* strDetachMessage)
{
  PUSER pUser = pProfile->pUser;

  /* detach all clients */
  while(pProfile->listClients.pFirst)
    ProfileDetach(pProfile,pProfile->listClients.pFirst,strDetachMessage);

  HASHLIST_REMOVE_ITEM(USER_PROFILE_HASHLIST,pUser->hashlistProfiles,pProfile);
}

int ProfileAttach(PPROFILE pProfile, PCLIENT pClient, const char* strMessage)
{
  PSERVER pServer = pProfile->pServer;
  PUSER pUser = pProfile->pUser;
  char bRealAttaching = !pProfile->listClients.pFirst;

  ASSERT(pUser);

  ASSERT(pClient->pProfile == 0);
  pClient->pProfile = pProfile;
  LIST_ADD(PROFILE_CLIENT_LIST,pProfile->listClients,pClient);

  if(pUser->strAllowedCommands && ConfigFindVar(pUser->strAllowedCommands,"PROFILE"))
    if(!ClientMessage(pClient,"Profile \"%s\" attached",pProfile->strName))
      return 0;

  /* try to change nick to server */
  ASSERT(pProfile->c_strNick);
  if(pServer && strcmp(pProfile->c_strNick,pServer->strNick))
  {
    if(pServer->bRegistered)
      ConnectionSendFormat(pServer->pConnection,"NICK :%s\r\n",pProfile->c_strNick);
    else
    {
      char* strNick;
      if(!(strNick = STRDUP(char*,pProfile->c_strNick)))
      {
        OUTOFMEMORY;
        ConnectionCloseAsync(pClient->pConnection);
        return 0;
      }
      if(pServer->strNick)
        FREE(char*,pServer->strNick);
      pServer->strNick = strNick;
    }
  }

  /* real attaching */
  if(pServer && pServer->bRegistered && bRealAttaching)
  {
    /* mark as not away */
    if(pProfile->c_bAway || pProfile->strAwayReason)
      ConnectionSend(pServer->pConnection,"AWAY\r\n",-1);
  }
  if(pProfile->strAwayReason)
  {
    FREE(char*,pProfile->strAwayReason);
    pProfile->strAwayReason = 0;
  }

  /* attach nick to client*/
  {
    char *strNick = pProfile->strNick;

    if(strcmp(pClient->strNick,strNick))
    {
      if(!(strNick = STRDUP(char*,strNick)))
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
  }
  
  /* attach mode to client */
  {
    char *strMode = (pServer && pServer->bRegistered) ? pServer->strMode : pProfile->c_strMode,
       strAdd[53],
       strRemove[53];

    if(!CompareIRCModes(pClient->strMode,strMode,strAdd,sizeof(strAdd),strRemove,sizeof(strRemove)))
    {
      ConnectionCloseAsync(pClient->pConnection);
      return 0;
    }

    if(strMode && !(strMode = STRDUP(char*,strMode)))
    {
      OUTOFMEMORY;
      ConnectionCloseAsync(pClient->pConnection);
      return 0;
    }

    if( (*strRemove && !ConnectionSendFormat(pClient->pConnection,":%s MODE %s :-%s\r\n",pClient->strNick,pClient->strNick,strRemove)) ||
      (*strAdd && !ConnectionSendFormat(pClient->pConnection,":%s MODE %s :+%s\r\n",pClient->strNick,pClient->strNick,strAdd)) )
    {
      FREE(char*,strMode);
      return 0;
    }

    if(pClient->strMode)
      FREE(char*,pClient->strMode);
    pClient->strMode = strMode;
  }

  /* send server welcome messages */
  if(pServer && pServer->bRegistered && pServer->listServerWelcomeMsgs.pFirst)
  {
    PSERVERWELCOMEMSG pServerWelcomeMsg;
    for(pServerWelcomeMsg = pClient->pProfile->pServer->listServerWelcomeMsgs.pFirst; pServerWelcomeMsg; pServerWelcomeMsg = pServerWelcomeMsg->llServer.pNext)
      if(!ConnectionSendFormat(pClient->pConnection,":%s %03hu %s %s",c_strBouncerName,pServerWelcomeMsg->nCode,pClient->strNick,pServerWelcomeMsg->strMsg))
        return 0;
  }
  
  /* send logs */

#ifdef DEBUG
  {
    PPROFILECHANNEL pProfileChannel;
    for(pProfileChannel = pProfile->hashlistProfileChannels.pFirst; pProfileChannel; pProfileChannel = pProfileChannel->hllProfile.pNext)
      ASSERT(!pProfileChannel->cReserve);
  }
#endif /* DEBUG */

  if(pProfile->listProfileLogMsgs.pFirst)
  {
    PPROFILELOGMSG pProfileLogMsg,pNextProfileLogMsg;
    PPROFILELOGMSG pLastAttProfileLogMsg = 0;
    unsigned int nID = pProfile->listProfileLogMsgs.pFirst->nID+1;

    for(pProfileLogMsg = pProfile->listProfileLogMsgs.pFirst; pProfileLogMsg; pProfileLogMsg = pNextProfileLogMsg)
    {
      pNextProfileLogMsg = pProfileLogMsg->llProfile.pNext;

      if(pLastAttProfileLogMsg)
      {
        if(pLastAttProfileLogMsg == pProfileLogMsg)
          pLastAttProfileLogMsg = 0;
      }
      else 
      {
        PPROFILELOGMSG pAttProfileLogMsg = pProfileLogMsg;
        do
        {
          pLastAttProfileLogMsg = pAttProfileLogMsg;
          if(pAttProfileLogMsg->pProfileChannel && !pAttProfileLogMsg->pProfileChannel->cReserve)
          {
            if(!ProfileChannelAttach(pAttProfileLogMsg->pProfileChannel,pClient))
              break;
            pAttProfileLogMsg->pProfileChannel->cReserve = 1;
          }
          pAttProfileLogMsg = pAttProfileLogMsg->llProfile.pNext;
        } while(pAttProfileLogMsg && pAttProfileLogMsg->nID == pProfileLogMsg->nID);
        if(pLastAttProfileLogMsg == pProfileLogMsg)
          pLastAttProfileLogMsg = 0;
      }

      if(nID != pProfileLogMsg->nID)
      {
        nID = pProfileLogMsg->nID;        
#ifdef DEBUG
        if(pProfileLogMsg->pProfileChannel)
          ASSERT(pProfileLogMsg->pProfileChannel->cReserve);
#endif /* DEBUG */
  
        if(!ProfileLogMsgSend(pProfileLogMsg,pClient->pConnection))
          break;

        if(CompareIRCPrefixNick(pProfileLogMsg->strMsg,pClient->strNick))
        {
          char* str = strchr(pProfileLogMsg->strMsg,' ');
          if(str)
          {
            str++;
            if(!strncasecmp(str,"NICK ",5))
            {
              char* strEnd;
              str += 5;
              if(*str == ':')
                str++;
              strEnd = strpbrk(str," \r\n");
              if(strEnd)
              {
                char* strNick = MALLOC(char*,strEnd-str+1);
                if(!strNick)
                {
                  OUTOFMEMORY;
                  ConnectionCloseAsync(pClient->pConnection);
                  break;
                }
                memcpy(strNick,str,strEnd-str);
                strNick[strEnd-str] = '\0';
                if(pClient->strNick)
                  FREE(char*,pClient->strNick);
                pClient->strNick = strNick;
              }
            }
          }
        }

        if(!pProfileLogMsg->pProfileChannel)
          ProfileLogMsgClose(pProfileLogMsg);
      }
    }
  }

  {
    PPROFILECHANNEL pProfileChannel;

    for(pProfileChannel = pProfile->hashlistProfileChannels.pFirst; pProfileChannel; pProfileChannel = pProfileChannel->hllProfile.pNext)
    {
      if(!pProfileChannel->cReserve)
        ProfileChannelAttach(pProfileChannel,pClient);
      pProfileChannel->cReserve = 0;
    }

    if(strcmp(pClient->strNick,pProfile->strNick))
    {
      if(!ConnectionSendFormat(pClient->pConnection,":%s!~%s@%s NICK :%s\r\n",pClient->strNick,pClient->strName,iptoa(pClient->pConnection->nIP),pProfile->strNick))
        return 0;
    }

    if(pClient->pConnection->bClosing)
      return 0;
  }

#ifdef DEBUG
  {
    PPROFILECHANNEL pProfileChannel;
    for(pProfileChannel = pProfile->hashlistProfileChannels.pFirst; pProfileChannel; pProfileChannel = pProfileChannel->hllProfile.pNext)
      ASSERT(!pProfileChannel->cReserve);
  }
#endif /* DEBUG */

  return 1;
  strMessage = 0;

}

int ProfileDetach(PPROFILE pProfile, PCLIENT pClient, const char* strMessage)
{
  int iReturn = 1;
  PUSER pUser = pProfile->pUser;

  ASSERT(pUser);

  ASSERT(pClient->pProfile == pProfile);
  LIST_REMOVE_ITEM(PROFILE_CLIENT_LIST,pProfile->listClients,pClient);
  pClient->pProfile = 0;

  /* detach channels */
  if(pProfile->hashlistProfileChannels.pFirst)
  {
    PPROFILECHANNEL pProfileChannel;
    for(pProfileChannel = pProfile->hashlistProfileChannels.pFirst; pProfileChannel; pProfileChannel = pProfileChannel->hllProfile.pNext)
      if(!ConnectionSendFormat(pClient->pConnection,":%s!~%s@%s PART %s\r\n",pClient->strNick,pClient->strName,iptoa(pClient->pConnection->nIP),pProfileChannel->strName))
          iReturn = 0;
  }

  if(pProfile->pServer && pProfile->pServer->bRegistered && !pProfile->listClients.pFirst) /* real detaching */
  {
    PSERVER pServer = pProfile->pServer;

    /* change nick */
    if(pProfile->c_strDetachNick && *pProfile->c_strDetachNick && strcmp(pProfile->c_strDetachNick,pServer->strNick))
      ConnectionSendFormat(pServer->pConnection,"NICK :%s\r\n",pProfile->c_strDetachNick);

    /* mark as away */
    if(pProfile->c_bAway)
    {
      ASSERT(!pProfile->strAwayReason);
      if(strMessage && *strMessage)
        pProfile->strAwayReason = STRDUP(char*,strMessage);
      ConnectionSendFormat(pServer->pConnection,"AWAY :%s\r\n",(strMessage && *strMessage) ? strMessage : pProfile->c_strAwayDefaultReason);
    }
  }

  if(pUser->strAllowedCommands && ConfigFindVar(pUser->strAllowedCommands,"PROFILE"))
    if(!ClientMessage(pClient,"Profile \"%s\" detached",pProfile->strName))
      iReturn = 0;

  return iReturn;
  strMessage = 0;
}

void ProfileMessage(PPROFILE pProfile, const char* format,...)
{
  if(pProfile->listClients.pFirst)
  {
    va_list ap;
    unsigned short sLength;
    PCLIENT pCli;

    sLength = strformat(g_strOutBuffer,sizeof(g_strOutBuffer)-2,":%s NOTICE %s :",c_strBouncerName,pProfile->c_strNick);

    va_start (ap, format);
    sLength += vstrformat(g_strOutBuffer+sLength,sizeof(g_strOutBuffer)-2-sLength,format, ap);
    va_end (ap);

    g_strOutBuffer[sLength++] = '\r';
    g_strOutBuffer[sLength++] = '\n';
    g_strOutBuffer[sLength] = '\0';

    for(pCli = pProfile->listClients.pFirst; pCli; pCli = pCli->llProfile.pNext)
      ConnectionSend(pCli->pConnection,g_strOutBuffer,sLength);
  }
}


void ProfileSendFormat(PPROFILE pProfile, const char* format,...)
{
  if(pProfile->listClients.pFirst)
  {
    va_list ap;
    unsigned short sLength;
    PCLIENT pCli;

    va_start (ap, format);
    sLength = vstrformat(g_strOutBuffer,sizeof(g_strOutBuffer),format, ap);
    va_end (ap);

    for(pCli = pProfile->listClients.pFirst; pCli; pCli = pCli->llProfile.pNext)
      ConnectionSend(pCli->pConnection,g_strOutBuffer,sLength);
  }
}

void ProfileSendFormatAndLog(PPROFILE pProfile, PPROFILECHANNEL pProfileChannel, char cLogFlags, const char* format,...)
{
  va_list ap;
  unsigned short sLength;
  PCLIENT pCli;

  va_start (ap, format);
  sLength = vstrformat(g_strOutBuffer,sizeof(g_strOutBuffer),format, ap);
  va_end (ap);

  for(pCli = pProfile->listClients.pFirst; pCli; pCli = pCli->llProfile.pNext)
    ConnectionSend(pCli->pConnection,g_strOutBuffer,sLength);

  ProfileLogMsgCreate(pProfile,pProfileChannel,++pProfile->nLogID,g_strOutBuffer,sLength,cLogFlags);
}

void ProfileSend(PPROFILE pProfile, const char* strBuffer, int iLength)
{
  if(pProfile->listClients.pFirst)
  {
    PCLIENT pCli;

    if(iLength < 0)
      iLength = strlen(strBuffer);

    for(pCli = pProfile->listClients.pFirst; pCli; pCli = pCli->llProfile.pNext)
      ConnectionSend(pCli->pConnection,strBuffer,iLength);
  }
}

void ProfileConnect(PPROFILE pProfile, char* strServer)
{
  ASSERT(!g_bShutdown);

  if(!strServer)
  {
    if(!pProfile->c_strServers || !*pProfile->c_strServers)
    {
      pProfile->iLastConnectServer = -1;
      return;
    }
    pProfile->iLastConnectServer++;
    {
      char* str = pProfile->c_strServers;
      char* strEnd;
      char* strFirst = 0;
      char* strFirstEnd = 0;
      int i = 0;
      do
      {  
        strEnd = strchr(str,';');
        if(strEnd)
          *strEnd = '\0';
        if(*str)
        {
          if(!strFirst)
          {
            strFirst = str;
            strFirstEnd = strEnd;
          }

          if(i == pProfile->iLastConnectServer)
          {
            ProfileConnect(pProfile,str);
            if(strEnd)
              *strEnd = ';';
            return;
          }
          i++;
        }
        if(strEnd)
        {
          *strEnd = ';';          
          str = strEnd+1;
        }          
      } while(strEnd);
      if(strFirst)
      {
        pProfile->iLastConnectServer = 0;
        if(strFirstEnd)
          *strFirstEnd = '\0';
        ProfileConnect(pProfile,strFirst);
        if(strFirstEnd)
          *strFirstEnd = ';';
        return;
      }
      pProfile->iLastConnectServer = -1;
      return;
    }
  }

  if(pProfile->pConnectTimer)
  {
    TimerFree(pProfile->pConnectTimer);
    pProfile->pConnectTimer = 0;
  }

  if(pProfile->pServer)
    return;
  {
    PCONNECTION pConnection;
    PSERVER pServer;
    unsigned short sPort;
    char* strPort;
    char* strPassword;

    if(! (pConnection = ConnectionCreate(0,0,0,g_timeNow,0,0)) )
      return;
    
    if((strPort = strchr(strServer,':')))
    {
      *(strPort++) = '\0';
      sPort = (unsigned short)atol(strPort);      
    }
    else
      sPort = 6667;

    strPassword = strchr(strServer,' ');
    if(strPassword)
      *(strPassword++) = '\0';

    if( !(pServer = ServerCreate(pProfile,strServer,sPort,(!pProfile->listClients.pFirst && pProfile->c_strDetachNick && *pProfile->c_strDetachNick) ? pProfile->c_strDetachNick : pProfile->c_strNick,strPassword)) )
    {
      if(strPort)
        strPort[-1] = ':';
      if(strPassword)
        strPassword[-1] = ' ';
      ASSERT(!pProfile->pServer);
      return;
    }

    ServerConnect(pServer,pConnection);

    if(strPort)
      strPort[-1] = ':';
    if(strPassword)
      strPassword[-1] = ' ';

    ASSERT(pProfile->pServer);
  }
}

void ProfileDisconnect(PPROFILE pProfile, const char* strMessage)
{
  PSERVER pServer = pProfile->pServer;
  if(!pServer)
    return;

  ConnectionSendFormat(pServer->pConnection,"QUIT :%s\r\n",strMessage ? strMessage : "");

  ServerDisconnect(pServer);
}

void ProfileConnectTimer(PPROFILE pProfile)
{
  pProfile->pConnectTimer = 0;
  if(!g_bShutdown)
    ProfileConnect(pProfile,0);
}

PROFILECONFIGVAR_TABLE_START

  PROFILECONFIGVAR("Servers",CV_STR|CV_READONLY|CV_DYNDEFAULT,c_strServers, &c_strDefaultServers, 256)
  PROFILECONFIGVAR("Nick",CV_STR|CV_READONLY,c_strNick, "", 16)
  PROFILECONFIGVAR("AlternativeNick",CV_STR,c_strAlternativeNick, "", 16)
  PROFILECONFIGVAR("DetachNick",CV_STR,c_strDetachNick, "", 16)
  PROFILECONFIGVAR("RealName",CV_STR,c_strRealName, "", 64)
  PROFILECONFIGVAR("Mode",CV_STR|CV_READONLY|CV_DYNDEFAULT,c_strMode, &c_strDefaultMode, 52)

  PROFILECONFIGVAR("Away",CV_BOOL|CV_DYNDEFAULT,c_bAway, &c_bDefaultAway, 0)
  PROFILECONFIGVAR("AwayDefaultReason",CV_STR|CV_DYNDEFAULT,c_strAwayDefaultReason, &c_strDefaultAwayDefaultReason, 256)

  PROFILECONFIGVAR("Channels",CV_STR|CV_READONLY|CV_DYNDEFAULT,c_strChannels, &c_strDefaultChannels, 256)
  PROFILECONFIGVAR("ChannelRejoin",CV_BOOL|CV_DYNDEFAULT,c_bChannelRejoin, &c_bDefaultChannelRejoin, 0)

  PROFILECONFIGVAR("LogChannels",CV_STR|CV_DYNDEFAULT,c_strLogChannels, &c_strDefaultLogChannels, 256)
  PROFILECONFIGVAR("LogChannelAdjustNicklen",CV_BOOL|CV_DYNDEFAULT,c_bLogChannelAdjustNicklen, &c_bDefaultLogChannelAdjustNicklen, 0)
  PROFILECONFIGVAR("LogChannelMessages",CV_UINT|CV_DYNDEFAULT,c_nLogChannelMessages, &c_nDefaultLogChannelMessages, 65535)
  PROFILECONFIGVAR("LogChannelTimestampFormat",CV_STR|CV_DYNDEFAULT,c_strLogChannelTimestampFormat, &c_strDefaultLogChannelTimestampFormat, 32)
  PROFILECONFIGVAR("LogPrivateMessages",CV_UINT|CV_DYNDEFAULT,c_nLogPrivateMessages, &c_nDefaultLogPrivateMessages, 65535)
  PROFILECONFIGVAR("LogPrivateTimestampFormat",CV_STR|CV_DYNDEFAULT,c_strLogPrivateTimestampFormat, &c_strDefaultLogPrivateTimestampFormat, 32)
  
  PROFILECONFIGVAR("Perform",CV_STR,c_strPerform, "", 256)

PROFILECONFIGVAR_TABLE_END

int ProfileConfigSetVar(char bInit /* set by program or user? */,PPROFILE pProfile, const char* var, const char* val, PPROFILECONFIGVAR* ppProfileConfigVar)
{
  PPROFILECONFIGVAR pProfileConfigVar;

  if(!(pProfileConfigVar = ProfileConfigGetVar(pProfile,var)))
    return -1;
  if(ppProfileConfigVar)
    *ppProfileConfigVar = pProfileConfigVar;

  if(!bInit && pProfileConfigVar->cType & CV_READONLY)
    return -2;

  switch(pProfileConfigVar->cType & CV_TYPEMASK)
  {
  case CV_STR:
    if(val)
    {
      if(*(char**)pProfileConfigVar->pVar && !strcmp(*(char**)pProfileConfigVar->pVar,val))
        return 1;
      {
        unsigned int n = strlen(val)+1;
        char* str;
        if(n > pProfileConfigVar->nSize)
          n = pProfileConfigVar->nSize;
        if(!(str = MALLOC(char*,n)))
        {
          OUTOFMEMORY;
          return 0;
        }
        memcpy(str,val,n-1);
        str[n-1] = '\0';
        if(*(char**)pProfileConfigVar->pVar && (pProfileConfigVar->cType & CV_DYNDEFAULT || *(char**)pProfileConfigVar->pVar != (char*)pProfileConfigVar->pDefault))
          FREE(char*,*(char**)pProfileConfigVar->pVar);
        *(char**)pProfileConfigVar->pVar = str;
      }
    }
    else
    {
      char* str;

      if(*(char**)pProfileConfigVar->pVar)
        if(!strcmp(*(char**)pProfileConfigVar->pVar,pProfileConfigVar->cType & CV_DYNDEFAULT ? *(char**)pProfileConfigVar->pDefault : (char*)pProfileConfigVar->pDefault))
          return 1;

      if(pProfileConfigVar->cType & CV_DYNDEFAULT)
      {
        if(!(str = STRDUP(char*,*(char**)pProfileConfigVar->pDefault)))
        {
          OUTOFMEMORY;
          return 0;
        }
      }
      else
        str = (char*)pProfileConfigVar->pDefault;

      if(*(char**)pProfileConfigVar->pVar && (pProfileConfigVar->cType & CV_DYNDEFAULT || *(char**)pProfileConfigVar->pVar != (char*)pProfileConfigVar->pDefault))
        FREE(char*,*(char**)pProfileConfigVar->pVar);
      *(char**)pProfileConfigVar->pVar = str;
    }
    break;
  case CV_UINT:
    if(val)
    {
      unsigned int n;
      if(sscanf(val,"%u",&n) != 1)
      {
        if( (strcasecmp(val,"no") == 0 || strcasecmp(val,"off") == 0) )
          n = 0;
        else
          return -3;
      }

      if(*(unsigned int*)pProfileConfigVar->pVar == n)
        return 1;

      if(n > pProfileConfigVar->nSize)
        n = pProfileConfigVar->nSize;
      *(unsigned int*)pProfileConfigVar->pVar = n;
    }
    else
    {
      unsigned int n = pProfileConfigVar->cType & CV_DYNDEFAULT ? *(unsigned int*)pProfileConfigVar->pDefault : (unsigned int)pProfileConfigVar->pDefault;
      if(*(unsigned int*)pProfileConfigVar->pVar == n)
        return 1;
      *(unsigned int*)pProfileConfigVar->pVar = n;
    }
    break;
  case CV_BOOL:
    if(val)
    {
      int n;
      if(strcasecmp(val,"yes") == 0 ||
        strcasecmp(val,"on") == 0 ||
        (*val == '1' && val[1] == '\0'))
        n = 1;
      else if(strcasecmp(val,"no") == 0 ||
        strcasecmp(val,"off") == 0 ||
        (*val == '0' && val[1] == '\0'))
        n = 0;
      else
        return -4;

      if(*(char*)pProfileConfigVar->pVar == n)
        return 1;

      *(char*)pProfileConfigVar->pVar = n;
    }
    else
    {
      char b = pProfileConfigVar->cType & CV_DYNDEFAULT ? (int)*(char*)pProfileConfigVar->pDefault : (int)pProfileConfigVar->pDefault;
      if(*(char*)pProfileConfigVar->pVar == b)
        return 1;
      *(char*)pProfileConfigVar->pVar = b;
    }
    break;
  }

  pProfile->pUser->bConfigChanged = 1;
  return 1;
}

int ProfileConfigAddVar(PPROFILE pProfile, const char* var, const char* add, PPROFILECONFIGVAR* ppProfileConfigVar)
{
  PPROFILECONFIGVAR pProfileConfigVar;

  ASSERT(add);

  if(!(pProfileConfigVar = ProfileConfigGetVar(pProfile,var)))
    return -1;
  if(ppProfileConfigVar)
    *ppProfileConfigVar = pProfileConfigVar;

  if((pProfileConfigVar->cType & CV_TYPEMASK) != CV_STR)
    return -2;

  {
    unsigned int n = strlen(*(char**)pProfileConfigVar->pVar)+1;
    char* str;
    if(n == 1)
    {
      if(!(str = STRDUP(char*,add)))
      {
        OUTOFMEMORY;
        return 0;
      }
    }
    else
    {
      unsigned int nAdd = strlen(add);
      if(n+nAdd > pProfileConfigVar->nSize)
        return -3;
      if(!(str = MALLOC(char*,n+(++nAdd))))
      {
        OUTOFMEMORY;
        return 0;
      }
      memcpy(str,*(char**)pProfileConfigVar->pVar,n-1);
      str[n-1] = ';';
      memcpy(str+n,add,nAdd);
    }
    if(*(char**)pProfileConfigVar->pVar && *(char**)pProfileConfigVar->pVar != (char*)pProfileConfigVar->pDefault)
      FREE(char*,*(char**)pProfileConfigVar->pVar);
    *(char**)pProfileConfigVar->pVar = str;
  }

  pProfile->pUser->bConfigChanged = 1;
  return 1;
}

int ProfileConfigRemoveVar(PPROFILE pProfile, char* val, const char* remove)
{
  ASSERT(remove);

  {
    char* str;
    unsigned int nRemove = strlen(remove);
    char* strVal = val;
    for(;;)
    {
      str = strcasestr(strVal,remove);
      if(!str)
        break;
      if(str == strVal || str[-1] == ';')
      {
        if(str[nRemove] == ' ' || str[nRemove] == ';' || str[nRemove] == '\0')
        {
          char* strEnd = strchr(str+nRemove,';');
          if(!strEnd)
          {
            if(str > val)
              str--;
            *str = '\0';
            pProfile->pUser->bConfigChanged = 1;
            return 1;
          }
          strcpy(str,strEnd+1);
          pProfile->pUser->bConfigChanged = 1;
          return 1;
        }
      }

      strVal = strchr(str+nRemove,';');
      if(!strVal)
        break;
      strVal++;
    } 
  }

  return 0;
}

PPROFILECONFIGVAR ProfileConfigGetVar(PPROFILE pProfile, const char* var)
{
  PPROFILECONFIGVAR pProfileConfigVar,pEndProfileConfigVar = g_pProfileConfigVars+PROFILECONFIGVAR_COUNT;
  for(pProfileConfigVar = g_pProfileConfigVars; pProfileConfigVar < pEndProfileConfigVar; pProfileConfigVar++)
    if(strcasecmp(pProfileConfigVar->strVarName,var) == 0)
    {
      pProfileConfigVar->pVar = ((char*)pProfileConfigVar->pVarPos)-((char*)&g_profileConfig)+((char*)pProfile);
      return pProfileConfigVar;
    }
  return 0;;
}

PPROFILECONFIGVAR ProfileConfigGetFirstVar(PPROFILE pProfile)
{
  g_pProfileConfigVars->pVar = ((char*)g_pProfileConfigVars->pVarPos)-((char*)&g_profileConfig)+((char*)pProfile);
  return g_pProfileConfigVars;
}

PPROFILECONFIGVAR ProfileConfigGetNextVar(PPROFILE pProfile, PPROFILECONFIGVAR pProfileConfigVar)
{
  pProfileConfigVar++;
  if(pProfileConfigVar >= &g_pProfileConfigVars[PROFILECONFIGVAR_COUNT])
    return 0;
  pProfileConfigVar->pVar = ((char*)pProfileConfigVar->pVarPos)-((char*)&g_profileConfig)+((char*)pProfile);
  return pProfileConfigVar;
}

int ProfilesLoad(char bInit) /* call it after UsersLoad() */
{
  PUSER pUser;
  char strFile[MAX_PATH];
  char* strAddName;
  FILE* fp;
  int iLine;
  char* str,*strEnd;
  PPROFILE pProfile;
  PPROFILECONFIGVAR pProfileConfigVar;

  /* create file path */
  BuildFilename(g_strConfigDir,CONFIGFILE,strFile,sizeof(strFile)-1);
  strAddName = strFile+strlen(strFile);
  *(strAddName++) = '.';
  *strAddName = '\0';

  for(pUser = g_hashlistUsers.pFirst; pUser; pUser = pUser->hll.pNext)
  {
    strncpy(strAddName,pUser->strName,sizeof(strFile)-(strAddName-strFile)-1);
    strFile[sizeof(strFile)-1] = '\0';

    pProfile = 0;

    /* read config file */
    fp = fopen(strFile,"r");
    if(!fp)
    {
      /* Log */
      Log("error: Couldn't load user config from \"%s\"",strFile);
      continue;
    }
    iLine = 0;

    while(fgets(g_strOutBuffer,sizeof(g_strOutBuffer),fp))
    {
      iLine++;
      if( *g_strOutBuffer == '#' ||
        *g_strOutBuffer == ';' ||
        isspace(*g_strOutBuffer))
        continue;

      strEnd = g_strOutBuffer+strlen(g_strOutBuffer)-1;
      while(strEnd > g_strOutBuffer && isspace(*strEnd))
        strEnd--;
      *(++strEnd) = '\0';

      str = g_strOutBuffer;
      while(*str && *str != ' ')
        str++;
      if(*str == ' ')
      {
        *(str++) = '\0';
        while(isspace(*str))
          str++;
      }
/*      else
      {
        Log("error: Missing user config value of \"%s\" (Line %u)",g_strOutBuffer,iLine);
        continue;
      }

*/      
      strEnd--;
      if(str != strEnd && *str == '"' && *strEnd == '"' )
      {
        str++;
        *strEnd = '\0';
      }

      if(!strcasecmp(g_strOutBuffer,"Profile"))
      {
        if(pProfile)
          if(!ProfileOnLoad(pProfile,1))
            ProfileClose(pProfile,0);

        /* profile exists */
        HASHLIST_LOOKUP(USER_PROFILE_HASHLIST,pUser->hashlistProfiles,str,pProfile);

        /* create profile */
        if( pProfile )
          pProfile = 0;
        else
        {
          if(!(pProfile = ProfileCreate(pUser,str,0,0,0)) )
            break;
        }

        continue;
      }


      if(!pProfile)
      {
        if(bInit)
          Log("error: Invalid user config directive position \"%s\" (Line %u)",g_strOutBuffer,iLine);
        continue;
      }

      switch(ProfileConfigSetVar(1,pProfile,g_strOutBuffer,str,&pProfileConfigVar))
      {
      case -1:
        Log("error: Unknown user config directive \"%s\" (Line %u)",g_strOutBuffer,iLine);
        break;
      case -3:
      case -4:
        Log("error: Invalid user config value for \"%s\" (Line %u)",pProfileConfigVar->strVarName,iLine);
        break;
      }
    }

    if(pProfile)
      if(!ProfileOnLoad(pProfile,1))
        ProfileClose(pProfile,0);

    if(bInit)
      pUser->bConfigChanged = 0;

    fclose(fp);
  }

  return 1;
}

int ProfileOnLoad(PPROFILE pProfile, char bInit)
{
  int iReturn = 1;

  if(bInit)
  {

    /* set nick */
    {
      char* strNick;
      ASSERT(!pProfile->strNick);
      ASSERT(*pProfile->c_strNick);
      strNick = STRDUP(char*,(pProfile->c_strDetachNick && *pProfile->c_strDetachNick) ? pProfile->c_strDetachNick : pProfile->c_strNick);
      if(!strNick)
        iReturn = 0;
      else
        pProfile->strNick = strNick;
    }

    /* init channels */
    {
      char* strChannel = pProfile->c_strChannels;
      char* strChannelEnd;
      char* strKey;
      PPROFILECHANNEL pProfileChannel;
      do
      {  
        strChannelEnd = strchr(strChannel,';');
        if(strChannelEnd)
          *strChannelEnd = '\0';
        if(*strChannel)
        {
          strKey = strchr(strChannel,' ');
          if(strKey)
            *(strKey++) = '\0';
          HASHLIST_LOOKUP(PROFILE_PROFILECHANNEL_HASHLIST,pProfile->hashlistProfileChannels,strChannel,pProfileChannel);
          if(!pProfileChannel && !ProfileChannelCreate(pProfile,strChannel,strKey))
            iReturn = 0;
          if(strKey)
            strKey[-1] = ' ';
        }
        if(strChannelEnd)
        {
          *strChannelEnd = ';';          
          strChannel = strChannelEnd+1;
        }          
      } while(strChannelEnd);
    }
  
    /* auto connect */
    ProfileConnect(pProfile,0);
  }
  else
  {
    /* reset channel bLog values */
    {
      PPROFILECHANNEL pProfileChannel;
      char* strChannel = pProfile->c_strLogChannels;
      char* strChannelEnd;
      char bDefLog = *strChannel == '*' ? 1 : 0;

      for(pProfileChannel = pProfile->hashlistProfileChannels.pFirst; pProfileChannel; pProfileChannel = pProfileChannel->hllProfile.pNext)
        pProfileChannel->bLog = bDefLog;

      if(!bDefLog)
        do
        {  
          strChannelEnd = strchr(strChannel,';');
          if(strChannelEnd)
            *strChannelEnd = '\0';
          if(*strChannel)
          {
            HASHLIST_LOOKUP(PROFILE_PROFILECHANNEL_HASHLIST,pProfile->hashlistProfileChannels,strChannel,pProfileChannel);
            if(pProfileChannel)
              pProfileChannel->bLog = 1;
          }
          if(strChannelEnd)
          {
            *strChannelEnd = ';';          
            strChannel = strChannelEnd+1;
          }          
        } while(strChannelEnd);

    }

  }

  if(pProfile->c_nLogChannelMessages > c_nLogChannelMaxMessages)
    pProfile->c_nLogChannelMessages = c_nLogChannelMaxMessages;
  if(pProfile->c_nLogPrivateMessages > c_nLogPrivateMaxMessages)
    pProfile->c_nLogPrivateMessages = c_nLogPrivateMaxMessages;

  return iReturn;
}

int ProfilesDump(void)
{
  PUSER pUser;
  PPROFILE pProfile;
  char strFile[MAX_PATH];
  char* strAddName;
  FILE* fp;
  struct tagPROFILECONFIGVAR* pProfileConfigVar;
  int iReturn = 1;

  /* create file path */
  BuildFilename(g_strConfigDir,CONFIGFILE,strFile,sizeof(strFile)-1);
  strAddName = strFile+strlen(strFile);
  *(strAddName++) = '.';
  *strAddName = '\0';

  if((unsigned int)(strAddName-strFile) >= sizeof(strFile))
    return 0;

  for(pUser = g_hashlistUsers.pFirst; pUser; pUser = pUser->hll.pNext)
    if(pUser->bConfigChanged)
    {
      strncpy(strAddName,pUser->strName,sizeof(strFile)-(strAddName-strFile)-1);
      strFile[sizeof(strFile)-1] = '\0';

      /* create config file */
      fp = fopen(strFile,"w");
      if(!fp)
      {
        /* Log */
        Log("error: Couldn't save user config in \"%s\"",strFile);
        iReturn = 0;
      }

      for(pProfile = pUser->hashlistProfiles.pFirst; pProfile; pProfile = pProfile->hllUser.pNext)
      {
        fprintf(fp,"Profile %s\n\n",pProfile->strName);
    
        pProfileConfigVar = ProfileConfigGetFirstVar(pProfile);
        do
        {
          switch(pProfileConfigVar->cType & CV_TYPEMASK)
          {
          case CV_STR:
            if( pProfileConfigVar->cType & CV_DYNDEFAULT || *(char**)pProfileConfigVar->pVar != (char*)pProfileConfigVar->pDefault) 
              fprintf(fp,"%s \"%s\"\n",pProfileConfigVar->strVarName,*(char**)pProfileConfigVar->pVar ? *(char**)pProfileConfigVar->pVar : "");
            break;
          case CV_UINT:
            if( pProfileConfigVar->cType & CV_DYNDEFAULT || *(unsigned int*)pProfileConfigVar->pVar != (unsigned int)pProfileConfigVar->pDefault) 
              fprintf(fp,"%s %u\n",pProfileConfigVar->strVarName,*(unsigned int*)pProfileConfigVar->pVar);
            break;
          case CV_BOOL:
            if( pProfileConfigVar->cType & CV_DYNDEFAULT || *(char*)pProfileConfigVar->pVar != (int)pProfileConfigVar->pDefault) 
              fprintf(fp,"%s %s\n",pProfileConfigVar->strVarName,*(char*)pProfileConfigVar->pVar ? "on" : "off");
            break;
          }
        } while((pProfileConfigVar = ProfileConfigGetNextVar(pProfile,pProfileConfigVar)));

        fputc('\n',fp);
        fputc('\n',fp);
      }

      fclose(fp);

      pUser->bConfigChanged = 0;
    }

  return iReturn;
}

/* client handlers */

int ClientHandlerProfile(PCLIENT pClient, char* strCommand, char* strParams)
{
  char* str;

  if(*strParams == ':')
    strParams++;
  str = NextArg(&strParams);

  if(*str)
  {
    if( !strcasecmp(str,"add") ||
      !strcasecmp(str,"create") )
    {
      char* strArg[1];
      unsigned int nArgs;
      nArgs = SplitLine(strParams,strArg,sizeof(strArg)/sizeof(*strArg),0);
      if(nArgs > 0)
      {
        PUSER pUser = pClient->pUser;
        PPROFILE pProfile;

        if( !pClient->bAdmin &&
          pClient->pUser->hashlistProfiles.nCount >= c_nUserMaxProfiles)
        {
          ClientMessage(pClient,"You may only have %u profiles",c_nUserMaxProfiles);
          return 1;
        }

        HASHLIST_LOOKUP(USER_PROFILE_HASHLIST,pUser->hashlistProfiles,strArg[0],pProfile);
        if(pProfile)
        {
          ClientMessage(pClient,"Profile \"%s\" already exists",strArg[0]);
          return 1;
        }

        if( !ProfileIsNameValid(strArg[0]) )
        {
          ClientMessage(pClient,"Invalid profile name \"%s\"",strArg[0]);
          return 1;
        }

        if( !(pProfile = ProfileCreate(pUser,strArg[0],pClient->strNick,pClient->strRealName,*c_strDefaultMode ? c_strDefaultMode : pClient->strMode)) )
        {
          ConnectionCloseAsync(pClient->pConnection);
          return 1;
        }

        if(!ClientMessage(pClient,"Profile \"%s\" added",strArg[0]))
          return 1;

        /* send help message */
        if(pUser->hashlistProfiles.nCount == 1)
        {
          if(!ClientMessage(pClient,"Use \"/%s attach <profile>\" to attach this profile.",strCommand) )
            return 1;
        }

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
        PUSER pUser = pClient->pUser;
        PPROFILE pProfile;

        HASH_LOOKUP(USER_PROFILE_HASHLIST,pUser->hashlistProfiles,strArg[0],pProfile);
        if(!pProfile)
        {
          ClientMessage(pClient,"No profile \"%s\" exists",strArg[0]);
          return 1;
        }
        
        if(pClient->pProfile == pProfile)
          ProfileDetach(pProfile,pClient,0);

        ClientMessage(pClient,"Profile \"%s\" removed",pProfile->strName);
        ProfileClose(pProfile,0);
        return 1;
      }
    }
    else if( !strcasecmp(str,"get") ||
           !strcasecmp(str,"show") ||
           !strcasecmp(str,"list") )
    {
      PPROFILE pProfile;

      if(!ClientMessage(pClient,"Profile list:"))
        return 1;

      for(pProfile = pClient->pUser->hashlistProfiles.pFirst; pProfile; pProfile = pProfile->hllUser.pNext)
      {
        if(pProfile->pServer)
        {
          if(!ClientMessage(pClient,"%s (connected to \"%s\")\r\n",pProfile->strName,pProfile->pServer->strServer))
            return 1;
        }
        else
        {
          if(!ClientMessage(pClient,"%s (disconnected)\r\n",pProfile->strName))
            return 1;
        }
      }

      return 1;
    }
    else if(!strcasecmp(str,"attach") ||
      !strcasecmp(str,"use") )
    {
      char* strArg[2];
      unsigned int nArgs;
      nArgs = SplitLine(strParams,strArg,sizeof(strArg)/sizeof(*strArg),1);
      if(nArgs > 0)
      {
        PUSER pUser = pClient->pUser;
        PPROFILE pProfile;

        HASHLIST_LOOKUP(USER_PROFILE_HASHLIST,pUser->hashlistProfiles,strArg[0],pProfile);
        if(!pProfile)
        {
          ClientMessage(pClient,"Profile \"%s\" doesn't exist",strArg[0]);
          return 1;
        }

        if(pClient->pProfile == pProfile)
          return 1; // do nothing since the profile is already attached

        if(pClient->pProfile)
        {
          PPROFILE pProfile = pClient->pProfile;
          if(!ProfileDetach(pProfile,pClient,0))
            return 1;
          ASSERT(pClient->pProfile == 0);
        }

        if(!ProfileAttach(pProfile,pClient,nArgs > 1 ? strArg[1] : 0))
          return 1;

        ASSERT(pClient->pProfile == pProfile);

        if(!pProfile->pServer && !pProfile->pConnectTimer)
        {
          if( (pUser->strAllowedCommands && ConfigFindVar(pUser->strAllowedCommands,"SERVER")) &&
            (!pProfile->c_strServers || !*pProfile->c_strServers) )
          {
            /* send help message */
            if(!strncasecmp(strCommand,"notice ",sizeof("notice ")-1))
            {
              if(!ClientMessage(pClient,"Use \"/notice %s SERVER add <server>[:<port>] [<password>]\" to add a server.",c_strBouncerName) )
                return 1;
            }
            else if(!strncasecmp(strCommand,"msg ",sizeof("msg ")-1))
            {
              if(!ClientMessage(pClient,"Use \"/msg %s SERVER add <server>[:<port>] [<password>]\" to add a server.",c_strBouncerName) )
                return 1;
            }
            else
            {
              if(!ClientMessage(pClient,"Use \"/SERVER add <server>[:<port>] [<password>]\" to add a server.") )
                return 1;
            }
          }
          else
            ProfileConnect(pProfile,0);
        }

        return 1;        
      }
    }
    else if(!strcasecmp(str,"detach"))
    {
      PPROFILE pProfile = pClient->pProfile;
      char* strArg[1];
      unsigned int nArgs;
      nArgs = SplitLine(strParams,strArg,sizeof(strArg)/sizeof(*strArg),1);

      if(!pProfile)
      {
        ClientMessage(pClient,"No profile attached");
        return 1;
      }
      if(!ProfileDetach(pProfile,pClient,nArgs > 0 ? strArg[0] : 0))
        return 1;
      ASSERT(pClient->pProfile == 0);
      return 1;
    }
  }
  
  if( !ClientMessage(pClient,"Usage is /%s add <profile>",strCommand) ||
    !ClientMessage(pClient,"         /%s remove <profile>",strCommand) ||
    !ClientMessage(pClient,"         /%s get",strCommand) ||
    !ClientMessage(pClient,"         /%s attach <profile> [<message>]",strCommand) ||
    !ClientMessage(pClient,"         /%s detach [<message>]",strCommand) )
    return 1;

  return 1;
  strCommand = 0;
}
