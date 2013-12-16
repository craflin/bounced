/*
         file: log.c
   desciption: channel log stuff
        begin: 12/14/03
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
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>

#include "bounced.h"

size_t vstrformat(char* str, size_t maxsize, const char* format, va_list ap);


PROFILEHANDLERUNLOG_TABLE_START

  PROFILEHANDLER("331",ProfileHandlerUnlogNumNoTopic) /* RPL_NOTOPIC */
  PROFILEHANDLER("332",ProfileHandlerUnlogNumTopic) /* RPL_TOPIC */
  PROFILEHANDLER("333",ProfileHandlerUnlogNumTopicSetBy) /* <channel> <setby> <time> */
  PROFILEHANDLER("324",ProfileHandlerUnlogNumChannelModeIs) /* RPL_CHANNELMODEIS */
  PROFILEHANDLER("353",ProfileHandlerUnlogNumNamReply) /* RPL_NAMREPLY */
  PROFILEHANDLER("366",ProfileHandlerUnlogNumEndOfNames) /* RPL_ENDOFNAMES */
  PROFILEHANDLER("329",ProfileHandlerUnlogNumChannelCreateTime) /* ? */

  PROFILEHANDLER("TOPIC",ProfileHandlerUnlogTopic)
  PROFILEHANDLER("MODE",ProfileHandlerUnlogMode)
  PROFILEHANDLER("JOIN",ProfileHandlerUnlogJoin)
  PROFILEHANDLER("PART",ProfileHandlerUnlogPart)
  PROFILEHANDLER("NICK",ProfileHandlerUnlogNick)
  PROFILEHANDLER("KICK",ProfileHandlerUnlogKick)
  PROFILEHANDLER("QUIT",ProfileHandlerUnlogQuit)

PROFILEHANDLERUNLOG_TABLE_END

int ProfileHandlersInit(void)
{
  unsigned int i;

  HASH_INIT(PROFILEHANDLERUNLOG_HASH,g_hashProfileHandlersUnlog);
  for(i = 0; i < PROFILEHANDLERUNLOG_COUNT; i++)
    HASH_ADD(PROFILEHANDLERUNLOG_HASH,g_hashProfileHandlersUnlog,&g_pProfileHandlersUnlog[i]);

  return 1;
}

PPROFILELOGMSG ProfileLogMsgCreateFormat(PPROFILE pProfile, PPROFILECHANNEL pProfileChannel, unsigned int nID, unsigned char cFlags, const char* format, ...)
{
  va_list ap;
  unsigned short sLength;

  va_start (ap, format);
  sLength = vstrformat(g_strOutBuffer,sizeof(g_strOutBuffer),format, ap);
  va_end (ap);

  return ProfileLogMsgCreate(pProfile,pProfileChannel,nID,g_strOutBuffer,sLength,cFlags);
}

