/*
         file: bounced.c
   desciption: main source file
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
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#if !defined(_WIN32) || defined(__CYGWIN__)
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/utsname.h>
#if defined(USE_UID) && defined (USE_GID)
#include <grp.h>
#include <pwd.h>
#endif /* defined(USE_UID) && defined (USE_GID) */
#else
#include <process.h>
#endif /* !defined(_WIN32) || defined(__CYGWIN__) */

#include "bounced.h"

char g_strVersion[sizeof(PACKAGE)+1+sizeof(VERSION)+1];
char g_strSystem[32];
time_t g_timeNow = 0;
time_t g_timeStart = 0;
time_t g_timeLastStats = 0;
char* g_strConfigDir = CONFIGDIR;
#ifndef _WINDOWS
int g_nfds = 0;
#endif /* _WINDOWS */
fd_set g_fdsR;
fd_set g_fdsW;
fd_set g_fdsE;
SOCKET g_sServer = 0;
char g_strOutBuffer[OUTBUFFER];
char g_strInBuffer[INBUFFER];
char g_strInBufferCopy[INBUFFER];
char* g_strCurrentCommand;
unsigned int g_nCurrentCommandLength;
char g_bShutdown = 0;
char g_bUsersChanged = 0;
char g_bConfigChanged = 0;
char* g_strDefaultPrefix = DEFAULT_PREFIX;
char* g_strDefaultChanModes = DEFAULT_CHANMODES;
#ifndef _WINDOWS
unsigned char g_bBackground = 1;
#endif

double g_dConnections = 0;
double g_dLogins = 0;
double g_dTotalBytesIn = 0;
double g_dTotalBytesOut = 0;
double g_dLastConnections = 0;
double g_dLastLogins = 0;
double g_dLastTotalBytesIn = 0;
double g_dLastTotalBytesOut = 0;
unsigned int g_nLastIncomingTraffic = 0;
unsigned int g_nLastOutgoingTraffic = 0;

char g_DebugHasMotdCopied = 0;/* 4 debug */
LIST(MOTD_LIST) g_DebuglistMotd; /* 4 debug */
MOTDSTR g_DebugMotdStrFirst;/* 4 debug */

LIST(CONNECTION_LIST) g_listConnections;
LIST(CONNECTIONASYNC_LIST) g_listConnectionAsyncs;
LIST(FDR_LIST) g_listFDR;
LIST(FDW_LIST) g_listFDW;
LIST(FDE_LIST) g_listFDE;
LIST(CLIENT_LIST) g_listClients;
LIST(SERVER_LIST) g_listServers;
HASHLIST(USER_HASHLIST) g_hashlistUsers;
LIST(MOTD_LIST) g_listMotd;
LIST(MOTDVAR_LIST) g_listMotdVars;

