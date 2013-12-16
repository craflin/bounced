/*
         file: motd.c
   desciption: everything what has to do with the motd
        begin: 03/03/02
    copyright: (C) 2002 by Colin Graf
        email: addition@users.sourceforge.net

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "bounced.h"

char* g_strMotdDefaultFormatTime = "%A, %d of %B %Y %X";
char* g_strMotdDefaultFormatUptime = "%#D days, %#H hours, %#M minutes and %#S seconds";
char* g_strMotdFormatTime = 0;
char* g_strMotdFormatUptime = 0;

char g_strMotdCurrentTime[48];
char g_strMotdTotalConnections[21];
char g_strMotdIP[16];
char g_strMotdTotalLogins[21];
char* g_strMotdNick;
char* g_strMotdUsername;
char* g_strMotdRealName;
char* g_strMotdEmail;
char g_strMotdUsers[11];
char g_strMotdUptime[48];
char g_strMotdTimeStarted[48];
char g_strMotdIncomingTraffic[11];
char g_strMotdOutgoingTraffic[11];
char g_strMotdTraffic[11];

char g_strMotdConnections[11];
char g_strMotdServers[11];
char g_strMotdClients[11];

char g_strMotdTotalIncomingBytes[21];
char g_strMotdTotalOutgoingBytes[21];
char g_strMotdTotalBytes[21];

MOTDVAR_TABLE_START
  MOTDVAR('C', MotdSetCurrentTime, g_strMotdCurrentTime, MOTDV_REFRESH)
  MOTDVAR('c', MotdSetTotalConnections, g_strMotdTotalConnections, MOTDV_REFRESH)
  MOTDVAR('I', MotdSetIP, g_strMotdIP, MOTDV_REFRESH)
  MOTDVAR('l', MotdSetTotalLogins, g_strMotdTotalLogins, MOTDV_REFRESH)
  MOTDVAR('B', 0, &c_strBouncerName, MOTDV_NORMAL|MOTDV_POINTER)
  MOTDVAR('N', MotdSetNick, &g_strMotdNick, MOTDV_REFRESH|MOTDV_POINTER)
  MOTDVAR('U', MotdSetUsername, &g_strMotdUsername, MOTDV_REFRESH|MOTDV_POINTER)
  MOTDVAR('R', MotdSetRealName, &g_strMotdRealName, MOTDV_REFRESH|MOTDV_POINTER)
  MOTDVAR('E', MotdSetEmail, &g_strMotdEmail, MOTDV_REFRESH|MOTDV_POINTER)
  MOTDVAR('S', 0, g_strSystem, MOTDV_NORMAL)
  MOTDVAR('u', MotdSetUsers, g_strMotdUsers, MOTDV_REFRESH)
  MOTDVAR('V', 0, g_strVersion, MOTDV_NORMAL)
  MOTDVAR('W', MotdSetUptime, g_strMotdUptime, MOTDV_REFRESH)
  MOTDVAR('w', MotdSetTimeStarted, g_strMotdTimeStarted, MOTDV_NORMAL)

  MOTDVAR('i', MotdSetIncomingTraffic, g_strMotdIncomingTraffic, MOTDV_REFRESH)
  MOTDVAR('o', MotdSetOutgoingTraffic, g_strMotdOutgoingTraffic, MOTDV_REFRESH)
  MOTDVAR('t', MotdSetTraffic, g_strMotdTraffic, MOTDV_REFRESH)

  MOTDVAR('D', MotdSetConnections, g_strMotdConnections, MOTDV_REFRESH)
  MOTDVAR('U', MotdSetServers, g_strMotdServers, MOTDV_REFRESH)
  MOTDVAR('L', MotdSetClients, g_strMotdClients, MOTDV_REFRESH)

  MOTDVAR('J', MotdSetTotalIncomingBytes, g_strMotdTotalIncomingBytes, MOTDV_REFRESH)
  MOTDVAR('O', MotdSetTotalOutgoingBytes, g_strMotdTotalOutgoingBytes, MOTDV_REFRESH)
  MOTDVAR('T', MotdSetTotalBytes, g_strMotdTotalBytes, MOTDV_REFRESH)


MOTDVAR_TABLE_END

void MotdSetCurrentTime(PMOTDVAR pMotdVar, PCLIENT pClient)
{
  strftimet(g_strMotdCurrentTime,sizeof(g_strMotdCurrentTime),g_strMotdFormatTime,g_timeNow);
  return;
  pMotdVar = 0;
  pClient = 0;
}

void MotdSetTotalConnections(PMOTDVAR pMotdVar, PCLIENT pClient)
{
  strformat(g_strMotdTotalConnections,sizeof(g_strMotdTotalConnections),"%.0f",g_dConnections);
  return;
  pMotdVar = 0;
  pClient = 0;
}

void MotdSetIP(PMOTDVAR pMotdVar, PCLIENT pClient)
{
  strcpy(g_strMotdIP,iptoa(pClient->pConnection->nIP));
  return;
  pMotdVar = 0;
}

void MotdSetTotalLogins(PMOTDVAR pMotdVar, PCLIENT pClient)
{
  strformat(g_strMotdTotalLogins,sizeof(g_strMotdTotalLogins),"%.0f",g_dLogins);
  return;
  pMotdVar = 0;
  pClient = 0;
}

void MotdSetConnections(PMOTDVAR pMotdVar, PCLIENT pClient)
{
  strformat(g_strMotdConnections,sizeof(g_strMotdConnections),"%u",g_listConnections.nCount);
  return;
  pMotdVar = 0;
  pClient = 0;
}

void MotdSetServers(PMOTDVAR pMotdVar, PCLIENT pClient)
{
  strformat(g_strMotdServers,sizeof(g_strMotdServers),"%u",g_listServers.nCount);
  return;
  pMotdVar = 0;
  pClient = 0;
}

void MotdSetClients(PMOTDVAR pMotdVar, PCLIENT pClient)
{
  strformat(g_strMotdClients,sizeof(g_strMotdClients),"%u",g_listClients.nCount);
  return;
  pMotdVar = 0;
  pClient = 0;
}

void MotdSetTotalIncomingBytes(PMOTDVAR pMotdVar, PCLIENT pClient)
{
  strformat(g_strMotdTotalIncomingBytes,sizeof(g_strMotdTotalIncomingBytes),"%.0f",g_dTotalBytesIn/1048576);
  return;
  pMotdVar = 0;
  pClient = 0;
}

void MotdSetTotalOutgoingBytes(PMOTDVAR pMotdVar, PCLIENT pClient)
{
  strformat(g_strMotdTotalOutgoingBytes,sizeof(g_strMotdTotalOutgoingBytes),"%.0f",g_dTotalBytesOut/1048576);
  return;
  pMotdVar = 0;
  pClient = 0;
}

void MotdSetTotalBytes(PMOTDVAR pMotdVar, PCLIENT pClient)
{
  strformat(g_strMotdTotalBytes,sizeof(g_strMotdTotalBytes),"%.0f",(g_dTotalBytesOut+g_dTotalBytesIn)/1048576);
  return;
  pMotdVar = 0;
  pClient = 0;
}

void MotdSetNick(PMOTDVAR pMotdVar, PCLIENT pClient)
{
  g_strMotdNick = pClient->strNick;
  return;
  pMotdVar = 0;
}

void MotdSetUsername(PMOTDVAR pMotdVar, PCLIENT pClient)
{
  g_strMotdUsername = pClient->pUser->strName;
  return;
  pMotdVar = 0;
}

void MotdSetRealName(PMOTDVAR pMotdVar, PCLIENT pClient)
{
  g_strMotdRealName = pClient->strRealName;
  return;
  pMotdVar = 0;
}

void MotdSetEmail(PMOTDVAR pMotdVar, PCLIENT pClient)
{
  g_strMotdEmail = pClient->pUser->strEmail;
  return;
  pMotdVar = 0;
}

void MotdSetUsers(PMOTDVAR pMotdVar, PCLIENT pClient)
{
  strformat(g_strMotdUsers,sizeof(g_strMotdUsers),"%u",g_hashlistUsers.nCount);
  return;
  pMotdVar = 0;
  pClient = 0;
}

void MotdSetTimeStarted(PMOTDVAR pMotdVar, PCLIENT pClient)
{
  strftimet(g_strMotdTimeStarted,sizeof(g_strMotdTimeStarted),g_strMotdFormatTime,g_timeStart);
  return;
  pMotdVar = 0;
  pClient = 0;
}

void MotdSetUptime(PMOTDVAR pMotdVar, PCLIENT pClient)
{
  strftimetspan(g_strMotdUptime,sizeof(g_strMotdUptime),g_strMotdFormatUptime,g_timeNow-g_timeStart);
  return;
  pMotdVar = 0;
  pClient = 0;
}

void MotdSetIncomingTraffic(PMOTDVAR pMotdVar, PCLIENT pClient)
{
  strformat(g_strMotdIncomingTraffic,sizeof(g_strMotdIncomingTraffic),"%.2f",((double)g_nLastIncomingTraffic)/1000);
  return;
  pMotdVar = 0;
  pClient = 0;
}

void MotdSetOutgoingTraffic(PMOTDVAR pMotdVar, PCLIENT pClient)
{
  strformat(g_strMotdOutgoingTraffic,sizeof(g_strMotdOutgoingTraffic),"%.2f",((double)g_nLastOutgoingTraffic)/1000);
  return;
  pMotdVar = 0;
  pClient = 0;
}

void MotdSetTraffic(PMOTDVAR pMotdVar, PCLIENT pClient)
{
  strformat(g_strMotdTraffic,sizeof(g_strMotdTraffic),"%.2f",((double)(g_nLastIncomingTraffic+g_nLastOutgoingTraffic))/1000);
  return;
  pMotdVar = 0;
  pClient = 0;
}

int MotdSend(PCLIENT pClient)
{
  if(g_listMotd.pFirst)
  {
    unsigned int n;
    char* str,*strStart;
    PMOTDSTR pMotdStr;
    PMOTDVAR pMotdVar;
    
    /* make motd up to date */
    for(pMotdVar = g_listMotdVars.pFirst; pMotdVar; pMotdVar = pMotdVar->ll.pNext)
      pMotdVar->pMotdVarProc(pMotdVar,pClient);

    if( !ConnectionSendFormat(pClient->pConnection,":%s 375 %s :- %s Message of the Day - \r\n",c_strBouncerName,pClient->strNick,c_strBouncerName) )
      return 0;

    strStart = g_strOutBuffer+strformat(g_strOutBuffer,sizeof(g_strOutBuffer)-2,":%s 372 %s :- ",c_strBouncerName,pClient->strNick);
    str = strStart;

    for(pMotdStr = g_listMotd.pFirst; pMotdStr; pMotdStr = pMotdStr->ll.pNext)
    {
      if(pMotdStr->p)
      {
        if(pMotdStr->i < 0)
        {
          pMotdVar = (PMOTDVAR)pMotdStr->p;
          if(pMotdVar->cType & MOTDV_POINTER)
          {
            n = strlen(*(char**)pMotdVar->pVar);
            if(str-g_strOutBuffer+n < sizeof(g_strOutBuffer)-3)
            {  
              memcpy(str,*(char**)pMotdVar->pVar,n);
              str += n;
              continue;
            }
          }
          else
          {
            n = strlen((char*)pMotdVar->pVar);
            if(str-g_strOutBuffer+n < sizeof(g_strOutBuffer)-3)
            {
              memcpy(str,(char*)pMotdVar->pVar,n);
              str += n;
              continue;
            }
          }
        }
        else
        {
          n = pMotdStr->i;
          if(str-g_strOutBuffer+n < sizeof(g_strOutBuffer)-3)
          {            
            memcpy(str,pMotdStr->p,n);
            str += n;
            continue;
          }
        }        
      }
      
      *(str++) = '\r';
      *(str++) = '\n';
      *str = '\0';

      if(!ConnectionSend(pClient->pConnection,g_strOutBuffer,str-g_strOutBuffer))
        return 0;

      str = strStart;
    }

    if(  !ConnectionSendFormat(pClient->pConnection,":%s 376 %s :End of /MOTD command\r\n",c_strBouncerName,pClient->strNick) )
      return 0;
  }
  else
  {
    if( !ConnectionSendFormat(pClient->pConnection,":%s 422 %s :MOTD File is missing\r\n",c_strBouncerName,pClient->strNick) )
      return 0;
  }

  return 1;
}

