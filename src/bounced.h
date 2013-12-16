/*
         file: bounced.h
   desciption: main header file
        begin: 11/25/03
    copyright: (C) 2003 by Colin Graf
        email: addition@users.sourceforge.net

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include <time.h>
#if defined(_WIN32) && !defined(__CYGWIN__)
#include <winsock.h>
#else
#include <sys/types.h>
#endif /* defined(_WIN32) && !defined(__CYGWIN__) */

#include "list.h"
#include "hash.h"
#include "hashlist.h"
#include "timer.h"
#include "dmemory.h"

/* debug */
#if defined(_DEBUG) && !defined(DEBUG)
#define DEBUG
#endif /* defined(_DEBUG) && !defined(DEBUG) */
#ifdef DEBUG
#include <assert.h>
#define ASSERT(exp) if(!(exp)) { Log("assert break %s(%d): %s",__FILE__,__LINE__,#exp); assert(exp); }
#define VERIFY(exp) ASSERT(exp)
#else
#define ASSERT(exp)
#define VERIFY(exp) exp
#endif /* _DEBUG */
#define OUTOFMEMORY { Log("fatal error: Out of memory in %s(%d)",__FILE__,__LINE__); ASSERT(0); }

/* win? */
#if defined(_WIN32) && !defined(__CYGWIN__)
#define _WINDOWS
#endif

/* win32 port stuff */
#ifdef _WINDOWS
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define ERRNO WSAGetLastError()
#define EWOULDBLOCK WSAEWOULDBLOCK
#define EINPROGRESS WSAEINPROGRESS
#else
#define ERRNO errno
#define SOCKET int
#define closesocket close
#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif /* !INVALID_SOCKET */
#endif /* _WINDOWS */

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif /* !INADDR_NONE */
#ifndef INADDR_ANY
#define INADDR_ANY 0
#endif /* !INADDR_ANY */
#ifndef INADDR_LOOPBACK
#define INADDR_LOOPBACK         0x7f000001
#endif /* !INADDR_LOOPBACK */

/* some global values (if there is no config.h) */
#ifndef HAVE_CONFIG_H
#define PACKAGE "bounced"
#define VERSION "0.27-dev4"
#define CONFIGFILE "config"
#define LOGFILE "log"
#define PIDFILE "pid"
#define USERSFILE "users"
#define MOTDFILE "motd"
#define INBUFFER (512+2+1)
#define OUTBUFFER (512+2+1+128)
#endif /* HAVE_CONFIG_H */
#ifndef CONFIGDIR
#define CONFIGDIR "config"
#endif /* CONFIGDIR */
#ifndef MAX_PATH
#define MAX_PATH 255
#endif /* MAX_PATH */
#define LOGOLDFILE LOGFILE".old"
#define LOGFILESIZE (500*1024)
#define DEFAULT_PREFIX "(ohv)@%+"
#define DEFAULT_CHANMODES "beI,k,l,imnpsta"
#define DEFAULT_NICKLEN 15
#define USERFLAGS "b" /* valid user flags */

/* PCLIENTHANDLER in g_hashClientHandlers */
#define CLIENTHANDLER_HASH_ITEM struct tagCLIENTHANDLER
#define CLIENTHANDLER_HASH_LINK hl
#define CLIENTHANDLER_HASH_KEY(pItem) pItem->strAction
#define CLIENTHANDLER_HASH_COMPAREPROC(pKey1,pKey2) !strcmp(pKey1,pKey2)
#define CLIENTHANDLER_HASH_CHECKSUMPROC(pKey) strsum(pKey)
#define CLIENTHANDLER_HASH_FREEPROC(pItem)
HASH_INITTYPE(CLIENTHANDLER_HASH,16+1);

/* PCLIENTHANDLER in g_hashClientHandlersUnregistered */
#define CLIENTHANDLERUNREGISTERED_HASH_ITEM struct tagCLIENTHANDLER
#define CLIENTHANDLERUNREGISTERED_HASH_LINK hl
#define CLIENTHANDLERUNREGISTERED_HASH_KEY(pItem) pItem->strAction
#define CLIENTHANDLERUNREGISTERED_HASH_COMPAREPROC(pKey1,pKey2) !strcmp(pKey1,pKey2)
#define CLIENTHANDLERUNREGISTERED_HASH_CHECKSUMPROC(pKey) strsum(pKey)
#define CLIENTHANDLERUNREGISTERED_HASH_FREEPROC(pItem)
HASH_INITTYPE(CLIENTHANDLERUNREGISTERED_HASH,3+1);

/* PSERVERHANDLER in g_hashServerHandlers */
#define SERVERHANDLER_HASH_ITEM struct tagSERVERHANDLER
#define SERVERHANDLER_HASH_LINK hl
#define SERVERHANDLER_HASH_KEY(pItem) pItem->strAction
#define SERVERHANDLER_HASH_COMPAREPROC(pKey1,pKey2) !strcmp(pKey1,pKey2)
#define SERVERHANDLER_HASH_CHECKSUMPROC(pKey) strsum(pKey)
#define SERVERHANDLER_HASH_FREEPROC(pItem)
HASH_INITTYPE(SERVERHANDLER_HASH,16+1);

/* PSERVERHANDLER in g_hashServerHandlersUnregistered */
#define SERVERHANDLERUNREGISTERED_HASH_ITEM struct tagSERVERHANDLER
#define SERVERHANDLERUNREGISTERED_HASH_LINK hl
#define SERVERHANDLERUNREGISTERED_HASH_KEY(pItem) pItem->strAction
#define SERVERHANDLERUNREGISTERED_HASH_COMPAREPROC(pKey1,pKey2) !strcmp(pKey1,pKey2)
#define SERVERHANDLERUNREGISTERED_HASH_CHECKSUMPROC(pKey) strsum(pKey)
#define SERVERHANDLERUNREGISTERED_HASH_FREEPROC(pItem)
HASH_INITTYPE(SERVERHANDLERUNREGISTERED_HASH,8+1);

/* PPROFILEHANDLER in g_hashProfileHandlersUnlog */
#define PROFILEHANDLERUNLOG_HASH_ITEM struct tagPROFILEHANDLER
#define PROFILEHANDLERUNLOG_HASH_LINK hl
#define PROFILEHANDLERUNLOG_HASH_KEY(pItem) pItem->strAction
#define PROFILEHANDLERUNLOG_HASH_COMPAREPROC(pKey1,pKey2) !strcmp(pKey1,pKey2)
#define PROFILEHANDLERUNLOG_HASH_CHECKSUMPROC(pKey) strsum(pKey)
#define PROFILEHANDLERUNLOG_HASH_FREEPROC(pItem)
HASH_INITTYPE(PROFILEHANDLERUNLOG_HASH,4+1);

