/*
         file: mode.c
   desciption: mode handlers
        begin: 11/09/03
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
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "bounced.h"

int IsIRCModeInEdit(const char* strEdit, char cMode)
{
  ASSERT(strEdit);
  ASSERT(cMode);

  if(!strEdit || !cMode)
    return 0;

  while(*strEdit)
  {
    if(*strEdit == '+' || *strEdit == '-')
    {
      strEdit++;
      for(; *strEdit && *strEdit != ' '; strEdit++)
        if(*strEdit == cMode)
          return 1;
      if(*strEdit == ' ')
        strEdit++;
    }
    else
    {
      strEdit = strchr(strEdit,' ');
      if(!strEdit)
        break;
      strEdit++;
    }
  }
  return 0;
}

int UpdateIRCMode(const char* strOldMode, const char* strEdit, char* strMode, unsigned int nMode)
{
  char* strM = strMode;
  char* str;
  char cAction = '+';

  ASSERT(strEdit);
  ASSERT(strMode);

  if(!strEdit || !strMode || nMode <= 1)
    return 0;
  nMode--; /* make space for \0 */

  /* copy all modes which are not in strEdit */
  if(strOldMode)
    for(; *strOldMode; strOldMode++)
    {
      if(!IsIRCModeInEdit(strEdit,*strOldMode))
      {
        *(strM++) = *strOldMode;
        if((unsigned int)(strM-strMode) >= nMode)
          break;
      }
    }

  /* add new modes */
  if((unsigned int)(strM-strMode) < nMode)
    while(*strEdit)
    {
      if(*strEdit == '+' || *strEdit == '-')
      {
        for(; *strEdit && *strEdit != ' '; strEdit++)
        {
          if(*strEdit == '+' || *strEdit == '-')
            cAction = *strEdit;
          else if(isalpha(*strEdit))
          {
            if(cAction == '+')
            {
              for(str = strMode; str < strM; str++)
                if(*str == *strEdit)
                  break;
              if(str >= strM)
              {
                *(strM++) = *strEdit;
                if((unsigned int)(strM-strMode) >= nMode)
                  break;
              }
            }
          }
        }
        if(*strEdit == ' ')
          strEdit++;
      }
      else
      {
        strEdit = strchr(strEdit,' ');
        if(!strEdit)
          break;
        strEdit++;
      }
    }

  *strM = '\0';
  return 1;
}

int CompareIRCModes(const char* strOldMode, const char* strMode, char* strAdd, unsigned int nAdd, char* strRemove, unsigned int nRemove)
{
  char* strA = strAdd;
  char* strR = strRemove;
  const char* strM;

  ASSERT(strAdd);
  ASSERT(strRemove);

  if(nAdd <= 1 || nRemove <= 1)
    return 0;
  nAdd--; /* make space for \0 */
  nRemove--; /* make space for \0 */

  if(strOldMode)
    for(strM = strOldMode; *strM; strM++)
      if(!strMode || !strchr(strMode,*strM))
      {
        *(strR++) = *strM;
        if((unsigned int)(strR-strRemove) >= nRemove)
          break;
      }

  if(strMode)
    for(strM = strMode; *strM; strM++)
      if(!strOldMode || !strchr(strOldMode,*strM))
      {
        *(strA++) = *strM;
        if((unsigned int)(strA-strAdd) >= nAdd)
          break;
      }

  *strA = '\0';
  *strR = '\0';
  return 1;
}

int IsIRCNickPrefix(char cPrefix, const char* strPrefix)
{
  ASSERT(cPrefix);
  ASSERT(strPrefix);
  if(!cPrefix || !strPrefix)
    return 0;
  if(*strPrefix == '(')
  {
    strPrefix = strchr(strPrefix,')');
    if(!strPrefix)
      return 0;
    strPrefix++;
  }
  if(strchr(strPrefix,cPrefix))
    return 1;
  return 0;
}

int IsIRCNickPrefixMode(char cMode, char* pcPrefix, const char* strPrefix)
{
  const char *strEnd,*strMode;
  ASSERT(cMode);
  ASSERT(strPrefix);
  ASSERT(pcPrefix);
  if(!cMode || !strPrefix || !pcPrefix)
    return 0;
  if(*strPrefix != '(')
    return 0;
  strPrefix++;
  strEnd = strchr(strPrefix,')');
  if(!strEnd)
    return 0;
  for(strMode = strPrefix; strMode < strEnd; strMode++)
    if(*strMode == cMode)
      break;
  if(strMode >= strEnd)
    return 0;
  strEnd++;
  cMode = strMode-strPrefix;
  if((unsigned char)cMode >= strlen(strEnd))
    return 0;
  *pcPrefix = strEnd[(unsigned int)cMode];
  return 1;
}

