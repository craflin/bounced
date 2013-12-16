/*
         file: tools.c
   desciption: logging functions and other helpful tools
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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#if !defined(_WIN32) || defined(__CYGWIN__)
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/stat.h>
#endif /* !defined(_WIN32) || defined(__CYGWIN__) */

#include "bounced.h"

size_t vstrformat(char* str, size_t maxsize, const char* format, va_list ap);


static unsigned char g_bLogInMulti = 0;

#ifndef _WINDOWS

/* #define STDOUT_FILENO fileno(stdout) */

int LogOpenFile(void)
{
  char strFile[MAX_PATH];
  int  fd;
  BuildFilename(g_strConfigDir,LOGFILE,strFile,sizeof(strFile));
  fd = open(strFile,O_CREAT|O_WRONLY|O_APPEND,S_IRUSR|S_IWUSR);
  if(fd > 0)
  {
    /* close stdout */
    if(dup2(fd, STDOUT_FILENO) == -1)
    {
      Log("error: Couldn't reopen stdout");
      return 0;
    }
    close(fd);
  }
  else
  {
    Log("error: Couldn't open log file \"%s\"",strFile);
    return 0;
  }
  return 1;
}

int LogReopenFile(void)
{
  char strFile[MAX_PATH];
  char strFileOld[MAX_PATH];
  int fd;
  BuildFilename(g_strConfigDir,LOGFILE,strFile,sizeof(strFile));
  BuildFilename(g_strConfigDir,LOGOLDFILE,strFileOld,sizeof(strFileOld));
  rename(strFile,strFileOld);
  fd = open(strFile,O_CREAT|O_WRONLY,S_IRUSR|S_IWUSR);
  if(fd > 0)
  {
    if(dup2(fd, STDOUT_FILENO) == -1)
    {
      Log("error: Couldn't reopen stdout");
      return 0;
    }
    close(fd);
  }
  else
  {
    Log("error: Couldn't open log file \"%s\"",strFile);
    return 0;
  }
  return 1;
}

#endif /*!_WINDOWS*/

int Log(const char* format, ...)
{
  char strLog[512+210+2];
  va_list ap;
  int iReturn;

  ASSERT(format);
  if(!format)
    return 0;

  if(g_bLogInMulti)
  {
    g_bLogInMulti = 0;
    fputc('\n',stdout);
  }

  iReturn = strftimet(strLog,sizeof(strLog),"[%X] ",g_timeNow);

  va_start (ap, format);
  iReturn += vstrformat(strLog+iReturn,sizeof(strLog)-iReturn-1,format, ap);
  va_end (ap);
  strLog[iReturn++] = '\n';
  strLog[iReturn] = '\0';

  /* reopen log file if file is to big */
#ifndef _WINDOWS
  if(g_bBackground)
  {
    struct stat buf;
    if( fstat(STDOUT_FILENO,&buf) == 0 &&
      buf.st_size+iReturn >= LOGFILESIZE)
      LogReopenFile();
  }
#endif /* !_WINDOWS */

  fputs(strLog,stdout);
  fflush(stdout);

  return iReturn;
}

int LogMultiStart(const char* format, ...)
{
  char strLog[320];
  va_list ap;
  int iReturn;

  ASSERT(format);
  if(!format)
    return 0;

  if(g_bLogInMulti)
  {
    g_bLogInMulti = 0;
    fputc('\n',stdout);
  }

  iReturn = strftimet(strLog,sizeof(strLog),"[%X] ",g_timeNow); 

  va_start (ap, format);
  iReturn += vstrformat(strLog+iReturn,sizeof(strLog)-iReturn,format, ap);
  va_end (ap);

  /* reopen log file if file is to big */
#ifndef _WINDOWS
  if(g_bBackground)
  {
    struct stat buf;
    if( fstat(STDOUT_FILENO,&buf) == 0 &&
      buf.st_size+iReturn+16 >= LOGFILESIZE)
      LogReopenFile();
  }
#endif /* !_WINDOWS */

  fputs(strLog,stdout);
  fflush(stdout);

  g_bLogInMulti = 1;

  return iReturn;
}

int LogMultiEnd(const char* str)
{
  ASSERT(str);
  if(!str)
    return 0;

  if(!g_bLogInMulti)
    return 0;

  fputs(str,stdout);
  fputc('\n',stdout);
  fflush(stdout);

  g_bLogInMulti = 0;

  return 1;
}

char* iptoa(unsigned int ip)
{
  struct in_addr in;
  memset(&in,0,sizeof(struct in_addr));
  in.s_addr = ip;
  return inet_ntoa(in);
}