int Args(int argc, char* argv[])
{
  int i;
  for(i = 0; i < argc; i++)
  {
    if((*argv[i] == '-' || *argv[i] == '/') && argv[i][1] != '\0' && argv[i][2] == '\0')
      switch(argv[i][1])
      {
      case 'c':
        if(i+1 < argc /* && *argv[i+1] != '-' && *argv[i+1] != '/' */)
        {
          i++;
          g_strConfigDir = argv[i];
        }
        break;
      case 'v':
          fprintf(stderr,"%s %s\n",PACKAGE,VERSION);
          fprintf(stderr,"Copyright (C) 2003 Colin Graf (addition@users.sourceforge.net)\n");
        exit(0);
        break;

#ifndef _WINDOWS
      case 'd':
        g_bBackground = 0;
        break;
#endif
      default:
#ifndef _WINDOWS
        fprintf(stderr,"usage: %s [ -d ] [ -c DIR ] [ -h ] [ -v ]\n",PACKAGE);
                 fputs("  -d    run not as a background process (debug)\n", stderr);
#else
        fprintf(stderr,"usage: %s [ -c DIR ] [ -h ] [ -v ]\n",PACKAGE);
#endif
        fprintf(stderr,"  -c DIR  read config files from DIR (default: %s)\n",CONFIGDIR);
                 fputs("  -h    print this help message\n", stderr);
                     fputs("  -v    display version information\n", stderr);
                exit(0);
        break;
      }
  }

  Log("Starting %s %s...",PACKAGE,VERSION);

  /* change umask to something more secure */
#ifndef _WINDOWS
    umask(077);
#endif

  /* switch user */
#if !defined(_WINDOWS) && defined(USE_UID) && defined (USE_GID)
  if(getuid () == 0)
  {
    unsigned int n;
    char *p;
    struct passwd *pw;
    struct group *gr;
    
    n = strtol(USE_GID,&p,10);
    if(*p)
    {
      /* probably a string */
      gr = getgrnam(USE_GID);
      if(!gr)
      {
        Log("error: Unable to find gid for group %s",USE_GID);
        return 0;
      }
      n = gr->gr_gid;
    }
    if(setgid(n))
    {
      Log("error: Couldn't switch to group %u",n);
      return 0;
    }

    n = strtol(USE_UID,&p,10);
    if(*p)
    {
      /* probably a string */
      pw = getpwnam(USE_UID);
      if(!pw)
      {
        Log("error: Unable to find uid for user %s",USE_UID);
        return 0;
      }
      n = pw->pw_uid;
    }
    if(setuid(n))
    {
      Log("error: Couldn't switch to user %u",n);
      return 0;
    }

    Log("Running as user %s and group %s",USE_UID,USE_GID);
  }
#endif /* !defined(_WINDOWS) && defined(USE_UID) && defined (USE_GID) */

  /* if running in daemon mode, reopen stdout as a log file */
#ifndef _WINDOWS
  if(g_bBackground)
    if(!LogOpenFile())
      return 0;
#endif

  /* switch to background */
#ifndef _WINDOWS
  if(g_bBackground)
  {
    if(fork() == 0)
      setsid();
    else
      exit(0);
  }
#endif /* _WINDOWS */

  return 1;
}


int Init(void)
{
  /* zero structs */
  LIST_INIT(CONNECTION_LIST,g_listConnections);
  LIST_INIT(CONNECTIONASYNC_LIST,g_listConnectionAsyncs);
  LIST_INIT(FDR_LIST,g_listFDR);
  LIST_INIT(FDW_LIST,g_listFDW);
  LIST_INIT(FDE_LIST,g_listFDE);
  LIST_INIT(CLIENT_LIST,g_listClients);
  LIST_INIT(SERVER_LIST,g_listServers);
  HASHLIST_INIT(USER_HASHLIST,g_hashlistUsers);
  LIST_INIT(MOTD_LIST,g_listMotd);
  LIST_INIT(MOTDVAR_LIST,g_listMotdVars);

  /* clean fd_sets */
  FD_ZERO(&g_fdsR);
  FD_ZERO(&g_fdsW);
  FD_ZERO(&g_fdsE);

  /* init debug stuff */
#ifdef DEBUG
  printf("Debug: Enabled!\n");
#endif
  if( !DebugMemoryInit() )
    return 0;

  /* init sockets */
#ifdef _WINDOWS
  {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(1, 1), &wsaData) ||
      MAKEWORD(1, 1) != wsaData.wVersion)
    {
      Log("error: WSAStartup failed");
      return 0;
    }
  }
#endif /*_WINDOWS*/

  /* hook signals */
#ifndef _WINDOWS
  {
    struct sigaction sa;
    memset(&sa,0,sizeof(sa));
    sa.sa_handler = SignalHandler;
    sigaction(SIGHUP,&sa,0);
    sigaction(SIGTERM,&sa,0);
    sigaction(SIGINT,&sa,0);
    sigaction(SIGUSR1,&sa,0);
    sigaction(SIGPIPE,&sa,0);
  }
#else
  signal(SIGABRT,SignalHandler);
  signal(SIGINT,SignalHandler);
  signal(SIGBREAK,SignalHandler);
  signal(SIGTERM,SignalHandler);
