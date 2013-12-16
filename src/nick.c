/*
         file: nick.c
   desciption: nick handlers
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
#include <stdlib.h>
#include <ctype.h>

#include "bounced.h"

/* helper functions */

int NickIsValid(char* strNick, unsigned int nMaxNicklen)
{
  ASSERT(strNick);
  ASSERT(nMaxNicklen > 0);

  if(!strNick || nMaxNicklen == 0)
    return 0;

/* old */
/*  if( !*strNick ||
    strchr("@%+#$&!:",*strNick) ||
    strchr(strNick,',') )
    return 0;
*/
  /* new (see rfc2812) */
  if( (*strNick < 'A' || *strNick > 'Z') && (*strNick < 'a' || *strNick > 'z') &&
    (*strNick < 0x5B || *strNick > 0x60) && (*strNick < 0x7B || *strNick > 0x7D) )
    return 0;
  {
    char* str;
    for(str = strNick+1; *str; str++)
    {
      if((unsigned int)(str-strNick) >= nMaxNicklen)
      {
        *str = '\0';
        return 1;
      }
      if( (*str < 'A' || *str > 'Z') && (*str < 'a' || *str > 'z') &&
        (*str < '0' || *str > '9') &&
        (*str < 0x5B || *str > 0x60) && (*str < 0x7B || *str > 0x7D) &&
        *str != '-' )
      return 0;
    }
  }

  return 1;
}

/* unregistered server handlers */

int ServerHandlerUnregisteredNickError(PSERVER pServer, char* strCommand, char* strParams)
{
  PPROFILE pProfile = pServer->pProfile;
  char* strAlternativeNick = 0;

  ASSERT(!pServer->bRegistered);

  ASSERT(pServer->strNick);
  if(!pServer->strNick)
    return 1;
  
  /* find alternative nick */
  if((pProfile->c_strDetachNick && *pProfile->c_strDetachNick) && strcmp(pProfile->c_strDetachNick,pProfile->c_strNick) && !strcmp(pProfile->c_strDetachNick,pServer->strNick))
    strAlternativeNick = pProfile->c_strNick;
  else if((pProfile->c_strAlternativeNick && *pProfile->c_strAlternativeNick) && strcmp(pProfile->c_strAlternativeNick,pProfile->c_strNick) && strcmp(pProfile->c_strAlternativeNick,pServer->strNick))
    strAlternativeNick = pProfile->c_strAlternativeNick;

  /* use alternative nick */
  if(strAlternativeNick)
  {  
    if( !ConnectionSendFormat(pServer->pConnection,"NICK :%s\r\n",strAlternativeNick) )
      return 1;

    if(!(strAlternativeNick = STRDUP(char*,strAlternativeNick)))
    {
      OUTOFMEMORY;
      ConnectionCloseAsync(pServer->pConnection);
      return 1;
    }

    FREE(char*,pServer->strNick);
    pServer->strNick = strAlternativeNick;
    return 1;
  }

  ConnectionCloseAsync(pServer->pConnection);

  return 0; /* 0, damit der client sieht, warum es nicht geklappt hat... */
  strParams = 0;
  strCommand = 0;
}

/* server handlers */

