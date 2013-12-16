/*
         file: admin.c
   desciption: handle admin commands
        begin: 01/05/04
    copyright: (C) 2004 by Colin Graf
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

#include "bounced.h"

/* client handlers */

int ClientHandlerAdmin(PCLIENT pClient, char* strCommand, char* strParams)
{
	char* str;
	
	if(*strParams == ':')
		strParams++;
	str = NextArg(&strParams);

	if(*str)
	{
		if(!strcasecmp(str,"user") )
		{
			str = strCommand+strlen(strCommand);
			*(str++) = ' ';
			strcpy(str,"user");
			return ClientSubhandlerAdminUser(pClient,strCommand,strParams);
		}
		if(!strcasecmp(str,"config") )
		{
			str = strCommand+strlen(strCommand);
			*(str++) = ' ';
			strcpy(str,"config");
			return ClientSubhandlerAdminConfig(pClient,strCommand,strParams);
		}
		if(!strcasecmp(str,"privmsg") )
		{
			str = strCommand+strlen(strCommand);
			*(str++) = ' ';
			strcpy(str,"privmsg");
			return ClientSubhandlerAdminPrivmsg(pClient,strCommand,strParams);
		}
		if(!strcasecmp(str,"notice") )
		{
			str = strCommand+strlen(strCommand);
			*(str++) = ' ';
			strcpy(str,"notice");
			return ClientSubhandlerAdminNotice(pClient,strCommand,strParams);
		}
		if(!strcasecmp(str,"wallops") )
		{
			str = strCommand+strlen(strCommand);
			*(str++) = ' ';
			strcpy(str,"wallops");
			return ClientSubhandlerAdminWallops(pClient,strCommand,strParams);
		}
	}

	if( !ClientMessage(pClient,"Usage is /%s user",strCommand) ||
		!ClientMessage(pClient,"         /%s config",strCommand) ||
		!ClientMessage(pClient,"         /%s privmsg",strCommand) ||
		!ClientMessage(pClient,"         /%s notice",strCommand) ||
		!ClientMessage(pClient,"         /%s wallops",strCommand) )
		return 1;

	return 1;
}

/* admin subhandlers */

int ClientSubhandlerAdminPrivmsg(PCLIENT pClient, char* strCommand, char* strParams)
{
	char* strArg[1];

	if(SplitLine(strParams,strArg,sizeof(strArg)/sizeof(*strArg),1) == sizeof(strArg)/sizeof(*strArg))
	{
		PCLIENT pCli;
		for(pCli = g_listClients.pFirst; pCli; pCli = pCli->ll.pNext)
			ConnectionSendFormat(pCli->pConnection,":%s PRIVMSG %s :%s\r\n",c_strBouncerName,pCli->strNick,strArg[0]);
		return 1;
	}

	if( !ClientMessage(pClient,"Usage is /%s <message>",strCommand) )
		return 1;

	return 1;
}

int ClientSubhandlerAdminNotice(PCLIENT pClient, char* strCommand, char* strParams)
{
	char* strArg[1];

	if(SplitLine(strParams,strArg,sizeof(strArg)/sizeof(*strArg),1) == sizeof(strArg)/sizeof(*strArg))
	{
		PCLIENT pCli;
		for(pCli = g_listClients.pFirst; pCli; pCli = pCli->ll.pNext)
			ConnectionSendFormat(pCli->pConnection,":%s NOTICE %s :%s\r\n",c_strBouncerName,pCli->strNick,strArg[0]);
		return 1;
	}

	if( !ClientMessage(pClient,"Usage is /%s <message>",strCommand) )
		return 1;

	return 1;
}

int ClientSubhandlerAdminWallops(PCLIENT pClient, char* strCommand, char* strParams)
{
	char* strArg[1];

	if(SplitLine(strParams,strArg,sizeof(strArg)/sizeof(*strArg),1) == sizeof(strArg)/sizeof(*strArg))
	{
		PCLIENT pCli;
		for(pCli = g_listClients.pFirst; pCli; pCli = pCli->ll.pNext)
			ConnectionSendFormat(pCli->pConnection,":%s WALLOPS :%s\r\n",c_strBouncerName,strArg[0]);
		return 1;
	}

	if( !ClientMessage(pClient,"Usage is /%s <message>",strCommand) )
		return 1;

	return 1;
}

