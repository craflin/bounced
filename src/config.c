/*
         file: config.c
   desciption: load config file
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
#include <ctype.h>

#include "bounced.h"

PTIMER g_pStatsTimer = 0;
PTIMER g_pDumpTimer = 0;

char* c_strAdminPassword;
char* c_strAdminUsername;
char* c_strBouncerName;
char* c_strListenInterface;
unsigned int c_nListenPort;
char* c_strConnectInterface;
unsigned int c_nConnectTimer;
unsigned int c_nConnectMaxTries;
unsigned int c_nStatsTimer;
unsigned int c_nUserMaxClients;
unsigned int c_nUserMaxProfiles;
char* c_strDefaultServers;
char* c_strDefaultMode;
char* c_strDefaultAllowedCommands;
char* c_strDefaultChannels;
char c_bDefaultChannelRejoin;
char* c_strDefaultLogChannels;
char c_bDefaultLogChannelAdjustNicklen;
unsigned int c_nDefaultLogChannelMessages;
char* c_strDefaultLogChannelTimestampFormat;
unsigned int c_nDefaultLogPrivateMessages;
char* c_strDefaultLogPrivateTimestampFormat;
unsigned int c_nLogChannelMaxMessages;
unsigned int c_nLogPrivateMaxMessages;
unsigned int c_nDumpTimer;
char c_bDefaultAway;
char* c_strDefaultAwayDefaultReason;
char* c_strDBAccessPassword;

CONFIGVAR_TABLE_START

	CONFIGVAR("AdminPassword", CV_STR|CV_HIDDEN|CV_NODUMP, c_strAdminPassword, "", 32)
	CONFIGVAR("AdminUsername", CV_STR|CV_NODUMP, c_strAdminUsername, "", 32)

	CONFIGVAR("BouncerName", CV_STR, c_strBouncerName, "Bounced", 64)

	CONFIGVAR("ConnectInterface", CV_STR, c_strConnectInterface, "0.0.0.0", 15)
	CONFIGVAR("ConnectTimer", CV_UINT, c_nConnectTimer, 30, -1) /* 30 seconds */
	CONFIGVAR("ConnectMaxTries", CV_UINT, c_nConnectMaxTries, 100, -1) /* 100 times */

	CONFIGVAR("DBAccessPassword", CV_STR|CV_HIDDEN, c_strDBAccessPassword, "", 32)

	CONFIGVAR("DefaultServers", CV_STR, c_strDefaultServers, "", 256)
	CONFIGVAR("DefaultMode", CV_STR, c_strDefaultMode, "", 52)

	CONFIGVAR("DefaultAllowedCommands", CV_STR, c_strDefaultAllowedCommands, "PROFILE;SERVER;PASSWORD;CONFIG;HELP", sizeof("PROFILE;SERVER;PASSWORD;CONFIG;HELP;ADMIN"))

	CONFIGVAR("DefaultAway", CV_BOOL, c_bDefaultAway, 0, 0)
	CONFIGVAR("DefaultAwayDefaultReason", CV_STR, c_strDefaultAwayDefaultReason, "", 256)

	CONFIGVAR("DefaultChannels", CV_STR, c_strDefaultChannels, "", 256)
	CONFIGVAR("DefaultChannelRejoin", CV_BOOL, c_bDefaultChannelRejoin, 1, 0)

	CONFIGVAR("DefaultLogChannels", CV_STR, c_strDefaultLogChannels, "*", 256)
	CONFIGVAR("DefaultLogChannelAdjustNicklen", CV_BOOL, c_bDefaultLogChannelAdjustNicklen, 1, 0)
	CONFIGVAR("DefaultLogChannelMessages", CV_UINT, c_nDefaultLogChannelMessages, 100, 65535)
	CONFIGVAR("DefaultLogChannelTimestampFormat", CV_STR, c_strDefaultLogChannelTimestampFormat, "[%I:%M%p] ", 32) /* [hh:mmXM]  */
	CONFIGVAR("DefaultLogPrivateMessages", CV_UINT, c_nDefaultLogPrivateMessages, 100, 65535)
	CONFIGVAR("DefaultLogPrivateTimestampFormat", CV_STR, c_strDefaultLogPrivateTimestampFormat, "[%I:%M%p] ", 32) /* [hh:mmXM]  */

	CONFIGVAR("DumpTimer", CV_UINT, c_nDumpTimer, 1800, -1) /* 30 minutes */

	CONFIGVAR("ListenInterface", CV_STR|CV_ONCE, c_strListenInterface, "0.0.0.0", 15)
	CONFIGVAR("ListenPort", CV_UINT|CV_ONCE, c_nListenPort, 6667, 65535)

	CONFIGVAR("LogChannelMaxMessages", CV_UINT, c_nLogChannelMaxMessages, 100, 65535)
	CONFIGVAR("LogPrivateMaxMessages", CV_UINT, c_nLogPrivateMaxMessages, 100, 65535)

	CONFIGVAR("StatsTimer", CV_UINT, c_nStatsTimer, 300, -1) /* 5 minutes */

	CONFIGVAR("UserMaxClients", CV_UINT, c_nUserMaxClients, 3, 256)
	CONFIGVAR("UserMaxProfiles", CV_UINT, c_nUserMaxProfiles, 3, 256)