#endif /* _WINDOWS */

  /* build version string */
  strformat(g_strVersion,sizeof(g_strVersion),"%s %s",PACKAGE,VERSION);

  /* build system info string */
  {
  #ifdef _WIN32
    DWORD dw = GetVersion();
  #ifdef __CYGWIN__
    char* strCYGWIN = " CYGWIN";
  #else
    char* strCYGWIN = "";
  #endif
    if(dw < 0x80000000) /* nt, win2k, winxp */ 
    {
      if(LOBYTE(LOWORD(dw)) == 5 && HIBYTE(LOWORD(dw)) == 0) /* win2k */
        strformat(g_strSystem,sizeof(g_strSystem),"Windows 2000%s",strCYGWIN);
      else if(LOBYTE(LOWORD(dw)) == 5 && HIBYTE(LOWORD(dw)) == 1) /* winxp */
        strformat(g_strSystem,sizeof(g_strSystem),"Windows XP%s",strCYGWIN);
      else
        strformat(g_strSystem,sizeof(g_strSystem),"Windows NT %u%s",LOBYTE(LOWORD(dw)),strCYGWIN);
    }
    else /* win98, win95, win3.1, winme?*/
    {
      if(LOBYTE(LOWORD(dw)) == 4 && HIBYTE(LOWORD(dw)) == 1) /* win95 */
        strformat(g_strSystem,sizeof(g_strSystem),"Windows 95%s",strCYGWIN);
      if(LOBYTE(LOWORD(dw)) == 4 && HIBYTE(LOWORD(dw)) == 6) /* win98 */
        strformat(g_strSystem,sizeof(g_strSystem),"Windows 98%s",strCYGWIN);
      else
        strformat(g_strSystem,sizeof(g_strSystem),"Windows %u.%u%s",LOBYTE(LOWORD(dw)),HIBYTE(LOWORD(dw)),strCYGWIN);
    }
  #else
    struct utsname buf;
    uname(&buf);
    strformat(g_strSystem,sizeof(g_strSystem),buf.release ? "%s %s" : "%s", buf.sysname,buf.release);
  #endif
  }

  /* init client handler.. (can't go wrong) */
  ClientHandlersInit();
  ServerHandlersInit(); /* same here */
  ProfileHandlersInit(); /* same here */

  /* load config */
  if(!ConfigLoad(1))
    return 0;

  /* load users */
  if(!UsersLoad())
    return 0;
  if(!ProfilesLoad(1))
    return 0;

  /* load motd */
  MotdLoad();

  return 1;
}


void Cleanup(void)
{
  ASSERT(g_listConnectionAsyncs.nCount == 0);
  LIST_REMOVE_ALL(CONNECTION_LIST,g_listConnections);
  ASSERT(g_listFDR.nCount == 0);
  ASSERT(g_listFDW.nCount == 0);
  ASSERT(g_listFDE.nCount == 0);

  ASSERT(g_listClients.nCount == 0);
  ASSERT(g_listServers.nCount == 0);

  HASHLIST_REMOVE_ALL(USER_HASHLIST,g_hashlistUsers);

  MotdFree();
  ASSERT(g_listMotd.nCount == 0);
  ASSERT(g_listMotdVars.nCount == 0);

  ConfigFree();

#ifdef _WINDOWS
  WSACleanup();
  WSACleanup();
#endif /*_WINDOWS*/

  /* cleanup debug stuff (look for memory leaks) */
  if( !DebugMemoryCleanup() )
    return;

  Log("Exit");
}

void SignalHandler(int iSig)
{
  Log("Received signal \"%d\"",iSig);
#ifndef _WINDOWS
  switch(iSig)
  {
  case SIGHUP:
    ConfigLoad(0);
    MotdLoad();
    break;
  case SIGPIPE: /* i have no idea why this signal sometimes appear. in my opinion are all sockets clean when i try to send to them */
    break; /* ignore them. don't let the process die */
  default:
#endif
    {
      SOCKET s = g_sServer;
      g_sServer = 0;
      g_bShutdown = 1;
      closesocket(s); /* this causes select stop */
    }
#ifndef _WINDOWS
    break;
  }
#endif
}