size_t strftimet(char *strDest, size_t maxsize, const char *format, time_t timet)
{
  struct tm* timeptr;
  size_t size;

  ASSERT(strDest);
  ASSERT(maxsize > 0);
  ASSERT(format);
  
  timeptr = localtime(&timet);
  maxsize--;
  size = strftime(strDest,maxsize,format,timeptr);
  if(/*size < 0 || */size >= maxsize)
  {
    strDest[maxsize] = '\0';
    return maxsize;
  }
  return size;
}

size_t strftimespan(char *strDest, size_t maxsize, const char *format, const struct tm *timeptr)
{
  unsigned char b;
  char *strMax,*str;

  ASSERT(strDest);
  ASSERT(maxsize > 0);
  ASSERT(format);
  ASSERT(timeptr);

  strMax = strDest+(maxsize-1);
  for(str = strDest; *format && str < strMax; format++)
  {
    if(*format == '%')
    {
      if(*(++format) == '#')
      {
        b = 1;
        format++;
      }
      else
        b = 0;
      switch(*format)
      {
      case 'S':
        str += strformat(str,strMax-str,b ? "%d" : "%02d",timeptr->tm_sec);
        break;
      case 'M':
        str += strformat(str,strMax-str,b ? "%d" : "%02d",timeptr->tm_min);
        break;
      case 'H':
        str += strformat(str,strMax-str,b ? "%d" : "%02d",timeptr->tm_hour);
        break;
      case 'D':
        str += strformat(str,strMax-str,b ? "%d" : "%03d",timeptr->tm_yday);
        break;
      case '%':
        *(str++) = *format;
        break;
      default:
        *(str++) = '%';
        if(str >= strMax)
          break;
        if(b)
        {
          *(str++) = '#';
          if(str >= strMax)
            break;
        }
        *(str++) = *format;
        break;
      }
    }
    else
      *(str++) = *format;
  }
  *str = '\0';
  return str-strDest;
}

size_t strftimetspan(char *strDest, size_t maxsize, const char *format, time_t timet)
{
  struct tm stm;
  ASSERT(strDest);
  ASSERT(maxsize > 0);
  ASSERT(format);
  if(!strDest || maxsize <= 0 || !format)
    return 0;
  memset(&stm,0,sizeof(struct tm));
  stm.tm_yday = timet/60/60/24;
  stm.tm_hour = (timet/60/60) % 24;
  stm.tm_min = (timet/60) % 60;
  stm.tm_sec = timet % 60;
  return strftimespan(strDest,maxsize,format,&stm);
}

unsigned int strsum(const char* str)
{
/*  unsigned int h,g;
  for (h = 0; *str; str++)
  {
    h = (h<<4)+*str;
    if((g = h) & 0xF0000000)
      h ^= g >> 24;
    h &= ~g;
  }
  return h;
*/
  unsigned int sum = 0;
/*  ASSERT(str);
  if(!str)
    return 0; */
  while(*str)
    sum = (sum<<4)+*(str++);
  return sum;
}

unsigned int strcasesum(const char* str)
{
/*  unsigned int h,g;
  for (h = 0; *str; str++)
  {
    h = (h<<4)+tolower(*str);
    if((g = h) & 0xF0000000)
      h ^= g >> 24;
    h &= ~g;
  }
  return h;
*/
  unsigned int sum = 0;
/*  ASSERT(str);
  if(!str)
    return 0; */
  while(*str)
    sum = (sum<<4)+tolower(*(str++));
  return sum;
}

