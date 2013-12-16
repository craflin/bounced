/*
         file: _hashlist.h
   desciption: double linked hashlist macros
        begin: 01/20/03
    copyright: (C) 2003 by Colin Graf
        email: addition@users.sourceforge.net

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#ifndef _HASHLIST_H
#define _HASHLIST_H

#define HASHLIST_INITTYPE(HASHLISTINFO,nSize) struct tag##HASHLISTINFO \
{ \
	HASHLISTINFO##_ITEM* ppHash[nSize]; \
	HASHLISTINFO##_ITEM* pFirst; \
	unsigned int nCount; \
}

#define HASHLIST_SIZEOF(HASHLISTINFO) (sizeof(struct tag##HASHLISTINFO))

#define HASHLIST(HASHLISTINFO) struct tag##HASHLISTINFO
#define PHASHLIST(HASHLISTINFO) struct tag##HASHLISTINFO*

#define HASHLISTLINK(HASHLISTINFO) struct \
{ \
	HASHLISTINFO##_ITEM** ppPreviousNext; \
	HASHLISTINFO##_ITEM* pNext; \
	HASHLISTINFO##_ITEM** ppRow; \
} HASHLISTINFO##_LINK

#define HASHLIST_SIZE(HASHLISTINFO,hashlistName) (sizeof((hashlistName).ppHash)/sizeof(*(hashlistName).ppHash))

#define HASHLIST_INIT(HASHLISTINFO,hashlistName) \
{ \
	(hashlistName).pFirst = 0; \
	(hashlistName).nCount = 0; \
	memset((hashlistName).ppHash,0,sizeof((hashlistName).ppHash)); \
}

#define HASHLIST_ADD(HASHLISTINFO,hashlistName,pItem) \
{ \
	(pItem)->HASHLISTINFO##_LINK.ppRow = \
		&(hashlistName).ppHash[(HASHLISTINFO##_CHECKSUMPROC((HASHLISTINFO##_KEY((pItem))))) % HASHLIST_SIZE(HASHLISTINFO,hashlistName)]; \
	if(((pItem)->HASHLISTINFO##_LINK.pNext = *(pItem)->HASHLISTINFO##_LINK.ppRow)) \
	{ \
		(pItem)->HASHLISTINFO##_LINK.ppPreviousNext = (pItem)->HASHLISTINFO##_LINK.pNext->HASHLISTINFO##_LINK.ppPreviousNext; \
		*(pItem)->HASHLISTINFO##_LINK.ppPreviousNext = (pItem); \
		(pItem)->HASHLISTINFO##_LINK.pNext->HASHLISTINFO##_LINK.ppPreviousNext = &(pItem)->HASHLISTINFO##_LINK.pNext; \
	} \
	else \
	{ \
		if(((pItem)->HASHLISTINFO##_LINK.pNext = (hashlistName).pFirst)) \
			(hashlistName).pFirst->HASHLISTINFO##_LINK.ppPreviousNext = &(pItem)->HASHLISTINFO##_LINK.pNext; \
		(hashlistName).pFirst = (pItem); \
		(pItem)->HASHLISTINFO##_LINK.ppPreviousNext = &(hashlistName).pFirst; \
	} \
	*(pItem)->HASHLISTINFO##_LINK.ppRow = (pItem); \
	(hashlistName).nCount++; \
}

#define HASHLIST_SEARCH_ITEM(HASHLISTINFO,hashlistName,pItem,pItemResult/*out*/) \
{ \
	HASHLISTINFO##_ITEM** ppRow = \
		&(hashlistName).ppHash[(HASHLISTINFO##_CHECKSUMPROC((HASHLISTINFO##_KEY((pItem))))) % HASHLIST_SIZE(HASHLISTINFO,hashlistName)]; \
	for((pItemResult) = *ppRow; (pItemResult) && (pItemResult)->HASHLISTINFO##_LINK.ppRow == ppRow; (pItemResult) = (pItemResult)->HASHLISTINFO##_LINK.pNext) \
		if((pItemResult) == (pItem)) \
			break; \
	if((pItemResult) && (pItemResult)->HASHLISTINFO##_LINK.ppRow != ppRow) \
		(pItemResult) = 0; \
}

#define HASHLIST_LOOKUP(HASHLISTINFO,hashlistName,pKey,pItemResult/*out*/) \
{ \
	HASHLISTINFO##_ITEM** ppRow = \
		&(hashlistName).ppHash[(HASHLISTINFO##_CHECKSUMPROC((pKey))) % HASHLIST_SIZE(HASHLISTINFO,hashlistName)]; \
	for((pItemResult) = *ppRow; (pItemResult) && (pItemResult)->HASHLISTINFO##_LINK.ppRow == ppRow; (pItemResult) = (pItemResult)->HASHLISTINFO##_LINK.pNext) \
		if(HASHLISTINFO##_COMPAREPROC((pKey),(HASHLISTINFO##_KEY((pItemResult))))) \
			break; \
	if((pItemResult) && (pItemResult)->HASHLISTINFO##_LINK.ppRow != ppRow) \
		(pItemResult) = 0; \
}

#define HASHLIST_REMOVE_ITEM(HASHLISTINFO,hashlistName,pItem) \
{ \
	if((*(pItem)->HASHLISTINFO##_LINK.ppPreviousNext = (pItem)->HASHLISTINFO##_LINK.pNext)) \
	{ \
		(pItem)->HASHLISTINFO##_LINK.pNext->HASHLISTINFO##_LINK.ppPreviousNext = (pItem)->HASHLISTINFO##_LINK.ppPreviousNext; \
		if(*(pItem)->HASHLISTINFO##_LINK.ppRow == (pItem)) \
			*(pItem)->HASHLISTINFO##_LINK.ppRow = (pItem)->HASHLISTINFO##_LINK.pNext->HASHLISTINFO##_LINK.ppRow == (pItem)->HASHLISTINFO##_LINK.ppRow ? (pItem)->HASHLISTINFO##_LINK.pNext : 0; \
	} \
	else if(*(pItem)->HASHLISTINFO##_LINK.ppRow == (pItem)) \
		*(pItem)->HASHLISTINFO##_LINK.ppRow = 0; \
	(hashlistName).nCount--; \
	HASHLISTINFO##_FREEPROC((pItem)); \
}

#define HASHLIST_REMOVE_ITEM_NOFREE(HASHLISTINFO,hashlistName,pItem) \
{ \
	if((*(pItem)->HASHLISTINFO##_LINK.ppPreviousNext = (pItem)->HASHLISTINFO##_LINK.pNext)) \
	{ \
		(pItem)->HASHLISTINFO##_LINK.pNext->HASHLISTINFO##_LINK.ppPreviousNext = (pItem)->HASHLISTINFO##_LINK.ppPreviousNext; \
		if(*(pItem)->HASHLISTINFO##_LINK.ppRow == (pItem)) \
			*(pItem)->HASHLISTINFO##_LINK.ppRow = (pItem)->HASHLISTINFO##_LINK.pNext->HASHLISTINFO##_LINK.ppRow == (pItem)->HASHLISTINFO##_LINK.ppRow ? (pItem)->HASHLISTINFO##_LINK.pNext : 0; \
	} \
	else if(*(pItem)->HASHLISTINFO##_LINK.ppRow == (pItem)) \
		*(pItem)->HASHLISTINFO##_LINK.ppRow = 0; \
	(hashlistName).nCount--; \
}

#define HASHLIST_REMOVE(HASHLISTINFO,hashlistName,pKey) \
{ \
	HASHLISTINFO##_ITEM *pItem,**ppRow = \
		&(hashlistName).ppHash[(HASHLISTINFO##_CHECKSUMPROC((pKey))) % HASHLIST_SIZE(HASHLISTINFO,hashlistName)]; \
	for(pItem = *ppRow; pItem && pItem->HASHLISTINFO##_LINK.ppRow == ppRow; pItem = pItem->HASHLISTINFO##_LINK.pNext) \
		if(HASHLISTINFO##_COMPAREPROC((pKey),(HASHLISTINFO##_KEY(pItem)))) \
		{ \
			if((*pItem->HASHLISTINFO##_LINK.ppPreviousNext = pItem->HASHLISTINFO##_LINK.pNext)) \
			{ \
				pItem->HASHLISTINFO##_LINK.pNext->HASHLISTINFO##_LINK.ppPreviousNext = pItem->HASHLISTINFO##_LINK.ppPreviousNext; \
				if(*ppRow == pItem) \
					*ppRow = pItem->HASHLISTINFO##_LINK.pNext->HASHLISTINFO##_LINK.ppRow == ppRow ? pItem->HASHLISTINFO##_LINK.pNext : 0; \
			} \
			else if(*ppRow == pItem) \
				*ppRow = 0; \
			(hashlistName).nCount--; \
			HASHLISTINFO##_FREEPROC(pItem); \
			break; \
		} \
}

#define HASHLIST_REMOVE_NOFREE(HASHLISTINFO,hashlistName,pKey) \
{ \
	HASHLISTINFO##_ITEM *pItem,**ppRow = \
		&(hashlistName).ppHash[(HASHLISTINFO##_CHECKSUMPROC((pKey))) % HASHLIST_SIZE(HASHLISTINFO,hashlistName)]; \
	for(pItem = *ppRow; pItem && pItem->HASHLISTINFO##_LINK.ppRow == ppRow; pItem = pItem->HASHLISTINFO##_LINK.pNext) \
		if(HASHLISTINFO##_COMPAREPROC((pKey),(HASHLISTINFO##_KEY(pItem)))) \
		{ \
			if((*pItem->HASHLISTINFO##_LINK.ppPreviousNext = pItem->HASHLISTINFO##_LINK.pNext)) \
			{ \
				pItem->HASHLISTINFO##_LINK.pNext->HASHLISTINFO##_LINK.ppPreviousNext = pItem->HASHLISTINFO##_LINK.ppPreviousNext; \
				if(*ppRow == pItem) \
					*ppRow = pItem->HASHLISTINFO##_LINK.pNext->HASHLISTINFO##_LINK.ppRow == ppRow ? pItem->HASHLISTINFO##_LINK.pNext : 0; \
			} \
			else if(*ppRow == pItem) \
				*ppRow = 0; \
			(hashlistName).nCount--; \
			break; \
		} \
}

#define HASHLIST_REMOVE_ALL(HASHLISTINFO,hashlistName) \
{ \
	HASHLISTINFO##_ITEM* pItem; \
	while((pItem = (hashlistName).pFirst)) \
	{ \
		if(((hashlistName).pFirst = pItem->HASHLISTINFO##_LINK.pNext)) \
		{ \
			(hashlistName).pFirst->HASHLISTINFO##_LINK.ppPreviousNext = &(hashlistName).pFirst; \
			if(*pItem->HASHLISTINFO##_LINK.ppRow == pItem) \
				*pItem->HASHLISTINFO##_LINK.ppRow = pItem->HASHLISTINFO##_LINK.pNext->HASHLISTINFO##_LINK.ppRow == pItem->HASHLISTINFO##_LINK.ppRow ? pItem->HASHLISTINFO##_LINK.pNext : 0; \
		} \
		else if(*(pItem)->HASHLISTINFO##_LINK.ppRow == pItem) \
			*(pItem)->HASHLISTINFO##_LINK.ppRow = 0; \
		(hashlistName).nCount--; \
		HASHLISTINFO##_FREEPROC(pItem); \
	} \
}
/*
#define HASHLIST_REMOVE_ALL_NOFREE(HASHLISTINFO,hashlistName) \
{ \
	HASHLISTINFO##_ITEM* pItem; \
	while((pItem = (hashlistName).pFirst)) \
	{ \
		if(((hashlistName).pFirst = pItem->HASHLISTINFO##_LINK.pNext)) \
		{ \
			(hashlistName).pFirst->HASHLISTINFO##_LINK.ppPreviousNext = &(hashlistName).pFirst; \
			if(*pItem->HASHLISTINFO##_LINK.ppRow == pItem) \
				*pItem->HASHLISTINFO##_LINK.ppRow = pItem->HASHLISTINFO##_LINK.pNext->HASHLISTINFO##_LINK.ppRow == pItem->HASHLISTINFO##_LINK.ppRow ? pItem->HASHLISTINFO##_LINK.pNext : 0; \
		} \
		else if(*(pItem)->HASHLISTINFO##_LINK.ppRow == pItem) \
			*(pItem)->HASHLISTINFO##_LINK.ppRow = 0; \
		(hashlistName).nCount--; \
	} \
}
*/
#define HASHLIST_REMOVE_ALL_NOFREE(HASHLISTINFO,hashlistName) HASHLIST_INIT(HASHLISTINFO,hashlistName)

#endif /* _HASHLIST_H */