int GetIRCModeParamType(char cMode, const char* strPrefix, const char* strChannelModes)
{
  ASSERT(cMode);
  ASSERT(strPrefix);
  ASSERT(strChannelModes);

  if(!cMode || !strPrefix || !strChannelModes)
    return 0;

  if(*strPrefix == '(')
    for(strPrefix++; *strPrefix && *strPrefix != ')'; strPrefix++)
      if(cMode == *strPrefix)
        return 1;

  {
    int iMode;
    for(iMode = 1; *strChannelModes; strChannelModes++)
    {
      if(cMode == *strChannelModes)
        return iMode;
      else if(*strChannelModes == ',')
        iMode++;
    }
  }

  if(isalpha(cMode))
    return 4;

  return 0;
}

int UpdateIRCModeParams(const char* strOldMode, char* strOldParams, const char* strEdit, char* strMode, unsigned int nMode, char* strParams, unsigned int nParams, const char* strPrefix, const char* strChannelModes, PTOGGLEPARAMPROC pToggleParamProc, void* pParam)
{
  char* strM = strMode;
  char* strP = strParams;
  char* str;
  char cAction = '+';
  int iMode;
  int i;
  const char* strEditP;
  char* strEditPEnd = 0;

  ASSERT(strEdit);
  ASSERT(strMode);
  ASSERT(strParams);

  if(!strEdit || !strMode || !strParams)
    return 0;

  if(nMode <= 1 || nParams <= 2)
    return 0;
  nMode--; /* make space for \0 */
  nParams -= 2; /* make space for " \0" */

  /* copy all modes which are not in strEdit */
  if(strOldMode)
    for(; *strOldMode; strOldMode++)
    {
      iMode = GetIRCModeParamType(*strOldMode,strPrefix,strChannelModes);

      if(iMode == 1 || IsIRCModeInEdit(strEdit,*strOldMode))
      {
        if(iMode <= 3)
        {
          if(!strOldParams || !*strOldParams)
            break;
          NextArg(&strOldParams);
        }
      }
      else
      {
        if(iMode <= 3 && (!strOldParams || !*strOldParams))
          break;

        *(strM++) = *strOldMode;

        if(iMode <= 3)
        {
          str = NextArg(&strOldParams);
          i = strlen(str);

          if((unsigned int)(strP-strParams+i) >= nParams)
            i = nParams-(strP-strParams);

          if(strP != strParams)
            *(strP++) = ' ';
          
          memcpy(strP,str,i);
          strP += i;

          if((unsigned int)(strP-strParams) >= nParams)
            break;
        }
        
        if((unsigned int)(strM-strMode) >= nMode)
          break;
      }
    }

  /* add new modes */
  strEditP = strEdit;
  if((unsigned int)(strM-strMode) < nMode && (unsigned int)(strP-strParams) < nParams)
    while(*strEdit)
    {
      if(*strEdit == '+' || *strEdit == '-')
      {
        for(; *strEdit && *strEdit != ' '; strEdit++)
        {
          if(*strEdit == '+' || *strEdit == '-')
            cAction = *strEdit;
          else if(isalpha(*strEdit))
          {
            iMode = GetIRCModeParamType(*strEdit,strPrefix,strChannelModes);

            if( (cAction == '+' && iMode <= 3) || /* find param */
              iMode == 1 || iMode == 2 )
            {
              if(*strEditP)
              {
                char *str;
                do
                {
                  str = strchr(strEditP,' ');
                  if(str)
                    strEditP = str+1;
                } while(str && *strEditP && (*strEditP == '+' || *strEditP == '-'));
                if(!str)
                  strEditP += strlen(strEditP);
                if(*strEditP)
                {
                  strEditPEnd = strpbrk(strEditP," \r\n");
                  if(strEditPEnd)
                    *strEditPEnd = '\0';
                }
              }

              if(pToggleParamProc)
                pToggleParamProc(cAction,*strEdit,strEditP,pParam);
            }

            if(iMode >= 2)
            {
              if(cAction == '+')
              {
                for(str = strMode; str < strM; str++)
                  if(*str == *strEdit)
                    break;

                if(str >= strM)
                {
                  *(strM++) = *strEdit;

                  if(iMode <= 3)
                  {
                    i = strlen(strEditP);

                    if((unsigned int)(strP-strParams+i) >= nParams)
                      i = nParams-(strP-strParams);

                    if(strP != strParams)
                      *(strP++) = ' ';
                    
                    memcpy(strP,strEditP,i);
                    strP += i;

                    if((unsigned int)(strP-strParams) >= nParams)
                      break;
                  }

                  if((unsigned int)(strM-strMode) >= nMode)
                    break;
                }
              }
            }

            if(strEditPEnd)
            {
              *strEditPEnd = ' ';
              strEditPEnd = 0;
            }
          }
        }
        if(*strEdit == ' ')
          strEdit++;
      }
      else
      {
        strEdit = strchr(strEdit,' ');
        if(!strEdit)
          break;
        strEdit++;
      }
    }

  *strP = '\0';
  *strM = '\0';
  return 1;
}

