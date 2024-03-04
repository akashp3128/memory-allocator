#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>


/* store info for every block of allocated memory. 
to do this i will add a header to every newly allocated memory blk*/

typedef char ALIGN[16];
union header
{
    struct{
        size_t size;
        unsigned is_free;
        union header_t *next; //linked list to keep track of memory allocated
    } s;
    ALIGN stub;

};
typedef union header header_t;


header_t *head, *tail; //head and tail to keep track of list

pthread_mutex_t global_malloc_lock;
header_t *get_free_block(size_t size);

void *malloc(size_t size)
{
    size_t total_size;
    void *block;
    header_t *header;
    if(!size)
        return NULL;
    pthread_mutex_lock(&global_malloc_lock);
    header = get_free_block(size);
    if(header){
        header->s.is_free = 0;
        pthread_mutex_unlock(&global_malloc_lock);
        return(void*)(header + 1);
    }
    total_size = sizeof(header_t) + size;
    block = (void*)sbrk(total_size);
    if (block == (void*) - 1){
        pthread_mutex_unlock(&global_malloc_lock);
    }

    header = block;
    header->s.size = size;
    header->s.is_free = 0;
    header->s.next = NULL;
    if(!head)
        head = header;
    if (tail)
        tail->s.next = header;
    tail = header;
    pthread_mutex_unlock(&global_malloc_lock);
    return(void*)(header + 1);
     
    }

    header_t *get_free_block(size_t size)
    {
        header_t *curr = head;
        while(curr){
            if(curr->s.is_free && curr->s.size >= size){
                return curr;
            }
            curr = curr->s.next;
        }
        return NULL;
    }
    void free(void *block){
        header_t *header, *tmp;
        if(!block)
            return;
        pthread_mutex_lock(&global_malloc_lock);
        header = (header_t*)block - 1;

        void* programbreak = sbrk(0);
        if((char*)block + header->s.size == programbreak){
            if(head == tail){
                head = tail = NULL;
            }else{
                tmp = head;
                while(tmp){
                    if(tmp->s.next == tail){
                        tmp->s.next = NULL;
                        tail = tmp;
                    }
                    tmp = tmp->s.next;
                }

            }
            sbrk(0- sizeof(header_t)- header->s.size);
            pthread_mutex_unlock(&global_malloc_lock);
            return;
        }
        header->s.is_free = 1;
        pthread_mutex_unlock(&global_malloc_lock);

    }

    void *calloc(size_t num, size_t nsize)
    {
        size_t size;
        void* block;
        if(!num || !nsize){
            return NULL;
        }
        size = num * nsize;

        if(nsize != size/num){
            return NULL;
        }
        block = malloc(size);
        if(!block)
            return NULL;
        memset(block, 0, size);
        return block;
    }

    void *realloc(void *block, size_t size)
    {
        header_t *header;
        void *ret;
        if(!block || !size){
            return malloc(size);
        }
        header = (header_t*)block -1;
        if(header->s.size >= size)
            return block;
        ret = malloc(size);
        if(ret){
            memcpy(ret, block, header->s.size);
            free(block);
        }
        return ret;
    }

    int main() {
    // Allocate memory
    void *memory_block = malloc(100);

    if (memory_block == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    // Use the allocated memory
    strcpy((char*)memory_block, "sweet it works!");

    // Print the content of the allocated memory
    printf("Allocated memory content: %s\n", (char*)memory_block);

    // Free the allocated memory
    free(memory_block);

    return 0;
}