/* PCONNECTION in g_listConnections */
#define CONNECTION_LIST_ITEM struct tagCONNECTION
#define CONNECTION_LIST_LINK ll
#define CONNECTION_LIST_FREEPROC(pItem) ConnectionFree(pItem)
LIST_INITTYPE(CONNECTION_LIST);

/* PCONNECTION in g_listFDR */
#define FDR_LIST_ITEM struct tagCONNECTION
#define FDR_LIST_LINK llFDR
#define FDR_LIST_FREEPROC(pItem)
LIST_INITTYPE(FDR_LIST);

/* PCONNECTION in g_listFDE */
#define FDW_LIST_ITEM struct tagCONNECTION
#define FDW_LIST_LINK llFDW
#define FDW_LIST_FREEPROC(pItem)
LIST_INITTYPE(FDW_LIST);

/* PCONNECTION in g_listFDE */
#define FDE_LIST_ITEM struct tagCONNECTION
#define FDE_LIST_LINK llFDE
#define FDE_LIST_FREEPROC(pItem)
LIST_INITTYPE(FDE_LIST);

/* PCONNECTION in g_listConnectionAsyncs */
#define CONNECTIONASYNC_LIST_ITEM struct tagCONNECTION
#define CONNECTIONASYNC_LIST_LINK llAsync
#define CONNECTIONASYNC_LIST_FREEPROC(pItem) LIST_REMOVE_ITEM(CONNECTION_LIST,g_listConnections,pItem)
LIST_INITTYPE(CONNECTIONASYNC_LIST);

/* PCLIENT in g_listClients */
#define CLIENT_LIST_ITEM struct tagCLIENT
#define CLIENT_LIST_LINK ll
#define CLIENT_LIST_FREEPROC(pItem) ClientFree(pItem)
LIST_INITTYPE(CLIENT_LIST);

/* PCLIENT in pProfile->listClients */
#define PROFILE_CLIENT_LIST_ITEM struct tagCLIENT
#define PROFILE_CLIENT_LIST_LINK llProfile
#define PROFILE_CLIENT_LIST_FREEPROC(pItem)
LIST_INITTYPE(PROFILE_CLIENT_LIST);

/* PSERVER in g_listServers */
#define SERVER_LIST_ITEM struct tagSERVER
#define SERVER_LIST_LINK ll
#define SERVER_LIST_FREEPROC(pItem) ServerFree(pItem)
LIST_INITTYPE(SERVER_LIST);

/* PPROFILECHANNEL in pProfile->hashlistProfileChannels */
#define PROFILE_PROFILECHANNEL_HASHLIST_ITEM struct tagPROFILECHANNEL
#define PROFILE_PROFILECHANNEL_HASHLIST_LINK hllProfile
#define PROFILE_PROFILECHANNEL_HASHLIST_KEY(pItem) pItem->strName
#define PROFILE_PROFILECHANNEL_HASHLIST_COMPAREPROC(pKey1,pKey2) !strcasecmp(pKey1,pKey2)
#define PROFILE_PROFILECHANNEL_HASHLIST_CHECKSUMPROC(pKey) strcasesum(pKey)
#define PROFILE_PROFILECHANNEL_HASHLIST_FREEPROC(pItem) ProfileChannelFree(pItem)
HASHLIST_INITTYPE(PROFILE_PROFILECHANNEL_HASHLIST,4+1);

/* PPROFILELOGMSG in pProfile->listProfileLogMsgs */
#define PROFILE_PROFILELOGMSG_LIST_ITEM struct tagPROFILELOGMSG
#define PROFILE_PROFILELOGMSG_LIST_LINK llProfile
#define PROFILE_PROFILELOGMSG_LIST_FREEPROC(pItem) 
LIST_INITTYPE(PROFILE_PROFILELOGMSG_LIST);

/* PPROFILELOGMSG in pProfile->listMsgProfileLogMsgs */
#define PROFILE_MSGPROFILELOGMSG_LIST_ITEM struct tagPROFILELOGMSG
#define PROFILE_MSGPROFILELOGMSG_LIST_LINK llMsgProfile
#define PROFILE_MSGPROFILELOGMSG_LIST_FREEPROC(pItem) ProfileLogMsgFree(pItem)
LIST_INITTYPE(PROFILE_MSGPROFILELOGMSG_LIST);

/* PPROFILECHANNELUSER in pProfileChannel->hashlistProfileChannelUsers */
#define PROFILECHANNEL_PROFILECHANNELUSER_HASHLIST_ITEM struct tagPROFILECHANNELUSER
#define PROFILECHANNEL_PROFILECHANNELUSER_HASHLIST_LINK hllProfileChannel
#define PROFILECHANNEL_PROFILECHANNELUSER_HASHLIST_KEY(pItem) pItem->strNick
#define PROFILECHANNEL_PROFILECHANNELUSER_HASHLIST_COMPAREPROC(pKey1,pKey2) !strcasecmp(pKey1,pKey2)
#define PROFILECHANNEL_PROFILECHANNELUSER_HASHLIST_CHECKSUMPROC(pKey) strcasesum(pKey)
#define PROFILECHANNEL_PROFILECHANNELUSER_HASHLIST_FREEPROC(pItem) ProfileChannelUserFree(pItem)
HASHLIST_INITTYPE(PROFILECHANNEL_PROFILECHANNELUSER_HASHLIST,16+1);

/* PPROFILELOGMSG in pProfileChannel->listProfileLogMsgs */
#define PROFILECHANNEL_PROFILELOGMSG_LIST_ITEM struct tagPROFILELOGMSG
#define PROFILECHANNEL_PROFILELOGMSG_LIST_LINK llProfileChannel
#define PROFILECHANNEL_PROFILELOGMSG_LIST_FREEPROC(pItem) ProfileLogMsgFree(pItem)
LIST_INITTYPE(PROFILECHANNEL_PROFILELOGMSG_LIST);

/* PSERVERWELCOMEMSG in pServer->listServerWelcomeMsgs */
#define SERVER_SERVERWELCOMEMSG_LIST_ITEM struct tagSERVERWELCOMEMSG
#define SERVER_SERVERWELCOMEMSG_LIST_LINK llServer
#define SERVER_SERVERWELCOMEMSG_LIST_FREEPROC(pItem) ServerWelcomeMsgFree(pItem)
LIST_INITTYPE(SERVER_SERVERWELCOMEMSG_LIST);

