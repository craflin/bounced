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

#include <string.h>
#include <ctype.h>

#include "bounced.h"

#include "md5.h"

int PasswordIsValid(const char* str)
{
  ASSERT(str);
  if(!str)
    return 0;
  if(!*str || *str == ':' || strchr(str,' '))
    return 0;
  return 1;
}

int PasswordCreate(const char* str, char* pcMD5)
{
  struct md5_ctx md;
  ASSERT(str);
  ASSERT(pcMD5);
  if(!str || !pcMD5)
    return 0;
  md5_init_ctx (&md);
  md5_process_bytes(str,strlen(str),&md);
  md5_finish_ctx(&md,pcMD5);
  return 1;
}

int PasswordCompare(const char* pcMD5Pass, const char* pcMD5PassTest)
{
  int i;
  ASSERT(pcMD5Pass);
  ASSERT(pcMD5PassTest);
  if(!pcMD5Pass || !pcMD5PassTest)
    return 0;
  for(i = 0; i < 16; i++)
    if(pcMD5Pass[i])
      break;
  if(i >= 16)
    return 1;
  return !memcmp(pcMD5Pass,pcMD5PassTest,16);
}

int PasswordStrTo(char* str, char* pcMD5)
{
  int i;
  ASSERT(str);
  ASSERT(pcMD5);
  if(!str || !pcMD5)
    return 0;
  for(i = 0; i < 16; i++)
  {
    *str = toupper(*str);
    if((*str < '0' || *str > '9') && (*str < 'A' || *str > 'F'))
      return 0;
    pcMD5[i] = ((*str >= 'A' ? (*str-'A'+10) : (*str-'0')) << 4);
    str++;
    *str = toupper(*str);
    if((*str < '0' || *str > '9') && (*str < 'A' || *str > 'F'))
      return 0;
    pcMD5[i] |= (*str >= 'A' ? (*str-'A'+10) : (*str-'0'));
    str++;
  }
  return i;
}

static char g_Hex[] = "0123456789ABCDEF";
int PasswordToStr(const char* pcMD5, char* strOut, unsigned int nOutSize)
{
    int i;
  ASSERT(pcMD5);
  ASSERT(strOut);
  ASSERT(nOutSize >= 33);
  if(!pcMD5 || !strOut || nOutSize < 33)
    return 0;
    for (i = 16-1; i >= 0; i--)
    {
    strOut[2*i+1] = g_Hex[pcMD5[i] & 0xf];
    strOut[2*i] = g_Hex[(pcMD5[i] >> 4) & 0xf];
    }
  strOut[32] = '\0';
  return 1;
}

/* client handlers */

int ClientHandlerPassword(PCLIENT pClient, char* strCommand, char* strParams)
{
  char* strArg[2];
  unsigned int nArgs;

  if(*strParams == ':')
    strParams++;

  ASSERT(pClient->pUser);

  nArgs = SplitLine(strParams,strArg,sizeof(strArg)/sizeof(*strArg),0);
  if(nArgs > 0)
  {
    if(!strcasecmp(strArg[0],"set") )
    {
      if(nArgs > 1)
      {
        char pcMD5Pass[16];

        if(!PasswordIsValid(strArg[1]) || !PasswordCreate(strArg[1],pcMD5Pass))
        {
          ClientMessage(pClient,"Invalid password");
          return 1;
        }

        memcpy(pClient->pUser->pcMD5Pass,pcMD5Pass,16);
      
        ClientMessage(pClient,"Changed password");
        return 1;
      }
    }
  }

  if( !ClientMessage(pClient,"Usage is /%s set <password>",strCommand) )
    return 1;

  return 1;
}
