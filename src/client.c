/*
         file: client.c
   desciption: handle client commands
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "bounced.h"

size_t vstrformat(char* str, size_t maxsize, const char* format, va_list ap);


CLIENTHANDLERUNREGISTERED_TABLE_START

  CLIENTHANDLER("PASS",ClientHandlerUnregisteredPass,CHF_NORMAL)
  CLIENTHANDLER("USER",ClientHandlerUnregisteredUser,CHF_NORMAL)
  CLIENTHANDLER("NICK",ClientHandlerUnregisteredNick,CHF_NORMAL)

  CLIENTHANDLER("DBACCESS",ClientHandlerUnregisteredDBAccess,CHF_NORMAL)

CLIENTHANDLERUNREGISTERED_TABLE_END

CLIENTHANDLER_TABLE_START

  CLIENTHANDLER("PASS"    ,ClientHandlerPass    , CHF_NORMAL)
  CLIENTHANDLER("USER"    ,ClientHandlerUser    , CHF_NORMAL)

  CLIENTHANDLER("NICK"    ,ClientHandlerNick    , CHF_NORMAL) /* nick.c */
  CLIENTHANDLER("QUIT"    ,ClientHandlerQuit    , CHF_NORMAL)
  CLIENTHANDLER("PING"    ,ClientHandlerPing    , CHF_NORMAL)
  CLIENTHANDLER("PONG"    ,ClientHandlerPong    , CHF_NORMAL) /* ping.c */
  CLIENTHANDLER("NOTICE"  ,ClientHandlerNotice  , CHF_NORMAL) /* privmsg.c */
  CLIENTHANDLER("PRIVMSG" ,ClientHandlerPrivmsg , CHF_NORMAL) /* privmsg.c */
  CLIENTHANDLER("JOIN"    ,ClientHandlerJoin    , CHF_NORMAL) /* channel.c */
  CLIENTHANDLER("PART"    ,ClientHandlerPart    , CHF_NORMAL) /* channel.c */
  CLIENTHANDLER("MODE"    ,ClientHandlerMode    , CHF_NORMAL) /* mode.c */

  CLIENTHANDLER("PROFILE" ,ClientHandlerProfile , CHF_BOUNCER)
  CLIENTHANDLER("SERVER"  ,ClientHandlerServer  , CHF_BOUNCER)
  CLIENTHANDLER("PASSWORD",ClientHandlerPassword, CHF_BOUNCER) /* password.c */
  CLIENTHANDLER("CONFIG"  ,ClientHandlerConfig  , CHF_BOUNCER)
  CLIENTHANDLER("HELP"    ,ClientHandlerHelp    , CHF_BOUNCER)

  CLIENTHANDLER("ADMIN"   ,ClientHandlerAdmin   , CHF_BOUNCER|CHF_ADMIN)
  
CLIENTHANDLER_TABLE_END

int ClientHandlersInit(void)
{
  unsigned int i;

  HASH_INIT(CLIENTHANDLER_HASH,g_hashClientHandlers);
  for(i = 0; i < CLIENTHANDLER_COUNT; i++)
    HASH_ADD(CLIENTHANDLER_HASH,g_hashClientHandlers,&g_pClientHandlers[i]);

  HASH_INIT(CLIENTHANDLERUNREGISTERED_HASH,g_hashClientHandlersUnregistered);
  for(i = 0; i < CLIENTHANDLERUNREGISTERED_COUNT; i++)
    HASH_ADD(CLIENTHANDLERUNREGISTERED_HASH,g_hashClientHandlersUnregistered,&g_pClientHandlersUnregistered[i]);

  return 1;
}

PCLIENT ClientCreate(void)
{
  PCLIENT pClient;

  if( !(pClient = CALLOC(PCLIENT,1,sizeof(CLIENT))) )
  {
    OUTOFMEMORY;
    if(pClient)
      ClientFree(pClient);
    return 0;
  }

  LIST_ADD(CLIENT_LIST,g_listClients,pClient);

  return pClient;
}

void ClientFree(PCLIENT pClient)
{
  if(pClient->pProfile)
  {
    ProfileDetach(pClient->pProfile,pClient,0);
    ASSERT(pClient->pProfile == 0);
  }

  if(pClient->pUser)
  {
    UserDetach(pClient->pUser,pClient);
    ASSERT(pClient->pUser == 0);
  }

  if(pClient->strName)
    FREE(char*,pClient->strName);
  if(pClient->strNick)
    FREE(char*,pClient->strNick);
  if(pClient->strRealName)
    FREE(char*,pClient->strRealName);
  if(pClient->strMode)
    FREE(char*,pClient->strMode);
  FREE(PCLIENT,pClient);
}

void ClientClose(PCLIENT pClient)
{
  ASSERT(0);
  return;
  pClient = 0;
}