/* PUSER in g_hashUsers */
#define USER_HASHLIST_ITEM struct tagUSER
#define USER_HASHLIST_LINK hll
#define USER_HASHLIST_KEY(pItem) pItem->strName
#define USER_HASHLIST_COMPAREPROC(pKey1,pKey2) !strcasecmp(pKey1,pKey2)
#define USER_HASHLIST_CHECKSUMPROC(pKey) strcasesum(pKey)
#define USER_HASHLIST_FREEPROC(pItem) UserFree(pItem)
HASHLIST_INITTYPE(USER_HASHLIST,4+1);

/* PPROFILE in pUser->hashlistProfiles */
#define USER_PROFILE_HASHLIST_ITEM struct tagPROFILE
#define USER_PROFILE_HASHLIST_LINK hllUser
#define USER_PROFILE_HASHLIST_KEY(pItem) pItem->strName
#define USER_PROFILE_HASHLIST_COMPAREPROC(pKey1,pKey2) !strcasecmp(pKey1,pKey2)
#define USER_PROFILE_HASHLIST_CHECKSUMPROC(pKey) strcasesum(pKey)
#define USER_PROFILE_HASHLIST_FREEPROC(pItem) ProfileFree(pItem)
HASHLIST_INITTYPE(USER_PROFILE_HASHLIST,4+1);

/* PCLIENT in pUser->listClients */
#define USER_CLIENT_LIST_ITEM struct tagCLIENT
#define USER_CLIENT_LIST_LINK llUser
#define USER_CLIENT_LIST_FREEPROC(pItem)
LIST_INITTYPE(USER_CLIENT_LIST);

/* PMOTDVAR in g_listMotdVars */
#define MOTDVAR_LIST_ITEM struct tagMOTDVAR
#define MOTDVAR_LIST_LINK ll
#define MOTDVAR_LIST_FREEPROC(pItem) MotdVarFree(pItem);
LIST_INITTYPE(MOTDVAR_LIST);

/* PMOTDSTR in g_listMotd */
#define MOTD_LIST_ITEM struct tagMOTDSTR
#define MOTD_LIST_LINK ll
#define MOTD_LIST_FREEPROC(pItem) MotdStrFree(pItem)
LIST_INITTYPE(MOTD_LIST);

/* config */
#define CV_STR			0x00
#define CV_UINT			0x01
#define CV_BOOL			0x02
#define CV_TYPEMASK		0x0F
#define	CV_HIDDEN		0x10 /* config.c only */
#define CV_READONLY		0x10 /* profile.c only */
#define	CV_ONCE			0x20 /* config.c only */
#define CV_NODUMP		0x40 /* config.c only */
#define CV_DYNDEFAULT   0x80 /* profile.c only */

#define CONFIGVAR_TABLE_START struct tagCONFIGVAR g_pConfigVars[] = {
#define CONFIGVAR_TABLE_END }; 
#define CONFIGVAR(name,type,var,def,size) {name,type,&var,(void*)(def),size},
#define CONFIGVAR_COUNT (sizeof(g_pConfigVars)/sizeof(struct tagCONFIGVAR))
typedef struct tagCONFIGVAR
{
	char* strVarName;
    unsigned char cType;
    void* pVar; /* (char**), (unsigned int*) or (char*)[bool] */
    void* pDefault;
	unsigned int nSize;
} *PCONFIGVAR;

/* profile config */
#define PROFILECONFIGVAR_TABLE_START PROFILE g_profileConfig; struct tagPROFILECONFIGVAR g_pProfileConfigVars[] = {
#define PROFILECONFIGVAR_TABLE_END }; 
#define PROFILECONFIGVAR(name,type,var,def,size) {name,type,&g_profileConfig.var,0,(void*)(def),size},
#define PROFILECONFIGVAR_COUNT (sizeof(g_pProfileConfigVars)/sizeof(struct tagPROFILECONFIGVAR))
typedef struct tagPROFILECONFIGVAR
{
	char* strVarName;
    unsigned char cType;
	void* pVarPos;
    void* pVar;
	void* pDefault;
	unsigned int nSize;
} *PPROFILECONFIGVAR;

/* client handlers */
extern HASH(CLIENTHANDLER_HASH) g_hashClientHandlers;
#define CLIENTHANDLER_TABLE_START HASH(CLIENTHANDLER_HASH) g_hashClientHandlers; struct tagCLIENTHANDLER g_pClientHandlers[] = {
#define CLIENTHANDLER_TABLE_END }; 
#define CLIENTHANDLER_COUNT (sizeof(g_pClientHandlers)/sizeof(struct tagCLIENTHANDLER))
#define CLIENTHANDLERUNREGISTERED_TABLE_START HASH(CLIENTHANDLERUNREGISTERED_HASH) g_hashClientHandlersUnregistered; struct tagCLIENTHANDLER g_pClientHandlersUnregistered[] = {
#define CLIENTHANDLERUNREGISTERED_TABLE_END };
#define CLIENTHANDLERUNREGISTERED_COUNT (sizeof(g_pClientHandlersUnregistered)/sizeof(struct tagCLIENTHANDLER))
#define CLIENTHANDLER(action,proc,flags) {action,proc,flags,{0}},
typedef int (*PCLIENTHANDLERPROC)(struct tagCLIENT* pClient, char* strCommand, char* strParams);
struct tagCLIENTHANDLER
{
	char* strAction;
    PCLIENTHANDLERPROC pProc;
	unsigned char nFlags;
	HASHLINK(CLIENTHANDLER_HASH);
};

#define CHF_NORMAL		0x00 /* std irc command */
#define CHF_BOUNCER		0x01 /* bouncer extention */
#define CHF_ADMIN		0x02 /* for /admin command */

/* server handlers */
#define SERVERHANDLER_TABLE_START HASH(SERVERHANDLER_HASH) g_hashServerHandlers; struct tagSERVERHANDLER g_pServerHandlers[] = {
#define SERVERHANDLER_TABLE_END }; 
#define SERVERHANDLER_COUNT (sizeof(g_pServerHandlers)/sizeof(struct tagSERVERHANDLER))
#define SERVERHANDLERUNREGISTERED_TABLE_START HASH(SERVERHANDLERUNREGISTERED_HASH) g_hashServerHandlersUnregistered; struct tagSERVERHANDLER g_pServerHandlersUnregistered[] = {
#define SERVERHANDLERUNREGISTERED_TABLE_END }; 
#define SERVERHANDLERUNREGISTERED_COUNT (sizeof(g_pServerHandlersUnregistered)/sizeof(struct tagSERVERHANDLER))
#define SERVERHANDLER(action,proc) {action,proc,{0}},
typedef int (*PSERVERHANDLERPROC)(struct tagSERVER* pServer, char* strCommand, char* strParams);
struct tagSERVERHANDLER
{
	char* strAction;
    PSERVERHANDLERPROC pProc;
	HASHLINK(SERVERHANDLER_HASH);
};