void StatsTimer(void* pData) /* called from timer */
{
  time_t timeDelta;

  timeDelta = g_timeNow-g_timeLastStats;
  if(!timeDelta)
    timeDelta = 1;

  g_nLastIncomingTraffic = (unsigned int)((g_dTotalBytesIn-g_dLastTotalBytesIn)/timeDelta);
  g_nLastOutgoingTraffic = (unsigned int)((g_dTotalBytesOut-g_dLastTotalBytesOut)/timeDelta);

  Log("************************ %s stats ************************",g_strVersion);
  strftimet(g_strInBuffer,sizeof(g_strInBuffer),"%A, %d of %B %Y %X",g_timeStart);
  Log("          Started: %s",g_strInBuffer);
  strftimetspan(g_strInBuffer,sizeof(g_strInBuffer),"%#D days, %#H hours, %#M minutes and %#S seconds",g_timeNow-g_timeStart);
  Log("           Uptime: %s",g_strInBuffer);
  Log("Total connections: %.0f (%.0f Logins)",g_dConnections,g_dLogins);
  Log("    Total traffic: %.0f MB in, %.0f MB out",g_dTotalBytesIn/1048576,g_dTotalBytesOut/1048576);
  Log("********************************************************************");
  Log("      Connections: %u (%u Clients, %u Servers)",g_listConnections.nCount,g_listClients.nCount,g_listServers.nCount);
  Log("            Users: %u",g_hashlistUsers.nCount);
  Log("          Traffic: %.2f kB/s (%.2f kB/s in, %.2f kB/s out)",((double)(g_nLastIncomingTraffic+g_nLastOutgoingTraffic))/1000,((double)g_nLastIncomingTraffic)/1000,((double)g_nLastOutgoingTraffic)/1000);
  Log("********************************************************************");

#ifdef DEBUG_MEMORY
  DebugMemoryShowStats();
#endif /* DEBUG_MEMORY */

  g_timeLastStats = g_timeNow;
  g_dLastConnections = g_dConnections;
  g_dLastLogins = g_dLogins;
  g_dLastTotalBytesIn = g_dTotalBytesIn;
  g_dLastTotalBytesOut = g_dTotalBytesOut;

  return;
  pData = 0;
}

