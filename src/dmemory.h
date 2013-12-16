/*
         file: debug.h
   desciption: malloc calloc realloc strdup free and "cast" debugging
        begin: 10/07/02
    copyright: (C) 2002 by Colin Graf
        email: addition@users.sourceforge.net

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#ifndef _DEBUG_H
#define _DEBUG_H

#ifdef DEBUG_MEMORY

extern int DebugMemoryInit(void);
extern int DebugMemoryCleanup(void);
extern int DebugMemoryCheckBlocks(void);
extern size_t DebugMemoryUsage(void);
extern int DebugMemoryShowStats(void);
extern void* DebugMemoryMalloc(const char* strType, const char* strFile, int iLine, size_t size);
extern void* DebugMemoryCalloc(const char* strType, const char* strFile, int iLine, size_t sizeNum, size_t size);
extern void* DebugMemoryRealloc(const char* strType, const char* strFile, int iLine, void* p, size_t size);
extern char* DebugMemoryStrdup(const char* strType, const char* strFile, int iLine, const char* str);
extern void DebugMemoryFree(const char* strType, const char* strFile, int iLine, void* p);
extern void* DebugMemoryCast(const char* strType, const char* strFile, int iLine, void* p);

#define MALLOC(type,size) ((type)DebugMemoryMalloc(#type,__FILE__,__LINE__,size))
#define CALLOC(type,num,size) ((type)DebugMemoryCalloc(#type,__FILE__,__LINE__,num,size))
#define REALLOC(type,memblock,size) ((type)DebugMemoryRealloc(#type,__FILE__,__LINE__,memblock,size))
#define STRDUP(type,str) ((type)DebugMemoryStrdup(#type,__FILE__,__LINE__,str))
#define FREE(type,memblock) {DebugMemoryFree(#type,__FILE__,__LINE__,memblock);(type)0;}

#define CAST(type,memblock) ((type)DebugMemoryCast(#type,__FILE__,__LINE__,memblock))

#else

#define DebugMemoryInit() 1
#define DebugMemoryCleanup() 1
#define DebugMemoryCheckBlocks() 1
#define DebugMemoryUsage() 0
#define DebugMemoryShowStats() 1

#define MALLOC(type,size) ((type)malloc(size))
#define CALLOC(type,num,size) ((type)calloc(num,size))
#define REALLOC(type,memblock,size) ((type)realloc(memblock,size))
#define STRDUP(type,str) ((type)strdup(str))
#define FREE(type,memblock) free(memblock)

#define CAST(type,memblock) ((type)memblock)

#endif /* DEBUG_MEMORY */

#endif /* _DEBUG_H_ */
