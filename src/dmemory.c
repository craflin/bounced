/*
         file: debug.c
   desciption: malloc calloc realloc strdup free and "cast" debugging
        begin: 10/07/02
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

#ifdef DEBUG_MEMORY

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "dmemory.h"

#include "bounced.h" /* 4 debug */

#define DEBUG_MEMORY_BLOCK_HASH_SIZE /*(1024*5+1)*/(1024+1)
#define DEBUG_MEMORY_TYPE_INFO_HASH_SIZE 32
#define DEBUG_MEMORY_MALLOCBYTE 0xcc
#define DEBUG_MEMORY_FREEBYTE 0xff

typedef struct tagDEBUGMEMORYBLOCK
{
	const char* strType;
	const char* strFile;
	int iLine;
	size_t size;
	unsigned int nSum;
	struct tagDEBUGMEMORYBLOCK* pNext;
} DEBUGMEMORYBLOCK, *PDEBUGMEMORYBLOCK;

typedef struct tagDEBUGMEMORYTYPEINFO
{
	const char* strType;
	size_t size;
	unsigned int nBlocks;
	struct tagDEBUGMEMORYTYPEINFO* pNext;
} DEBUGMEMORYTYPEINFO, *PDEBUGMEMORYTYPEINFO;

PDEBUGMEMORYBLOCK g_pDebugMemoryBlockHash[DEBUG_MEMORY_BLOCK_HASH_SIZE];
size_t g_sizeDebugMemoryUsed = 0;
PDEBUGMEMORYTYPEINFO g_pDebugMemoryTypeInfoHash[DEBUG_MEMORY_TYPE_INFO_HASH_SIZE];

unsigned int DebugMemoryBlockSum(PDEBUGMEMORYBLOCK pBlock)
{
	unsigned int sum/* = 0*/;
	sum = /*(sum<<4)+*/(unsigned int)pBlock->strType;
	sum = (sum<<4)+(unsigned int)pBlock->strFile;
	sum = (sum<<4)+(unsigned int)pBlock->iLine;
	sum = (sum<<4)+(unsigned int)pBlock->size;
	return sum;
}

int DebugMemoryInit(void)
{
	if(g_sizeDebugMemoryUsed)
		return 0;
	memset(g_pDebugMemoryBlockHash,0,sizeof(g_pDebugMemoryBlockHash));
	memset(g_pDebugMemoryTypeInfoHash,0,sizeof(g_pDebugMemoryTypeInfoHash));
	printf("DebugMemory: Enabled!\n");
	return 1;
}

int DebugMemoryCleanup(void)
{
	unsigned int n;
	int iReturn = 1;
	PDEBUGMEMORYBLOCK pDMB, pNextDMB;
	for(n = 0; n < (sizeof(g_pDebugMemoryBlockHash)/sizeof(PDEBUGMEMORYBLOCK)); n++)
		for(pDMB = g_pDebugMemoryBlockHash[n]; pDMB; pDMB = pNextDMB)
		{
			pNextDMB = pDMB->pNext;
			printf("DebugMemory: Memory leak! %s(%d) Type %s Size %u Pointer 0x%08x\n",pDMB->strFile,pDMB->iLine,pDMB->strType,pDMB->size,(unsigned int)pDMB+sizeof(DEBUGMEMORYBLOCK));
			free(pDMB);
			iReturn = 0;
		}
	if(!iReturn)
		assert(0);
	return iReturn;
}

int DebugMemoryCheckBlocks(void)
{
	int iReturn = 1;
	PDEBUGMEMORYBLOCK *ppDMB,*ppEndDMB,pDMB;
	ppDMB = g_pDebugMemoryBlockHash;
	ppEndDMB = ppDMB+(sizeof(g_pDebugMemoryBlockHash)/sizeof(PDEBUGMEMORYBLOCK));
	printf("DebugMemoery: Checking blocks...\n");
	for(; ppDMB < ppEndDMB; ppDMB++)
		for(pDMB = *ppDMB; pDMB; pDMB = pDMB->pNext)
			if(pDMB->nSum != DebugMemoryBlockSum(pDMB))
			{
				
				printf("DebugMemory: Brocken Memory detected! %s(%d) Type %s Size %u Pointer 0x%08x\n",pDMB->strFile,pDMB->iLine,pDMB->strType,pDMB->size,(unsigned int)pDMB+sizeof(DEBUGMEMORYBLOCK));
				iReturn = 0;
			}

	if(g_DebugHasMotdCopied)
	{
		if(g_DebuglistMotd.pFirst != g_listMotd.pFirst ||
			g_DebuglistMotd.nCount != g_listMotd.nCount ||
			g_DebugMotdStrFirst.ll.pNext != g_listMotd.pFirst->ll.pNext ||
			g_DebugMotdStrFirst.p != g_listMotd.pFirst->p ||
			g_DebugMotdStrFirst.i != g_listMotd.pFirst->i )
		{
			printf("OMG! the motd fucked up again!\n\n");
			printf("OMG! the motd fucked up again!\n\n");
			printf("OMG! the motd fucked up again!\n\n");
			iReturn = 0;
		}
	}

	if(!iReturn)
		assert(0);
	return iReturn;
}

