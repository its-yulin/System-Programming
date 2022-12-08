//
//  my_malloc.c
//  Lab1: Malloc
//
//THIS  CODE  IS  OUR  OWN  WORK,  IT  WAS  WRITTEN  WITHOUT  CONSULTING  A  TUTOR  OR  CODE  WRITTEN  BY OTHER STUDENTS OUTSIDE OF OUR TEAM. - Yulin Hu

#include "my_malloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stddef.h>

void insert_chunk(FreeListNode node);
void remove_chunk(FreeListNode chunk);
FreeListNode find_chunk(uint32_t sz);
FreeListNode split_chunk(FreeListNode chunk, uint32_t sz);


MyErrorNo my_errno=MYNOERROR;
FreeListNode head = NULL;

/*Helper Functions*/
FreeListNode find_chunk(uint32_t sz){
    FreeListNode itr = head;
    if (itr == NULL){                           // head is null
        return 0;
    }
    else if (head->size >= sz){                 // head is good
        remove_chunk(itr);
        return itr;
    }
    else if (head->flink == NULL){              // head is small
        return 0;
    }
    else{                                       // traverse FreeListNode
        while (itr != NULL){
            if(itr->size >- sz){
                remove_chunk(itr);
                return itr;
            }
            else if(itr->flink != NULL){
                itr = itr->flink;
            }
            else return 0;
        }
        return 0;
    }
}

FreeListNode split_chunk(FreeListNode chunk, uint32_t sz){
    FreeListNode rem = (FreeListNode)((uintptr_t)chunk + (uintptr_t)sz);
    rem->size = chunk->size - sz;
    rem->flink = NULL;
    chunk->size = sz;
    insert_chunk(rem);
    return chunk;
}

void insert_chunk(FreeListNode chunk){
    FreeListNode itr = head;
    if (itr ==  NULL){                                      // no head
        head = chunk;
    }
    else if (chunk <= itr){                                  // insert before head
        chunk->flink = itr;
        head = chunk;
    }
    else{                                                   // insert after head
        while(itr->flink != NULL && itr->flink < chunk){
            itr = itr->flink;
        }
        chunk->flink = itr->flink;
        itr->flink = chunk;
    }
}

void remove_chunk(FreeListNode chunk){
    if (chunk == head){                                      // remove head
        FreeListNode temp = head->flink;
        head = head->flink;
        return;
    }
    else{
        FreeListNode itr = head;                             // remove node
        while (itr->flink != NULL){
            if (itr->flink == chunk){
                itr->flink = itr->flink->flink;
                chunk->flink = NULL;
                return;
            }
            itr = itr->flink;
        }
    }  
}

/* my_malloc implementation*/
void *my_malloc(uint32_t size)
{
    if(size == 0){
        return 0;
    }
    void *chunk_start = (void *) sbrk(0);
    int total_chunk_size;

    // fix the size
    size += CHUNKHEADERSIZE;
    int min_size = 16;
    if(size < min_size){
        size = min_size;
    }
    else if (size % 8 != 0){
        size += 8 - size % 8;
    }
    // heap
    static uintptr_t heap_start;
    static uintptr_t heap_end;
    
    if(heap_start == 0) {
        heap_start = (uintptr_t) sbrk(8192);
        heap_end = (uintptr_t) sbrk(0);
        if (heap_start == 0){
            my_errno = MYENOMEM;
            return 0;
        }
        FreeListNode first = (FreeListNode) (heap_start);
        first->flink = 0;
        first->size = (uint32_t) (heap_end - heap_start);
        insert_chunk(first);
        heap_start += heap_end - heap_start;
    }
    // find from free list
    FreeListNode chunk = find_chunk(size);
    if (chunk != NULL){
        total_chunk_size = chunk->size;
        chunk_start = &(*chunk);
    }
    // call sbrk()
    else{
        if (size > 8192) {
            total_chunk_size = size;
            long check_error = (long) sbrk(size);
            if (check_error == -1){
                my_errno = MYENOMEM;
                return NULL;
            }
        }
        else{
            sbrk(size);
            total_chunk_size = size;
        }
    }

    *((__int32_t *) chunk_start) = size;
    *((__int32_t *)(chunk_start + sizeof(__int32_t))) = 9999;

    // split chunk
    if (total_chunk_size - size >= min_size){
        FreeListNode temp = split_chunk(chunk, size);
        chunk_start = (void *) temp;
    }
    else{
        *((__int32_t *)chunk_start) = total_chunk_size;
    }

    return chunk_start + CHUNKHEADERSIZE; 
}
      
void my_free(void *ptr)
{
    if (ptr == NULL || *((__int32_t *)(ptr-sizeof(__int32_t))) != 9999){
        my_errno = MYBADFREEPTR;
        return;
    }
    FreeListNode free;
    free = (FreeListNode) (ptr - CHUNKHEADERSIZE);
    free->size = *((__int32_t *)(ptr - CHUNKHEADERSIZE));
    free->flink = NULL;
    insert_chunk(free);
}

FreeListNode free_list_begin()
{
    return head;
}

void coalesce_free_list()
{
    FreeListNode temp = head;
    while(temp != NULL && temp->flink != NULL){
            void *temp_end = (void *) (temp) + (temp->size);
            if (temp_end == temp->flink){
                temp->size += temp->flink->size;
                temp->flink = temp->flink->flink;
            }
            else{
                temp = temp->flink; 
            }
    }
}