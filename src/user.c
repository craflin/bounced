/*
         file: user.c
   desciption: load user file
        begin: 10/25/03
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

#include "bounced.h"

int UserIsNameValid(const char* strName)
{
  ASSERT(strName);
  if(!strName)
    return 0;
  if( !*strName ||
    strpbrk(strName,"!~@ "))
    return 0;
  return 1;
}

int UserSetAllowedCommands(PUSER pUser, const char* strCommands) /* 0 = out of memory, 1 = success */
{
  ASSERT(pUser);
  ASSERT(strCommands);
  {
    const char* strCommand;
    const char* strCommandEnd;
    char strCommandCopy[8+1];
    struct tagCLIENTHANDLER* pClientHandler;
    unsigned int nLength;
     
    /* remove all currently allowed commands */
    if(pUser->strAllowedCommands)
    {
      FREE(char*,pUser->strAllowedCommands);
      pUser->strAllowedCommands = 0;
    }

    /* check and add commands */
    strCommand = strCommands;
    do
    {  
      strCommandEnd = strchr(strCommand,';');
      nLength = strCommandEnd ? (unsigned int)(strCommandEnd-strCommand) : strlen(strCommand);

      if(nLength < sizeof(strCommandCopy))
      {
        memcpy(strCommandCopy,strCommand,nLength);
        strCommandCopy[nLength] = '\0';
      }
      else
        *strCommandCopy = '\0';

      if(*strCommandCopy)
      {
        HASH_LOOKUP(CLIENTHANDLER_HASH,g_hashClientHandlers,strCommandCopy,pClientHandler);
        if(pClientHandler)
          if(!UserAllowCommand(pUser,strCommandCopy))
            return 0;
      }
        
      if(strCommandEnd)
        strCommand = strCommandEnd+1;
    } while(strCommandEnd);
  }
          
  return 1;
}

int UserAllowCommand(PUSER pUser, const char* strCommand)
{
  ASSERT(pUser);
  ASSERT(strCommand);

  /* check if command is valid */
/*  {
    PCLIENTHANDLER pClientHandler;
    HASH_LOOKUP(CLIENTHANDLER_HASH,g_hashClientHandlers,strCommand,pClientHandler);
    if(!pClientHandler)
      return 0;
  }
*/
  if(!pUser->strAllowedCommands)
  {
    if(!(pUser->strAllowedCommands = STRDUP(char*,strCommand)))
    {
      OUTOFMEMORY;
      return 0;
    }
    return 1;
  }

  if(ConfigFindVar(pUser->strAllowedCommands,strCommand))
    return 1;

  {
    int n = strlen(pUser->strAllowedCommands);
    char* strAllowedCommands;
    if(n > 0 && pUser->strAllowedCommands[n-1] == ';')
      pUser->strAllowedCommands[--n] = '\0';
    if(!(strAllowedCommands = MALLOC(char*,n+1+strlen(strCommand)+1)))
    {
      OUTOFMEMORY;
      return 0;
    }
    memcpy(strAllowedCommands,pUser->strAllowedCommands,n);
    strAllowedCommands[n++] = ';';
    strcpy(strAllowedCommands+n,strCommand);
    FREE(char*,pUser->strAllowedCommands);
    pUser->strAllowedCommands = strAllowedCommands;
  }
  return 1;
}