PPROFILELOGMSG ProfileLogMsgCreate(PPROFILE pProfile, PPROFILECHANNEL pProfileChannel, unsigned int nID, char* strMsg, unsigned int nMsgLength, unsigned char cFlags)
{
  PPROFILELOGMSG pProfileLogMsg;

  ASSERT(strMsg);

  if( !(pProfileLogMsg = CALLOC(PPROFILELOGMSG,1,sizeof(PROFILELOGMSG))) ||
    !(pProfileLogMsg->strMsg = MALLOC(char*,nMsgLength+1)) )
  {
    OUTOFMEMORY;
    if(pProfileLogMsg)
      ProfileLogMsgFree(pProfileLogMsg);
    return 0;
  }

  memcpy(pProfileLogMsg->strMsg,strMsg,nMsgLength+1);

  if(pProfileChannel)
    pProfile = pProfileChannel->pProfile;
  ASSERT(pProfile);

  pProfileLogMsg->nMsgLength = nMsgLength;
  pProfileLogMsg->pProfileChannel = pProfileChannel;
  pProfileLogMsg->pProfile = pProfile;
  pProfileLogMsg->nID = nID;
  pProfileLogMsg->cFlags = cFlags;
  pProfileLogMsg->timeMsg = g_timeNow;
  
  /* create second link */
  {
    if(pProfile->pLastProfileLogMsg)
      LIST_INSERT_AFTER(PROFILE_PROFILELOGMSG_LIST,pProfile->listProfileLogMsgs,pProfile->pLastProfileLogMsg,pProfileLogMsg)
    else
      LIST_ADD(PROFILE_PROFILELOGMSG_LIST,pProfile->listProfileLogMsgs,pProfileLogMsg);
    pProfile->pLastProfileLogMsg = pProfileLogMsg;
  }

  /* create main link */
  if(pProfileChannel)
  {
    if(pProfileChannel->pLastProfileLogMsg)
      LIST_INSERT_AFTER(PROFILECHANNEL_PROFILELOGMSG_LIST,pProfileChannel->listProfileLogMsgs,pProfileChannel->pLastProfileLogMsg,pProfileLogMsg)
    else
      LIST_ADD(PROFILECHANNEL_PROFILELOGMSG_LIST,pProfileChannel->listProfileLogMsgs,pProfileLogMsg);
    pProfileChannel->pLastProfileLogMsg = pProfileLogMsg;
  }
  else
  {
    if(pProfile->pLastMsgProfileLogMsg)
      LIST_INSERT_AFTER(PROFILE_MSGPROFILELOGMSG_LIST,pProfile->listMsgProfileLogMsgs,pProfile->pLastMsgProfileLogMsg,pProfileLogMsg)
    else
      LIST_ADD(PROFILE_MSGPROFILELOGMSG_LIST,pProfile->listMsgProfileLogMsgs,pProfileLogMsg);
    pProfile->pLastMsgProfileLogMsg = pProfileLogMsg;
  }

  if(cFlags & PLMF_LISTITEM)
  {
    ASSERT(pProfileChannel);
    if(!pProfile->pFirstListProfileLogMsg)
      pProfile->pFirstListProfileLogMsg = pProfileLogMsg;
  }
  else
    pProfile->pFirstListProfileLogMsg = 0;

  /* remove old log msgs if there are to many */
  if(pProfileChannel)
  {
    PPROFILELOGMSG pCurProfileLogMsg = pProfileChannel->listProfileLogMsgs.pFirst;
    unsigned int nMaxMessafes = pProfileChannel->bLog ? pProfile->c_nLogChannelMessages : 0;
    while(pCurProfileLogMsg && (pProfileChannel->listProfileLogMsgs.nCount > nMaxMessafes || pCurProfileLogMsg->cFlags & PLMF_LISTITEM) )
    {
      if(pCurProfileLogMsg == pProfile->pFirstListProfileLogMsg)
        break;
      {
        PPROFILELOGMSG pRemoveProfileLogMsg = pCurProfileLogMsg;
        pCurProfileLogMsg = pCurProfileLogMsg->llProfileChannel.pNext;
        if(pRemoveProfileLogMsg == pProfileLogMsg)
          pProfileLogMsg = 0;
        ProfileLogMsgClose(pRemoveProfileLogMsg);
      }
    }
  }
  else
  {
    PPROFILELOGMSG pCurProfileLogMsg = pProfile->listMsgProfileLogMsgs.pFirst;
    while(pCurProfileLogMsg && pProfile->listMsgProfileLogMsgs.nCount > pProfile->c_nLogPrivateMessages)
    {
      PPROFILELOGMSG pRemoveProfileLogMsg = pCurProfileLogMsg;
      pCurProfileLogMsg = pCurProfileLogMsg->llProfileChannel.pNext;
      if(pRemoveProfileLogMsg == pProfileLogMsg)
          pProfileLogMsg = 0;
      ProfileLogMsgClose(pRemoveProfileLogMsg);
    }
  }

  return pProfileLogMsg;
}

