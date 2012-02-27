/*!********************************************************************
libusbK - Multi-driver USB library.
Copyright (C) 2012 Travis Lee Robinson. All Rights Reserved.
libusb-win32.sourceforge.net

Development : Travis Lee Robinson  (libusbdotnet@gmail.com)
Testing     : Xiaofan Chen         (xiaofanc@gmail.com)

At the discretion of the user of this library, this software may be
licensed under the terms of the GNU Public License v3 or a BSD-Style
license as outlined in the following files:
* LICENSE-gpl3.txt
* LICENSE-bsd.txt

License files are located in a license folder at the root of source and
binary distributions.
********************************************************************!*/
#ifndef __INTERLOCKED_ARRAY_LIST_
#define __INTERLOCKED_ARRAY_LIST_

#if !defined(AL_MemAlloc) && !defined(AL_MemFree)

#define AL_MemAlloc(mHeap,mSize)	HeapAlloc(mHeap,HEAP_ZERO_MEMORY,(size_t)mSize)
#define AL_MemFree(mHeap,mMemory)	HeapFree(mHeap,0,mMemory)

#endif

#define ALDEF_LIST_FUNCTIONS(ALCorALH,mArray_List_TypeName,mArray_List_Name)   											\
	ALCorALH##_PushHead(mArray_List_TypeName, mArray_List_Name);   														\
	ALCorALH##_PushTail(mArray_List_TypeName, mArray_List_Name);   														\
	ALCorALH##_PopHead(mArray_List_TypeName, mArray_List_Name);															\
	ALCorALH##_PopTail(mArray_List_TypeName, mArray_List_Name);															\
	\
	ALCorALH##_Create(mArray_List_TypeName, mArray_List_Name); 															\
	ALCorALH##_CreateItem(mArray_List_TypeName, mArray_List_Name); 														\
	\
	ALCorALH##_Destroy(mArray_List_TypeName, mArray_List_Name);															\
	ALCorALH##_DestroyItem(mArray_List_TypeName, mArray_List_Name)


#define ALDEF_LIST_SRC(mArray_List_TypeName,mArray_List_Name)  															\
	ALDEF_LOCKED_ARRAY_LIST(mArray_List_TypeName); 																		\
	ALDEF_LIST_FUNCTIONS(ALC,mArray_List_TypeName,mArray_List_Name)


#define ALDEF_LIST_HDR(mArray_List_TypeName,mArray_List_Name)  															\
	ALDEF_LOCKED_ARRAY_LIST(mArray_List_TypeName); 																		\
	ALDEF_LIST_FUNCTIONS(ALH,mArray_List_TypeName,mArray_List_Name)

#define ALDEF_LOCKED_ARRAY_LIST(mArray_List_TypeName)  						\
	typedef struct _##mArray_List_TypeName##_LIST   							\
	{   																		\
		\
		volatile P##mArray_List_TypeName##_ITEM* Items; 						\
		long MaxCount;  														\
		\
		struct  																\
		{   																	\
			volatile long Count;												\
			volatile long Head; 												\
			volatile long Tail; 												\
		}Idx;   																\
		\
	}mArray_List_TypeName##_LIST,*P##mArray_List_TypeName##_LIST

#define AL_Inc_Idx(mArray_List,mIdxField) ((((ULONG)InterlockedIncrement(&((mArray_List)->Idx.mIdxField))) % (mArray_List)->MaxCount))
#define AL_Dec_Idx(mArray_List,mIdxField) ((((ULONG)InterlockedDecrement(&((mArray_List)->Idx.mIdxField))) % (mArray_List)->MaxCount))

#define AL_ExgInc_Idx(mArray_List,mIdxField) ((((ULONG)InterlockedExchangeAdd(&((mArray_List)->Idx.mIdxField),1)) % (mArray_List)->MaxCount))
#define AL_ExgDec_Idx(mArray_List,mIdxField) ((((ULONG)InterlockedExchangeAdd(&((mArray_List)->Idx.mIdxField),-1)) % (mArray_List)->MaxCount))

#define ALH_Create(mArray_List_TypeName,mArray_List_Name)			DWORD AL_Create_##mArray_List_Name				(P##mArray_List_TypeName##_LIST* mArray_List_Name##List, HANDLE Heap, long MaxCount)
#define ALH_Destroy(mArray_List_TypeName,mArray_List_Name)			DWORD AL_Destroy_##mArray_List_Name				(P##mArray_List_TypeName##_LIST* mArray_List_Name##List, HANDLE Heap)
#define ALH_CreateItem(mArray_List_TypeName,mArray_List_Name)		DWORD AL_Create_##mArray_List_Name##Item		(P##mArray_List_TypeName##_ITEM* mArray_List_Name##Item, HANDLE Heap)
#define ALH_DestroyItem(mArray_List_TypeName,mArray_List_Name)		DWORD AL_Destroy_##mArray_List_Name##Item		(P##mArray_List_TypeName##_ITEM* mArray_List_Name##Item, HANDLE Heap)

