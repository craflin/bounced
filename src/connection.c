/*
         file: connection.c
   desciption: handle socket connections
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
#endif /*HAVE_CONFIG_H*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#if !defined(_WIN32) || defined(__CYGWIN__)
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#endif /* !defined(_WIN32) || defined(__CYGWIN__) */

#include "bounced.h"

size_t vstrformat(char* str, size_t maxsize, const char* format, va_list ap);

/* helper functions */

int ConnectionSetNonBlocking(SOCKET s)
{
#ifdef _WINDOWS
  int val = 1;
#endif
  ASSERT(s >= 0);
  if(s < 0)
    return 0;
#ifdef _WINDOWS
  if(ioctlsocket(s,FIONBIO,&val))
#else
  if(fcntl(s,F_SETFL,O_NONBLOCK))
#endif /* !_WINDOWS */
  {
    Log("error: Couldn't set socket to nonblocking: %s (%u)",strerror(ERRNO),ERRNO);
    return 0;
  }
  return 1;
}

int ConnectionSetKeepAlive(SOCKET s)
{
  int val = 1;
  ASSERT(s >= 0);
  if(s < 0)
    return 0;
  if(setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (char*) & val, sizeof (val)) != 0)
  {
    Log("error: Couldn't set socket to keepalive: %s (%u)",strerror(ERRNO),ERRNO);
    return 0;
  }
  return 1;
}

int ConnectionSetReuseAddress(SOCKET s)
{
  int val = 1;
  ASSERT(s >= 0);
  if(s < 0)
    return 0;
  if(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*) & val, sizeof (val)) != 0)
  {
    Log("error: Couldn't set socket to reuse address: %s (%u)",strerror(ERRNO),ERRNO);
    return 0;      
  }
  return 1;
}