void ProfileLogMsgFree(PPROFILELOGMSG pProfileLogMsg)
{
  PPROFILE pProfile = pProfileLogMsg->pProfile;

  if(pProfileLogMsg->pProfileChannel)
  {
    PPROFILECHANNEL pProfileChannel = pProfileLogMsg->pProfileChannel;
    if(pProfile->pFirstListProfileLogMsg == pProfileLogMsg)
      pProfile->pFirstListProfileLogMsg = 0;
    if(pProfileChannel->pLastProfileLogMsg == pProfileLogMsg)
      pProfileChannel->pLastProfileLogMsg = 0;
  }
  else
  {
    if(pProfile->pLastMsgProfileLogMsg == pProfileLogMsg)
      pProfile->pLastMsgProfileLogMsg = 0;
  }

  LIST_REMOVE_ITEM(PROFILE_PROFILELOGMSG_LIST,pProfile->listProfileLogMsgs,pProfileLogMsg);
  if(pProfile->pLastProfileLogMsg == pProfileLogMsg)
  {
    if(pProfile->listProfileLogMsgs.pFirst)
    {
      if(pProfile->pLastMsgProfileLogMsg && !pProfile->pLastMsgProfileLogMsg->llProfile.pNext)
        pProfile->pLastProfileLogMsg = pProfile->pLastMsgProfileLogMsg;
      else
      {
        PPROFILECHANNEL pProfileChannel;
        for(pProfileChannel = pProfile->hashlistProfileChannels.pFirst; pProfileChannel; pProfileChannel = pProfileChannel->hllProfile.pNext)
          if(pProfileChannel->pLastProfileLogMsg && !pProfileChannel->pLastProfileLogMsg->llProfile.pNext)
          {
            pProfile->pLastProfileLogMsg = pProfileChannel->pLastProfileLogMsg;
            break;
          }
        if(!pProfileChannel)
          pProfile->pLastProfileLogMsg = 0;        
      }
    }
    else
      pProfile->pLastProfileLogMsg = 0;
  }

  if(pProfileLogMsg->strMsg)
    FREE(char*,pProfileLogMsg->strMsg);
  FREE(PPROFILELOGMSG,pProfileLogMsg);
}

int ProfileLogMsgClose(PPROFILELOGMSG pProfileLogMsg)
{
  PPROFILE pProfile = pProfileLogMsg->pProfile;

  ASSERT(pProfileLogMsg->strMsg)

#ifdef DEBUG_PROTOCOL
    Log("%s X %s",pProfileLogMsg->pProfile->pUser->strName,pProfileLogMsg->strMsg);
#endif /* DEBUG_PROTOCOL */

  if( !g_bShutdown && pProfileLogMsg->pProfileChannel )
  {
    char* strCommand = pProfileLogMsg->strMsg;
    char *strAction,*str;

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
        *(str++) = '\0';
        break;
      }
      else if(*str == '\r' || *str == '\n')
      {
        *str = '\0';
        break;
      }
      *(str++) = toupper(*str);
    } while(*str);

    if(isdigit(*strAction))
    {
      char* strNumEnd;

      if(!*str)
        return 1;

      strNumEnd = strchr(str,' ');
      if(strNumEnd)
        str = strNumEnd+1;
      else
        str += strlen(str);
    }

    {
      struct tagPROFILEHANDLER* pHandler;
      HASH_LOOKUP(PROFILEHANDLERUNLOG_HASH,g_hashProfileHandlersUnlog,strAction,pHandler);
      if(pHandler)
      {
        char cInList = pProfile->cInList;

        pHandler->pProc(pProfileLogMsg,strCommand,str);

        if(cInList == pProfile->cInList)
          pProfile->cInList = 0;
      }
    }
  }

  if(pProfileLogMsg->pProfileChannel)
  {
    PPROFILECHANNEL pProfileChannel = pProfileLogMsg->pProfileChannel;
    LIST_REMOVE_ITEM(PROFILECHANNEL_PROFILELOGMSG_LIST,pProfileChannel->listProfileLogMsgs,pProfileLogMsg)
  }
  else
    LIST_REMOVE_ITEM(PROFILE_MSGPROFILELOGMSG_LIST,pProfile->listMsgProfileLogMsgs,pProfileLogMsg);
  return 1;
}