#define PROFILEHANDLERUNLOG_TABLE_START HASH(PROFILEHANDLERUNLOG_HASH) g_hashProfileHandlersUnlog; struct tagPROFILEHANDLER g_pProfileHandlersUnlog[] = {
#define PROFILEHANDLERUNLOG_TABLE_END }; 
#define PROFILEHANDLERUNLOG_COUNT (sizeof(g_pProfileHandlersUnlog)/sizeof(struct tagPROFILEHANDLER))
#define PROFILEHANDLER(action,proc) {action,proc,{0}},
typedef int (*PPROFILEHANDLERPROC)(struct tagPROFILELOGMSG* pProfileLogMsg, char* strAction, char* strParams);
struct tagPROFILEHANDLER
{
	char* strAction;
    PPROFILEHANDLERPROC pProc;
	HASHLINK(PROFILEHANDLERUNLOG_HASH);
};

/* structs */

typedef struct tagCONNECTION
{
	LISTLINK(CONNECTION_LIST);
	LISTLINK(FDR_LIST);
	LISTLINK(FDW_LIST);
	LISTLINK(FDE_LIST);
	LISTLINK(CONNECTIONASYNC_LIST);
	
	char bClosing;

	SOCKET s;
	unsigned int nIP;
	unsigned short nPort;
	time_t timeConnected;

	char* strInBuffer;
	unsigned int nInBuffer;

	char* strOutBuffer;
	unsigned int nOutBuffer;
	unsigned int nOutBufferUse;
	unsigned int nOutBufferOffset;

	unsigned char cType;
	void* pData;
} CONNECTION, *PCONNECTION;

#define CT_CLIENT 1
#define CT_SERVER 2

typedef struct tagCLIENT
{
	LISTLINK(CLIENT_LIST);
	LISTLINK(PROFILE_CLIENT_LIST);
	LISTLINK(USER_CLIENT_LIST);

	struct tagCONNECTION* pConnection;
	struct tagUSER* pUser;
	struct tagPROFILE* pProfile;

	char* strName;
	char pcMD5Pass[16];
	char* strNick;
	char* strRealName;
	char bRegistered:1;
	char bWantPong:1;
	char bAdmin:1;
	char cReserve:5;

	char* strMode;

	unsigned char cMessageMode;

} CLIENT, *PCLIENT;

#define CMM_NOTICE 0
#define CMM_PRIVMSG 1
#define CMM_DEFAULT CMM_NOTICE

typedef struct tagSERVER
{
	LISTLINK(SERVER_LIST);

	struct tagCONNECTION* pConnection;
	struct tagPROFILE* pProfile;

	char* strPassword;
	char* strServer;
	unsigned short nPort;
	char cInList;
	char bConnected:1;
	char bRegistered:1;
	char cReserve:6;

	LIST(SERVER_SERVERWELCOMEMSG_LIST) listServerWelcomeMsgs;
	struct tagSERVERWELCOMEMSG* pLastServerWelcomeMsg;

	char* strNick;
	char* strMode;

} SERVER, *PSERVER;

typedef struct tagPROFILECHANNEL
{
	HASHLISTLINK(PROFILE_PROFILECHANNEL_HASHLIST);
	struct tagPROFILE* pProfile;
	char* strName;
	char* strKey;
	char bSyncing:1;
	char bSynced:1;
	char bLog:1;
	char cReserve:5;

	HASHLIST(PROFILECHANNEL_PROFILECHANNELUSER_HASHLIST) hashlistProfileChannelUsers;
	char* strMode;
	time_t timeCreate;
	char* strTopic;
	char* strTopicSetBy;
	time_t timeTopicSetTime;
	
	HASHLIST(PROFILECHANNEL_PROFILECHANNELUSER_HASHLIST) hashlistLogProfileChannelUsers;
	char* strLogMode;
	time_t timeLogCreate;
	char* strLogTopic;
	char* strLogTopicSetBy;
	time_t timeLogTopicSetTime;
	char* strLogNick;

	LIST(PROFILECHANNEL_PROFILELOGMSG_LIST) listProfileLogMsgs;
	struct tagPROFILELOGMSG* pLastProfileLogMsg;

} PROFILECHANNEL, *PPROFILECHANNEL;

typedef struct tagPROFILECHANNELUSER
{
	HASHLISTLINK(PROFILECHANNEL_PROFILECHANNELUSER_HASHLIST);
	struct tagPROFILECHANNEL* pProfileChannel;
	char* strNick;
	char cPrefix;
#ifdef DEBUG
	char bLog;
#endif
} PROFILECHANNELUSER, *PPROFILECHANNELUSER;

typedef struct tagPROFILELOGMSG
{
	LISTLINK(PROFILE_PROFILELOGMSG_LIST);

	LISTLINK(PROFILECHANNEL_PROFILELOGMSG_LIST);
	struct tagPROFILECHANNEL* pProfileChannel;

	LISTLINK(PROFILE_MSGPROFILELOGMSG_LIST);
	struct tagPROFILE* pProfile;

	unsigned int nID;
	char* strMsg;
	unsigned int nMsgLength;
	time_t timeMsg;

	unsigned char cFlags;

} PROFILELOGMSG, *PPROFILELOGMSG;

#define PLMF_LISTITEM		0x01
#define PLMF_TIMESTAMP		0x02
#define PLMF_ADJUSTNICKLEN	0x04

typedef struct tagSERVERWELCOMEMSG
{
	LISTLINK(SERVER_SERVERWELCOMEMSG_LIST);
	unsigned short nCode;
	char* strMsg;
} SERVERWELCOMEMSG, *PSERVERWELCOMEMSG;

typedef struct tagUSER
{
	HASHLISTLINK(USER_HASHLIST);

	char* strName;
	char pcMD5Pass[16];
	char* strEmail;
	time_t timeRegistered;
	time_t timeLastSeen;

	char bConfigChanged:1;
	char cReserve:7;
	char strFlags[sizeof(USERFLAGS)];
	char* strAllowedCommands;
	char* strConnectInterface; /* ConnectInterface overwrite */

	HASHLIST(USER_PROFILE_HASHLIST) hashlistProfiles;
	LIST(USER_CLIENT_LIST) listClients;
} USER, *PUSER;