#define ALH_Push(mHeadTail,mArray_List_TypeName,mArray_List_Name)	DWORD AL_Push##mHeadTail##_##mArray_List_Name	(P##mArray_List_TypeName##_LIST mArray_List_Name##List, P##mArray_List_TypeName##_ITEM  mArray_List_Name##Item)
#define ALH_Pop(mHeadTail,mArray_List_TypeName,mArray_List_Name)	DWORD AL_Pop##mHeadTail##_##mArray_List_Name	(P##mArray_List_TypeName##_LIST mArray_List_Name##List, P##mArray_List_TypeName##_ITEM* mArray_List_Name##Item)

#define ALH_PushHead(mArray_List_TypeName,mArray_List_Name)		ALH_Push(Head,mArray_List_TypeName,mArray_List_Name)
#define ALH_PushTail(mArray_List_TypeName, mArray_List_Name)	ALH_Push(Tail,mArray_List_TypeName, mArray_List_Name)
#define ALH_PopHead(mArray_List_TypeName, mArray_List_Name)		ALH_Pop(Head,mArray_List_TypeName, mArray_List_Name)
#define ALH_PopTail(mArray_List_TypeName, mArray_List_Name)		ALH_Pop(Tail,mArray_List_TypeName, mArray_List_Name)

#define AL_Synchronize_Pop(mArray_List_TypeName,mArray_List_Name,mIdx)	(((*(mArray_List_Name##Item)=(P##mArray_List_TypeName##_ITEM)InterlockedCompareExchangePointer((volatile PVOID*)&((mArray_List_Name##List)->Items[mIdx]),NULL,(mArray_List_Name##List)->Items[mIdx]))) == NULL)
#define AL_Synchronize_Push(mArray_List_TypeName,mArray_List_Name,mIdx)	((InterlockedCompareExchangePointer((volatile PVOID*)&((mArray_List_Name##List)->Items[mIdx]),(mArray_List_Name##Item),NULL))!=NULL)

#define AL_DBG(format,...)
// #define AL_DBG(format,...) printf(format,__VA_ARGS__)