unsigned int SplitLine(char *pkt, char **template, unsigned int templatecount, char bPseudoTrailing)
{
  unsigned int i = 0;
  ASSERT(pkt);
  ASSERT(template);
  if(!pkt || !template)
    return 0;
    while(*pkt && i < templatecount)
    {
    while(isspace(*pkt))
      pkt++;
    if(*pkt == '"')
    {
      /* quoted string */
      template[i++] = ++pkt;
      pkt = strpbrk(pkt, "\"\r\n");
      if(!pkt)
      {
        /* bogus line */
        return 0;
      }
      *pkt++ = 0;
      if(!*pkt)
        break;
      pkt++;    /* skip the space */
    }
    else if(*pkt)
    {
      template[i++] = pkt;
      if(i == templatecount && bPseudoTrailing)
        pkt = strpbrk (pkt, "\r\n");
      else
        pkt = strpbrk (pkt, " \t\r\n");
      if(!pkt)
        break;
      *pkt++ = 0;
    }
    }
    return i;
}
/*
unsigned int SplitLinePointer(char **ppkt, char **template, unsigned int templatecount)
{
  unsigned int i = 0;
  char* pkt;
  ASSERT(ppkt);
  ASSERT(template);
  if(!ppkt || !template)
    return 0;
  pkt = *ppkt;
  if(!pkt)
    return 0;
    while(*pkt && i < templatecount)
    {
    while(isspace(*pkt))
      pkt++;
    if(*pkt == '"')
    {
      / * quoted string * /
      template[i++] = ++pkt;
      pkt = strchr(pkt, '"');
      if(!pkt)
      {
        / * bogus line * /
        return 0;
      }
      *pkt++ = 0;
      if(!*pkt)
        break;
      pkt++;    / * skip the space * /
    }
    else
    {
      template[i++] = pkt;
      pkt = strpbrk (pkt, " \t\r\n");
      if(!pkt)
      {
        i--;
        pkt = template[i]+strlen(template[i]);
        i++;
        break;
      }
      *pkt++ = 0;
    }
    }
  *ppkt = pkt;
    return i;
}
*/
int SplitIRCParams(char *pkt, char **template, unsigned int templatecount, char bAllowTrailing)
{
  unsigned int i = 0;
  ASSERT(pkt);
  ASSERT(template);
  if(!pkt || !template)
    return 0;
    while(*pkt && i < templatecount)
    {
    if(*pkt == ':')
    {
      template[i++] = ++pkt;
      if(bAllowTrailing)
        pkt = strpbrk(pkt,"\r\n");
      else
        pkt = strpbrk(pkt," \r\n");
      if(!pkt)
        break;
      *pkt++ = '\0';
      break;
    }
    else
    {
      if(*pkt == ' ')
        pkt++;
      template[i++] = pkt;
      pkt = strpbrk(pkt," \r\n");
      if(!pkt)
        break;
      *pkt++ = '\0';
    }
    }
  return i;
}

int SplitIRCParamsPointer(char **ppkt, char **template, unsigned int templatecount, char bAllowTrailing)
{
  unsigned int i = 0;
  char* pkt;
  ASSERT(ppkt);
  ASSERT(template);
  if(!ppkt || !template)
    return 0;
  pkt = *ppkt;
  if(!pkt)
    return -1;
    while(*pkt && i < templatecount)
    {
    if(*pkt == ':')
    {
      template[i++] = ++pkt;
      if(bAllowTrailing)
        pkt = strpbrk(pkt,"\r\n");
      else
        pkt = strpbrk(pkt," \r\n");
      if(!pkt)
        break;
      *pkt++ = '\0';
      break;
    }
    else
    {
      if(*pkt == ' ')
        pkt++;
      template[i++] = pkt;
      pkt = strpbrk(pkt," \r\n");
      if(!pkt)
        break;
      *pkt++ = '\0';
    }
    }
  *ppkt = pkt;
  return i;
}

int SplitIRCPrefixNick(char* strCommand, char** strNick)
{
  ASSERT(strCommand);
  ASSERT(strNick);
  if(!strCommand || !strNick)
    return 0;
  if(*strCommand == ':')
  {    
    char* str = strpbrk(++strCommand,"! ");
    if(str)
    {
      *strNick = strCommand;
      *str = '\0';
      return 1;
    }
  }
  *strNick = 0;
  return 0;
}

int SplitIRCPrefix(char* strCommand, char** strPrefix)
{
  ASSERT(strCommand);
  ASSERT(strPrefix);
  if(!strCommand || !strPrefix)
    return 0;
  if(*strCommand == ':')
  {    
    char* str = strchr(++strCommand,' ');
    if(str)
    {
      *strPrefix = strCommand;
      *str = '\0';
      return 1;
    }
  }
  *strPrefix = 0;
  return 0;
}


int CompareIRCPrefixNick(const char* strCommand, const char* strNick)
{
  int i = strlen(strNick);
  ASSERT(strCommand);
  ASSERT(strNick);
  if(!strCommand || !strNick)
    return 0;
  if(*strCommand != ':')
    return 0;
  strCommand++;
  if(!strncasecmp(strCommand,strNick,i) && (strCommand[i] == '!' || isspace(strCommand[i])) )
    return 1;
  return 0;
}