int ProfileLogMsgSend(PPROFILELOGMSG pProfileLogMsg, PCONNECTION pConnection)
{
  ASSERT(pProfileLogMsg->strMsg);
  ASSERT(*pProfileLogMsg->strMsg);

  if(pProfileLogMsg->cFlags & PLMF_TIMESTAMP)
  {
    unsigned int i;
    char* str;
    char* strBuffer = g_strOutBuffer;
    unsigned int nBuffer = sizeof(g_strOutBuffer)-1;

    str = strstr(pProfileLogMsg->strMsg+1," :");
    if(str)
    {
      str++;

      if(!strncmp(str,"\001ACTION ",8))
      {
        str += 8;
        /* pProfileLogMsg->cFlags &= ~PLMF_ADJUSTNICKLEN; */
      }

      i = str-pProfileLogMsg->strMsg;
      if(i > nBuffer)
      {
        if(!ConnectionSend(pConnection,pProfileLogMsg->strMsg,pProfileLogMsg->nMsgLength))
          return 0;
        return 1;
      }
      memcpy(strBuffer,pProfileLogMsg->strMsg,i);
      strBuffer += i;
      nBuffer -= i;

      if(pProfileLogMsg->cFlags & PLMF_ADJUSTNICKLEN && pProfileLogMsg->pProfile->c_bLogChannelAdjustNicklen)
      {
        char* strNickEnd = strpbrk(pProfileLogMsg->strMsg+1,"! ");
        if(strNickEnd && (unsigned int)(strNickEnd-(pProfileLogMsg->strMsg+1)) < pProfileLogMsg->pProfile->nNickLen)
        {
          i = pProfileLogMsg->pProfile->nNickLen-(strNickEnd-(pProfileLogMsg->strMsg+1));
          if(i > nBuffer)
          {
            if(!ConnectionSend(pConnection,pProfileLogMsg->strMsg,pProfileLogMsg->nMsgLength))
              return 0;
            return 1;
          }
          memset(strBuffer,' ',i);
          strBuffer += i;
          nBuffer -= i;
        }
      }

      if(!nBuffer)
      {
        if(!ConnectionSend(pConnection,pProfileLogMsg->strMsg,pProfileLogMsg->nMsgLength))
          return 0;
        return 1;
      }
      i = strftimet(strBuffer,nBuffer,pProfileLogMsg->pProfileChannel ? pProfileLogMsg->pProfile->c_strLogChannelTimestampFormat : pProfileLogMsg->pProfile->c_strLogPrivateTimestampFormat,pProfileLogMsg->timeMsg);
      strBuffer += i;
      nBuffer -= i;

      i = pProfileLogMsg->nMsgLength-(str-pProfileLogMsg->strMsg);
      if(i > nBuffer)
      {
        if(!ConnectionSend(pConnection,pProfileLogMsg->strMsg,pProfileLogMsg->nMsgLength))
          return 0;
        return 1;
      }
      memcpy(strBuffer,str,i);
      strBuffer += i;
      *(strBuffer) = '\0';
    }
    else
    {
      str = pProfileLogMsg->strMsg+pProfileLogMsg->nMsgLength-1;
      while(str > pProfileLogMsg->strMsg && isspace(*str))
        str--;
      str++;

      i = str-pProfileLogMsg->strMsg;
      if(i > nBuffer)
      {
        if(!ConnectionSend(pConnection,pProfileLogMsg->strMsg,pProfileLogMsg->nMsgLength))
          return 0;
        return 1;
      }
      memcpy(strBuffer,pProfileLogMsg->strMsg,i);
      strBuffer += i;
      nBuffer -= i;

      if(2 > nBuffer)
      {
        if(!ConnectionSend(pConnection,pProfileLogMsg->strMsg,pProfileLogMsg->nMsgLength))
          return 0;
        return 1;
      }
      *(strBuffer++) = ' ';
      *(strBuffer++) = ':';
      nBuffer -= 2;

      if(!nBuffer)
      {
        if(!ConnectionSend(pConnection,pProfileLogMsg->strMsg,pProfileLogMsg->nMsgLength))
          return 0;
        return 1;
      }
      i = strftimet(strBuffer,nBuffer,pProfileLogMsg->pProfileChannel ? pProfileLogMsg->pProfile->c_strLogChannelTimestampFormat : pProfileLogMsg->pProfile->c_strLogPrivateTimestampFormat,pProfileLogMsg->timeMsg);
      while(i > 0 && strBuffer[i-1] == ' ')
        i--;
      strBuffer += i;
      nBuffer -= i;

      i = pProfileLogMsg->nMsgLength-(str-pProfileLogMsg->strMsg);
      if(i > nBuffer)
      {
        if(!ConnectionSend(pConnection,pProfileLogMsg->strMsg,pProfileLogMsg->nMsgLength))
          return 0;
        return 1;
      }
      memcpy(strBuffer,str,i);
      strBuffer += i;
      *(strBuffer) = '\0';
    }

    if(!ConnectionSend(pConnection,g_strOutBuffer,strBuffer-g_strOutBuffer))
      return 0;
  }
  else
  {
    if(!ConnectionSend(pConnection,pProfileLogMsg->strMsg,pProfileLogMsg->nMsgLength))
      return 0;
  }
  return 1;
}