CONFIGVAR_TABLE_END

int ConfigSetVar(unsigned char bInit, const char* var, const char* val, PCONFIGVAR* ppConfigVar)
{
	PCONFIGVAR pConfigVar;

	ASSERT(var);

	if(!(pConfigVar = ConfigGetVar(var)))
		return -1;
	if(ppConfigVar)
		*ppConfigVar = pConfigVar;
	
	if(pConfigVar->cType & CV_ONCE && !bInit)
		return -2;

	switch(pConfigVar->cType & CV_TYPEMASK)
	{
	case CV_STR:
		if(val)
		{
			if(*(char**)pConfigVar->pVar && !strcmp(*(char**)pConfigVar->pVar,val))
				return 1;
			{
				unsigned int n = strlen(val)+1;
				char* str;
				if(n > pConfigVar->nSize)
					n = pConfigVar->nSize;
				if(!(str = MALLOC(char*,n)))
				{
					OUTOFMEMORY;
					return 0;
				}
				memcpy(str,val,n-1);
				str[n-1] = '\0';
				if(*(char**)pConfigVar->pVar && *(char**)pConfigVar->pVar != (char*)pConfigVar->pDefault)
					FREE(char*,*(char**)pConfigVar->pVar);
				*(char**)pConfigVar->pVar = str;
			}
		}
		else
		{
			if(*(char**)pConfigVar->pVar)
			{
				if(!strcmp(*(char**)pConfigVar->pVar,(char*)pConfigVar->pDefault))
					return 1;
				if(*(char**)pConfigVar->pVar != (char*)pConfigVar->pDefault)
					FREE(char*,*(char**)pConfigVar->pVar);
			}
			*(char**)pConfigVar->pVar = (char*)pConfigVar->pDefault;
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

			if(*(unsigned int*)pConfigVar->pVar == n)
				return 1;

			if(n > pConfigVar->nSize)
				n = pConfigVar->nSize;
			*(unsigned int*)pConfigVar->pVar = n;
		}
		else
		{
			if(*(unsigned int*)pConfigVar->pVar == (unsigned int)pConfigVar->pDefault)
				return 1;

			*(unsigned int*)pConfigVar->pVar = (unsigned int)pConfigVar->pDefault;
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

			if(*(char*)pConfigVar->pVar == n)
				return 1;

			*(char*)pConfigVar->pVar = n;
		}
		else
		{
			if(*(char*)pConfigVar->pVar == (int)pConfigVar->pDefault)
				return 1;

			*(char*)pConfigVar->pVar = (int)pConfigVar->pDefault;
		}
		break;
	}

	g_bConfigChanged = 1;
	return 1;
}