typedef struct tagPROFILE
{
	HASHLISTLINK(USER_PROFILE_HASHLIST);

	char* strName;
	struct tagUSER* pUser;
	struct tagSERVER* pServer;
	char cInList;
	unsigned int nLogID;
	char* strPrefix;
	char* strChanModes;
	unsigned int nNickLen;

	int iLastConnectServer;
	unsigned int nConnectTry;
	PTIMER pConnectTimer;
	char* strNick; /* like pClient->strNick */
	char* strAwayReason; /* detach reason */

	char* c_strNick; /* prefered nick */
	char* c_strMode;
	char* c_strAlternativeNick;
	char* c_strDetachNick;
	char* c_strRealName;
	char* c_strServers;
	char c_bChannelRejoin;
	char* c_strChannels;
	char* c_strLogChannels;
	char c_bLogChannelAdjustNicklen;
	unsigned int c_nLogChannelMessages;
	char* c_strLogChannelTimestampFormat;
	unsigned int c_nLogPrivateMessages;
	char* c_strLogPrivateTimestampFormat;
	char c_bAway;
	char* c_strAwayDefaultReason;
	char* c_strPerform;

	LIST(PROFILE_CLIENT_LIST) listClients;
	HASHLIST(PROFILE_PROFILECHANNEL_HASHLIST) hashlistProfileChannels;

	LIST(PROFILE_PROFILELOGMSG_LIST) listProfileLogMsgs;
	struct tagPROFILELOGMSG* pLastProfileLogMsg;
	struct tagPROFILELOGMSG* pFirstListProfileLogMsg;

	LIST(PROFILE_MSGPROFILELOGMSG_LIST) listMsgProfileLogMsgs;
	struct tagPROFILELOGMSG* pLastMsgProfileLogMsg;

} PROFILE, *PPROFILE;

/* motd */
typedef void (*PMOTDVARPROC) (struct tagMOTDVAR*, struct tagCLIENT*);

typedef struct tagMOTDVAR
{
	LISTLINK(MOTDVAR_LIST);
	char cName;
    PMOTDVARPROC pMotdVarProc;
    void* pVar;
	unsigned char cType;
} *PMOTDVAR;

#define MOTDV_NORMAL	0x00
#define MOTDV_REFRESH	0x01
#define MOTDV_POINTER	0x02

#define MOTDVAR_TABLE_START static struct tagMOTDVAR g_pMotdVars[] = {
#define MOTDVAR_TABLE_END };
#define MOTDVAR(name,def,var,type) {{0,0},name,def,var,type},

typedef struct tagMOTDSTR
{
	LISTLINK(MOTD_LIST);
	int i; /* length */ /* -1 = str is variable; else str should be deleteted */
	void* p; /* 0 = line end */ /* if i == -1 p is PMOTDVAR */
} MOTDSTR, *PMOTDSTR;

/* global variables */
extern char g_strVersion[sizeof(PACKAGE)+1+sizeof(VERSION)+1];
extern char g_strSystem[32];
extern time_t g_timeNow;
extern time_t g_timeLastStats;
extern time_t g_timeStart;
extern char* g_strConfigDir;
extern char g_strOutBuffer[OUTBUFFER];
extern char g_strInBuffer[INBUFFER];
extern char g_strInBufferCopy[INBUFFER];
extern char* g_strCurrentCommand;
extern unsigned int g_nCurrentCommandLength;
#ifndef _WINDOWS
extern int g_nfds;
#else
#define g_nfds 0
#endif /* _WINDOWS */
extern fd_set g_fdsR;
extern fd_set g_fdsW;
extern fd_set g_fdsE;
extern SOCKET g_sServer;
extern char g_bShutdown;
extern char g_bUsersChanged;
extern char g_bConfigChanged;
extern char* g_strDefaultPrefix;
extern char* g_strDefaultChanModes;
#ifndef _WINDOWS
extern unsigned char g_bBackground;
#endif

extern double g_dConnections; /* there have been %.0f connections to this server */
extern double g_dLogins;
extern double g_dTotalBytesIn;
extern double g_dTotalBytesOut;
extern double g_dLastConnections;
extern double g_dLastLogins;
extern double g_dLastTotalBytesIn;
extern double g_dLastTotalBytesOut;
extern unsigned int g_nLastIncomingTraffic;
extern unsigned int g_nLastOutgoingTraffic;

extern char g_DebugHasMotdCopied;/* 4 debug */
extern LIST(MOTD_LIST) g_DebuglistMotd; /* 4 debug */
extern MOTDSTR g_DebugMotdStrFirst;/* 4 debug */

extern LIST(CONNECTION_LIST) g_listConnections;
extern LIST(CONNECTIONASYNC_LIST) g_listConnectionAsyncs;
extern LIST(FDR_LIST) g_listFDR;
extern LIST(FDW_LIST) g_listFDW;
extern LIST(FDE_LIST) g_listFDE;
extern LIST(CLIENT_LIST) g_listClients;
extern LIST(SERVER_LIST) g_listServers;
extern HASHLIST(USER_HASHLIST) g_hashlistUsers;
extern LIST(MOTD_LIST) g_listMotd; /* motd text elements, item = PMOTDSTR */
extern LIST(MOTDVAR_LIST) g_listMotdVars; /* list of update procs to keep motd up-to-date, PMOTDVAR */

/* config variables */
extern char* c_strAdminPassword;
extern char* c_strAdminUsername;
extern char* c_strBouncerName;
extern char* c_strListenInterface;
extern unsigned int c_nListenPort;
extern char* c_strConnectInterface;
extern unsigned int c_nConnectTimer;
extern unsigned int c_nConnectMaxTries;
extern unsigned int c_nStatsTimer;
extern unsigned int c_nUserMaxClients;
extern unsigned int c_nUserMaxProfiles;
extern char* c_strDefaultServers;
extern char* c_strDefaultMode;
extern char* c_strDefaultAllowedCommands;
extern char* c_strDefaultChannels;
extern char c_bDefaultChannelRejoin;
extern char* c_strDefaultLogChannels;
extern char c_bDefaultLogChannelAdjustNicklen;
extern unsigned int c_nDefaultLogChannelMessages;
extern char* c_strDefaultLogChannelTimestampFormat;
extern unsigned int c_nDefaultLogPrivateMessages;
extern char* c_strDefaultLogPrivateTimestampFormat;
extern unsigned int c_nLogChannelMaxMessages;
extern unsigned int c_nLogPrivateMaxMessages;
extern unsigned int c_nDumpTimer;
extern char c_bDefaultAway;
extern char* c_strDefaultAwayDefaultReason;
extern char* c_strDBAccessPassword;


/* functions */

int Args(int argc, char* argv[]);
void StatsTimer(void* data);
void SignalHandler(int iSig);
int Init(void);
void Cleanup(void);

int ConfigSetVar(unsigned char bInit, const char* var, const char* val, PCONFIGVAR* ppConfigVar);
PCONFIGVAR ConfigGetVar(const char* var);
PCONFIGVAR ConfigGetFirstVar(void);
PCONFIGVAR ConfigGetNextVar(PCONFIGVAR pConfigVar);
int ConfigLoad(unsigned char bInit);
void ConfigFree(void);
int ConfigDump(void);
int ConfigOnLoad(char bInit);
int ConfigOnUnload(void);
char* ConfigFindVar(char* strVal, const char* find);
void ConfigDumpTimer(void* pData);