int ConnectionSetInterface(SOCKET s, unsigned short sPort, unsigned int nInterfaceIP)
{
  struct sockaddr_in sin;
  ASSERT(s > 0);
  if(s < 0)
    return 0;

  /* create sin */
  memset(&sin,0,sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_port = htons(sPort);
  sin.sin_addr.s_addr = (nInterfaceIP == INADDR_NONE) ? INADDR_ANY : nInterfaceIP;

  if((sin.sin_addr.s_addr != INADDR_ANY || sin.sin_port != 0) &&
    bind(s,(struct sockaddr*)&sin,sizeof(sin)) < 0)
  {
    Log("error: Couldn't set socket to interface %s:%u: %s (%u)",iptoa(sin.sin_addr.s_addr),htons(sin.sin_port),strerror(ERRNO),ERRNO);  
    return 0;
  }
  return 1;
}

void ConnectionFDRSet(PCONNECTION pConnection)
{
  SOCKET s = pConnection->s;

  ASSERT(!pConnection->bClosing);
  ASSERT(pConnection->llFDR.ppPreviousNext == 0);
  LIST_ADD(FDR_LIST,g_listFDR,pConnection);

  FD_SET(s,&g_fdsR);
#ifndef _WINDOWS
  if((int)++s > g_nfds)
    g_nfds = s;
#endif /* _WINDOWS */
}

void ConnectionFDRClear(PCONNECTION pConnection)
{
  SOCKET s = pConnection->s;

  ASSERT(pConnection->llFDR.ppPreviousNext != 0);
  LIST_REMOVE_ITEM(FDR_LIST,g_listFDR,pConnection);
  pConnection->llFDR.ppPreviousNext = 0;

  FD_CLR(s,&g_fdsR);
#ifndef _WINDOWS
  if((int)s+1 >= g_nfds)
  {
    g_nfds = g_sServer;
    for(pConnection = g_listFDR.pFirst; pConnection; pConnection = pConnection->llFDR.pNext)
      if((int)pConnection->s > g_nfds)
        g_nfds = pConnection->s;
    g_nfds++;
  }
#endif /* _WINDOWS */
}

void ConnectionFDWSet(PCONNECTION pConnection)
{
  ASSERT(!pConnection->bClosing);
  ASSERT(pConnection->llFDW.ppPreviousNext == 0);
  LIST_ADD(FDW_LIST,g_listFDW,pConnection);

  FD_SET(pConnection->s,&g_fdsW);
}

void ConnectionFDWClear(PCONNECTION pConnection)
{
  ASSERT(pConnection->llFDW.ppPreviousNext != 0);
  LIST_REMOVE_ITEM(FDW_LIST,g_listFDW,pConnection);
  pConnection->llFDW.ppPreviousNext = 0;

  FD_CLR(pConnection->s,&g_fdsW);
}

void ConnectionFDESet(PCONNECTION pConnection)
{
  ASSERT(!pConnection->bClosing);
  ASSERT(pConnection->llFDE.ppPreviousNext == 0);
  LIST_ADD(FDE_LIST,g_listFDE,pConnection);

  FD_SET(pConnection->s,&g_fdsE);
}

void ConnectionFDEClear(PCONNECTION pConnection)
{
  ASSERT(pConnection->llFDE.ppPreviousNext != 0);
  LIST_REMOVE_ITEM(FDE_LIST,g_listFDE,pConnection);
  pConnection->llFDE.ppPreviousNext = 0;

  FD_CLR(pConnection->s,&g_fdsE);
}

/* global functions */

PCONNECTION ConnectionCreate(SOCKET s, unsigned int nIP, unsigned short sPort, time_t timeConnected, unsigned char cType, void* pData)
{
  PCONNECTION pConnection;

  if( !(pConnection = CALLOC(PCONNECTION,1,sizeof(CONNECTION))) )
  {
    OUTOFMEMORY;
    if(pConnection)
      ConnectionFree(pConnection);
    return 0;
  }
  pConnection->s = s;
  pConnection->nIP = nIP;
  pConnection->nPort = sPort;
  pConnection->timeConnected = timeConnected;
  pConnection->cType = cType;
  pConnection->pData = pData;

  LIST_ADD(CONNECTION_LIST,g_listConnections,pConnection);

  return pConnection;
}

void ConnectionFree(PCONNECTION pConnection)
{
  if(pConnection->cType == CT_CLIENT)
    LIST_REMOVE_ITEM(CLIENT_LIST,g_listClients,(PCLIENT)pConnection->pData)
  else if(pConnection->cType == CT_SERVER)
    LIST_REMOVE_ITEM(SERVER_LIST,g_listServers,(PSERVER)pConnection->pData);

  if(pConnection->llFDR.ppPreviousNext)
    ConnectionFDRClear(pConnection);
  if(pConnection->llFDW.ppPreviousNext)
    ConnectionFDWClear(pConnection);
  if(pConnection->llFDE.ppPreviousNext)
    ConnectionFDEClear(pConnection);

  if(pConnection->strInBuffer)
    FREE(char*,pConnection->strInBuffer);

  if(pConnection->s)
    closesocket(pConnection->s);

  FREE(PCONNECTION,pConnection);
}

void ConnectionCloseAsync(PCONNECTION pConnection)
{
  if(pConnection->bClosing)
    return;

  pConnection->bClosing = 1;
  LIST_ADD(CONNECTIONASYNC_LIST,g_listConnectionAsyncs,pConnection);

  if(pConnection->llFDR.ppPreviousNext)
    ConnectionFDRClear(pConnection);
  if(pConnection->llFDW.ppPreviousNext)
    ConnectionFDWClear(pConnection);
  if(pConnection->llFDE.ppPreviousNext)
    ConnectionFDEClear(pConnection);
}

void ConnectionsClose(void)
{
  LIST_REMOVE_ALL(CONNECTIONASYNC_LIST,g_listConnectionAsyncs);
}

int ConnectionConnect(PCONNECTION pConnection, unsigned int nIP, unsigned short sPort, unsigned int nInterfaceIP)
{
  SOCKET s;
  struct sockaddr_in sin;

  ASSERT(pConnection->s == 0);
  ASSERT(!pConnection->bClosing);

  if((s = socket(AF_INET,SOCK_STREAM,0)) == INVALID_SOCKET)
  {
    ConnectionCloseAsync(pConnection);
    return 0;
  }

  if( !ConnectionSetNonBlocking(s) ||
    !ConnectionSetKeepAlive(s) ||
#ifndef _WINDOWS
    !ConnectionSetReuseAddress(s) ||
#endif /*!_WINDOWS*/
    !ConnectionSetInterface(s,0,nInterfaceIP) )
  {
    closesocket(s);
    ConnectionCloseAsync(pConnection);
    return 0;
  }

  memset(&sin,0,sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_port = htons(sPort);
  pConnection->nIP = sin.sin_addr.s_addr = nIP;

  if(connect(s,(struct sockaddr*)&sin,sizeof(sin)) < 0)
  {
    int iErrno = ERRNO;
    if(iErrno && iErrno != EINPROGRESS
#ifdef _WINDOWS
      && iErrno != EWOULDBLOCK
#endif
      )
    {
      pConnection->nIP = 0;
      closesocket(s);
      ConnectionCloseAsync(pConnection);
      return 0;
    }
  }

  pConnection->s = s;

  ConnectionFDRSet(pConnection);
  ConnectionFDWSet(pConnection);
  ConnectionFDESet(pConnection);

  return 1;
}

int ConnectionSend(PCONNECTION pConnection, const char* strBuffer, int iLength)
{
  ASSERT(pConnection);
  ASSERT(strBuffer);

  if( pConnection->bClosing ||
    pConnection->llFDE.ppPreviousNext )
    return 0;

  if(iLength < 0)
    iLength = strlen(strBuffer);

#ifdef DEBUG_PROTOCOL
  /*if(pConnection->nIP == INADDR_LOOPBACK)
  {
    FILE* fp;
    fp = fopen("127.0.0.1.log","a");
    if(fp)
    {
      fprintf(fp,"%s",strBuffer);
      fclose(fp);
    }
  }*/
  Log("%s < %s",(pConnection->cType == CT_CLIENT) ? (((PCLIENT)pConnection->pData)->strName ? ((PCLIENT)pConnection->pData)->strName : iptoa(pConnection->nIP)) : ((PSERVER)pConnection->pData)->strServer,strBuffer);
#endif /* DEBUG_PROTOCOL */

  if(pConnection->strOutBuffer)
  {
    if(pConnection->nOutBuffer-(pConnection->nOutBufferOffset+pConnection->nOutBufferUse) >= (unsigned int)iLength)
    {
      memcpy(pConnection->strOutBuffer+(pConnection->nOutBufferOffset+pConnection->nOutBufferUse),strBuffer,iLength);
      pConnection->nOutBufferUse += iLength;
    }
    else
    {
      if(pConnection->nOutBuffer-pConnection->nOutBufferUse >= (unsigned int)iLength)
      {
        memcpy(pConnection->strOutBuffer,pConnection->strOutBuffer+pConnection->nOutBufferOffset,pConnection->nOutBufferUse);
        pConnection->nOutBufferOffset = 0;
        memcpy(pConnection->strOutBuffer+pConnection->nOutBufferUse,strBuffer,iLength);
        pConnection->nOutBufferUse += iLength;
      }
      else
      {
        char* strNew;
        pConnection->nOutBuffer = pConnection->nOutBufferUse+iLength;
        if( !(strNew = MALLOC(char*,pConnection->nOutBuffer)) )
        {
          OUTOFMEMORY;
          ConnectionCloseAsync(pConnection);
          return 0;
        }
        memcpy(strNew,pConnection->strOutBuffer+pConnection->nOutBufferOffset,pConnection->nOutBufferUse);
        pConnection->nOutBufferOffset = 0;
        memcpy(strNew+pConnection->nOutBufferUse,strBuffer,iLength);
        pConnection->nOutBufferUse += iLength;
        FREE(char*,pConnection->strOutBuffer);
        pConnection->strOutBuffer = strNew;
      }
    }
    return 1;
  }
  else
  {
    int i;
    i = send(pConnection->s,strBuffer,iLength,0);
    if(i <= 0)
    {
      if(i == 0 || ERRNO != EWOULDBLOCK)
      {
        ConnectionCloseAsync(pConnection);
        return 0;
      }
      i = 0;
    }
    g_dTotalBytesOut += i;
    if(i == iLength)
      return 1;
    
    ASSERT(pConnection->nOutBufferOffset == 0);
    ASSERT(pConnection->nOutBuffer == 0);
    ASSERT(pConnection->nOutBufferUse == 0);
    ASSERT(pConnection->strOutBuffer == 0);
    ASSERT(pConnection->llFDW.ppPreviousNext == 0);

    pConnection->nOutBufferUse = iLength-i;
    pConnection->nOutBuffer = pConnection->nOutBufferUse;
    if( !(pConnection->strOutBuffer = MALLOC(char*,pConnection->nOutBuffer)) )
    {
      ConnectionCloseAsync(pConnection);
      return 0;
    }
    memcpy(pConnection->strOutBuffer,strBuffer+i,pConnection->nOutBufferUse);

    ConnectionFDWSet(pConnection);

    return 1;
  }
}

int ConnectionSendFormat(PCONNECTION pConnection, const char* format,...)
{
  va_list ap;
  unsigned short sLength;

  if(pConnection->bClosing)
    return 0;

  va_start (ap, format);
  sLength = vstrformat(g_strOutBuffer,sizeof(g_strOutBuffer),format, ap);
  va_end (ap);

  return ConnectionSend(pConnection,g_strOutBuffer,sLength);
}

int ConnectionRead(PCONNECTION pConnection)
{
  unsigned int nInBuffer = 0;
  int i;

  ASSERT(pConnection->cType == CT_CLIENT || pConnection->cType == CT_SERVER);

  if(pConnection->bClosing)
    return 0;

  if(pConnection->strInBuffer)
  {
    ASSERT(pConnection->nInBuffer);
    memcpy(g_strInBuffer,pConnection->strInBuffer,pConnection->nInBuffer);
    nInBuffer += pConnection->nInBuffer;
    FREE(char*,pConnection->strInBuffer);
    pConnection->strInBuffer = 0;
    pConnection->nInBuffer = 0;  
  }

  if((i = recv(pConnection->s,g_strInBuffer+nInBuffer,sizeof(g_strInBuffer)-nInBuffer-1,0)) <= 0)
  {
    if(i == 0 || ERRNO != EWOULDBLOCK)
    {
      if(pConnection->llFDE.ppPreviousNext)
        return ConnectionError(pConnection);
      ConnectionCloseAsync(pConnection);
      return 0;
    }
    i = 0;
  }
  if(i > 0)
  {
    char *str,*strStart;
    char c;

    g_dTotalBytesIn += i;
    nInBuffer += i;
    g_strInBuffer[nInBuffer] = '\0';

    for(strStart = g_strInBuffer; (str = strpbrk(strStart,"\n\r")); strStart = str)
    {
      str++;
      if(isspace(*str))
        str++;

      c = *str;
      *str = '\0';

#ifdef DEBUG_PROTOCOL
      Log("%s > %s",(pConnection->cType == CT_CLIENT) ? (((PCLIENT)pConnection->pData)->strName ? ((PCLIENT)pConnection->pData)->strName : iptoa(pConnection->nIP)) : ((PSERVER)pConnection->pData)->strServer,strStart);
#endif /* DEBUG_PROTOCOL */

      if(pConnection->cType == CT_CLIENT)
        ClientHandleCommand((PCLIENT)pConnection->pData,strStart,str-strStart);
      else
        ServerHandleCommand((PSERVER)pConnection->pData,strStart,str-strStart);

#ifdef DEBUG_MEMORY
      DebugMemoryCheckBlocks(); /* check if memory ain't brocken after this cmd */
#endif

      if(pConnection->bClosing)
        return 0;
      
      *str = c;
    }

    /* only crap received */
    if(strStart == g_strInBuffer && nInBuffer == sizeof(g_strInBuffer)-1)
    {
      Log("error: %s in buffer overflow",iptoa(pConnection->nIP));
      ConnectionCloseAsync(pConnection);
      return 0;
    }

    if(strStart < g_strInBuffer+nInBuffer)
    {
      pConnection->nInBuffer = nInBuffer-(strStart-g_strInBuffer);
      if( !(pConnection->strInBuffer = MALLOC(char*,pConnection->nInBuffer)) )
      {
        pConnection->nInBuffer = 0;
        OUTOFMEMORY;
        ConnectionCloseAsync(pConnection);
        return 0;
      }
      memcpy(pConnection->strInBuffer,strStart,pConnection->nInBuffer);
    }
  }
  else if(nInBuffer)
  {
    if( !(pConnection->strInBuffer = MALLOC(char*,nInBuffer)) )
    {
      OUTOFMEMORY;
      ConnectionCloseAsync(pConnection);
      return 0;
    }
    pConnection->nInBuffer = nInBuffer;
    memcpy(pConnection->strInBuffer,g_strInBuffer,nInBuffer);
  }

  return 1;
}

int ConnectionWrite(PCONNECTION pConnection)
{
  int i;

  if(pConnection->bClosing) /* wenn das connecten unterbrochen wurde, als schon ein write/error selected war */
    return 0;

  if(pConnection->llFDE.ppPreviousNext)
    return ConnectionComplete(pConnection);

  ASSERT(pConnection->nOutBuffer);
  ASSERT(pConnection->nOutBufferUse);
  ASSERT(pConnection->strOutBuffer);

  i = send(pConnection->s,pConnection->strOutBuffer+pConnection->nOutBufferOffset,pConnection->nOutBufferUse,0);
  if(i <= 0)
  {
    if(i == 0 || ERRNO != EWOULDBLOCK)
    {
      ConnectionCloseAsync(pConnection);
      return 0;
    }
    i = 0;
  }
  g_dTotalBytesOut += i;

  if((unsigned int)i >= pConnection->nOutBufferUse)
  {
    pConnection->nOutBufferUse = 0;
    pConnection->nOutBufferOffset = 0;
    pConnection->nOutBuffer = 0;
    FREE(char*,pConnection->strOutBuffer);
    pConnection->strOutBuffer = 0;

    ConnectionFDWClear(pConnection);
  }
  else
  {
    pConnection->nOutBufferUse -= i;
    pConnection->nOutBufferOffset += i;
  }

  return 1;
}

int ConnectionError(PCONNECTION pConnection)
{
  ASSERT(pConnection->cType == CT_SERVER);

  if(pConnection->bClosing)  /* wenn das connecten unterbrochen wurde, als schon ein write/error selected war */
    return 0;

  ConnectionFDRClear(pConnection);
  ConnectionFDEClear(pConnection);
  ConnectionFDWClear(pConnection);

  ConnectionCloseAsync(pConnection);

  return 0;
}

int ConnectionComplete(PCONNECTION pConnection)
{
  ConnectionFDEClear(pConnection);
  ConnectionFDWClear(pConnection);

  ASSERT(pConnection->cType == CT_SERVER);

  if(!ServerRegister(pConnection->pData))
    return 0;

  return 1;
}

int ConnectionsAccept(void)
{
  /* variables */
  SOCKET s;
  struct sockaddr_in sin;

  /* try to accept */
  {
    int val = sizeof(sin);
    if((s = accept(g_sServer,(struct sockaddr *)&sin,&val)) == INVALID_SOCKET)
      return 0;
  }

  g_dConnections++;

  /* set nonblocking and keepalive */
  if( !ConnectionSetNonBlocking(s) ||
    !ConnectionSetKeepAlive(s) )
  {
    closesocket(s);
    return 0;
  }

  /* hard limit reached (i hope this will never happen) */
  if(g_listConnections.nCount+1 >= FD_SETSIZE)
  {
    closesocket(s);
    return 0;
  }

  /* create client struct */
  {
    PCONNECTION pConnection;
    PCLIENT pClient;

    if( !(pClient = ClientCreate()) )
      return 0;

    if(!(pConnection = ConnectionCreate(s,sin.sin_addr.s_addr,ntohs(sin.sin_port),g_timeNow,CT_CLIENT,pClient)))
    {
      ClientFree(pClient);
      closesocket(s);
      return 0; /* out of memory */
    }
    ConnectionFDRSet(pConnection);

    pClient->pConnection = pConnection;

#ifdef DEBUG
    Log("%s accept",iptoa(pConnection->nIP));
#endif /* DEBUG */
    
  }

  return 1;
}