PCONFIGVAR ConfigGetVar(const char* var)
{
	PCONFIGVAR pConfigVar,pEndConfigVar = g_pConfigVars+CONFIGVAR_COUNT;
	ASSERT(var);
	for(pConfigVar = g_pConfigVars; pConfigVar < pEndConfigVar; pConfigVar++)
		if(strcasecmp(pConfigVar->strVarName,var) == 0)
			return pConfigVar;
	return 0;
}

PCONFIGVAR ConfigGetFirstVar(void)
{
	return g_pConfigVars;
}

PCONFIGVAR ConfigGetNextVar(PCONFIGVAR pConfigVar)
{
	ASSERT(pConfigVar);
	pConfigVar++;
	return pConfigVar < (g_pConfigVars+CONFIGVAR_COUNT) ? pConfigVar : 0;
}

void ConfigFree(void)
{
	/* variables */
	PCONFIGVAR pConfigVar,pEndConfigVar = g_pConfigVars+CONFIGVAR_COUNT;

	/* run unload function */
	ConfigOnUnload();

	/* delete current values */
	for(pConfigVar = g_pConfigVars; pConfigVar < pEndConfigVar; pConfigVar++)
	{
		if( (pConfigVar->cType & CV_TYPEMASK) == CV_STR && 
			*(char**)pConfigVar->pVar && *(char**)pConfigVar->pVar != (char*)pConfigVar->pDefault)
				FREE(char*,*(char**)pConfigVar->pVar);
	}
}

int ConfigLoad(unsigned char bInit)
{
	/* variables */
	char strFile[MAX_PATH];
	char* str,*strEnd;
	unsigned int iLine = 0;
	FILE* fp;
	PCONFIGVAR pConfigVar,pEndConfigVar = g_pConfigVars+CONFIGVAR_COUNT;

	/* create file path */
	/* snprintf(strFile,sizeof(strFile),"%s/%s",g_strConfigDir,CONFIGFILE); */
	BuildFilename(g_strConfigDir,CONFIGFILE,strFile,sizeof(strFile));

	/* read config file */
	fp = fopen(strFile,"r");
	if(!fp)
	{
		/* Log */
		Log("error: Couldn't load config from \"%s\"",strFile);
		return 0;
	}

	/* init all config variables */
	if(bInit)
	{
		for(pConfigVar = g_pConfigVars; pConfigVar < pEndConfigVar; pConfigVar++)
			switch(pConfigVar->cType & CV_TYPEMASK)
			{
			case CV_STR:
				*(char**)pConfigVar->pVar = (char*)pConfigVar->pDefault;
				break;
			case CV_UINT:
				*(unsigned int*)pConfigVar->pVar = (unsigned int)pConfigVar->pDefault;
				break;
			case CV_BOOL:
				*(char*)pConfigVar->pVar = (int)pConfigVar->pDefault;
				break;
			}
	}

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
/*		else
		{
			Log("error: Missing config value of \"%s\" (Line %u)",g_strOutBuffer,iLine);
			continue;
		}
*/
		strEnd--;
		if(str != strEnd && *str == '"' && *strEnd == '"' )
		{
			str++;
			*strEnd = '\0';
		}

		switch(ConfigSetVar(bInit,g_strOutBuffer,str,&pConfigVar))
		{
		case -1:
			Log("error: Unknown directive \"%s\" in config file (Line %u)",g_strOutBuffer,iLine);
			break;
		case -3:
		case -4:
			Log("error: Invalid value for \"%s\" in config file (Line %u)",pConfigVar->strVarName,iLine);
			break;
		}
	}

	fclose(fp);

	/* show config values */
	for(pConfigVar = g_pConfigVars; pConfigVar < pEndConfigVar; pConfigVar++)
	{
		if(pConfigVar->cType & CV_HIDDEN)
			Log("%s = (not shown)",pConfigVar->strVarName);
		else
		{
			switch(pConfigVar->cType & CV_TYPEMASK)
			{
			case CV_STR:
				Log("%s = %s",pConfigVar->strVarName,*(char**)pConfigVar->pVar);
				break;
			case CV_UINT:
				Log("%s = %u",pConfigVar->strVarName,*(unsigned int*)pConfigVar->pVar);
				break;
			case CV_BOOL:
				Log("%s = %s",pConfigVar->strVarName,*(char*)pConfigVar->pVar ? "on" : "off");
				break;
			}
		}	
	}

	if(bInit)
		g_bConfigChanged = 0;

	/* init config */
	if(!ConfigOnLoad(bInit))
		return 0;

	/* Log */
	Log("Loaded config from \"%s\"",strFile);
	return 1;
}