int ClientMessage(PCLIENT pClient, const char* format,...)
{
  va_list ap;
  unsigned short sLength;

  if(pClient->pConnection->bClosing)
    return 0;

  if(pClient->cMessageMode == CMM_PRIVMSG)
    sLength = strformat(g_strOutBuffer,sizeof(g_strOutBuffer)-2,":%s PRIVMSG %s :",c_strBouncerName,pClient->strNick);
  else
    sLength = strformat(g_strOutBuffer,sizeof(g_strOutBuffer)-2,":%s NOTICE %s :",c_strBouncerName,pClient->strNick);

  va_start (ap, format);
  sLength += vstrformat(g_strOutBuffer+sLength,sizeof(g_strOutBuffer)-2-sLength,format, ap);
  va_end (ap);

  g_strOutBuffer[sLength++] = '\r';
  g_strOutBuffer[sLength++] = '\n';
  g_strOutBuffer[sLength] = '\0';

  return ConnectionSend(pClient->pConnection,g_strOutBuffer,sLength);
}

int ClientRegister(PCLIENT pClient)
{
  PUSER pUser;
  char bAdmin;

  ASSERT(pClient->strNick);
  ASSERT(pClient->strName);
  ASSERT(!pClient->bRegistered);

  HASHLIST_LOOKUP(USER_HASHLIST,g_hashlistUsers,pClient->strName,pUser);
  if(!pUser || !PasswordCompare(pUser->pcMD5Pass,pClient->pcMD5Pass) )
  {
    Log("Invalid login from %s (%s)",iptoa(pClient->pConnection->nIP),pClient->strName);
    if(!ConnectionSendFormat(pClient->pConnection,":%s 464 %s :Password incorrect\r\n",c_strBouncerName,pClient->strNick))
      return 0;
    ConnectionCloseAsync(pClient->pConnection);
    return 0;
  }

  bAdmin = pUser->strAllowedCommands && ConfigFindVar(pUser->strAllowedCommands,"ADMIN");
  if( (!bAdmin && pUser->listClients.nCount >= c_nUserMaxClients) ||
    (!bAdmin && UserHasFlag(pUser,'b')) )
  {
    if(!ConnectionSendFormat(pClient->pConnection,":%s 464 %s :Password incorrect\r\n",c_strBouncerName,pClient->strNick))
      return 0;
    ConnectionCloseAsync(pClient->pConnection);
    return 0;
  }

  g_dLogins++;

  pUser->timeLastSeen = g_timeNow;
  g_bUsersChanged = 1;
  pClient->bAdmin = bAdmin;
  pClient->bRegistered = 1;
  strcpy(pClient->strName,pUser->strName); /* maybe case is different */

  if(!UserAttach(pUser,pClient))
    return 0;

  return 1;
}

int ClientHandleCommand(PCLIENT pClient, char* strCommand, unsigned int nLength)
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

  {
    struct tagCLIENTHANDLER* pHandler;
    if(pClient->bRegistered)
    {
      HASH_LOOKUP(CLIENTHANDLER_HASH,g_hashClientHandlers,strAction,pHandler);
      if(pHandler && ( 
        (pHandler->nFlags & CHF_BOUNCER && !(pHandler->nFlags & CHF_ADMIN) && (!pClient->pUser->strAllowedCommands || !ConfigFindVar(pClient->pUser->strAllowedCommands,pHandler->strAction))) ||
        (pHandler->nFlags & CHF_ADMIN && !pClient->bAdmin) ) )
        pHandler = 0;
    }
    else
      HASH_LOOKUP(CLIENTHANDLERUNREGISTERED_HASH,g_hashClientHandlersUnregistered,strAction,pHandler)    
      
    if(pHandler)
    {      
      memcpy(g_strInBufferCopy,strCommand,nLength+1);

      if(strActionEnd)
        *strActionEnd = cActionEnd;

      g_strCurrentCommand = strCommand;
      g_nCurrentCommandLength = nLength;

      if( !pHandler->pProc(pClient,g_strInBufferCopy,g_strInBufferCopy+(str-strCommand)) &&
        pClient->pProfile && pClient->pProfile->pServer && pClient->pProfile->pServer->bRegistered)
        return ConnectionSend(pClient->pProfile->pServer->pConnection,strCommand,nLength);
    }
    else
    {
      

      if(!pClient->bRegistered)
        return ConnectionSendFormat(pClient->pConnection,":%s 451 %s :You have not registered\r\n",c_strBouncerName,pClient->strNick ? pClient->strNick : "*");

      if(pClient->pProfile && pClient->pProfile->pServer && pClient->pProfile->pServer->bRegistered)
      {
        if(strActionEnd)
          *strActionEnd = cActionEnd;
        return ConnectionSend(pClient->pProfile->pServer->pConnection,strCommand,nLength);
      }
    }
  }


  return 1;
}

/* unrigistered client handlers */