int UserDisallowCommand(PUSER pUser, const char* strCommand)
{
  ASSERT(strCommand);

  if(!pUser->strAllowedCommands)
    return 1;

  {
    char* str;
    unsigned int nRemove = strlen(strCommand);
    char* strVal = pUser->strAllowedCommands;
    for(;;)
    {
      str = strcasestr(strVal,strCommand);
      if(!str)
        break;
      if(str == strVal || str[-1] == ';')
      {
        if(str[nRemove] == ' ' || str[nRemove] == ';' || str[nRemove] == '\0')
        {
          char* strEnd = strchr(str+nRemove,';');
          if(!strEnd)
          {
            if(str > pUser->strAllowedCommands)
              str--;
            else if(str == pUser->strAllowedCommands)
            {
              FREE(char*,pUser->strAllowedCommands);
              pUser->strAllowedCommands = 0;
            }
            else
              *str = '\0';
            return 1;

          }
          strcpy(str,strEnd+1);
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

PUSER UserCreate(const char* strName, const char* pcMD5Pass, const char* strEmail, const char* strFlags, const char* strAllowedCommands, const char* strConnectInterface, time_t timeRegistered, time_t timeLastSeen)
{
  PUSER pUser;
  ASSERT(strName);
  ASSERT(strAllowedCommands);
  if( !(pUser = CALLOC(PUSER,1,sizeof(USER))) ||
    !(pUser->strName = STRDUP(char*,strName)) ||
    !(pUser->strEmail = STRDUP(char*,strEmail ? strEmail : ""))  ||
    (strAllowedCommands && !UserSetAllowedCommands(pUser,strAllowedCommands)) ||
    !(pUser->strConnectInterface = STRDUP(char*,strConnectInterface ? strConnectInterface : ""))  )
  {
    OUTOFMEMORY;
    if(pUser)
      UserFree(pUser);
    return 0;
  }
  memcpy(pUser->pcMD5Pass,pcMD5Pass,16);
  pUser->timeRegistered = timeRegistered;
  pUser->timeLastSeen = timeLastSeen;
  if(strFlags)
    UserSetFlags(pUser,strFlags);

  HASHLIST_ADD(USER_HASHLIST,g_hashlistUsers,pUser);
  g_bUsersChanged = 1;

  return pUser;
}

void UserFree(PUSER pUser)
{
  HASHLIST_REMOVE_ALL(USER_PROFILE_HASHLIST,pUser->hashlistProfiles);

  /* disconnect all clients */
  while(pUser->listClients.pFirst)
  {
    PCLIENT pClient = pUser->listClients.pFirst;
    UserDetach(pUser,pClient);
    ASSERT(pClient->pUser == 0);
    ConnectionCloseAsync(pClient->pConnection);
  }

  if(pUser->strName)
    FREE(char*,pUser->strName);
  if(pUser->strEmail)
    FREE(char*,pUser->strEmail);
  if(pUser->strAllowedCommands)
    FREE(char*,pUser->strAllowedCommands);
  if(pUser->strConnectInterface)
    FREE(char*,pUser->strConnectInterface);
  FREE(PUSER,pUser);
  g_bUsersChanged = 1;
}

void UserClose(PUSER pUser)
{
  HASHLIST_REMOVE_ITEM(USER_HASHLIST,g_hashlistUsers,pUser);
}

int UserAddFlag(PUSER pUser, char cFlag)
{
  char* str = pUser->strFlags;
  ASSERT(strchr(USERFLAGS,cFlag) != 0)
  while(*str)
  {
    if(*str == cFlag)
      return 0;
    str++;
  }
  if((unsigned int)(str-pUser->strFlags) >= sizeof(pUser->strFlags)-1)
    return 0;
  *(str++) = cFlag;
  *str = '\0';
/*
  switch(cFlag)
  {
  case 'b':
    break;
  }
*/
  return 1;
}

int UserHasFlag(PUSER pUser, char cFlag)
{
  ASSERT(strchr(USERFLAGS,cFlag) != 0)
  return strchr(pUser->strFlags,cFlag) ? 1 : 0;
}

int UserSetFlags(PUSER pUser, const char* strFlags)
{
  *pUser->strFlags = '\0';
  while(*strFlags)
  {
    if(strchr(USERFLAGS,*strFlags))
      UserAddFlag(pUser,*strFlags);
    strFlags++;
  }
  return 1;
}

int UsersLoad(void)
{
  /* variables */
  char strFile[MAX_PATH];
  unsigned int nLine = 0;
  FILE* fp;
  char* strArg[8];
  unsigned int nArgs;
  PUSER pUser;
  char pcMD5Pass[16];
  char* str;
  unsigned int nOldUsersCount = g_hashlistUsers.nCount;

  /* create file path */
  /* snprintf(strFile,sizeof(strFile),"%s/%s",g_strConfigDir,USERSFILE); */
  BuildFilename(g_strConfigDir,USERSFILE,strFile,sizeof(strFile));

  /* read config file */
  fp = fopen(strFile,"r");
  if(!fp)
  {
    /* Log */
    Log("error: Couldn't load users from \"%s\"",strFile);
    return 0;
  }

  while(fgets(g_strOutBuffer,sizeof(g_strOutBuffer),fp))
  {
    nLine++;
    if(g_strOutBuffer[0] == '#' ||
      g_strOutBuffer[0] == ';' ||
      isspace(g_strOutBuffer[0]))
      continue;

    str = g_strOutBuffer;
    while(*str && *str != ' ')
      str++;
    if(*str == ' ')
    {
      *(str++) = '\0';
      while(isspace(*str))
        str++;
    }

    if( !strcasecmp(g_strOutBuffer,"User") )
    {
      if( (nArgs = SplitLine(str,strArg,sizeof(strArg)/sizeof(*strArg),0)) < 2 ||
        !*strArg[0] || !*strArg[1] )
      {
        Log("error: Invalid user line in users file (Line %u)",nLine);
        continue;
      }

      if(!UserIsNameValid(strArg[0]))
      {
        Log("error: Invalid username in users file (Line %u)",nLine);
        continue;
      }

      if(!PasswordIsValid(strArg[1]) || !PasswordStrTo(strArg[1],pcMD5Pass))
      {
        Log("error: Invalid password in users file (Line %u)",nLine);
        continue;
      }

      /* already exists? */
      HASHLIST_LOOKUP(USER_HASHLIST,g_hashlistUsers,strArg[0],pUser);
      if(pUser)
      {
        memcpy(pUser->pcMD5Pass,pcMD5Pass,16);
        /* Log("error: Found \"%s\" twice in users file (Line %u)",strArg[0],nLine); */
        continue;
      }

      if(!UserCreate(strArg[0],pcMD5Pass,(nArgs > 2 && *strArg[2]) ? strArg[2] : 0,(nArgs > 3) ? strArg[3] : 0,nArgs > 6 ? strArg[6] : c_strDefaultAllowedCommands,nArgs > 7 ? strArg[7] : 0,nArgs > 4 ? atoul(strArg[4]) : 0,nArgs > 5 ? atoul(strArg[5]) : 0))
        continue;
    }
    else
      Log("error: Unknown directive \"%s\" in users file (Line %u)",g_strOutBuffer,nLine);
  }

  fclose(fp);

  /* Log */
  Log("Loaded %u users from \"%s\"",g_hashlistUsers.nCount-nOldUsersCount,strFile);
  if(nOldUsersCount == 0)
    g_bUsersChanged = 0;

  
  /* init users (add admin user!?) */
  if(!UsersOnLoad())
    return 0;

  return 1;
}

int UsersOnLoad(void)
{
  int iReturn = 1;

  /* create or set admin user */
  if(*c_strAdminUsername && *c_strAdminPassword)
  {
    if(!UserIsNameValid(c_strAdminUsername))
    {
      Log("error: Invalid \"AdminUsername\" (%s)",c_strAdminUsername);
      iReturn  = 0;
    }
    else
    {
      char pcMD5Pass[16];
      if(!PasswordCreate(c_strAdminPassword,pcMD5Pass))
        iReturn  = 0;
      else
      {
        PUSER pUser;
        HASHLIST_LOOKUP(USER_HASHLIST,g_hashlistUsers,c_strAdminUsername,pUser);
        if(pUser)
        {
          if(!PasswordCompare(pcMD5Pass,pUser->pcMD5Pass))
          {
            memcpy(pUser->pcMD5Pass,pcMD5Pass,16);
            g_bUsersChanged = 1;
          }
          Log("Updated admin user \"%s\"",c_strAdminUsername);
        }
        else
        {
          if( !(pUser = UserCreate(c_strAdminUsername,pcMD5Pass,0,"",c_strDefaultAllowedCommands,0,g_timeNow,0)) )
            iReturn = 0;
          else
            Log("Created admin user \"%s\"",c_strAdminUsername);
        }
        if(!UserAllowCommand(pUser,"ADMIN"))
          iReturn = 0;
      }
      g_bConfigChanged = 1;
    }
  }
  if(*c_strAdminUsername)
    ConfigSetVar(0,"AdminUsername",0,0);
  if(*c_strAdminPassword)
    ConfigSetVar(0,"AdminPassword",0,0);

  return iReturn;
}

int UsersDump(void)
{
  if(g_bUsersChanged)
  {
    /* variables */
    char strFile[MAX_PATH];
    PUSER pUser;
    char strPass[33];
    FILE* fp;

    /* create file path */
    BuildFilename(g_strConfigDir,USERSFILE,strFile,sizeof(strFile));

    /* create config file */
    fp = fopen(strFile,"w");
    if(!fp)
    {
      /* Log */
      Log("error: Couldn't save users in \"%s\"",strFile);
      return 0;
    }

    for(pUser = g_hashlistUsers.pFirst; pUser; pUser = pUser->hll.pNext)
    {
      PasswordToStr(pUser->pcMD5Pass,strPass,sizeof(strPass));
      fprintf(fp,"User %s %s \"%s\" \"%s\" %u %u \"%s\" \"%s\"\n",pUser->strName,strPass,pUser->strEmail ? pUser->strEmail : "",pUser->strFlags,(unsigned int)pUser->timeRegistered,(unsigned int)pUser->timeLastSeen,pUser->strAllowedCommands ? pUser->strAllowedCommands : "",pUser->strConnectInterface ? pUser->strConnectInterface : "");
    }

    fclose(fp);

    /* Log */
    Log("Saved %u users in \"%s\"",g_hashlistUsers.nCount,strFile);
    g_bUsersChanged = 0;
  }
  return 1;
}

int UserAttach(PUSER pUser, PCLIENT pClient)
{
  LIST_ADD(USER_CLIENT_LIST,pUser->listClients,pClient);

  pClient->pUser = pUser;

  /* welcome this client */
  if( !ConnectionSendFormat(pClient->pConnection,":%s 001 %s :Welcome to the Internet Relay Network %s!~%s@%s\r\n",c_strBouncerName,pClient->strNick,pClient->strNick,pClient->strName,iptoa(pClient->pConnection->nIP)) ||
    !ConnectionSendFormat(pClient->pConnection,":%s 002 %s :Your host is %s, running version %s-%s\r\n",c_strBouncerName,pClient->strNick,c_strBouncerName,PACKAGE,VERSION) ||
#ifdef __TIMESTAMP__
    !ConnectionSendFormat(pClient->pConnection,":%s 003 %s :This server was created %s\r\n",c_strBouncerName,pClient->strNick,__TIMESTAMP__) 
#else
    !ConnectionSendFormat(pClient->pConnection,":%s 003 %s :This server was created %s %s\r\n",c_strBouncerName,pClient->strNick,__DATE__,__TIME__) 
#endif
    ) 
    return 0;

  /* send my own motd */
  if(!MotdSend(pClient))
    return 0;

  if( !ConnectionSendFormat(pClient->pConnection,":%s NOTICE %s :Welcome %s!\r\n",c_strBouncerName,pClient->strNick,pClient->strNick) /*||
    !ConnectionSendFormat(pClient->pConnection,":%s NOTICE %s :Type \"/msg %s help\" to get a list of commands\r\n",c_strBouncerName,pClient->strNick,c_strBouncerName) */)
    return 0;

  /* send admin info */
  if(pClient->bAdmin)
    if( !ConnectionSendFormat(pClient->pConnection,":%s NOTICE %s :You are admin.\r\n",c_strBouncerName,pClient->strNick) )
      return 0;

  if(pUser->strAllowedCommands && ConfigFindVar(pUser->strAllowedCommands,"PROFILE"))
  {
    /* send profile info */
    if(pClient->pUser->hashlistProfiles.pFirst)
    {
      PPROFILE pProfile;

      if( !ConnectionSendFormat(pClient->pConnection,":%s NOTICE %s :You have these profiles:\r\n",c_strBouncerName,pClient->strNick) )
        return 0;

      for(pProfile = pClient->pUser->hashlistProfiles.pFirst; pProfile; pProfile = pProfile->hllUser.pNext)
      {
        if(pProfile->pServer)
        {
          if(!ConnectionSendFormat(pClient->pConnection,pProfile->pServer->bRegistered ? ":%s NOTICE %s :%s (connected to \"%s\")\r\n" : ":%s NOTICE %s :%s (connecting to \"%s\")\r\n",c_strBouncerName,pClient->strNick,pProfile->strName,pProfile->pServer->strServer))
            return 0;
        }
        else
        {
          if(!ConnectionSendFormat(pClient->pConnection,":%s NOTICE %s :%s (disconnected)\r\n",c_strBouncerName,pClient->strNick,pProfile->strName))
            return 0;
        }
      }

      if( !ConnectionSendFormat(pClient->pConnection,":%s NOTICE %s :Use \"/notice %s PROFILE attach <profile>\" to attach one of these profiles.\r\n",c_strBouncerName,pClient->strNick,c_strBouncerName) )
        return 0;
    }
    else
    {
      if( !ConnectionSendFormat(pClient->pConnection,":%s NOTICE %s :You have no profiles.\r\n",c_strBouncerName,pClient->strNick,pClient->strNick) ||
        !ConnectionSendFormat(pClient->pConnection,":%s NOTICE %s :Use \"/notice %s PROFILE add <profile>\" to create a new profile.\r\n",c_strBouncerName,pClient->strNick,c_strBouncerName) )
        return 0;
    }
  }

  /* auto profile mode */
  else
  {
    PPROFILE pProfile = pUser->hashlistProfiles.pFirst;

    if( !pProfile && 
      !(pProfile = ProfileCreate(pUser,pUser->strName,pClient->strNick,pClient->strRealName,*c_strDefaultMode ? c_strDefaultMode : pClient->strMode)) )
    {
      ConnectionCloseAsync(pClient->pConnection);
      return 0;
    }

    ProfileAttach(pProfile,pClient,0);

    if(!pProfile->pServer && !pProfile->pConnectTimer)
    {
      if( (pUser->strAllowedCommands && ConfigFindVar(pUser->strAllowedCommands,"SERVER")) && 
        (!pProfile->c_strServers || !*pProfile->c_strServers) )
      {
        /* send help message */
        if(!ClientMessage(pClient,"Use \"/notice %s SERVER add <server>[:<port>] [<password>]\" to add a server.",c_strBouncerName) )
          return 0;
      }
      else
        ProfileConnect(pProfile,0);
/*
      if( ConfigFindVar(c_strRestrictCommands,"SERVER") ||  m???????? nix versteh
        (pProfile->c_strServers && *pProfile->c_strServers) )
      {
        if(!pProfile->pConnectTimer)
          ProfileConnect(pProfile,0);
      }
      else
      {
        / * send help message * /
        if(!ClientMessage(pClient,"Use \"/notice %s SERVER add <server>[:<port>] [<password>]\" to add a server.",c_strBouncerName) )
          return 0;
      }
*/
    }
  }

  return 1;
}

int UserDetach(PUSER pUser, PCLIENT pClient)
{
  ASSERT(pClient->pUser == pUser);

  LIST_REMOVE_ITEM(USER_CLIENT_LIST,pUser->listClients,pClient);
  pClient->pUser = 0;

  /* ? */

  return 1;
}

/* admin subhandlers */

int ClientSubhandlerAdminUser(PCLIENT pClient, char* strCommand, char* strParams)
{
  char* strArg[7];
  unsigned int nArgs;

  nArgs = SplitLine(strParams,strArg,sizeof(strArg)/sizeof(*strArg),1);
  if(nArgs > 0)
  {
    if( !strcasecmp(strArg[0],"add") ||
      !strcasecmp(strArg[0],"create") )
    {
      if(nArgs > 2)
      {
        PUSER pUser;
        char pcMD5Pass[16];

        HASHLIST_LOOKUP(USER_HASHLIST,g_hashlistUsers,strArg[1],pUser);
        if(pUser)
        {
          ClientMessage(pClient,"User \"%s\" already exists",strArg[1]);
          return 1;
        }

        if(!UserIsNameValid(strArg[1]))
        {
          ClientMessage(pClient,"Invalid username \"%s\"",strArg[1]);
          return 1;
        }

        if(!PasswordIsValid(strArg[2]) || !PasswordCreate(strArg[2],pcMD5Pass))
        {
          ClientMessage(pClient,"Invalid password");
          return 1;
        }

        if(!UserCreate(strArg[1],pcMD5Pass,nArgs > 3 ? strArg[3] : 0,nArgs > 4 ? strArg[4] : 0,nArgs > 5 ? strArg[5] : c_strDefaultAllowedCommands,nArgs > 6 ? strArg[6] : 0,g_timeNow,0))
        {
          ConnectionCloseAsync(pClient->pConnection);
          return 1;
        }

        ClientMessage(pClient,"User \"%s\" added",strArg[1]);
        return 1;
      }
    }
    else if( !strcasecmp(strArg[0],"remove") ||
      !strcasecmp(strArg[0],"delete") )
    {
      if(nArgs > 1)
      {
        PUSER pUser;
        
        HASHLIST_LOOKUP(USER_HASHLIST,g_hashlistUsers,strArg[1],pUser);
        if(!pUser)
        {
          ClientMessage(pClient,"User \"%s\" doesn't exist",strArg[1]);
          return 1;
        }

        if((pUser->strAllowedCommands && ConfigFindVar(pUser->strAllowedCommands,"ADMIN")) || 
          pUser == pClient->pUser)
        {
          ClientMessage(pClient,"Can't remove admin user");
          return 1;
        }

        UserClose(pUser);
        ClientMessage(pClient,"User \"%s\" removed",strArg[1]);
        return 1;
      }
    }
    else if( !strcasecmp(strArg[0],"password") )
    {
      if(nArgs > 2)
      {
        PUSER pUser;
        char pcMD5Pass[16];
        
        HASHLIST_LOOKUP(USER_HASHLIST,g_hashlistUsers,strArg[1],pUser);
        if(!pUser)
        {
          ClientMessage(pClient,"User \"%s\" doesn't exist",strArg[1]);
          return 1;
        }

        if(!PasswordIsValid(strArg[2]) || !PasswordCreate(strArg[2],pcMD5Pass))
        {
          ClientMessage(pClient,"Invalid password");
          return 1;
        }

        memcpy(pUser->pcMD5Pass,pcMD5Pass,16);
        g_bUsersChanged = 1;
      
        ClientMessage(pClient,"Changed password");
        return 1;
      }
    }
    else if( !strcasecmp(strArg[0],"email") )
    {
      if(nArgs > 2)
      {
        PUSER pUser;
        char* strEmail;
        
        HASHLIST_LOOKUP(USER_HASHLIST,g_hashlistUsers,strArg[1],pUser);
        if(!pUser)
        {
          ClientMessage(pClient,"User \"%s\" doesn't exist",strArg[1]);
          return 1;
        }

        if(!(strEmail = STRDUP(char*,strArg[2])))
        {
          ConnectionCloseAsync(pClient->pConnection);
          return 1;
        }

        if(pUser->strEmail)
          FREE(char*,pUser->strEmail);
        pUser->strEmail = strEmail;
        g_bUsersChanged = 1;

        ClientMessage(pClient,"Changed user email");
        return 1;
      }
    }
    else if( !strcasecmp(strArg[0],"flags") )
    {
      if(nArgs > 2)
      {
        PUSER pUser;
        
        HASHLIST_LOOKUP(USER_HASHLIST,g_hashlistUsers,strArg[1],pUser);
        if(!pUser)
        {
          ClientMessage(pClient,"User \"%s\" doesn't exist",strArg[1]);
          return 1;
        }

        UserSetFlags(pUser,strArg[2]);
        g_bUsersChanged = 1;

        ClientMessage(pClient,"Changed user flags");
        return 1;
      }
    }
    else if( !strcasecmp(strArg[0],"commands") )
    {
      if(nArgs > 2)
      {
        PUSER pUser;
        
        HASHLIST_LOOKUP(USER_HASHLIST,g_hashlistUsers,strArg[1],pUser);
        if(!pUser)
        {
          ClientMessage(pClient,"User \"%s\" doesn't exist",strArg[1]);
          return 1;
        }

        if(!UserSetAllowedCommands(pUser,strArg[2]))
        {
          ConnectionCloseAsync(pClient->pConnection);
          return 1;
        }
        g_bUsersChanged = 1;

        ClientMessage(pClient,"Changed user commands");
        return 1;
      }
    }
    else if( !strcasecmp(strArg[0],"interface") )
    {
      if(nArgs > 2)
      {
        PUSER pUser;
        char* strInterface;
        
        HASHLIST_LOOKUP(USER_HASHLIST,g_hashlistUsers,strArg[1],pUser);
        if(!pUser)
        {
          ClientMessage(pClient,"User \"%s\" doesn't exist",strArg[1]);
          return 1;
        }

        if(*strArg[2] && SolveIP(strArg[2]) == INADDR_NONE)
        {
          ClientMessage(pClient,"Invalid interface");
          return 1;
        }

        if(!(strInterface = STRDUP(char*,strArg[2])))
        {
          ConnectionCloseAsync(pClient->pConnection);
          return 1;
        }

        if(pUser->strConnectInterface)
          FREE(char*,pUser->strConnectInterface);
        pUser->strConnectInterface = strInterface;
        g_bUsersChanged = 1;

        ClientMessage(pClient,"Changed user interface");
        return 1;
      }
    }
    else if( !strcasecmp(strArg[0],"get") ||
           !strcasecmp(strArg[0],"show") ||
           !strcasecmp(strArg[0],"list"))
    {
      PUSER pUser;

      if(!ClientMessage(pClient,"User list:"))
        return 1;

      for(pUser = g_hashlistUsers.pFirst; pUser; pUser = pUser->hll.pNext)
        if(!ClientMessage(pClient,"%s \"%s\" \"%s\" \"%s\" %s (%s)",pUser->strName,pUser->strEmail ? pUser->strEmail : "",pUser->strFlags,pUser->strAllowedCommands ? pUser->strAllowedCommands : "",(pUser->strConnectInterface && *pUser->strConnectInterface) ? pUser->strConnectInterface : "default",pUser->listClients.pFirst ? "online" : "offline"))
          return 1;
      return 1;
    }
    else if( !strcasecmp(strArg[0],"attach") ||
      !strcasecmp(strArg[0],"use") )
    {
      if(nArgs > 1)
      {
        PUSER pUser;
        
        HASHLIST_LOOKUP(USER_HASHLIST,g_hashlistUsers,strArg[1],pUser);
        if(!pUser)
        {
          ClientMessage(pClient,"User \"%s\" doesn't exist",strArg[1]);
          return 1;
        }

        /* detach current profile */
        if(pClient->pProfile)
          if(!ProfileDetach(pClient->pProfile,pClient,0))
            return 1;
      
        /* detach from current user */
        ASSERT(pClient->pUser);
        if(pClient->pUser)
          if(!UserDetach(pClient->pUser,pClient))
            return 1;

        /* attach new user */
        if(!UserAttach(pUser,pClient))
          return 1;

        ClientMessage(pClient,"User \"%s\" attached",strArg[1]);
        return 1;
      }
    }
  }

  if( !ClientMessage(pClient,"Usage is /%s add <username> <password> [<email> [<flags> [<allowed commands> [<connect interface>]]]]",strCommand) ||
    !ClientMessage(pClient,"         /%s remove <username>",strCommand) ||
    !ClientMessage(pClient,"         /%s password <username> <password>",strCommand) ||
    !ClientMessage(pClient,"         /%s email <username> <email>",strCommand) ||
    !ClientMessage(pClient,"         /%s flags <username> <flags>",strCommand) ||
    !ClientMessage(pClient,"         /%s commands <username> <allowed commands>",strCommand) ||
    !ClientMessage(pClient,"         /%s interface <username> <connect interface>",strCommand) ||
    !ClientMessage(pClient,"         /%s get",strCommand) ||
    !ClientMessage(pClient,"         /%s attach <username>",strCommand) )
    return 1;

  return 1;
}