/* da die unlog handlers alles nur hooks sind, niemals vorher abbrechen und bis zum ende durchlaufen lassen. dann return 0 */

int ProfileHandlerUnlogNumNoTopic(PPROFILELOGMSG pProfileLogMsg, char* strCommand, char* strParams)
{
  PPROFILECHANNEL pProfileChannel = pProfileLogMsg->pProfileChannel;

  ASSERT(pProfileChannel);

  if(pProfileChannel->strLogTopic)
  {
    FREE(char*,pProfileChannel->strLogTopic);
    pProfileChannel->strLogTopic = 0;
  }
  if(pProfileChannel->strLogTopicSetBy)
  {
    FREE(char*,pProfileChannel->strLogTopicSetBy);
    pProfileChannel->strLogTopicSetBy = 0;
  }
  pProfileChannel->timeLogTopicSetTime = 0;

  return 0;
  strCommand = 0;
  strParams = 0;
}

int ProfileHandlerUnlogNumTopic(PPROFILELOGMSG pProfileLogMsg, char* strCommand, char* strParams)
{
  char* strArg[2];

  if(SplitIRCParams(strParams,strArg,sizeof(strArg)/sizeof(*strArg),1) == sizeof(strArg)/sizeof(*strArg))
  {
    PPROFILECHANNEL pProfileChannel = pProfileLogMsg->pProfileChannel;

    ASSERT(pProfileChannel);

    if(pProfileChannel->strLogTopic)
    {
      FREE(char*,pProfileChannel->strLogTopic);
      pProfileChannel->strLogTopic = 0;
    }
    if(pProfileChannel->strLogTopicSetBy)
    {
      FREE(char*,pProfileChannel->strLogTopicSetBy);
      pProfileChannel->strLogTopicSetBy = 0;
    }
    pProfileChannel->timeLogTopicSetTime = 0;
    pProfileChannel->strLogTopic = STRDUP(char*,strArg[1]);
  }

  return 0;
  strCommand = 0;
}

int ProfileHandlerUnlogNumTopicSetBy(PPROFILELOGMSG pProfileLogMsg, char* strCommand, char* strParams)
{
  char* strArg[3];

  if(SplitIRCParams(strParams,strArg,sizeof(strArg)/sizeof(*strArg),0) == sizeof(strArg)/sizeof(*strArg))
  {
    PPROFILECHANNEL pProfileChannel = pProfileLogMsg->pProfileChannel;

    ASSERT(pProfileChannel);

    if(pProfileChannel->strLogTopicSetBy)
    {
      FREE(char*,pProfileChannel->strLogTopicSetBy);
      pProfileChannel->strLogTopicSetBy = 0;
    }
    pProfileChannel->timeLogTopicSetTime = 0;

    if( (pProfileChannel->strLogTopicSetBy = STRDUP(char*,strArg[1])) )
      pProfileChannel->timeLogTopicSetTime = atoul(strArg[2]);
  }

  return 0;
  strCommand = 0;
}