int ConnectionSetNonBlocking(SOCKET s);
int ConnectionSetKeepAlive(SOCKET s);
int ConnectionSetReuseAddress(SOCKET s);
int ConnectionSetInterface(SOCKET s, unsigned short sPort, unsigned int nInterfaceIP);
void ConnectionFDRSet(PCONNECTION pConnection);
void ConnectionFDRClear(PCONNECTION pConnection);
void ConnectionFDWSet(PCONNECTION pConnection);
void ConnectionFDWClear(PCONNECTION pConnection);
void ConnectionFDESet(PCONNECTION pConnection);
void ConnectionFDEClear(PCONNECTION pConnection);

PCONNECTION ConnectionCreate(SOCKET s, unsigned int nIP, unsigned short sPort, time_t timeConnected, unsigned char cType, void* pData);
void ConnectionFree(PCONNECTION pConnection);
void ConnectionCloseAsync(PCONNECTION pConnection);
void ConnectionsClose(void);
int ConnectionConnect(PCONNECTION pConnection, unsigned int nIP, unsigned short sPort, unsigned int nInterfaceIP);
int ConnectionSend(PCONNECTION pConnection, const char* strBuffer, int iLength);
int ConnectionSendFormat(PCONNECTION pConnection, const char* format,...);
int ConnectionRead(PCONNECTION pConnection);
int ConnectionWrite(PCONNECTION pConnection);
int ConnectionError(PCONNECTION pConnection);
int ConnectionComplete(PCONNECTION pConnection);
int ConnectionsAccept(void);

int ClientHandlersInit(void);
PCLIENT ClientCreate(void);
void ClientFree(PCLIENT pClient);
void ClientClose(PCLIENT pClient);
int ClientRegister(PCLIENT pClient);
int ClientMessage(PCLIENT pClient, const char* format,...);
int ClientHandleCommand(PCLIENT pClient, char* strCommand, unsigned int nLength);

int ClientHandlerUnregisteredPass(PCLIENT pClient, char* strCommand, char* strParams);
int ClientHandlerUnregisteredUser(PCLIENT pClient, char* strCommand, char* strParams);
int ClientHandlerUnregisteredNick(PCLIENT pClient, char* strCommand, char* strParams);
int ClientHandlerUnregisteredDBAccess(PCLIENT pClient, char* strCommand, char* strParams);

int ClientHandlerPass(PCLIENT pClient, char* strCommand, char* strParams);
int ClientHandlerUser(PCLIENT pClient, char* strCommand, char* strParams);
int ClientHandlerNick(PCLIENT pClient, char* strCommand, char* strParams);
int ClientHandlerQuit(PCLIENT pClient, char* strCommand, char* strParams);
int ClientHandlerProfile(PCLIENT pClient, char* strCommand, char* strParams);
int ClientHandlerServer(PCLIENT pClient, char* strCommand, char* strParams);
int ClientHandlerHelp(PCLIENT pClient, char* strCommand, char* strParams);
int ClientHandlerNotice(PCLIENT pClient, char* strCommand, char* strParams);
int ClientHandlerPrivmsg(PCLIENT pClient, char* strCommand, char* strParams);
int ClientHandlerPing(PCLIENT pClient, char* strCommand, char* strParams);
int ClientHandlerPong(PCLIENT pClient, char* strCommand, char* strParams);
int ClientHandlerJoin(PCLIENT pClient, char* strCommand, char* strParams);
int ClientHandlerPart(PCLIENT pClient, char* strCommand, char* strParams);
int ClientHandlerMode(PCLIENT pClient, char* strCommand, char* strParams);
int ClientHandlerConfig(PCLIENT pClient, char* strCommand, char* strParams);
int ClientHandlerMode(PCLIENT pClient, char* strCommand, char* strParams);
int ClientHandlerAdmin(PCLIENT pClient, char* strCommand, char* strParams);
int ClientHandlerPassword(PCLIENT pClient, char* strCommand, char* strParams);

int ClientSubhandlerAdminUser(PCLIENT pClient, char* strCommand, char* strParams);
int ClientSubhandlerAdminConfig(PCLIENT pClient, char* strCommand, char* strParams);
int ClientSubhandlerAdminPrivmsg(PCLIENT pClient, char* strCommand, char* strParams);
int ClientSubhandlerAdminNotice(PCLIENT pClient, char* strCommand, char* strParams);
int ClientSubhandlerAdminWallops(PCLIENT pClient, char* strCommand, char* strParams);

int ServerHandlersInit(void);
PSERVER ServerCreate(PPROFILE pProfile, const char* strServer, unsigned short sPort, const char* strNick, const char* strPassword);
void ServerFree(PSERVER pServer);
void ServerClose(PSERVER pServer);
int ServerConnect(PSERVER pServer, PCONNECTION pConnection);
int ServerDisconnect(PSERVER pServer);
int ServerRegister(PSERVER pServer); /* login */
int ServerRegistered(PSERVER pServer, char* strCommand); /* loggedin */
PSERVERWELCOMEMSG ServerWelcomeMsgCreate(PSERVER pServer, unsigned int nCode, const char* strMsg);
void ServerWelcomeMsgFree(PSERVERWELCOMEMSG pServerWelcomeMsg);
int ServerHandleCommand(PSERVER pServer, char* strCommand, unsigned int nLength);

int ServerHandlerUnregisteredNickError(PSERVER pServer, char* strCommand, char* strParams);
int ServerHandlerUnregisteredPing(PSERVER pServer, char* strCommand, char* strParams);

