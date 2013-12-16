/*
         file: list.h
   desciption: double linked list macros
        begin: 01/19/03
    copyright: (C) 2003 by Colin Graf
        email: addition@users.sourceforge.net

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#ifndef _LIST_H
#define _LIST_H

#define LIST_INITTYPE(LISTINFO) struct tag##LISTINFO \
{ \
  LISTINFO##_ITEM* pFirst; \
  unsigned int nCount; \
}

#define LIST_SIZEOF(LISTINFO) (sizeof(struct tag##LISTINFO))

#define LIST(LISTINFO) struct tag##LISTINFO
#define PLIST(LISTINFO) struct tag##LISTINFO*

#define LISTLINK(LISTINFO) struct \
{ \
  LISTINFO##_ITEM** ppPreviousNext; \
  LISTINFO##_ITEM* pNext; \
} LISTINFO##_LINK

#define LIST_INIT(LISTINFO,listexName) \
{ \
  (listexName).nCount = 0; \
  (listexName).pFirst = 0; \
}

#define LIST_ADD(LISTINFO,listexName,pItem) \
{ \
  if(((pItem)->LISTINFO##_LINK.pNext = (listexName).pFirst)) \
    (listexName).pFirst->LISTINFO##_LINK.ppPreviousNext = &(pItem)->LISTINFO##_LINK.pNext; \
  (pItem)->LISTINFO##_LINK.ppPreviousNext = &(listexName).pFirst; \
  (listexName).pFirst = (pItem); \
  (listexName).nCount++; \
}

#define LIST_INSERT_AFTER(LISTINFO,listexName,pItem,pNewItem) \
{ \
  if(((pNewItem)->LISTINFO##_LINK.pNext = (pItem)->LISTINFO##_LINK.pNext)) \
    (pItem)->LISTINFO##_LINK.pNext->LISTINFO##_LINK.ppPreviousNext = &(pNewItem)->LISTINFO##_LINK.pNext; \
  (pNewItem)->LISTINFO##_LINK.ppPreviousNext = &(pItem)->LISTINFO##_LINK.pNext; \
  (pItem)->LISTINFO##_LINK.pNext = (pNewItem); \
  (listexName).nCount++; \
}
/*
#define LIST_INSERT_BEFORE(LISTINFO,listexName,pItem,pNewItem) \
{ \
  (pNewItem)->LISTINFO##_LINK.pNext = (pItem); \
  (pNewItem)->LISTINFO##_LINK.ppPreviousNext = (pItem)->LISTINFO##_LINK.ppPreviousNext; \
  (pItem)->LISTINFO##_LINK.ppPreviousNext = &(pNewItem)->LISTINFO##_LINK.pNext; \
  *(pNewItem)->LISTINFO##_LINK.ppPreviousNext = (pNewItem);
  (listexName).nCount++; \
}
*/
#define LIST_INSERT(LISTINFO,listexName,ppInsertPreviousNext,pNewItem) \
{ \
  if(((pNewItem)->LISTINFO##_LINK.pNext = *(ppInsertPreviousNext))) \
    (pNewItem)->LISTINFO##_LINK.pNext->LISTINFO##_LINK.ppPreviousNext = &(pNewItem)->LISTINFO##_LINK.pNext; \
  (pNewItem)->LISTINFO##_LINK.ppPreviousNext = (ppInsertPreviousNext); \
  *(ppInsertPreviousNext) = (pNewItem); \
  (listexName).nCount++; \
}

#define LIST_SEARCH_ITEM(LISTINFO,listexName,pItem,pItemResult/*out*/) \
{ \
  for((pItemResult) = (listexName).pFirst; \
    (pItemResult); (pItemResult) = (pItemResult)->LISTINFO##_LINK.pNext) \
    if((pItemResult) == (pItem)) \
      break; \
}

#define LIST_REMOVE_ITEM(LISTINFO,listexName,pItem) \
{ \
  if((*(pItem)->LISTINFO##_LINK.ppPreviousNext = (pItem)->LISTINFO##_LINK.pNext)) \
    (pItem)->LISTINFO##_LINK.pNext->LISTINFO##_LINK.ppPreviousNext = (pItem)->LISTINFO##_LINK.ppPreviousNext; \
  (listexName).nCount--; \
  LISTINFO##_FREEPROC((pItem)); \
}

#define LIST_REMOVE_ITEM_NOFREE(LISTINFO,listexName,pItem) \
{ \
  if((*(pItem)->LISTINFO##_LINK.ppPreviousNext = (pItem)->LISTINFO##_LINK.pNext)) \
    (pItem)->LISTINFO##_LINK.pNext->LISTINFO##_LINK.ppPreviousNext = (pItem)->LISTINFO##_LINK.ppPreviousNext; \
  (listexName).nCount--; \
}

#define LIST_REMOVE_FIRST(LISTINFO,listexName) \
  LIST_REMOVE_ITEM(LISTINFO,listexName,(listexName).pFirst)

#define LIST_REMOVE_FIRST_NOFREE(LISTINFO,listexName) \
  LIST_REMOVE_ITEM_NOFREE(LISTINFO,listexName,(listexName).pFirst)

#define LIST_REMOVE_NEXT(LISTINFO,listexName,pItem) \
  LIST_REMOVE_ITEM(LISTINFO,listexName,(pItem)->LISTINFO##_LINK.pNext)

#define LIST_REMOVE_NEXT_NOFREE(LISTINFO,listexName,pItem) \
  LIST_REMOVE_ITEM_NOFREE(LISTINFO,listexName,(pItem)->LISTINFO##_LINK.pNext)

#define LIST_REMOVE_ALL(LISTINFO,listexName) \
{ \
  LISTINFO##_ITEM* pItem; \
  while((pItem = (listexName).pFirst)) \
  { \
    if(((listexName).pFirst = pItem->LISTINFO##_LINK.pNext)) \
      (listexName).pFirst->LISTINFO##_LINK.ppPreviousNext = &(listexName).pFirst; \
    (listexName).nCount--; \
    LISTINFO##_FREEPROC(pItem); \
  } \
}
/*
#define LIST_REMOVE_ALL_NOFREE(LISTINFO,listexName) \
{ \
  while((listexName).pFirst) \
  { \
    if(((listexName).pFirst = (listexName).pFirst->LISTINFO##_LINK.pNext)) \
      (listexName).pFirst->LISTINFO##_LINK.ppPreviousNext = &(listexName).pFirst; \
    (listexName).nCount--; \
  } \
}
*/
#define LIST_REMOVE_ALL_NOFREE(LISTINFO,listexName) LIST_INIT(LISTINFO,listexName)

#endif /* _LIST_H */
