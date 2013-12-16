/*
         file: _hash.h
   desciption: hash macros
        begin: 01/17/03
    copyright: (C) 2003 by Colin Graf
        email: addition@users.sourceforge.net

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#ifndef _HASH_H
#define _HASH_H

#define HASH_INITTYPE(HASHINFO,nSize) struct tag##HASHINFO \
{ \
	HASHINFO##_ITEM* ppHash[nSize]; \
	unsigned int nCount; \
}

#define HASH_SIZEOF(HASHINFO) (sizeof(struct tag##HASHINFO))

#define HASH(HASHINFO) struct tag##HASHINFO
#define PHASH(HASHINFO) struct tag##HASHINFO*

#define HASHLINK(HASHINFO) struct \
{ \
	HASHINFO##_ITEM* pNext; \
} HASHINFO##_LINK

#define HASH_SIZE(HASHINFO,hashName) (sizeof((hashName).ppHash)/sizeof(*(hashName).ppHash))

#define HASH_INIT(HASHINFO,hashName) \
{ \
	(hashName).nCount = 0; \
	memset((hashName).ppHash,0,sizeof((hashName).ppHash)); \
}

#define HASH_ADD(HASHINFO,hashName,pItem) \
{ \
	HASHINFO##_ITEM** ppItem = \
		&(hashName).ppHash[(HASHINFO##_CHECKSUMPROC((HASHINFO##_KEY((pItem))))) % HASH_SIZE(HASHINFO,hashName)]; \
	(pItem)->HASHINFO##_LINK.pNext = *ppItem; \
	*ppItem = (pItem); \
	(hashName).nCount++; \
}

#define HASH_SEARCH_ITEM(HASHINFO,hashName,pItem,pItemResult/*out*/) \
{ \
	for((pItemResult) = (hashName).ppHash[(HASHINFO##_CHECKSUMPROC((HASHINFO##_KEY((pItem))))) % HASH_SIZE(HASHINFO,hashName)]; \
		(pItemResult); (pItemResult) = (pItemResult)->HASHINFO##_LINK.pNext) \
		if((pItemResult) == (pItem)) \
			break; \
}

#define HASH_LOOKUP(HASHINFO,hashName,pKey,pItemResult/*out*/) \
{ \
	for((pItemResult) = (hashName).ppHash[(HASHINFO##_CHECKSUMPROC((pKey))) % HASH_SIZE(HASHINFO,hashName)]; \
		(pItemResult); (pItemResult) = (pItemResult)->HASHINFO##_LINK.pNext) \
		if(HASHINFO##_COMPAREPROC((pKey),(HASHINFO##_KEY((pItemResult))))) \
			break; \
}

#define HASH_LOOKUP_POINTER(HASHINFO,hashName,pKey,ppItemResult/*out*/) \
{ \
	for((ppItemResult) = &(hashName).ppHash[(HASHINFO##_CHECKSUMPROC((pKey))) % HASH_SIZE(HASHINFO,hashName)]; \
		*(ppItemResult); (ppItemResult) = &(*(ppItemResult))->HASHINFO##_LINK.pNext) \
		if(HASHINFO##_COMPAREPROC((pKey),(HASHINFO##_KEY((*(ppItemResult)))))) \
			break; \
}

#define HASH_REMOVE_ITEM(HASHINFO,hashName,pItem) \
{ \
	HASHINFO##_ITEM** ppItem; \
	for(ppItem = &(hashName).ppHash[(HASHINFO##_CHECKSUMPROC((HASHINFO##_KEY((pItem))))) % HASH_SIZE(HASHINFO,hashName)]; \
		*ppItem; ppItem = &(*ppItem)->HASHINFO##_LINK.pNext) \
		if(*ppItem == (pItem)) \
		{ \
			*ppItem = (pItem)->HASHINFO##_LINK.pNext; \
			(hashName).nCount--; \
			HASHINFO##_FREEPROC((pItem)); \
			break; \
		} \
}

#define HASH_REMOVE_ITEM_NOFREE(HASHINFO,hashName,pItem) \
{ \
	HASHINFO##_ITEM** ppItem; \
	for(ppItem = &(hashName).ppHash[(HASHINFO##_CHECKSUMPROC((HASHINFO##_KEY((pItem))))) % HASH_SIZE(HASHINFO,hashName)]; \
		*ppItem; ppItem = &(*ppItem)->HASHINFO##_LINK.pNext) \
		if(*ppItem == (pItem)) \
		{ \
			*ppItem = (pItem)->HASHINFO##_LINK.pNext; \
			(hashName).nCount--; \
			break; \
		} \
}

#define HASH_REMOVE_ITEM_POINTER(HASHINFO,hashName,ppItem) \
{ \
	HASHINFO##_ITEM* pItem = (*ppItem); \
	*(ppItem) = (*(ppItem))->HASHINFO##_LINK.pNext; \
	(hashName).nCount--; \
	HASHINFO##_FREEPROC(pItem); \
}

#define HASH_REMOVE_ITEM_POINTER_NOFREE(HASHINFO,hashName,ppItem) \
{ \
	*(ppItem) = (*(ppItem))->HASHINFO##_LINK.pNext; \
	(hashName).nCount--; \
}

#define HASH_REMOVE(HASHINFO,hashName,pKey) \
{ \
	HASHINFO##_ITEM** ppItem; \
	for(ppItem = &(hashName).ppHash[(HASHINFO##_CHECKSUMPROC((pKey))) % HASH_SIZE(HASHINFO,hashName)]; \
		*ppItem; ppItem = &(*ppItem)->HASHINFO##_LINK.pNext) \
		if(HASHINFO##_COMPAREPROC((pKey),(HASHINFO##_KEY((*ppItem))))) \
		{ \
			HASHINFO##_ITEM* pItem = *ppItem; \
			*ppItem = pItem->HASHINFO##_LINK.pNext; \
			(hashName).nCount--; \
			HASHINFO##_FREEPROC(pItem); \
			break; \
		} \
}

#define HASH_REMOVE_NOFREE(HASHINFO,hashName,pKey) \
{ \
	HASHINFO##_ITEM** ppItem; \
	for(ppItem = &(hashName).ppHash[(HASHINFO##_CHECKSUMPROC((pKey))) % HASH_SIZE(HASHINFO,hashName)]; \
		*ppItem; ppItem = &(*ppItem)->HASHINFO##_LINK.pNext) \
		if(HASHINFO##_COMPAREPROC((pKey),(HASHINFO##_KEY((*ppItem))))) \
		{ \
			HASHINFO##_ITEM* pItem = *ppItem; \
			*ppItem = pItem->HASHINFO##_LINK.pNext; \
			(hashName).nCount--; \
			break; \
		} \
}

#define HASH_REMOVE_ALL(HASHINFO,hashName) \
{ \
	HASHINFO##_ITEM **ppItem,*pItem,**ppItemEnd = &(hashName).ppHash[HASH_SIZE(HASHINFO,hashName)]; \
	for(ppItem = (hashName).ppHash; ppItem < ppItemEnd; ppItem++) \
		while((pItem = *ppItem)) \
		{ \
			*ppItem = pItem->HASHINFO##_LINK.pNext; \
			(hashName).nCount--; \
			HASHINFO##_FREEPROC(pItem); \
		} \
}
/*
#define HASH_REMOVE_ALL_NOFREE(HASHINFO,hashName) \
{ \
	HASHINFO##_ITEM **ppItem,*pItem,**ppItemEnd = &(hashName).ppHash[HASH_SIZE(HASHINFO,hashName)]; \
	for(ppItem = (hashName).ppHash; ppItem < ppItemEnd; ppItem++) \
		while((pItem = *ppItem)) \
		{ \
			*ppItem = pItem->HASHINFO##_LINK.pNext; \
			(hashName).nCount--; \
		} \
}
*/
#define HASH_REMOVE_ALL_NOFREE(HASHINFO,hashName) HASH_INIT(HASHINFO,hashName)

#endif /* _HASH_H */