int ServerHandlerNick(PSERVER pServer, char* strCommand, char* strParams);
int ServerHandlerJoin(PSERVER pServer, char* strCommand, char* strParams);
int ServerHandlerPart(PSERVER pServer, char* strCommand, char* strParams);
int ServerHandlerKick(PSERVER pServer, char* strCommand, char* strParams);
int ServerHandlerPing(PSERVER pServer, char* strCommand, char* strParams);
int ServerHandlerWelcomeMsg(PSERVER pServer, char* strCommand, char* strParams);
int ServerHandlerMotd(PSERVER pServer, char* strCommand, char* strParams);
void ServerHandlerModeToggleParam(char cAction, char cMode, const char* strArg, PPROFILECHANNEL pProfileChannel);
int ServerHandlerMode(PSERVER pServer, char* strCommand, char* strParams);
int ServerHandlerTopic(PSERVER pServer, char* strCommand, char* strParams);
int ServerHandlerTopicInfo(PSERVER pServer, char* strCommand, char* strParams);
int ServerHandlerTopicChange(PSERVER pServer, char* strCommand, char* strParams);
int ServerHandlerJoinError(PSERVER pServer, char* strCommand, char* strParams);
int ServerHandlerPrivmsg(PSERVER pServer, char* strCommand, char* strParams);
int ServerHandlerNotice(PSERVER pServer, char* strCommand, char* strParams);
int ServerHandlerQuit(PSERVER pServer, char* strCommand, char* strParams);
int ServerHandlerTopic(PSERVER pServer, char* strCommand, char* strParams);
int ServerHandlerNumNoTopic(PSERVER pServer, char* strCommand, char* strParams);
int ServerHandlerNumTopic(PSERVER pServer, char* strCommand, char* strParams);
int ServerHandlerNumTopicSetBy(PSERVER pServer, char* strCommand, char* strParams);
int ServerHandlerNumNamReply(PSERVER pServer, char* strCommand, char* strParams);
int ServerHandlerNumEndOfNames(PSERVER pServer, char* strCommand, char* strParams);
int ServerHandlerNumISupport(PSERVER pServer, char* strCommand, char* strParams);
int ServerHandlerNumChannelModeIs(PSERVER pServer, char* strCommand, char* strParams);
int ServerHandlerNumChannelCreateTime(PSERVER pServer, char* strCommand, char* strParams);

PPROFILECHANNEL ProfileChannelCreate(PPROFILE pProfile, const char* strName, const char* strKey);
void ProfileChannelFree(PPROFILECHANNEL pProfileChannel);
void ProfileChannelClose(PPROFILECHANNEL pProfileChannel);
PPROFILECHANNELUSER ProfileChannelUserCreate(PPROFILECHANNEL pProfileChannel, char* strNick, char cPrefix, char bLog);
void ProfileChannelUserFree(PPROFILECHANNELUSER pProfileChannelUser);
void ProfileChannelUserClose(PPROFILECHANNELUSER pProfileChannelUser, char bLog);
int ProfileChannelUserUpdatePrefix(PPROFILECHANNELUSER pProfileChannelUser, char cPrefix, char cAction);
int ProfileChannelAttach(PPROFILECHANNEL pProfileChannel, PCLIENT pClient);

PPROFILELOGMSG ProfileLogMsgCreate(PPROFILE pProfile, PPROFILECHANNEL pProfileChannel, unsigned int nID, char* strMsg, unsigned int nMsgLength, unsigned char cFlags);
PPROFILELOGMSG ProfileLogMsgCreateFormat(PPROFILE pProfile, PPROFILECHANNEL pProfileChannel, unsigned int nID, unsigned char cFlags, const char* format, ...);
void ProfileLogMsgFree(PPROFILELOGMSG pProfileLogMsg);
int ProfileLogMsgClose(PPROFILELOGMSG pProfileLogMsg);
int ProfileLogMsgSend(PPROFILELOGMSG pProfileLogMsg, PCONNECTION pConnection);

int UserIsNameValid(const char* strName);
PUSER UserCreate(const char* strName, const char* strPass, const char* strEmail, const char* strFlags, const char* strAllowedCommands, const char* strConnectInterface, time_t timeRegistered, time_t timeLastSeen);
void UserFree(PUSER pUser);
void UserClose(PUSER pUser);
int UsersLoad(void);
int UsersOnLoad(void);
int UsersDump(void);
int UserAddFlag(PUSER pUser, char cFlag);
int UserHasFlag(PUSER pUser, char cFlag);
int UserSetFlags(PUSER pUser, const char* strFlags);
int UserAttach(PUSER pUser, PCLIENT pClient);
int UserDetach(PUSER pUser, PCLIENT pClient);
int UserSetAllowedCommands(PUSER pUser, const char* strCommands); /* 0 = out of memory, 1 = success */
int UserAllowCommand(PUSER pUser, const char* strCommand);
int UserDisallowCommand(PUSER pUser, const char* strCommand);

int ProfileIsNameValid(const char* strName);
PPROFILE ProfileCreate(PUSER pUser, const char* strName, const char* strNick, const char* strRealName, const char* strMode);
void ProfileFree(PPROFILE pProfile);
void ProfileClose(PPROFILE pProfile, const char* strDetachMessage);
int ProfileAttach(PPROFILE pProfile, PCLIENT pClient, const char* strMessage);
int ProfileDetach(PPROFILE pProfile, PCLIENT pClient, const char* strMessage);
void ProfileMessage(PPROFILE pProfile, const char* format,...);
void ProfileConnect(PPROFILE pProfile, char* strServer);
void ProfileDisconnect(PPROFILE pProfile, const char* strMessage);
void ProfileConnectTimer(PPROFILE pProfile);
void ProfileSend(PPROFILE pProfile, const char* strBuffer, int iLength);
void ProfileSendFormat(PPROFILE pProfile, const char* format,...);
void ProfileSendFormatAndLog(PPROFILE pProfile, PPROFILECHANNEL pProfileChannel, char cLogFlags, const char* format,...);
int ProfileConfigSetVar(char bInit, PPROFILE pProfile, const char* var, const char* val, PPROFILECONFIGVAR* ppProfileConfigVar);
PPROFILECONFIGVAR ProfileConfigGetVar(PPROFILE pProfile, const char* var);
PPROFILECONFIGVAR ProfileConfigGetFirstVar(PPROFILE pProfile);
PPROFILECONFIGVAR ProfileConfigGetNextVar(PPROFILE pProfile, PPROFILECONFIGVAR pProfileConfigVar);
int ProfileConfigAddVar(PPROFILE pProfile, const char* var, const char* add, PPROFILECONFIGVAR* ppProfileConfigVar);
int ProfileConfigRemoveVar(PPROFILE pProfile, char* val, const char* remove);
int ProfilesDump(void);
int ProfilesLoad(char bInit);
int ProfileOnLoad(PPROFILE pProfile, char bInit);