size_t DebugMemoryUsage(void)
{
	return g_sizeDebugMemoryUsed;
}

int DebugMemoryShowStats(void)
{
	unsigned int n;
	PDEBUGMEMORYTYPEINFO pDTI;
	for(n = 0; n < sizeof(g_pDebugMemoryTypeInfoHash)/sizeof(PDEBUGMEMORYTYPEINFO); n++)
		for(pDTI = g_pDebugMemoryTypeInfoHash[n]; pDTI; pDTI = pDTI->pNext)
			printf("DebugMemory: %s: %u Bytes (%u MB), %u Blocks\n",pDTI->strType,pDTI->size,pDTI->size/1024/1024,pDTI->nBlocks);
	return 1;
}

void* DebugMemoryMalloc(const char* strType, const char* strFile, int iLine, size_t size)
{
	PDEBUGMEMORYBLOCK pDMB;
	PDEBUGMEMORYTYPEINFO* ppDTI;
	unsigned int nRow;
	if(!size)
		printf("DebugMemory: Warning: Allocating zero bytes! %s(%d) Type %s Size %u\n",strFile,iLine,strType,size);

	{
		const char* str = strType;
		for(nRow = *str; *str; str++)
			nRow += *str;
	}

	for(ppDTI = &g_pDebugMemoryTypeInfoHash[nRow % (sizeof(g_pDebugMemoryTypeInfoHash)/sizeof(PDEBUGMEMORYTYPEINFO))]; *ppDTI; ppDTI = &(*ppDTI)->pNext)
			if(!strcmp(strType,(*ppDTI)->strType))
				break;
		if(!*ppDTI)
		{
			if(!(*ppDTI = malloc(sizeof(DEBUGMEMORYTYPEINFO))))
			{
				printf("DebugMemory: Out of memory! %s(%d) Type %s Size %u\n",strFile,iLine,strType,size);
				assert(0);
				return 0;
			}
			(*ppDTI)->strType = strType;
			(*ppDTI)->size = 0;
			(*ppDTI)->nBlocks = 0;
			(*ppDTI)->pNext = 0;
		}


	if(!(pDMB = malloc(sizeof(DEBUGMEMORYBLOCK)+size)))
	{
		if(pDMB)
			free(pDMB);
		if(*ppDTI && !(*ppDTI)->nBlocks)
		{
			free(*ppDTI);
			*ppDTI = 0;
		}	
		printf("DebugMemory: Out of memory! %s(%d) Type %s Size %u\n",strFile,iLine,strType,size);
		assert(0);
		return 0;
	}
	pDMB->strType = strType;
	pDMB->strFile = strFile;
	pDMB->iLine = iLine;
	pDMB->size = size;
	pDMB->nSum = DebugMemoryBlockSum(pDMB);
	g_sizeDebugMemoryUsed += size;
	nRow = ((unsigned int)pDMB) % (sizeof(g_pDebugMemoryBlockHash)/sizeof(PDEBUGMEMORYBLOCK));
	pDMB->pNext = g_pDebugMemoryBlockHash[nRow];
	g_pDebugMemoryBlockHash[nRow] = pDMB;
	(char*)pDMB += sizeof(DEBUGMEMORYBLOCK);
	memset(pDMB,DEBUG_MEMORY_MALLOCBYTE,size);

	(*ppDTI)->nBlocks++;
	(*ppDTI)->size += size;

	return pDMB;
}

void* DebugMemoryCalloc(const char* strType, const char* strFile, int iLine, size_t sizeNum, size_t size)
{
	void* p;
	if(!(p = DebugMemoryMalloc(strType,strFile,iLine,sizeNum*size)))
		return 0;
	memset(p,0,sizeNum*size);
	return p;
}

void* DebugMemoryRealloc(const char* strType, const char* strFile, int iLine, void* p, size_t size)
{
	(char*)p -= sizeof(DEBUGMEMORYBLOCK);
	{
		unsigned int nRow = ((unsigned int)p) % (sizeof(g_pDebugMemoryBlockHash)/sizeof(PDEBUGMEMORYBLOCK));
		PDEBUGMEMORYBLOCK pDMB;
		for(pDMB = g_pDebugMemoryBlockHash[nRow]; pDMB; pDMB = pDMB->pNext)
			if(pDMB == p)
			{
				void* pNew;
				if(strcmp(pDMB->strType,strType))
				{
					printf("DebugMemory: Tryed to realloc %s to %s! %s(%d) Size %u NewSize %u Pointer 0x%08x\n",pDMB->strType,strType,strFile,iLine,pDMB->size,size,(unsigned int)p+sizeof(DEBUGMEMORYBLOCK));
					assert(0);
				}
				if(!p)
					printf("DebugMemory: Warning: Reallocating zero pointer! %s(%d) Type %s Size %u NewSize %u Pointer 0x%08x\n",strFile,iLine,strType,pDMB->size,size,(unsigned int)p+sizeof(DEBUGMEMORYBLOCK));

				if(!(pNew = DebugMemoryMalloc(strType,strFile,iLine,size)))
					return 0;
				(char*)p += sizeof(DEBUGMEMORYBLOCK);
				memcpy(pNew,p,size < pDMB->size ? size : pDMB->size);
				DebugMemoryFree(strType,strFile,iLine,p);
				return pNew;
			}
		printf("DebugMemory: Tryed to realloc invalid pointer! %s(%d) Type %s NewSize %u Pointer 0x%08x\n",strFile,iLine,strType,size,(unsigned int)p+sizeof(DEBUGMEMORYBLOCK));
		assert(0);
	}
	return 0;
}