#define ALC_PushHead(mArray_List_TypeName,mArray_List_Name)																\
	ALH_PushHead(mArray_List_TypeName, mArray_List_Name)   																\
	{  																													\
		long cnt,idx;  																									\
		while (((cnt = (mArray_List_Name##List)->Idx.Count+1)) <= (mArray_List_Name##List)->MaxCount)  					\
		{  																												\
			if (InterlockedCompareExchange(&((mArray_List_Name##List)->Idx.Count),cnt,cnt-1)==cnt-1)   					\
			{  																											\
				idx = AL_ExgDec_Idx((mArray_List_Name##List),Head);AL_DBG("idx:%d\n",idx); 								\
				while(AL_Synchronize_Push(mArray_List_TypeName,mArray_List_Name,idx)) SleepEx(0,TRUE);					\
				return ERROR_SUCCESS;  																					\
				\
			}  																											\
		}  																												\
		return ERROR_NO_MORE_ITEMS;																						\
	}

#define ALC_PushTail(mArray_List_TypeName, mArray_List_Name)   															\
	ALH_PushTail(mArray_List_TypeName, mArray_List_Name)   																\
	{  																													\
		long cnt,idx;  																									\
		while (((cnt = (mArray_List_Name##List)->Idx.Count+1)) <= (mArray_List_Name##List)->MaxCount)  					\
		{  																												\
			if (InterlockedCompareExchange(&((mArray_List_Name##List)->Idx.Count),cnt,cnt-1)==cnt-1)   					\
			{  																											\
				idx = AL_ExgInc_Idx(mArray_List_Name##List,Tail);AL_DBG("idx:%d\n",idx);  									\
				while(AL_Synchronize_Push(mArray_List_TypeName,mArray_List_Name,idx)) SleepEx(0,TRUE); 				\
				return ERROR_SUCCESS;  																					\
				\
			}  																											\
		}  																												\
		return ERROR_NO_MORE_ITEMS;																						\
	}

#define ALC_PopHead(mArray_List_TypeName, mArray_List_Name)																\
	ALH_PopHead(mArray_List_TypeName, mArray_List_Name)																	\
	{  																													\
		long cnt,idx;  																									\
		while (((cnt = (mArray_List_Name##List)->Idx.Count-1)) >= 0)   													\
		{  																												\
			if (InterlockedCompareExchange(&((mArray_List_Name##List)->Idx.Count),cnt,cnt+1)==cnt+1)   					\
			{  																											\
				idx = AL_ExgInc_Idx(mArray_List_Name##List,Head);AL_DBG("idx:%d\n",idx);  									\
				while(AL_Synchronize_Pop(mArray_List_TypeName,mArray_List_Name,idx)) SleepEx(0,TRUE); 					\
				return ERROR_SUCCESS;  																					\
				\
			}  																											\
		}  																												\
		return ERROR_NO_MORE_ITEMS;																						\
	}

#define ALC_PopTail(mArray_List_TypeName, mArray_List_Name)																\
	ALH_PopTail(mArray_List_TypeName, mArray_List_Name)																	\
	{  																													\
		long cnt,idx;  																									\
		while (((cnt = (mArray_List_Name##List)->Idx.Count-1)) >= 0)   													\
		{  																												\
			if (InterlockedCompareExchange(&((mArray_List_Name##List)->Idx.Count),cnt,cnt+1)==cnt+1)   					\
			{  																											\
				idx = AL_ExgDec_Idx(mArray_List_Name##List,Tail); AL_DBG("idx:%d\n",idx);  								\
				while(AL_Synchronize_Pop(mArray_List_TypeName,mArray_List_Name,idx)) SleepEx(0,TRUE); 					\
				return ERROR_SUCCESS;  																					\
				\
			}  																											\
		}  																												\
		return ERROR_NO_MORE_ITEMS;																						\
	}

#define ALC_Create(mArray_List_TypeName,mArray_List_Name)  																\
	ALH_Create(mArray_List_TypeName, mArray_List_Name) 																	\
	{  																													\
		(*(mArray_List_Name##List))=(P##mArray_List_TypeName##_LIST)AL_MemAlloc(Heap,sizeof(mArray_List_TypeName##_LIST));	\
		if ((*(mArray_List_Name##List)) == NULL) return GetLastError();													\
		(*(mArray_List_Name##List))->Items = (P##mArray_List_TypeName##_ITEM*)AL_MemAlloc(Heap,sizeof(P##mArray_List_TypeName##_ITEM)*(MaxCount+1)); 	\
		if ((*(mArray_List_Name##List))->Items == NULL) return GetLastError(); 											\
		(*(mArray_List_Name##List))->MaxCount = MaxCount;  																\
		\
		return ERROR_SUCCESS;  																							\
	}

#define ALC_Destroy(mArray_List_TypeName,mArray_List_Name) 																\
	ALH_Destroy(mArray_List_TypeName, mArray_List_Name)																	\
	{  																													\
		if ((mArray_List_Name##List) && (*(mArray_List_Name##List)))   													\
		{  																												\
			if ((*(mArray_List_Name##List))->Items != NULL) AL_MemFree(Heap,(LPVOID)(*(mArray_List_Name##List))->Items);  	\
			(*(mArray_List_Name##List))->Items = NULL; 																	\
			AL_MemFree(Heap,(LPVOID)(*(mArray_List_Name##List))); 														\
			*(mArray_List_Name##List) = NULL;  																			\
		}  																												\
		return ERROR_SUCCESS;  																							\
	}

#define ALC_CreateItem(mArray_List_TypeName,mArray_List_Name)  															\
	ALH_CreateItem(mArray_List_TypeName,mArray_List_Name)  																\
	{  																													\
		(*(mArray_List_Name##Item))=(P##mArray_List_TypeName##_ITEM)AL_MemAlloc(Heap,sizeof(mArray_List_TypeName##_ITEM));	\
		return ((*(mArray_List_Name##Item)) == NULL) ? GetLastError() : ERROR_SUCCESS; 									\
	}

#define ALC_DestroyItem(mArray_List_TypeName,mArray_List_Name) 															\
	ALH_DestroyItem(mArray_List_TypeName, mArray_List_Name)																\
	{  																													\
		if ((mArray_List_Name##Item) && (*(mArray_List_Name##Item)))   													\
		{  																												\
			AL_MemFree(Heap,(*(mArray_List_Name##Item))); 																\
			*(mArray_List_Name##Item) = NULL;  																			\
		}  																												\
		return ERROR_SUCCESS;  																							\
	}

#endif
