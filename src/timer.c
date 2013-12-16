/*
         file: timer.c
   desciption: timer
        begin: Mon Jan 29 17:35 CET 2002
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

#include <stdlib.h>

#include "timer.h"

extern void TimerSchedule(PTIMER pTimer);
extern int TimerUnschedule(PTIMER pTimer);

PTIMER g_pTimersPending = 0;

void TimerSchedule(PTIMER pTimer)
{
    PTIMER* ppSlot;

    if(!pTimer->iEvents)
    return;

    for(ppSlot = &g_pTimersPending; *ppSlot; ppSlot = &(*ppSlot)->pNext)
    {
    if(pTimer->timeNext < (*ppSlot)->timeNext)
      break;
    }
    pTimer->pNext = *ppSlot;
    *ppSlot = pTimer;
}

int TimerUnschedule(PTIMER pTimer)
{
    PTIMER* ppSlot;

    for(ppSlot = &g_pTimersPending; *ppSlot; ppSlot = &(*ppSlot)->pNext)
    {
    if(pTimer == *ppSlot)
    {
      *ppSlot = pTimer->pNext;
      return 1;
    }
    }
  return 0;
}

PTIMER TimerAdd(time_t timeNow, time_t timeInterval, int iEvents, PTIMERPROC pTimerProc, void* pParams)
{
    PTIMER pNew;

    if(!iEvents || !pTimerProc)
    return 0;

    if(!(pNew = malloc(sizeof(TIMER))))
    return 0;
  pNew->iEvents = iEvents;
  pNew->pNext = 0;
  pNew->pParams = pParams;
  pNew->pTimerProc = pTimerProc;
  pNew->timeInterval = timeInterval;
  pNew->timeNext = timeNow+timeInterval;

  TimerSchedule(pNew);

    return pNew;
}

void TimerExec(time_t timeNow)
{
    PTIMER pCurrent;

    while(g_pTimersPending && g_pTimersPending->timeNext <= timeNow)
    {
    pCurrent = g_pTimersPending;
    g_pTimersPending = pCurrent->pNext;
    if(pCurrent->iEvents == 1 || pCurrent->iEvents == 0)
    {
      pCurrent->pTimerProc(pCurrent->pParams);
      free(pCurrent);
    }
    else
    {
      if(pCurrent->iEvents > 0)
        pCurrent->iEvents--;
      pCurrent->timeNext += pCurrent->timeInterval;
      TimerSchedule(pCurrent);
      pCurrent->pTimerProc(pCurrent->pParams); /* do this here because otherwise thet timer proc can't delete this timer with TimerFree() */
    }
    }
}

time_t TimerNext(time_t timeNow, time_t timeDefault)
{
    if(g_pTimersPending)
    {
    if(g_pTimersPending->timeNext <= timeNow)
    {
      TimerExec(timeNow);
        return TimerNext(timeNow,timeDefault);
    }
    return g_pTimersPending->timeNext-timeNow;
  }
  return timeDefault;
}

void TimersFree(void)
{
    PTIMER pTimer;
    while(g_pTimersPending)
    {
    pTimer = g_pTimersPending;
    g_pTimersPending = g_pTimersPending->pNext;
    free(pTimer);
    }
}


int TimerSetInterval(PTIMER pTimer, time_t timeInterval)
{
  if(!pTimer)
    return 0;

  TimerUnschedule(pTimer);
  pTimer->timeNext = pTimer->timeNext-pTimer->timeInterval+timeInterval;
  pTimer->timeInterval = timeInterval;
  TimerSchedule(pTimer);

  return 1;
}

int TimerReset(PTIMER pTimer, time_t timeNow, time_t timeInterval)
{
  if(!pTimer)
    return 0;

  TimerUnschedule(pTimer);
  pTimer->timeNext = timeNow+timeInterval;
  pTimer->timeInterval = timeInterval;
  TimerSchedule(pTimer);

  return 1;
}

int TimerFree(PTIMER pTimer)
{
  if(!pTimer)
    return 0;
  if(!TimerUnschedule(pTimer))
    return 0;
  free(pTimer);
  return 1;
}

int TimerSet(PTIMER* ppTimer, time_t timeNow, time_t timeInterval, int iEvents, PTIMERPROC pTimerProc, void* pParams)
{
  if(!ppTimer || !iEvents || !pTimerProc)
    return 0;
  if(*ppTimer)
  {
    if(timeInterval)
      TimerSetInterval(*ppTimer,timeInterval);
    else
    {
      TimerFree(*ppTimer);
      *ppTimer = 0;
    }
  }
  else if(timeInterval && !(*ppTimer = TimerAdd(timeNow,timeInterval,iEvents,pTimerProc,pParams)))
    return 0;
  return 1;
}