int ProfileHandlersInit(void);
int ProfileHandlerUnlogNumNoTopic(PPROFILELOGMSG pProfileLogMsg, char* strCommand, char* strParams);
int ProfileHandlerUnlogNumTopic(PPROFILELOGMSG pProfileLogMsg, char* strCommand, char* strParams);
int ProfileHandlerUnlogNumTopicSetBy(PPROFILELOGMSG pProfileLogMsg, char* strCommand, char* strParams);
int ProfileHandlerUnlogNumChannelModeIs(PPROFILELOGMSG pProfileLogMsg, char* strCommand, char* strParams);
int ProfileHandlerUnlogNumNamReply(PPROFILELOGMSG pProfileLogMsg, char* strCommand, char* strParams);
int ProfileHandlerUnlogNumEndOfNames(PPROFILELOGMSG pProfileLogMsg, char* strCommand, char* strParams);
int ProfileHandlerUnlogNumChannelCreateTime(PPROFILELOGMSG pProfileLogMsg, char* strCommand, char* strParams);
int ProfileHandlerUnlogTopic(PPROFILELOGMSG pProfileLogMsg, char* strCommand, char* strParams);
void ProfileHandlerUnlogModeToggleParam(char cAction, char cMode, const char* strArg, PPROFILECHANNEL pProfileChannel);
int ProfileHandlerUnlogMode(PPROFILELOGMSG pProfileLogMsg, char* strCommand, char* strParams);
int ProfileHandlerUnlogJoin(PPROFILELOGMSG pProfileLogMsg, char* strCommand, char* strParams);
int ProfileHandlerUnlogPart(PPROFILELOGMSG pProfileLogMsg, char* strCommand, char* strParams);
int ProfileHandlerUnlogNick(PPROFILELOGMSG pProfileLogMsg, char* strCommand, char* strParams);
int ProfileHandlerUnlogKick(PPROFILELOGMSG pProfileLogMsg, char* strCommand, char* strParams);
int ProfileHandlerUnlogQuit(PPROFILELOGMSG pProfileLogMsg, char* strCommand, char* strParams);

int PasswordCreate(const char* str, char* pcMD5);
int PasswordStrTo(char* str, char* pcMD5);
int PasswordToStr(const char* pcMD5, char* strOut, unsigned int nOutSize);
int PasswordCompare(const char* pcMD5Pass, const char* pcMD5PassTest);
int PasswordIsValid(const char* str);

void MotdSetCurrentTime(PMOTDVAR pMotdVar, PCLIENT pClient);
void MotdSetTotalConnections(PMOTDVAR pMotdVar, PCLIENT pClient);
void MotdSetIP(PMOTDVAR pMotdVar, PCLIENT pClient);
void MotdSetTotalLogins(PMOTDVAR pMotdVar, PCLIENT pClient);
void MotdSetNick(PMOTDVAR pMotdVar, PCLIENT pClient);
void MotdSetUsername(PMOTDVAR pMotdVar, PCLIENT pClient);
void MotdSetUsers(PMOTDVAR pMotdVar, PCLIENT pClient);
void MotdSetTimeStarted(PMOTDVAR pMotdVar, PCLIENT pClient);
void MotdSetUptime(PMOTDVAR pMotdVar, PCLIENT pClient);
void MotdSetIncomingTraffic(PMOTDVAR pMotdVar, PCLIENT pClient);
void MotdSetOutgoingTraffic(PMOTDVAR pMotdVar, PCLIENT pClient);
void MotdSetTraffic(PMOTDVAR pMotdVar, PCLIENT pClient);
void MotdSetRealName(PMOTDVAR pMotdVar, PCLIENT pClient);
void MotdSetEmail(PMOTDVAR pMotdVar, PCLIENT pClient);
void MotdSetConnections(PMOTDVAR pMotdVar, PCLIENT pClient);
void MotdSetServers(PMOTDVAR pMotdVar, PCLIENT pClient);
void MotdSetClients(PMOTDVAR pMotdVar, PCLIENT pClient);
void MotdSetTotalIncomingBytes(PMOTDVAR pMotdVar, PCLIENT pClient);
void MotdSetTotalOutgoingBytes(PMOTDVAR pMotdVar, PCLIENT pClient);
void MotdSetTotalBytes(PMOTDVAR pMotdVar, PCLIENT pClient);

int MotdSend(PCLIENT pClient);
PMOTDSTR MotdStrCreate(PMOTDSTR** pppMotdStr, void* p, int i);
void MotdStrFree(PMOTDSTR pMotdStr);
void MotdVarFree(PMOTDVAR pMotdVar);
int MotdLoad(void);
void MotdFree(void);

#ifndef _WINDOWS
int LogOpenFile(void);
int LogReopenFile(void);
#endif /* !_WINDOWS */
int Log(const char* format, ...);
int LogMultiStart(const char* format, ...);
int LogMultiEnd(const char* str);
char* iptoa(unsigned int ip);
size_t strftimet(char *strDest, size_t maxsize, const char *format, time_t timet);
size_t strftimespan(char *strDest, size_t maxsize, const char *format, const struct tm *timeptr);
size_t strftimetspan(char *strDest, size_t maxsize, const char *format, time_t timet);
unsigned int strcasesum(const char* str);
unsigned int SplitLine(char *pkt, char **template, unsigned int templatecount, char bPseudoTrailing);
/* unsigned int SplitLinePointer(char **ppkt, char **template, unsigned int templatecount); */
char* NextArg(char **pkt);
unsigned int strsum(const char* str);
int SplitIRCParams(char *pkt, char **template, unsigned int templatecount, char bAllowTrailing);
int CompareIRCPrefixNick(const char* strCommand, const char* strNick);
int CompareIRCAddressNick(const char* strParams, const char* strNick);
unsigned int SolveHostname(const char* host);
unsigned int SolveIP(const char* ip);
int BuildFilename(const char* strPath, const char* strFile, char* strOut, unsigned int nSize);
unsigned long atoul(const char* str);
char* strcasestr(const char* str, const char* sea);
int NickIsValid(char* strNick, unsigned int nMaxNicklen);
int SplitIRCPrefixNick(char* strCommand, char** strNick);
int SplitIRCPrefix(char* strCommand, char** strPrefix);
int SplitIRCParamsPointer(char **ppkt, char **template, unsigned int templatecount, char bAllowTrailing);
int IsIRCModeInEdit(const char* strEdit, char cMode);
int UpdateIRCMode(const char* strOldMode, const char* strEdit, char* strMode, unsigned int nMode);
int CompareIRCModes(const char* strOldMode, const char* strMode, char* strAdd, unsigned int nAdd, char* strRemove, unsigned int nRemove);
int IsIRCNickPrefix(char cPrefix, const char* strPrefix);
int IsIRCNickPrefixMode(char cMode, char* pcPrefix, const char* strPrefix);
int GetIRCModeParamType(char cMode, const char* strPrefix, const char* strChannelModes);
typedef void (*PTOGGLEPARAMPROC)(char cAction, char cMode, const char* strArg, void* pParam);
int UpdateIRCModeParams(const char* strOldMode, char* strOldParams, const char* strEdit, char* strMode, unsigned int nMode, char* strParams, unsigned int nParams, const char* strPrefix, const char* strChannelModes, PTOGGLEPARAMPROC pToggleParamProc, void* pParam);
size_t strformat(char* str, size_t maxsize, const char* format, ...);