int ConfigOnLoad(char bInit)
{
	int iReturn = 1;

	/* set stats timer */
	if(!TimerSet(&g_pStatsTimer,g_timeNow,c_nStatsTimer,-1,StatsTimer,&g_timeNow))
	{
		OUTOFMEMORY;
		iReturn = 0;
	}

	/* set dump timer */
	if(!TimerSet(&g_pDumpTimer,g_timeNow,c_nDumpTimer,-1,ConfigDumpTimer,0))
	{
		OUTOFMEMORY;
		iReturn = 0;
	}

	return iReturn;
	bInit = 0;
}

int ConfigOnUnload(void)
{
	if(g_pStatsTimer)
		TimerFree(g_pStatsTimer);

	return 1;
}

int ConfigDump(void)
{
	if(g_bConfigChanged)
	{
		/* variables */
		PCONFIGVAR pConfigVar,pEndConfigVar = g_pConfigVars+CONFIGVAR_COUNT;
		char strFile[MAX_PATH];
		FILE* fp;

		/* create file path */
		BuildFilename(g_strConfigDir,CONFIGFILE,strFile,sizeof(strFile));

		/* create config file */
		fp = fopen(strFile,"w");
		if(!fp)
		{
			/* Log */
			Log("error: Couldn't save config in \"%s\"",strFile);
			return 0;
		}

		/* dump config values */
		for(pConfigVar = g_pConfigVars; pConfigVar < pEndConfigVar; pConfigVar++)
		{
			if(!(pConfigVar->cType & CV_NODUMP))  
				switch(pConfigVar->cType & CV_TYPEMASK)
				{
				case CV_STR:
					if(*(char**)pConfigVar->pVar != (char*)pConfigVar->pDefault)
						fprintf(fp,"%s \"%s\"\n",pConfigVar->strVarName,*(char**)pConfigVar->pVar);
					break;
				case CV_UINT:
					if(*(unsigned int*)pConfigVar->pVar != (unsigned int)pConfigVar->pDefault)
						fprintf(fp,"%s %u\n",pConfigVar->strVarName,*(unsigned int*)pConfigVar->pVar);
					break;
				case CV_BOOL:
					if(*(char*)pConfigVar->pVar != (int)pConfigVar->pDefault)
						fprintf(fp,"%s %s\n",pConfigVar->strVarName,*(char*)pConfigVar->pVar ? "on" : "off");
					break;
				}
		}

		/* close file */
		fclose(fp);

		/* Log */
		Log("Saved config in \"%s\"",strFile);
		g_bConfigChanged = 0;
	}

	return 1;
}

void ConfigDumpTimer(void* pData)
{
	ConfigDump();
	UsersDump();
	ProfilesDump();
	return;
	pData = 0;
}