int ProfileHandlerUnlogNumChannelModeIs(PPROFILELOGMSG pProfileLogMsg, char* strCommand, char* strParams)
{
  char* strArg[1];

  if(SplitIRCParamsPointer(&strParams,strArg,sizeof(strArg)/sizeof(*strArg),0) == sizeof(strArg)/sizeof(*strArg))
  {
    PPROFILECHANNEL pProfileChannel = pProfileLogMsg->pProfileChannel;

    ASSERT(pProfileChannel);

    {
      char* str = strpbrk(strParams,"\r\n");
      if(str)
        *str = '\0';
    }
    if(*strParams == '+')
      strParams++;

    if(pProfileChannel->strLogMode)
      FREE(char*,pProfileChannel->strLogMode);
    pProfileChannel->strLogMode = STRDUP(char*,strParams);
  }

  return 0;
  strCommand = 0;
}

int ProfileHandlerUnlogNumNamReply(PPROFILELOGMSG pProfileLogMsg, char* strCommand, char* strParams)
{
  char* strArg[3];

  if(SplitIRCParams(strParams,strArg,sizeof(strArg)/sizeof(*strArg),1) == sizeof(strArg)/sizeof(*strArg))
  {
    PPROFILECHANNEL pProfileChannel = pProfileLogMsg->pProfileChannel;
    PPROFILE pProfile = pProfileLogMsg->pProfile;
    char* str,*strEnd;
    char cPrefix;

    ASSERT(pProfileChannel);

    if(!pProfile->cInList)
      HASHLIST_REMOVE_ALL(PROFILECHANNEL_PROFILECHANNELUSER_HASHLIST,pProfileChannel->hashlistLogProfileChannelUsers);
    pProfile->cInList++;

    str = strArg[2];
    do
    {
      strEnd = strchr(str,' ');
      if(strEnd)
        *(strEnd++) = '\0';
      if(*str)
      {
        if(IsIRCNickPrefix(*str,pProfile->strPrefix))
        {
          cPrefix = *str;
          str++;
        }
        else
          cPrefix = 0;
        if(*str)
          ProfileChannelUserCreate(pProfileChannel,str,cPrefix,1);
      }
      str = strEnd;
    } while(str);
  }
  
  return 0;
  strCommand = 0;
}

int ProfileHandlerUnlogNumEndOfNames(PPROFILELOGMSG pProfileLogMsg, char* strCommand, char* strParams)
{
  pProfileLogMsg->pProfile->cInList = 0;
  return 0;
  strCommand = 0;
  strParams = 0;
}

int ProfileHandlerUnlogNumChannelCreateTime(PPROFILELOGMSG pProfileLogMsg, char* strCommand, char* strParams)
{
  char* strArg[2];

  if(SplitIRCParams(strParams,strArg,sizeof(strArg)/sizeof(*strArg),0) == sizeof(strArg)/sizeof(*strArg))
  {
    PPROFILECHANNEL pProfileChannel = pProfileLogMsg->pProfileChannel;

    ASSERT(pProfileChannel);

    pProfileChannel->timeLogCreate = atoul(strArg[1]);
  }

  return 0;
  strCommand = 0;
}

int ProfileHandlerUnlogTopic(PPROFILELOGMSG pProfileLogMsg, char* strCommand, char* strParams)
{
  char* strArg[2];
  unsigned int nArgs;

  if((nArgs = SplitIRCParams(strParams,strArg,sizeof(strArg)/sizeof(*strArg),1)) >= 1)
  {
    PPROFILECHANNEL pProfileChannel = pProfileLogMsg->pProfileChannel;

    ASSERT(pProfileChannel);

    if(pProfileChannel->strLogTopic)
    {
      FREE(char*,pProfileChannel->strLogTopic);
      pProfileChannel->strLogTopic = 0;
    }
    if(pProfileChannel->strLogTopicSetBy)
    {
      FREE(char*,pProfileChannel->strLogTopicSetBy);
      pProfileChannel->strLogTopicSetBy = 0;
    }
    pProfileChannel->timeLogTopicSetTime = 0;

    if( nArgs > 1 && 
      (pProfileChannel->strLogTopic = STRDUP(char*,strArg[1])) )
    {
      char* str;

      if( SplitIRCPrefix(pProfileLogMsg->strMsg,&str) &&
        (pProfileChannel->strLogTopicSetBy = STRDUP(char*,str)) )
        pProfileChannel->timeLogTopicSetTime = g_timeNow;
    }
  }

  return 0;
  strCommand = 0;
}