int ServerHandlerNick(PSERVER pServer, char* strCommand, char* strParams)
{
  char* strArg[1];
  char* strNick;

  if( SplitIRCParams(strParams,strArg,sizeof(strArg)/sizeof(*strArg),0) != sizeof(strArg)/sizeof(*strArg) ||
    !SplitIRCPrefixNick(strCommand,&strNick))
    return 1;

  if(!strcmp(strNick,strArg[0]) || !NickIsValid(strArg[0],pServer->pProfile->nNickLen) )
    return 1;
  
  /* is our nick change? */
  if(!strcasecmp(strNick,pServer->strNick))
  {
    PPROFILE pProfile = pServer->pProfile;
    PCLIENT pClient;
    char* strNewNick;

    ASSERT(pProfile);

    /* change nick in welcome message */
    if(pServer->listServerWelcomeMsgs.pFirst && pServer->listServerWelcomeMsgs.pFirst->nCode == 1)
    {
      PSERVERWELCOMEMSG pServerWelcomeMsg = pServer->listServerWelcomeMsgs.pFirst;
      char* str;
      unsigned int nNick = strlen(pServer->strNick);
      str = strstr(pServerWelcomeMsg->strMsg,pServer->strNick);
      while(str)  
      {
        if(isspace(str[nNick]) || str[nNick] == '!')
        {
          char* strCommand;
          unsigned int nNewNick = strlen(strArg[0]);
          unsigned int nMsg = strlen(pServerWelcomeMsg->strMsg);

          strCommand = MALLOC(char*,nMsg-nNick+nNewNick+1);
          if(!strCommand)
          {
            OUTOFMEMORY;
            ConnectionCloseAsync(pServer->pConnection);
            return 1;  
          }

          memcpy(strCommand,pServerWelcomeMsg->strMsg,str-pServerWelcomeMsg->strMsg);
          memcpy(strCommand+(str-pServerWelcomeMsg->strMsg),strArg[0],nNewNick);
          memcpy(strCommand+(str-pServerWelcomeMsg->strMsg)+nNewNick,str+nNick,nMsg-nNick-(str-pServerWelcomeMsg->strMsg)+1);

          FREE(char*,pServerWelcomeMsg->strMsg);
          pServerWelcomeMsg->strMsg = strCommand;

          break;
        }
        str = strstr(str+1,pServer->strNick);
      }
    }

    /* change nick in SERVER struct */
    if(!(strNewNick = STRDUP(char*,strArg[0])))
    {
      OUTOFMEMORY;
      ConnectionCloseAsync(pServer->pConnection);
      return 1;
    }
    if(pServer->strNick)
      FREE(char*,pServer->strNick);
    pServer->strNick = strNewNick;

    /* change nick in PROFILE struct */
    if(!(strNewNick = STRDUP(char*,strArg[0])))
    {
      OUTOFMEMORY;
      ConnectionCloseAsync(pServer->pConnection);
      return 1;
    }
    if(pProfile->strNick)
      FREE(char*,pProfile->strNick);
    pProfile->strNick = strNewNick;

    /* change nicks in CLIENT structs */
    for(pClient = pProfile->listClients.pFirst; pClient; pClient = pClient->llProfile.pNext)
    {
      if(!ConnectionSendFormat(pClient->pConnection,":%s!~%s@%s NICK :%s\r\n",pClient->strNick,pClient->strName,iptoa(pClient->pConnection->nIP),strArg[0]))
        continue;

      if(!(strNewNick = STRDUP(char*,strArg[0])))
      {
        OUTOFMEMORY;
        ConnectionCloseAsync(pClient->pConnection);
        continue;
      }

      if(pClient->strNick)
        FREE(char*,pClient->strNick);
      pClient->strNick = strNewNick;
    }

    /* force adding nick command to channel logs (not only the channels where HASHLIST_LOOKUP is successful) */
    { 
      PPROFILE pProfile = pServer->pProfile;
      PPROFILECHANNEL pProfileChannel;
      PPROFILECHANNELUSER pProfileChannelUser;
      char* strNewNick;

      pProfile->nLogID++;

      for(pProfileChannel = pProfile->hashlistProfileChannels.pFirst; pProfileChannel; pProfileChannel = pProfileChannel->hllProfile.pNext)
      {
        ProfileLogMsgCreate(0,pProfileChannel,pProfile->nLogID,g_strCurrentCommand,g_nCurrentCommandLength,0);

        HASHLIST_LOOKUP(PROFILECHANNEL_PROFILECHANNELUSER_HASHLIST,pProfileChannel->hashlistProfileChannelUsers,strNick,pProfileChannelUser);
        if(pProfileChannelUser)
        {
          strNewNick = STRDUP(char*,strArg[0]);
          if(strNewNick)
          {
            HASHLIST_REMOVE_ITEM_NOFREE(PROFILECHANNEL_PROFILECHANNELUSER_HASHLIST,pProfileChannel->hashlistProfileChannelUsers,pProfileChannelUser);
            if(pProfileChannelUser->strNick)
              FREE(char*,pProfileChannelUser->strNick);
            pProfileChannelUser->strNick = strNewNick;
            HASHLIST_ADD(PROFILECHANNEL_PROFILECHANNELUSER_HASHLIST,pProfileChannel->hashlistProfileChannelUsers,pProfileChannelUser);
          }
        }
      }
    }

    return 1;
  }
  else
  {
    /* add nick command to channel logs*/
    {
      PPROFILE pProfile = pServer->pProfile;
      PPROFILECHANNEL pProfileChannel;
      PPROFILECHANNELUSER pProfileChannelUser;
      char* strNewNick;

      pProfile->nLogID++;

      for(pProfileChannel = pProfile->hashlistProfileChannels.pFirst; pProfileChannel; pProfileChannel = pProfileChannel->hllProfile.pNext)
      {
        HASHLIST_LOOKUP(PROFILECHANNEL_PROFILECHANNELUSER_HASHLIST,pProfileChannel->hashlistProfileChannelUsers,strNick,pProfileChannelUser);
        if(pProfileChannelUser)
        {
          ProfileLogMsgCreate(0,pProfileChannel,pProfile->nLogID,g_strCurrentCommand,g_nCurrentCommandLength,0);

          strNewNick = STRDUP(char*,strArg[0]);
          if(strNewNick)
          {
            HASHLIST_REMOVE_ITEM_NOFREE(PROFILECHANNEL_PROFILECHANNELUSER_HASHLIST,pProfileChannel->hashlistProfileChannelUsers,pProfileChannelUser);
            if(pProfileChannelUser->strNick)
              FREE(char*,pProfileChannelUser->strNick);
            pProfileChannelUser->strNick = strNewNick;
            HASHLIST_ADD(PROFILECHANNEL_PROFILECHANNELUSER_HASHLIST,pProfileChannel->hashlistProfileChannelUsers,pProfileChannelUser);
          }
        }
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
    if(strPreferedNick && !strcasecmp(strNick,strPreferedNick) && strcasecmp(pServer->strNick,strPreferedNick))
      ConnectionSendFormat(pServer->pConnection,"NICK :%s\r\n",strPreferedNick);
  }

  return 0;
}

/* client handlers */

int ClientHandlerNick(PCLIENT pClient, char* strCommand, char* strParams)
{
  char* strArg[1];

  if(SplitIRCParams(strParams,strArg,sizeof(strArg)/sizeof(*strArg),0) != sizeof(strArg)/sizeof(*strArg))
  {
    ConnectionSendFormat(pClient->pConnection,":%s 431 %s :No nickname given\r\n",c_strBouncerName,pClient->strNick);
    return 1;
  }

  if(pClient->pProfile && pClient->pProfile->pServer &&
    !strcmp(pClient->pProfile->pServer->strNick,strArg[0]))
    return 1;

  if(!NickIsValid(strArg[0],pClient->pProfile ? pClient->pProfile->nNickLen : DEFAULT_NICKLEN) )
  {
    ConnectionSendFormat(pClient->pConnection,":%s 432 %s <nick> :Erroneus nickname\r\n",c_strBouncerName,pClient->strNick);
    return 1;
  }

  /* no profile.. just accept the new nick */
  if(!pClient->pProfile)
  {
    char* strNewNick;
  
    if(!ConnectionSendFormat(pClient->pConnection,":%s!~%s@%s NICK :%s\r\n",pClient->strNick,pClient->strName,iptoa(pClient->pConnection->nIP),strArg[0]))
      return 1;

    if(!(strNewNick = STRDUP(char*,strArg[0])))
    {
      OUTOFMEMORY;
      ConnectionCloseAsync(pClient->pConnection);
      return 1;
    }
    if(pClient->strNick)
      FREE(char*,pClient->strNick);
    pClient->strNick = strNewNick;

    return 1;
  }

  else
  {
    char* strNewNick;
    PPROFILE pProfile = pClient->pProfile;

    /* use nick as prefered nick */
    if(ProfileConfigSetVar(1,pProfile,"Nick",strArg[0],0) <= 0)
    {
      ConnectionCloseAsync(pClient->pConnection);
      return 1;
    }

    /* if server connection, send nickchange to server */
    if(pProfile->pServer && pProfile->pServer->bRegistered)
    {
      ConnectionSendFormat(pProfile->pServer->pConnection,"NICK :%s\r\n",strArg[0]);
      return 1;
    }
    else
    {
      /* attach nick to all profile clients */
      PCLIENT pClient;
      for(pClient = pProfile->listClients.pFirst; pClient; pClient = pClient->llProfile.pNext)
      {
        if(!ConnectionSendFormat(pClient->pConnection,":%s!~%s@%s NICK :%s\r\n",pClient->strNick,pClient->strName,iptoa(pClient->pConnection->nIP),strArg[0]))
          continue;

        if(!(strNewNick = STRDUP(char*,strArg[0])))
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
  }
 
  return 1;
  strCommand = 0;
}