int CompareIRCAddressNick(const char* strParams, const char* strNick)
{
  int i = strlen(strNick);
  ASSERT(strParams);
  ASSERT(strNick);
  if(!strParams || !strNick)
    return 0;
  if(!*strNick)
    return 0;
  if(*strParams == ':')
    strParams++;
  if(!strncasecmp(strParams,strNick,i) && isspace(strParams[i]) )
    return 1;
  strParams = strchr(strParams,' ');
  if(!strParams)
    return 0;
  strParams++;
  if(*strParams == ':')
    strParams++;
  if(!strncasecmp(strParams,strNick,i) && isspace(strParams[i]) )
    return 1;
  return 0;
}

char* NextArg(char **pkt)
{
  char* strReturn;
  ASSERT(pkt);
  ASSERT(*pkt);
    if(!pkt || !*pkt)
    return 0;
    while (isspace (**pkt))
    (*pkt)++;
  if (**pkt == '"')
  {
    strReturn = ++(*pkt);
    *pkt = strchr (*pkt, '"');
    if (!*pkt || !**pkt)
    {
      *pkt = strReturn +strlen(strReturn);
      return strReturn;
    }
    *(*pkt)++ = '\0';
  }
  else
  {
    strReturn = *pkt;
    *pkt = strpbrk (*pkt, " \t\r\n");
    if (!*pkt || !**pkt)
    {
      *pkt = strReturn +strlen(strReturn);
      return strReturn;
    }
    *(*pkt)++ = '\0';
  }
  return strReturn;
}

unsigned int SolveHostname(const char* host)
{
  struct in_addr  sin_addr;
  ASSERT(host);
  if(!host)
    return 0;
  if((sin_addr.s_addr = inet_addr(host)) == INADDR_NONE)
  {
    struct hostent *pHost;
    LogMultiStart("Solving hostname %s... ",host);
    if(!(pHost = (struct hostent*)gethostbyname(host)))
    {
      LogMultiEnd("failed");
      return INADDR_NONE;
    }
    sin_addr.s_addr = ((struct in_addr*)pHost->h_addr)->s_addr;
    LogMultiEnd(iptoa(sin_addr.s_addr));
  }
  return sin_addr.s_addr;
}

unsigned int SolveIP(const char* ip)
{
  ASSERT(ip);
  if(!ip)
    return 0;
  return inet_addr(ip);
}

int BuildFilename(const char* strPath, const char* strFile, char* strOut, unsigned int nSize)
{
  ASSERT(strPath);
  ASSERT(strFile);
  ASSERT(strOut);
  ASSERT(nSize > 0);
  if(!strPath || !strFile || !strOut || nSize <= 0)
    return 0;
  if(*strFile == '/')
    strformat(strOut,nSize,"%s",strFile);
  else if(*strFile == '~')
    strformat(strOut,nSize,"%s%s",getenv("HOME"),strFile+1);
  else if(*strPath == '~')
    strformat(strOut,nSize,"%s%s/%s",getenv("HOME"),strPath+1,strFile);
  else
    strformat(strOut,nSize,"%s/%s",strPath,strFile);
  return 1;
}

unsigned long atoul(const char* str)
{
/*  ASSERT(str);
  if(!str)
    return 0;
*/  if(*str >= '0' && *str <= '9')
  {
    unsigned long l = *(str++)-'0';
    while(*str >= '0' && *str <= '9')
    {
      l *= 10;
      l += *(str++)-'0';
    }
    return l;
  }
  return 0;
}

char* strcasestr(const char* str, const char* sea)
{
  const char *strcom,*seacom;
/*  ASSERT(str);
  ASSERT(sea);
  if(!str || !sea)
    return 0;
*/  for(; *str; str++)
    if(tolower(*str) == tolower(*sea))
    {
      for(seacom = sea+1,strcom = str+1; *seacom && *strcom; seacom++,strcom++)
        if(tolower(*strcom) != tolower(*seacom))
          break;
      if(!*seacom)
        return (char*)str;
    }
  return 0;
}

size_t vstrformat(char* str, size_t maxsize, const char* format, va_list ap)
{
  size_t size;

  ASSERT(str);
  ASSERT(maxsize > 0);
  ASSERT(format);
  ASSERT(ap);

  maxsize--;
#ifdef _WINDOWS
  size = _vsnprintf(str,maxsize,format,ap);
#else
  size = vsnprintf(str,maxsize,format,ap);
#endif
  if(/*size < 0 || */ size >= maxsize)
  {
    str[maxsize] = '\0';
    return maxsize;
  }
  return size;
}

size_t strformat(char* str, size_t maxsize, const char* format, ...)
{
  va_list ap;
  size_t size;

  va_start (ap, format);
  size = vstrformat(str,maxsize,format,ap);
  va_end (ap);

  return size;
}