/* server handlers */

void ServerHandlerModeToggleParam(char cAction, char cMode, const char* strArg, PPROFILECHANNEL pProfileChannel)
{
  char cPrefix;
  if(IsIRCNickPrefixMode(cMode,&cPrefix,pProfileChannel->pProfile->strPrefix))
  {
    PPROFILECHANNELUSER pProfileChannelUser;

    HASHLIST_LOOKUP(PROFILECHANNEL_PROFILECHANNELUSER_HASHLIST,pProfileChannel->hashlistProfileChannelUsers,strArg,pProfileChannelUser);
    if(pProfileChannelUser)
      ProfileChannelUserUpdatePrefix(pProfileChannelUser,cPrefix,cAction);
  }
  else if(cMode == 'k')
  {
    PPROFILE pProfile = pProfileChannel->pProfile;

    if(pProfileChannel->strKey)
    {
      FREE(char*,pProfileChannel->strKey);
      pProfileChannel->strKey = 0;
    }
    ProfileConfigRemoveVar(pProfile,pProfile->c_strChannels,pProfileChannel->strName);

    if(cAction == '+')
    {
      strformat(g_strOutBuffer,sizeof(g_strOutBuffer),"%s %s",pProfileChannel->strName,strArg);
      ProfileConfigAddVar(pProfile,"Channels",g_strOutBuffer,0);
      pProfileChannel->strKey = STRDUP(char*,strArg);
    }
  }
}