PMOTDSTR MotdStrCreate(PMOTDSTR** pppMotdStr, void* p, int i)
{
  PMOTDSTR pMotdStr;
  if( !(pMotdStr = CALLOC(PMOTDSTR,1,sizeof(MOTDSTR))) ||
    (p && i > -1 && !(pMotdStr->p = STRDUP(char*,p))) )
  {
    if(pMotdStr)
    {
      if(pMotdStr->p)
        FREE(char*,pMotdStr->p);
    }
    OUTOFMEMORY;
    return 0;
  }
  if(i < 0)
    pMotdStr->p = p;
  pMotdStr->i = i;
  
  LIST_INSERT(MOTD_LIST,g_listMotd,*pppMotdStr,pMotdStr);
  *pppMotdStr = &pMotdStr->ll.pNext;

  return pMotdStr;
}

void MotdStrFree(PMOTDSTR pMotdStr)
{
  if(pMotdStr->p && pMotdStr->i > -1)
    FREE(char*,pMotdStr->p);
  FREE(PMOTDSTR,pMotdStr);
}

void MotdVarFree(PMOTDVAR pMotdVar)
{
  pMotdVar->ll.pNext = 0;
}

int MotdLoad(void)
{
  /* variables */
  char strFile[MAX_PATH];
  int iLength;
  unsigned int iLine = 0,
             iMotdLines = 0,
       nStopLength;
  unsigned int n;
  FILE* fp;
  char *str,
     *strStop;
  PMOTDSTR* ppMotdStr = &g_listMotd.pFirst;

  /* delete current motd */
  MotdFree();

  /* create file path */
  /* snprintf(strFile,sizeof(strFile),"%s/%s",g_strConfigDir,MOTDFILE); */
  BuildFilename(g_strConfigDir,MOTDFILE,strFile,sizeof(strFile));

  /* read config file */
  fp = fopen(strFile,"r");
  if(!fp)
  {
    /* Log */
    /* Log("error: Couldn't load motd from \"%s\"",strFile); */
    return 0;
  }

  while(fgets(g_strOutBuffer,sizeof(g_strOutBuffer),fp))
  {
    iLine++;
    if( *g_strOutBuffer == '#' ||
      *g_strOutBuffer == ';' ||
      isspace(*g_strOutBuffer))
      continue;
    
    iLength = strlen(g_strOutBuffer);
    while(iLength > 0 && isspace(g_strOutBuffer[iLength-1]))
      iLength--;
    g_strOutBuffer[iLength] = '\0';

    str = g_strOutBuffer;
    while(*str && *str != ' ')
      str++;
    if(*str == ' ')
      *(str++) = '\0';

    if(!strcasecmp(g_strOutBuffer,"-"))
    {
      iMotdLines++;
      strStop = str;
      nStopLength = 0;
      for(str = strStop;*str;str++)
      {
        if(*str == '%' && *(++str))
        {
          if(*str == '%')
          {
            memcpy(str,str+1,iLength-(str-g_strOutBuffer));
            nStopLength += 2;
            continue;
          }
          for(n = 0; n < sizeof(g_pMotdVars)/sizeof(struct tagMOTDVAR); n++)
          {
            if(*str == g_pMotdVars[n].cName)
            {
              str[-1] = '\0';
              MotdStrCreate(&ppMotdStr,strStop,nStopLength);
              nStopLength = -1;
              strStop = str+1;

              if(g_pMotdVars[n].pMotdVarProc)
              {
                if(g_pMotdVars[n].cType & MOTDV_REFRESH)
                {
                  PMOTDVAR pMotdVar;
                  for(pMotdVar = g_listMotdVars.pFirst; pMotdVar; pMotdVar = pMotdVar->ll.pNext)
                    if(pMotdVar == &g_pMotdVars[n])
                      break;
                  if(!pMotdVar)
                    LIST_ADD(MOTDVAR_LIST,g_listMotdVars,&g_pMotdVars[n]);
                }
                else
                  g_pMotdVars[n].pMotdVarProc(&g_pMotdVars[n],0);
              }
              else
              {
                ASSERT(!(g_pMotdVars[n].cType & MOTDV_REFRESH));
              }

              if(g_pMotdVars[n].cType & MOTDV_REFRESH)
              {
                MotdStrCreate(&ppMotdStr,&g_pMotdVars[n],-1);
              }
              else
              {
                if(g_pMotdVars[n].cType & MOTDV_POINTER)
                  MotdStrCreate(&ppMotdStr,*(char**)g_pMotdVars[n].pVar,(int)strlen(*(char**)g_pMotdVars[n].pVar));
                else
                  MotdStrCreate(&ppMotdStr,(char*)g_pMotdVars[n].pVar,(int)strlen((char*)g_pMotdVars[n].pVar));
              }

              break;
            }
          }
          if(n >= sizeof(g_pMotdVars)/sizeof(struct tagMOTDVAR))
          {
            Log("error: Unknown motd variable \"%c\" (Line %u)",*str,iLine);
            nStopLength++;
          }
        }
        nStopLength++;
      }

      if(nStopLength)
        MotdStrCreate(&ppMotdStr,strStop,nStopLength);
      MotdStrCreate(&ppMotdStr,0,0); /* line end */
    }
    else
    {
      char* strEnd = &g_strOutBuffer[iLength-1];
      if(str != strEnd && *str == '"' && *strEnd == '"' )
      {
        str++;
        *strEnd = '\0';
      }

      if(!strcasecmp(g_strOutBuffer,"FormatTime"))
      {
        char* strVal = STRDUP(char*,str);
        if(strVal)
        {
          if(g_strMotdFormatTime && g_strMotdFormatTime != g_strMotdDefaultFormatTime)
            FREE(char*,g_strMotdFormatTime);
          g_strMotdFormatTime = strVal;
        }
      }
      else if(!strcasecmp(g_strOutBuffer,"FormatUptime"))
      {
        char* strVal = STRDUP(char*,str);
        if(strVal)
        {
          if(g_strMotdFormatUptime && g_strMotdFormatUptime != g_strMotdDefaultFormatUptime)
            FREE(char*,g_strMotdFormatUptime);
          g_strMotdFormatUptime = strVal;
        }
      }
      else
        Log("error: Unknown directive \"%s\" in motd file (Line %u)",g_strOutBuffer,iLine);
    }
  }

  fclose(fp);

  /* debug */
  g_DebugHasMotdCopied = 1;
  g_DebuglistMotd.pFirst = g_listMotd.pFirst;
  g_DebuglistMotd.nCount = g_listMotd.nCount;
    g_DebugMotdStrFirst.ll.pNext = g_listMotd.pFirst->ll.pNext;
  g_DebugMotdStrFirst.p = g_listMotd.pFirst->p;
  g_DebugMotdStrFirst.i = g_listMotd.pFirst->i;

  Log("Loaded %u motd lines from \"%s\"",iMotdLines,strFile);
  return 1;
}

void MotdFree(void)
{
  if(g_strMotdFormatTime && g_strMotdFormatTime != g_strMotdDefaultFormatTime)
    FREE(char*,g_strMotdFormatTime);
  g_strMotdFormatTime = g_strMotdDefaultFormatTime;
  if(g_strMotdFormatUptime && g_strMotdFormatUptime != g_strMotdDefaultFormatUptime)
    FREE(char*,g_strMotdFormatUptime);
  g_strMotdFormatUptime = g_strMotdDefaultFormatUptime;

  /* delete current motd */
  LIST_REMOVE_ALL(MOTD_LIST,g_listMotd);
  LIST_REMOVE_ALL(MOTDVAR_LIST,g_listMotdVars);
}