void ProfileHandlerUnlogModeToggleParam(char cAction, char cMode, const char* strArg, PPROFILECHANNEL pProfileChannel)
{
  char cPrefix;
  if(IsIRCNickPrefixMode(cMode,&cPrefix,pProfileChannel->pProfile->strPrefix))
  {
    PPROFILECHANNELUSER pProfileChannelUser;

    HASHLIST_LOOKUP(PROFILECHANNEL_PROFILECHANNELUSER_HASHLIST,pProfileChannel->hashlistLogProfileChannelUsers,strArg,pProfileChannelUser);
    if(pProfileChannelUser)
      ProfileChannelUserUpdatePrefix(pProfileChannelUser,cPrefix,cAction);
  }
}

int ProfileHandlerUnlogMode(PPROFILELOGMSG pProfileLogMsg, char* strCommand, char* strParams)
{
  char* strArg[1];

  if(SplitIRCParamsPointer(&strParams,strArg,sizeof(strArg)/sizeof(*strArg),0) == sizeof(strArg)/sizeof(*strArg))
  {
    PPROFILECHANNEL pProfileChannel = pProfileLogMsg->pProfileChannel;
    PPROFILE pProfile = pProfileLogMsg->pProfile;
    char* strOldParams;
    char strMode[53];
    char* strNewParams = g_strOutBuffer;
    unsigned int nOldMode;

    ASSERT(pProfileChannel);

    if(pProfileChannel->strLogMode)
    {
      nOldMode = strlen(pProfileChannel->strLogMode);
    
      strOldParams = strchr(pProfileChannel->strLogMode,' ');
      if(strOldParams)
      {
        strOldParams++;
        strOldParams[-1] = '\0';
      }
    }
    else
    {
      strOldParams = 0;
      nOldMode = 0;
    }

    if(!UpdateIRCModeParams(pProfileChannel->strLogMode,strOldParams,strParams,strMode,53,strNewParams,sizeof(g_strOutBuffer),pProfile->strPrefix,pProfile->strChanModes,(PTOGGLEPARAMPROC)ProfileHandlerUnlogModeToggleParam,pProfileChannel))
    {
      ASSERT(0);
    }
    else
    {
      unsigned int nMode = strlen(strMode);
      unsigned int nParams = strlen(strNewParams);
      unsigned int nLength = nMode+1+nParams;
      char* str;

      if(nOldMode && nLength <= nOldMode)
        str = pProfileChannel->strLogMode;
      else
      {
        if(pProfileChannel->strLogMode)
        {
          FREE(char*,pProfileChannel->strLogMode);
          pProfileChannel->strLogMode = 0;
        }
        str = MALLOC(char*,nLength+1);
      }

      if(str)
      {
        memcpy(str,strMode,nMode);
        if(nParams)
        {
          str[nMode] = ' ';
          memcpy(&str[nMode+1],strNewParams,nParams);
          str[nMode+1+nParams] = '\0';
        }
        else
          str[nMode] = '\0';
        pProfileChannel->strLogMode = str;
      }
    }
  }

  return 0;
  strCommand = 0;
}

int ProfileHandlerUnlogJoin(PPROFILELOGMSG pProfileLogMsg, char* strCommand, char* strParams)
{
  char* strNick;

  if( SplitIRCPrefixNick(strCommand,&strNick) )
  {
    PPROFILECHANNEL pProfileChannel = pProfileLogMsg->pProfileChannel;

    ASSERT(pProfileChannel);

    ProfileChannelUserCreate(pProfileChannel,strNick,0,1);
  }

  return 0;
  strParams = 0;
}

int ProfileHandlerUnlogPart(PPROFILELOGMSG pProfileLogMsg, char* strCommand, char* strParams)
{
  char* strNick;

  if( SplitIRCPrefixNick(strCommand,&strNick) )
  {
    PPROFILECHANNEL pProfileChannel = pProfileLogMsg->pProfileChannel;
    PPROFILECHANNELUSER pProfileChannelUser;

    ASSERT(pProfileChannel);

    HASHLIST_LOOKUP(PROFILECHANNEL_PROFILECHANNELUSER_HASHLIST,pProfileChannel->hashlistLogProfileChannelUsers,strNick,pProfileChannelUser);
    ASSERT(pProfileChannelUser);
    if(pProfileChannelUser)
      ProfileChannelUserClose(pProfileChannelUser,1);
  }

  return 0;
  strParams = 0;
}

