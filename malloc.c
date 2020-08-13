#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>


//this struct implements singly linked list which contains information about memory which we've just allocated
struct block_meta
{
	size_t size;
	struct block_meta *next;
	int free;
};

#define META_SIZE sizeof(struct block_meta)


//head of ours singly linked list
void *global_base = NULL;


/*When freeing memory, we do not return it to the operating system, but store it further.
*At the same time, when the user makes a new request for memory,
we will check whether we already have a sufficient amount of memory allocated and whether we can use it indulgently.*/
struct block_meta *find_free_block(struct block_meta** last, size_t size)
{
	struct block_meta *current = global_base;
	
	while (current && !(current->free && current->size >= size))
	{
		*last = current;
		current = current->next;
	}
	
	return current;
}


//If we don't have enough space, then we ask the operating system to allocate more memory for the new block
struct block_meta* request_space(struct block_meta *last, size_t size)
{
	struct block_meta *block;
	
	block = sbrk(0);
	
	void *request = sbrk(size + META_SIZE);
	assert((void *)block == request);
	
	if (request == (void *) -1)
	{
		return NULL;
	}
	
	if (last)
	{
		last->next = block;
	}
	
	block->size = size;
	block->next = NULL;
	block->free = 0;
	
	return block;
}

//implementation malloc function
void *malloc(size_t size)
{
	struct block_meta *block;
	//TODO: allign size?
	
	if (size <= 0)
	{
		return NULL;
	}
	
	if (!global_base)
	{
		block = request_space(NULL, size);
		
		if (!block)
		{
			return NULL;
		}
		
		global_base = block;
	}
	else
	{
		struct block_meta *last = global_base;
		block = find_free_block(&last, size);
		
		if (!block)
		{
			block = request_space(last, size);
			if (!block)
			{
				return NULL;
			}
			else
			{
				//TODO: consider splitting block here
				block->free = 0;
			}
		}
	}
	
	return block+1;
}

//when we initialize our block of memory, we store information of block in start of part memory
//and we need to get pointer to that part of memory
struct block_meta *get_block_ptr(void* ptr)
{
	return (struct block_meta *)ptr-1;
}

//implementation free function
void free(void* ptr)
{
	if (!ptr)
	{
		return;
	}
	
	//TODO: consider merging blocks once splitting block is emplemented
	struct block_meta* block_ptr = get_block_ptr(ptr);
	
	assert(block_ptr->free == 0);
  	
  	block_ptr->free = 1;
}

//implementation realloc function
void *realloc(void *ptr, size_t size)
{
	if (!ptr)
	{
		return malloc(size);
	}
	
	struct block_meta *block_ptr = get_block_ptr(ptr);
	
	if (block_ptr->size >= size)
	{
		return ptr;
	}
	
	void *new_ptr;
	new_ptr = malloc(size);
	
	if (!new_ptr)
	{
		return NULL;
	}
	
	memcpy(new_ptr, ptr, block_ptr->size);
	
	free(ptr);
	
	return new_ptr;
}

//implementation calloc function
void *calloc(size_t nelem, size_t elsize)
{
	size_t size = nelem * elsize;
	void *ptr = malloc(size);
	
	memset(ptr, 0, size);
	
	return ptr;
}

//simple tests
int main(void)
{
	int *a = (int *)malloc(100*sizeof(int));
	int *b = (int *)malloc(sizeof(int));
	
	free(a);
	free(b);
	
	return 0;
}