int ClientHandlerUnregisteredPass(PCLIENT pClient, char* strCommand, char* strParams)
{
  char* strArg[1];

  ASSERT(!pClient->bRegistered);

  if(SplitIRCParams(strParams,strArg,sizeof(strArg)/sizeof(*strArg),1) != sizeof(strArg)/sizeof(*strArg))
  {
    ConnectionSendFormat(pClient->pConnection,":%s 461 %s PASS :Not enough parameters\r\n",c_strBouncerName,pClient->strNick ? pClient->strNick : "*");
    return 1;
  }

  PasswordCreate(strArg[0],pClient->pcMD5Pass);
  return 1;
  strCommand = 0;
}

int ClientHandlerUnregisteredUser(PCLIENT pClient, char* strCommand, char* strParams)
{
  char* strArg[4];

  ASSERT(!pClient->bRegistered);

  if(SplitIRCParams(strParams,strArg,sizeof(strArg)/sizeof(*strArg),1) != sizeof(strArg)/sizeof(*strArg))
  {
    ConnectionSendFormat(pClient->pConnection,":%s 461 %s USER :Not enough parameters\r\n",c_strBouncerName,pClient->strNick ? pClient->strNick : "*");
    return 1;
  }

  if(!UserIsNameValid(strArg[0]))
  {
    ConnectionCloseAsync(pClient->pConnection);
    return 1;
  }

  if(pClient->strName)
    FREE(char*,pClient->strName);
  if(!(pClient->strName = STRDUP(char*,strArg[0])))
  {
    OUTOFMEMORY;
    ConnectionCloseAsync(pClient->pConnection);
    return 1;
  }
  if(pClient->strRealName)
    FREE(char*,pClient->strRealName);
  if(!(pClient->strRealName = STRDUP(char*,strArg[3])))
  {
    OUTOFMEMORY;
    ConnectionCloseAsync(pClient->pConnection);
    return 1;
  }

  if(pClient->strNick)
  {
    ClientRegister(pClient);
    return 1;
  }

  return 1;
  strCommand = 0;
}

int ClientHandlerUnregisteredNick(PCLIENT pClient, char* strCommand, char* strParams)
{
  char* strArg[1];

  ASSERT(!pClient->bRegistered);

  if(SplitIRCParams(strParams,strArg,sizeof(strArg)/sizeof(*strArg),0) != sizeof(strArg)/sizeof(*strArg))
  {
    ConnectionSendFormat(pClient->pConnection,":%s 431 %s :No nickname given\r\n",c_strBouncerName,pClient->strNick ? pClient->strNick : "*");
    return 1;
  }

  if(pClient->strNick && !strcmp(pClient->strNick,strArg[0]))
    return 1;

  if(!NickIsValid(strArg[0],DEFAULT_NICKLEN) )
  {
    ConnectionSendFormat(pClient->pConnection,":%s 432 %s <nick> :Erroneus nickname\r\n",c_strBouncerName,pClient->strNick ? pClient->strNick : "*");
    return 1;
  }

  if(pClient->strNick)
    FREE(char*,pClient->strNick);
  if(!(pClient->strNick = STRDUP(char*,strArg[0])))
  {
    OUTOFMEMORY;
    ConnectionCloseAsync(pClient->pConnection);
    return 0;
  }

  if(pClient->strName)
  {
    ClientRegister(pClient);
    return 1;
  }

  return 1;
  strCommand = 0;
}

/* client handlers */

int ClientHandlerPass(PCLIENT pClient, char* strCommand, char* strParams)
{
  ConnectionSendFormat(pClient->pConnection,":%s 462 %s :You may not reregister\r\n",c_strBouncerName,pClient->strNick);
  return 1;
  strParams = 0;
  strCommand = 0;
}

int ClientHandlerUser(PCLIENT pClient, char* strCommand, char* strParams)
{
  ConnectionSendFormat(pClient->pConnection,":%s 462 %s :You may not reregister\r\n",c_strBouncerName,pClient->strNick);
  return 1;
  strParams = 0;
  strCommand = 0;
}

int ClientHandlerHelp(PCLIENT pClient, char* strCommand, char* strParams)
{
  unsigned int i;
  char* strRealAction;
  char b = 0;
  PUSER pUser = pClient->pUser;

  ASSERT(pUser);
  
  strRealAction = strrchr(strCommand,' ');
  if(strRealAction)
    strRealAction++;
  else
    strRealAction = strCommand;

  for(i = 0; i < CLIENTHANDLER_COUNT; i++)
    if( g_pClientHandlers[i].nFlags & CHF_BOUNCER &&
      g_pClientHandlers[i].pProc != ClientHandlerHelp &&
      ((pUser->strAllowedCommands && ConfigFindVar(pUser->strAllowedCommands,g_pClientHandlers[i].strAction)) || (g_pClientHandlers[i].nFlags & CHF_ADMIN && pClient->bAdmin)) )
    {
      strcpy(strRealAction,g_pClientHandlers[i].strAction);
      if(!ClientMessage(pClient,b ? "         /%s help" : "Usage is /%s help",strCommand) )
        return 1;
      b = 1;
    }

  return 1;
  strParams = 0;
  strCommand = 0;
}