char* ConfigFindVar(char* strVal, const char* find)
{
	ASSERT(strVal);
	ASSERT(find);

	if(!strVal || !find)
		return 0;

	{
		char* str;
		unsigned int nFind = strlen(find);
		for(;;)
		{
			str = strcasestr(strVal,find);
			if(!str)
				break;
			if(str == strVal || str[-1] == ';')
			{
				if(str[nFind] == ' ' || str[nFind] == ';' || str[nFind] == '\0')
					return str;
			}

			strVal = strchr(str+nFind,';');
			if(!strVal)
				break;
			strVal++;
		}
	}

	return 0;
}

/* client handlers */

int ClientHandlerConfig(PCLIENT pClient, char* strCommand, char* strParams)
{
	char* strArg[3];
	unsigned int nArgs;

	if(!pClient->pProfile)
	{
		ClientMessage(pClient,"Attach profile first");
		return 1;
	}

	if(*strParams == ':')
		strParams++;

	nArgs = SplitLine(strParams,strArg,sizeof(strArg)/sizeof(*strArg),1);
	if(nArgs >= 1)
	{
		if(!strcasecmp(strArg[0],"set") )
		{
			if(nArgs > 1)
			{	
				PPROFILE pProfile = pClient->pProfile;
				struct tagPROFILECONFIGVAR* pProfileConfigVar = 0;

				if(nArgs <= 2)
				{
					strArg[2] = "";
					nArgs++;
				}

				switch(ProfileConfigSetVar(0,pProfile,strArg[1],strArg[2],&pProfileConfigVar))
				{
				case 0: /* out of memory */
					ConnectionCloseAsync(pClient->pConnection);
					return 1;
				case -1: /* invalid var (strArg[1]) */
					ClientMessage(pClient,"Unknown config directive \"%s\"",strArg[1]);
					return 1;
				case -2:
					ASSERT(pProfileConfigVar);
					ClientMessage(pClient,"Can't edit config value for \"%s\"",pProfileConfigVar->strVarName);
					return 1;
				case -3:
				case -4:
					ASSERT(pProfileConfigVar);
					ClientMessage(pClient,"Invalid config value for \"%s\"",pProfileConfigVar->strVarName);
					return 1;
				}
				ASSERT(pProfileConfigVar);
				ProfileOnLoad(pProfile,0);
				switch(pProfileConfigVar->cType & CV_TYPEMASK)
				{
/*				case CV_STR:
					ClientMessage(pClient,"Config value of \"%s\" is now \"%s\"",pProfileConfigVar->strVarName,*(char**)pProfileConfigVar->pVar);
					return 1;
				case CV_UINT:
					ClientMessage(pClient,"Config value of \"%s\" is now \"%u\"",pProfileConfigVar->strVarName,*(unsigned int*)pProfileConfigVar->pVar);
					return 1;
				case CV_BOOL:
					ClientMessage(pClient,"Config value of \"%s\" is now \"%s\"",pProfileConfigVar->strVarName,*(char*)pProfileConfigVar->pVar ? "on" : "off");
					return 1;
*/				case CV_STR:
					ClientMessage(pClient,"%s = %s",pProfileConfigVar->strVarName,*(char**)pProfileConfigVar->pVar ? *(char**)pProfileConfigVar->pVar : "");
					return 1;
				case CV_UINT:
					ClientMessage(pClient,"%s = %u",pProfileConfigVar->strVarName,*(unsigned int*)pProfileConfigVar->pVar);
					return 1;
				case CV_BOOL:
					ClientMessage(pClient,"%s = %s",pProfileConfigVar->strVarName,*(char*)pProfileConfigVar->pVar ? "on" : "off");
					return 1;
				}
				ASSERT(0);				
			}
		}
		else if(!strcasecmp(strArg[0],"get") ||
			!strcasecmp(strArg[0],"show") ||
			!strcasecmp(strArg[0],"list") )
		{
			if(nArgs > 1)
			{
				PPROFILE pProfile = pClient->pProfile;
				struct tagPROFILECONFIGVAR* pProfileConfigVar;

				if(!(pProfileConfigVar = ProfileConfigGetVar(pProfile,strArg[1])))
				{
					ClientMessage(pClient,"Unknown config directive \"%s\"",strArg[1]);
					return 1;
				}

				ASSERT(pProfileConfigVar);
				switch(pProfileConfigVar->cType & CV_TYPEMASK)
				{
				case CV_STR:
					ClientMessage(pClient,"%s = %s",pProfileConfigVar->strVarName,*(char**)pProfileConfigVar->pVar ? *(char**)pProfileConfigVar->pVar : "");
					return 1;
				case CV_UINT:
					ClientMessage(pClient,"%s = %u",pProfileConfigVar->strVarName,*(unsigned int*)pProfileConfigVar->pVar);
					return 1;
				case CV_BOOL:
					ClientMessage(pClient,"%s = %s",pProfileConfigVar->strVarName,*(char*)pProfileConfigVar->pVar ? "on" : "off");
					return 1;
				}
				ASSERT(0);				
			}
			else
			{
				PPROFILE pProfile = pClient->pProfile;
				struct tagPROFILECONFIGVAR* pProfileConfigVar;

				pProfileConfigVar = ProfileConfigGetFirstVar(pProfile);
				do
				{
					switch(pProfileConfigVar->cType & CV_TYPEMASK)
					{
					case CV_STR:
						if(!ClientMessage(pClient,"%s = %s",pProfileConfigVar->strVarName,*(char**)pProfileConfigVar->pVar ? *(char**)pProfileConfigVar->pVar : ""))
							return 1;
						break;
					case CV_UINT:
						if(!ClientMessage(pClient,"%s = %u",pProfileConfigVar->strVarName,*(unsigned int*)pProfileConfigVar->pVar))
							return 1;
						break;
					case CV_BOOL:
						if(!ClientMessage(pClient,"%s = %s",pProfileConfigVar->strVarName,*(char*)pProfileConfigVar->pVar ? "on" : "off"))
							return 1;
						break;
					}

				} while((pProfileConfigVar = ProfileConfigGetNextVar(pProfile,pProfileConfigVar)));
				
				return 1;
			}
		}
	}

	if( !ClientMessage(pClient,"Usage is /%s set <directive> <value>",strCommand) ||
		!ClientMessage(pClient,"         /%s get [<directive>]",strCommand) )
		return 1;

	return 1;
}

