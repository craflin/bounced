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

#ifndef _TIMER_H
#define _TIMER_H

#include <time.h>

typedef void (*PTIMERPROC) (void *);

typedef struct tagTIMER
{
  struct tagTIMER * pNext;
  PTIMERPROC pTimerProc;
  void* pParams;
  time_t timeNext;
  time_t timeInterval;
  int iEvents;
} TIMER, *PTIMER;

extern PTIMER TimerAdd(time_t timeNow, time_t timeInterval, int iEvents, PTIMERPROC pTimerProc, void* pParams);
extern void TimerExec(time_t timeNow);
extern time_t TimerNext(time_t timeNow, time_t timeDefault);
extern void TimersFree(void);
extern int TimerSetInterval(PTIMER pTimer, time_t timeInterval);
extern int TimerReset(PTIMER pTimer, time_t timeNow, time_t timeInterval);
extern int TimerFree(PTIMER pTimer);
extern int TimerSet(PTIMER* ppTimer, time_t timeNow, time_t timeInterval, int iEvents, PTIMERPROC pTimerProc, void* pParams);

#endif /* _TIMER_H */