int ServerHandlerMode(PSERVER pServer, char* strCommand, char* strParams)
{
  if(CompareIRCAddressNick(strParams,pServer->strNick))
  {
    char* strArg[1];
    char* strParams2 = strParams;
    
    if(SplitIRCParamsPointer(&strParams2,strArg,sizeof(strArg)/sizeof(*strArg),0) == sizeof(strArg)/sizeof(*strArg))
    {  
      char *strMode = g_strOutBuffer,*strNewMode,
         strAdd[53],
         strRemove[53];
      PCLIENT pClient;

      if( !UpdateIRCMode(pServer->strMode,strParams2,strMode,53) ||
        !CompareIRCModes(pServer->strMode,strMode,strAdd,sizeof(strAdd),strRemove,sizeof(strRemove)) )
      {
        ConnectionCloseAsync(pServer->pConnection);
        return 1;
      }

      if(!(strNewMode = STRDUP(char*,strMode)))
      {
        OUTOFMEMORY;
        ConnectionCloseAsync(pServer->pConnection);
        return 1; /* out of memory */
      }

      if(pServer->strMode)
        FREE(char*,pServer->strMode);
      pServer->strMode = strNewMode;

      for(pClient = pServer->pProfile->listClients.pFirst; pClient; pClient = pClient->llProfile.pNext)
      {
        if(!(strNewMode = STRDUP(char*,pServer->strMode)))
        {
          OUTOFMEMORY;
          ConnectionCloseAsync(pClient->pConnection);
          continue;
        }

        if( (*strRemove && !ConnectionSendFormat(pClient->pConnection,":%s MODE %s :-%s\r\n",pClient->strNick,pClient->strNick,strRemove)) ||
          (*strAdd    && !ConnectionSendFormat(pClient->pConnection,":%s MODE %s :+%s\r\n",pClient->strNick,pClient->strNick,strAdd   )) )
        {
          FREE(char*,strNewMode);
          continue;
        }

        if(pClient->strMode)
          FREE(char*,pClient->strMode);
        pClient->strMode = strNewMode;
      }

      return 1;
    }
  }

  /* recover strCommand and strParams */
  memcpy(strCommand,g_strCurrentCommand,g_nCurrentCommandLength+1);

  /* hook for log message */
  {
    char* strArg[1];

    if(SplitIRCParamsPointer(&strParams,strArg,sizeof(strArg)/sizeof(*strArg),0) == sizeof(strArg)/sizeof(*strArg))
    {
      PPROFILECHANNEL pProfileChannel;
      PPROFILE pProfile;

      pProfile = pServer->pProfile;
      HASHLIST_LOOKUP(PROFILE_PROFILECHANNEL_HASHLIST,pProfile->hashlistProfileChannels,strArg[0],pProfileChannel);
      if(pProfileChannel)
      {
        char* strOldParams;
        char strMode[53];
        char* strNewParams = g_strOutBuffer;
        unsigned int nOldMode;

        ProfileLogMsgCreate(0,pProfileChannel,++pProfile->nLogID,g_strCurrentCommand,g_nCurrentCommandLength,0);

        if(pProfileChannel->strMode)
        {
          nOldMode = strlen(pProfileChannel->strMode);
        
          strOldParams = strchr(pProfileChannel->strMode,' ');
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

        if(!UpdateIRCModeParams(pProfileChannel->strMode,strOldParams,strParams,strMode,53,strNewParams,sizeof(g_strOutBuffer),pProfile->strPrefix,pProfile->strChanModes,(PTOGGLEPARAMPROC)ServerHandlerModeToggleParam,pProfileChannel))
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
            str = pProfileChannel->strMode;
          else
          {
            if(pProfileChannel->strMode)
            {
              FREE(char*,pProfileChannel->strMode);
              pProfileChannel->strMode = 0;
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
            pProfileChannel->strMode = str;
          }
        }
      }
    }
  }
  
  return 0;
  strCommand = 0;
}

int ServerHandlerNumChannelModeIs(PSERVER pServer, char* strCommand, char* strParams)
{
  /* hook for log message */
  {
    char* strArg[1];

    if(SplitIRCParamsPointer(&strParams,strArg,sizeof(strArg)/sizeof(*strArg),0) == sizeof(strArg)/sizeof(*strArg))
    {
      PPROFILECHANNEL pProfileChannel;
      PPROFILE pProfile;

      pProfile = pServer->pProfile;
      HASHLIST_LOOKUP(PROFILE_PROFILECHANNEL_HASHLIST,pProfile->hashlistProfileChannels,strArg[0],pProfileChannel);
      if(pProfileChannel)
      {
        ProfileLogMsgCreate(0,pProfileChannel,++pProfile->nLogID,g_strCurrentCommand,g_nCurrentCommandLength,0);

        {
          char* str = strpbrk(strParams,"\r\n");
          if(str)
            *str = '\0';
        }
        if(*strParams == '+')
          strParams++;

        if(pProfileChannel->strMode)
          FREE(char*,pProfileChannel->strMode);
        pProfileChannel->strMode = STRDUP(char*,strParams);
      }
    }
  }

  return 0;
  strCommand = 0;
}

int ServerHandlerNumChannelCreateTime(PSERVER pServer, char* strCommand, char* strParams)
{
  /* hook for log message */
  {
    char* strArg[2];

    if(SplitIRCParams(strParams,strArg,sizeof(strArg)/sizeof(*strArg),0) == sizeof(strArg)/sizeof(*strArg))
    {
      PPROFILECHANNEL pProfileChannel;
      PPROFILE pProfile;

      pProfile = pServer->pProfile;
      HASHLIST_LOOKUP(PROFILE_PROFILECHANNEL_HASHLIST,pProfile->hashlistProfileChannels,strArg[0],pProfileChannel);
      if(pProfileChannel)
      {
        ProfileLogMsgCreate(0,pProfileChannel,++pProfile->nLogID,g_strCurrentCommand,g_nCurrentCommandLength,0);

        pProfileChannel->timeCreate = atoul(strArg[1]);
      }
    }
  }

  return 0;
  strCommand = 0;
}

/* client handlers */

int ClientHandlerMode(PCLIENT pClient, char* strCommand, char* strParams)
{
  if(CompareIRCAddressNick(strParams,pClient->strNick))
  {
    char* strArg[1];

    if(SplitIRCParamsPointer(&strParams,strArg,sizeof(strArg)/sizeof(*strArg),0) == sizeof(strArg)/sizeof(*strArg))
    {
      if(*strParams && !isspace(*strParams))
      {
        if(!pClient->pProfile)
        {
          char* strMode = g_strOutBuffer;
          char strAdd[53],strRemove[53];

          if( !UpdateIRCMode(pClient->strMode,strParams,strMode,53) ||
            !CompareIRCModes(pClient->strMode,strMode,strAdd,sizeof(strAdd),strRemove,sizeof(strRemove)) )

          {
            ConnectionCloseAsync(pClient->pConnection);
            return 1;
          }

          if(!(strMode = STRDUP(char*,strMode)))
          {
            OUTOFMEMORY;
            ConnectionCloseAsync(pClient->pConnection);
            return 1;
          }

          if( (*strRemove && !ConnectionSendFormat(pClient->pConnection,":%s MODE %s :-%s\r\n",pClient->strNick,pClient->strNick,strRemove)) ||
            (*strAdd    && !ConnectionSendFormat(pClient->pConnection,":%s MODE %s :+%s\r\n",pClient->strNick,pClient->strNick,strAdd   )) )
          {
            FREE(char*,strMode);
            return 1;
          }

          if(pClient->strMode)
            FREE(char*,pClient->strMode);
          pClient->strMode = strMode;

          return 1;
        }
        else
        {
          PPROFILE pProfile = pClient->pProfile;
          char* strMode = g_strOutBuffer,*strNewMode;
          char strAdd[53],strRemove[53];

          if( !UpdateIRCMode(pProfile->c_strMode,strParams,strMode,53) ||
            !CompareIRCModes(pProfile->c_strMode,strMode,strAdd,sizeof(strAdd),strRemove,sizeof(strRemove)) )
          {
            ConnectionCloseAsync(pClient->pConnection);
            return 1;
          }

          if(ProfileConfigSetVar(1,pProfile,"Mode",strMode,0) <= 0)
          {
            ConnectionCloseAsync(pClient->pConnection);
            return 1;
          }

          if(pProfile->pServer && pProfile->pServer->bRegistered)
            return 0;
          else
          {
            PCLIENT pClient;
            for(pClient = pProfile->listClients.pFirst; pClient; pClient = pClient->llProfile.pNext)
            {
              if(!(strNewMode = STRDUP(char*,pProfile->c_strMode)))
              {
                OUTOFMEMORY;
                ConnectionCloseAsync(pClient->pConnection);
                continue;
              }

              if( (*strRemove && !ConnectionSendFormat(pClient->pConnection,":%s MODE %s :-%s\r\n",pClient->strNick,pClient->strNick,strRemove)) ||
                (*strAdd    && !ConnectionSendFormat(pClient->pConnection,":%s MODE %s :+%s\r\n",pClient->strNick,pClient->strNick,strAdd   )) )
              {
                FREE(char*,strNewMode);
                continue;
              }

              if(pClient->strMode)
                FREE(char*,pClient->strMode);
              pClient->strMode = strNewMode;
            }
            return 1;
          }
        }
      }
    }

    return 0;
  }

  if(pClient->pProfile)
  {
    char* strArg[2];

    if(SplitIRCParams(strParams,strArg,sizeof(strArg)/sizeof(*strArg),0) == 1)
    {
      PPROFILE pProfile = pClient->pProfile;
      PPROFILECHANNEL pProfileChannel;

      HASHLIST_LOOKUP(PROFILE_PROFILECHANNEL_HASHLIST,pProfile->hashlistProfileChannels,strArg[0],pProfileChannel);
      if(pProfileChannel)
      {
        if(pProfileChannel->bSynced && pProfileChannel->strMode)
        {
          if( !ConnectionSendFormat(pClient->pConnection,":%s 324 %s %s +%s\r\n",c_strBouncerName,pClient->strNick,pProfileChannel->strName,pProfileChannel->strMode) ||
            (pProfileChannel->timeCreate && !ConnectionSendFormat(pClient->pConnection,":%s 329 %s %s %u\r\n",c_strBouncerName,pClient->strNick,pProfileChannel->strName,pProfileChannel->timeCreate)) )
          {
            ConnectionCloseAsync(pClient->pConnection);
            return 1;
          }
        }
        return 1;
      }
    }
  }

  return 0;
  strCommand = 0;
}