int main(int argc, char* argv[])
{
  PCONNECTION pCon,pConnection;
  struct timeval tv;
  fd_set fdsr, fdsw, fdse;

  tv.tv_usec = 0;

  /* get start time */
  g_timeNow = g_timeStart = time(0);

  /* parse args */
  if(!Args(argc,argv))
    return 1;

  Log("*bounce*");

  /* init */
  if(!Init())
  {
    Cleanup();
    return 1;
  }

  /* init server socket */
  {    
    /* create socket */
    if((g_sServer = socket(AF_INET,SOCK_STREAM,0)) == INVALID_SOCKET ||
      !ConnectionSetNonBlocking(g_sServer) ||
#ifndef _WINDOWS
      !ConnectionSetReuseAddress(g_sServer) ||
#endif
      !ConnectionSetInterface(g_sServer,(unsigned short)c_nListenPort,SolveIP(c_strListenInterface)) )
    {
      if(g_sServer < 0)
        Log("error: Couldn't listen on port \"%u\": %s (%u)",c_nListenPort,strerror(ERRNO),ERRNO);  
      else
        closesocket(g_sServer);
      Cleanup();
      return 1;
    }

    /* listen */
    if(listen(g_sServer, SOMAXCONN) < 0)
    {
      Log("error: Couldn't listen on port \"%u\": %s (%u)",c_nListenPort,strerror(ERRNO),ERRNO);  
      if(g_sServer > 0)
        closesocket(g_sServer);
      Cleanup();
      return 1;
    }

    /* add socket to select() */
#ifndef _WINDOWS
    g_nfds = g_sServer+1;
#endif /* _WINDOWS */
    FD_SET(g_sServer, &g_fdsR);
    Log("Listening on port \"%u\"",c_nListenPort);  
  }

  /* dump pid */
#ifndef _WINDOWS
  {
    char strFile[MAX_PATH];
    FILE *fp;
    /* if(*PIDFILE == '/')
      strcpy(strFile,PIDFILE);
    else
      snprintf(strFile,sizeof(strFile),"%s/%s",g_strConfigDir,PIDFILE); */
    BuildFilename(g_strConfigDir,PIDFILE,strFile,sizeof(strFile));
    if(!(fp = fopen(strFile,"w")))
    {
      Log("error: Couldn't open pid file \"%s\"",strFile);
      if(g_sServer > 0)
        closesocket(g_sServer);
      Cleanup();
      return 1;
    }
    fprintf(fp,"%u",getpid());
    fclose(fp);
  }
#endif /* _WINDOWS */
  
  /* show stats (just for fun) */
  StatsTimer(&g_timeNow);

  /* maybe there are already connections to close (when dns lookup failed after profile loading) */
  ConnectionsClose();

  /* select loop */    
  while(g_sServer)
  {
    /* set timeout */
    tv.tv_sec = TimerNext(g_timeNow,60);

    fdsr = g_fdsR;
    fdsw = g_fdsW;
    fdse = g_fdsE;

    /* wait for any action */
#ifdef DEBUG
    Log("select: Read=%u Write=%u Error=%u Timeout=%u",g_listFDR.nCount+1,g_listFDW.nCount,g_listFDE.nCount,tv.tv_sec);
#endif
    if(select(g_nfds,&fdsr, &fdsw, &fdse, &tv) < 0)
    {
      if(g_sServer)
        Log("error: Select returned \"-1\": %s (%u)",strerror(ERRNO),ERRNO);
      continue;
    }
    /* ASSERT(ERRNO == 0); */

    /* get time */
    g_timeNow = time(0);

    /* new connection? */
    if(FD_ISSET(g_sServer, &fdsr))
      ConnectionsAccept();

    /* where can i read ? */
    pCon = g_listFDR.pFirst;
    while(pCon)
    {
      if(FD_ISSET(pCon->s,&fdsr))
      {
        pConnection = pCon;
        pCon = pCon->llFDR.pNext;
        ConnectionRead(pConnection);
        continue;
      }
      pCon = pCon->llFDR.pNext;
    }

    /* where can i write ? */
    pCon = g_listFDW.pFirst;
    while(pCon)
    {
      if(FD_ISSET(pCon->s,&fdsw))
      {
        pConnection = pCon;
        pCon = pCon->llFDW.pNext;
        ConnectionWrite(pConnection);
        continue;
      }
      pCon = pCon->llFDW.pNext;
    }

    /* where r errors? */
    pCon = g_listFDE.pFirst;
    while(pCon)
    {
      if(FD_ISSET(pCon->s,&fdse))
      {
        pConnection = pCon;
        pCon = pCon->llFDE.pNext;
        ConnectionError(pConnection);
        continue;
      }
      pCon = pCon->llFDE.pNext;
    }

    TimerExec(g_timeNow);
    ConnectionsClose();

    /* debug.. check if everything is ok */
#ifdef DEBUG_MEMORY
    DebugMemoryCheckBlocks();
#endif
  }

  /* close socket */
  if(g_sServer)
  {
    closesocket(g_sServer);
    g_sServer = 0;
  }
  Log("Stopped listening");
  g_bShutdown = 1;

  /* dump config stuff */
  ConfigDump();
  UsersDump();
  ProfilesDump();

  /* show stats at end (just for fun) */
  g_timeNow = time(0);
  StatsTimer(&g_timeNow);

  /* delete pid file*/
#ifndef _WINDOWS
  {
    char strFile[MAX_PATH];
    /* if(*PIDFILE == '/')
      strcpy(strFile,PIDFILE);
    else
      snprintf(strFile,sizeof(strFile),"%s/%s",g_strConfigDir,PIDFILE); */
    BuildFilename(g_strConfigDir,PIDFILE,strFile,sizeof(strFile));
    if(unlink(strFile))
      Log("error: Couldn't delete pid file \"%s\"",strFile);
  }
#endif /* _WINDOWS */

  /* cleanup */
  Cleanup();
  return 0;
}