/* admin subhandlers */

int ClientSubhandlerAdminConfig(PCLIENT pClient, char* strCommand, char* strParams)
{
	char* strArg[3];
	unsigned int nArgs;

	nArgs = SplitLine(strParams,strArg,sizeof(strArg)/sizeof(*strArg),1);
	if(nArgs > 0)
	{
		if( !strcasecmp(strArg[0],"get") ||
			!strcasecmp(strArg[0],"show") ||
			!strcasecmp(strArg[0],"list") )
		{
			if(nArgs > 1)
			{
				PCONFIGVAR pConfigVar;

				if(!(pConfigVar = ConfigGetVar(strArg[1])))
				{
					ClientMessage(pClient,"Unknown config directive \"%s\"",strArg[1]);
					return 1;
				}

				ASSERT(pConfigVar);
				if(pConfigVar->cType & CV_HIDDEN)
					ClientMessage(pClient,"%s = (not shown)",pConfigVar->strVarName);
				else
				{
					switch(pConfigVar->cType & CV_TYPEMASK)
					{
					case CV_STR:
						ClientMessage(pClient,"%s = %s",pConfigVar->strVarName,*(char**)pConfigVar->pVar ? *(char**)pConfigVar->pVar : "");
						return 1;
					case CV_UINT:
						ClientMessage(pClient,"%s = %u",pConfigVar->strVarName,*(unsigned int*)pConfigVar->pVar);
						return 1;
					case CV_BOOL:
						ClientMessage(pClient,"%s = %s",pConfigVar->strVarName,*(char*)pConfigVar->pVar ? "on" : "off");
						return 1;
					}
				}
				ASSERT(0);
				return 1;
			}
			else
			{
				PCONFIGVAR pConfigVar,pEndConfigVar = g_pConfigVars+CONFIGVAR_COUNT;

				/* show config values */
				for(pConfigVar = g_pConfigVars; pConfigVar < pEndConfigVar; pConfigVar++)
				{
					if(pConfigVar->cType & CV_HIDDEN)
						ClientMessage(pClient,"%s = (not shown)",pConfigVar->strVarName);
					else
					{
						switch(pConfigVar->cType & CV_TYPEMASK)
						{
						case CV_STR:
							if(!ClientMessage(pClient,"%s = %s",pConfigVar->strVarName,*(char**)pConfigVar->pVar))
								return 1;
							break;
						case CV_UINT:
							if(!ClientMessage(pClient,"%s = %u",pConfigVar->strVarName,*(unsigned int*)pConfigVar->pVar))
								return 1;
							break;
						case CV_BOOL:
							if(!ClientMessage(pClient,"%s = %s",pConfigVar->strVarName,*(char*)pConfigVar->pVar ? "on" : "off"))
								return 1;
							break;
						}
					}	
				}
				return 1;
			}

		}
		else if( !strcasecmp(strArg[0],"set") )
		{
			if(nArgs > 1)
			{
				PCONFIGVAR pConfigVar = 0;

				if(nArgs <= 2)
				{
					strArg[2] = "";
					nArgs++;
				}

				switch(ConfigSetVar(0,strArg[1],strArg[2],&pConfigVar))
				{
				case 0: /* out of memory */
					ConnectionCloseAsync(pClient->pConnection);
					return 1;
				case -1: /* invalid var (strArg[1]) */
					ClientMessage(pClient,"Unknown config directive \"%s\"",strArg[1]);
					return 1;
				case -2:
					ASSERT(pConfigVar);
					ClientMessage(pClient,"Can't edit config value for \"%s\"",pConfigVar->strVarName);
					return 1;
				case -3:
				case -4:
					ASSERT(pConfigVar);
					ClientMessage(pClient,"Invalid config value for \"%s\"",pConfigVar->strVarName);
					return 1;
				}
				ASSERT(pConfigVar);
				ConfigOnLoad(0);
				if(pConfigVar->cType & CV_HIDDEN)
					ClientMessage(pClient,"%s = (not shown)",pConfigVar->strVarName);
				else
				{
					switch(pConfigVar->cType & CV_TYPEMASK)
					{
					case CV_STR:
						ClientMessage(pClient,"%s = %s",pConfigVar->strVarName,*(char**)pConfigVar->pVar);
						return 1;
					case CV_UINT:
						ClientMessage(pClient,"%s = %u",pConfigVar->strVarName,*(unsigned int*)pConfigVar->pVar);
						return 1;
					case CV_BOOL:
						ClientMessage(pClient,"%s = %s",pConfigVar->strVarName,*(char*)pConfigVar->pVar ? "on" : "off");
						return 1;
					}
				}
				ASSERT(0);			
				return 1;
			}
		}
		else if( !strcasecmp(strArg[0],"save") )
		{
			ConfigDump();
			ClientMessage(pClient,"Saved config file");
			return 1;
		}
		else if( !strcasecmp(strArg[0],"reload") )
		{
			ConfigLoad(0);
			MotdLoad();
			ClientMessage(pClient,"Reloaded config and motd file");
			return 1;
		}
	}

	if(	!ClientMessage(pClient,"         /%s get [<directive>]",strCommand) ||
		!ClientMessage(pClient,"         /%s set <directive> <value>",strCommand) ||
		!ClientMessage(pClient,"         /%s save",strCommand) ||
		!ClientMessage(pClient,"         /%s reload",strCommand) )
		return 1;

	return 1;
}