char* DebugMemoryStrdup(const char* strType, const char* strFile, int iLine, const char* str)
{
	char* p;
	size_t size = strlen(str)+1;
	if(!str)
		printf("DebugMemory: Warning: Strdup zero pointer! %s(%d) Type %s Size %u Pointer 0x%08x\n",strFile,iLine,strType,size,(unsigned int)str);
	if(!(p = DebugMemoryMalloc(strType,strFile,iLine,size)))
		return 0;
	memcpy(p,str,size);
	return p;
}

void DebugMemoryFree(const char* strType, const char* strFile, int iLine, void* p)
{
	(char*)p -= sizeof(DEBUGMEMORYBLOCK);
	{
		unsigned int nRow = ((unsigned int)p) % (sizeof(g_pDebugMemoryBlockHash)/sizeof(PDEBUGMEMORYBLOCK));
		PDEBUGMEMORYBLOCK* ppDMB;
		for(ppDMB = &g_pDebugMemoryBlockHash[nRow]; *ppDMB; ppDMB = &(*ppDMB)->pNext)
			if(*ppDMB == p)
			{
				PDEBUGMEMORYBLOCK pDMB = *ppDMB;
				
				{
					PDEBUGMEMORYTYPEINFO* ppDTI;
					const char* str = strType;
					for(nRow = *str; *str; str++)
						nRow += *str;
					for(ppDTI = &g_pDebugMemoryTypeInfoHash[nRow % (sizeof(g_pDebugMemoryTypeInfoHash)/sizeof(PDEBUGMEMORYTYPEINFO))]; *ppDTI; ppDTI = &(*ppDTI)->pNext)
						if(!strcmp(strType,(*ppDTI)->strType))
							break;
					if(!*ppDTI)
						assert(0);	/* strange error no type info for strType.... shouldn't happen */
					else
					{
						(*ppDTI)->size -= pDMB->size;
						if(!--(*ppDTI)->nBlocks)
						{
							PDEBUGMEMORYTYPEINFO pDTI = *ppDTI;;
							*ppDTI = pDTI->pNext;
							free(pDTI);
						}
					}
				}

				*ppDMB = pDMB->pNext;
				if(strcmp(pDMB->strType,strType))
				{
					printf("DebugMemory: Tryed to free %s as %s! %s(%d) Size %u Pointer 0x%08x\n",pDMB->strType,strType,strFile,iLine,pDMB->size,(unsigned int)p);
					assert(0);
				}
				g_sizeDebugMemoryUsed -= pDMB->size;
				memset((char*)p+sizeof(DEBUGMEMORYBLOCK),DEBUG_MEMORY_FREEBYTE,pDMB->size);
				free(pDMB);

				return;
			}
		printf("DebugMemory: Tryed to free invalid pointer! %s(%d) Type %s Pointer 0x%08x\n",strFile,iLine,strType,(unsigned int)p+sizeof(DEBUGMEMORYBLOCK));
		assert(0);
	}
}

void* DebugMemoryCast(const char* strType, const char* strFile, int iLine, void* p)
{
	if(p)
	{
		(char*)p -= sizeof(DEBUGMEMORYBLOCK);
		{
			unsigned int nRow = ((unsigned int)p) % (sizeof(g_pDebugMemoryBlockHash)/sizeof(PDEBUGMEMORYBLOCK));
			PDEBUGMEMORYBLOCK pDMB;
			for(pDMB = g_pDebugMemoryBlockHash[nRow]; pDMB; pDMB = pDMB->pNext)
				if(pDMB == p)
				{
					if(strcmp(pDMB->strType,strType))
					{
						printf("DebugMemory: Tryed to cast %s to %s! %s(%d) Size %u Pointer 0x%08x\n",pDMB->strType,strType,strFile,iLine,pDMB->size,(unsigned int)p+sizeof(DEBUGMEMORYBLOCK));
						assert(0);
					}
					return (char*)p+sizeof(DEBUGMEMORYBLOCK);
				}
			printf("DebugMemory: Tryed to cast invalid point to %s! %s(%d) Pointer 0x%08x\n",strType,strFile,iLine,(unsigned int)p+sizeof(DEBUGMEMORYBLOCK));
			assert(0);
		}
	}
	return (char*)p+sizeof(DEBUGMEMORYBLOCK);
}

#endif /* DEBUG_MEMORY */