int ProfileHandlerUnlogNick(PPROFILELOGMSG pProfileLogMsg, char* strCommand, char* strParams)
{
  char* strNick;
  char* strArg[1];

  if( SplitIRCParams(strParams,strArg,sizeof(strArg)/sizeof(*strArg),0) == sizeof(strArg)/sizeof(*strArg) &&
    SplitIRCPrefixNick(strCommand,&strNick) )
  {
    PPROFILECHANNEL pProfileChannel = pProfileLogMsg->pProfileChannel;
    PPROFILECHANNELUSER pProfileChannelUser;

    ASSERT(pProfileChannel);

    HASHLIST_LOOKUP(PROFILECHANNEL_PROFILECHANNELUSER_HASHLIST,pProfileChannel->hashlistLogProfileChannelUsers,strNick,pProfileChannelUser);
    /* ASSERT(pProfileChannelUser); */
    if(pProfileChannelUser)
    {
      char* strNewNick = STRDUP(char*,strArg[0]);
      if(strNewNick)
      {
        HASHLIST_REMOVE_ITEM_NOFREE(PROFILECHANNEL_PROFILECHANNELUSER_HASHLIST,pProfileChannel->hashlistLogProfileChannelUsers,pProfileChannelUser);
        if(pProfileChannelUser->strNick)
          FREE(char*,pProfileChannelUser->strNick);
        pProfileChannelUser->strNick = strNewNick;
        HASHLIST_ADD(PROFILECHANNEL_PROFILECHANNELUSER_HASHLIST,pProfileChannel->hashlistLogProfileChannelUsers,pProfileChannelUser);
      }
    }

    if(!strcasecmp(strNick,pProfileChannel->strLogNick))
    {
      char* strNewNick = STRDUP(char*,strArg[0]);
      if(strNewNick)
      {
        if(pProfileChannel->strLogNick)
          FREE(char*,pProfileChannel->strLogNick);
        pProfileChannel->strLogNick = strNewNick;
      }
    }
  }

  return 0;
}

int ProfileHandlerUnlogKick(PPROFILELOGMSG pProfileLogMsg, char* strCommand, char* strParams)
{
  char* strArg[2];
  
  if( SplitIRCParams(strParams,strArg,sizeof(strArg)/sizeof(*strArg),0) == sizeof(strArg)/sizeof(*strArg) )
  {
    PPROFILECHANNEL pProfileChannel = pProfileLogMsg->pProfileChannel;
    PPROFILECHANNELUSER pProfileChannelUser;

    ASSERT(pProfileChannel);

    HASHLIST_LOOKUP(PROFILECHANNEL_PROFILECHANNELUSER_HASHLIST,pProfileChannel->hashlistLogProfileChannelUsers,strArg[1],pProfileChannelUser);
    ASSERT(pProfileChannelUser);
    if(pProfileChannelUser)
      ProfileChannelUserClose(pProfileChannelUser,1);
  }

  return 0;
  strCommand = 0;
}

int ProfileHandlerUnlogQuit(PPROFILELOGMSG pProfileLogMsg, char* strCommand, char* strParams)
{
  char* strNick;

  if( SplitIRCPrefixNick(strCommand,&strNick) )
  {
    PPROFILECHANNEL pProfileChannel = pProfileLogMsg->pProfileChannel;
    PPROFILECHANNELUSER pProfileChannelUser;

    ASSERT(pProfileChannel);

    HASHLIST_LOOKUP(PROFILECHANNEL_PROFILECHANNELUSER_HASHLIST,pProfileChannel->hashlistLogProfileChannelUsers,strNick,pProfileChannelUser);
    ASSERT(pProfileChannelUser);
    if(pProfileChannelUser)
      ProfileChannelUserClose(pProfileChannelUser,1);
  }

  return 0;
  strParams = 0;
}

