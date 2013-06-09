#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

/* TODO
 * 1. make better use of the len field. Use it for free chunks as well. This will speed up
 * the process of looking for free chunk.
 * 2. Implement merging algorithm to get rid of fragmentation.
 * 3. Improve testing to see that the content of the block is consistent. E.g. each block
 * can have the same value as it's index.
 */
typedef struct chunk_tag
{
    unsigned char *addr;
    int free;
    int index;
    int len; // number of chunks when it's allocated OR free
} chunk_t;


#define CHUNK_SIZE (2*1024)
#define NUM_CHUNKS (2*1024)
#define HEAP_SIZE (CHUNK_SIZE*NUM_CHUNKS)
#define MAX_FREE_BLOCKS 1024

#define DEBUG 1

unsigned char _the_heap[HEAP_SIZE];
unsigned char *the_heap = _the_heap;
chunk_t the_chunks[NUM_CHUNKS];
int free_list[MAX_FREE_BLOCKS]; // -1 means it's not occupied. other value stores the index of chunk that's free

static void mark_chunks(int start, int len);
static size_t sizeof_cont_chuncks(int ch);
static int find_chunks(size_t size);
static void free_list_add(int idx);
static void free_list_remove(int idx);
static void unmark_chunks(int start, int len);

void init(void)
{
    int i;

    memset(the_heap, HEAP_SIZE, 0);

    for (i = 0; i < NUM_CHUNKS; i++)
    {
        the_chunks[i].addr = the_heap + i* CHUNK_SIZE;
        the_chunks[i].free = 1;
        the_chunks[i].index = i;
        the_chunks[i].len = 0;
    }

    for (i = 0; i < MAX_FREE_BLOCKS; i++)
    {
        free_list[i] = -1;
    }
    // Initial state
    free_list_add( 0 );
    the_chunks[0].len = NUM_CHUNKS;
}

void *mmalloc(size_t size)
{
    int ch;
    int len;
    int hole = 0;

    if ( size == 0 )
        size = 1;

    ch = find_chunks(size);
    if ( ch < 0 )
    {
        return NULL;
    }

    len = size / CHUNK_SIZE;
    if ( size % CHUNK_SIZE != 0 )
    {
        len ++;
    }

    mark_chunks( ch, len );

    return (void*)(the_chunks[ch].addr);
}

void ffree(void *ptr)
{
    unsigned char *p = ptr;
    int ch;

    if ( p == NULL )
    {
#ifdef DEBUG
        printf("WARNING: trying to free an NULL pointer.\n");
#endif
        return;
    }

    if ( ((p - the_heap) % CHUNK_SIZE) != 0 || p < the_heap )
    {
#ifdef DEBUG
        printf("WARNING: trying to free an unaligned pointer: %p.\n", ptr);
#endif
        return;
    }

    ch = (p - the_heap) / CHUNK_SIZE;
    unmark_chunks( ch, the_chunks[ch].len );
}

/* Helper function */
static void free_list_remove(int idx)
{
    int i;

    for (i = 0; i < MAX_FREE_BLOCKS; i++)
    {
        if ( free_list[i] == idx )
        {
            free_list[i] = -1;
            break;
        }
    }
}

static void free_list_add(int idx)
{
    int i;

    for (i = 0; i < MAX_FREE_BLOCKS; i++)
    {
        if ( free_list[i] == -1 )
        {
            free_list[i] = idx;
            break;
        }
    }
}

/*
 * start: index of the first chunk to mark
 * len: number of chunks to mark
 **/
static void mark_chunks(int start, int len)
{
    int i;

    assert( start < NUM_CHUNKS );
    assert( start+len < NUM_CHUNKS );

    for (i = 0; i < len; i++)
    {
        assert( the_chunks[start+i].free == 1 );
        the_chunks[start+i].free = 0;
    }
    the_chunks[start].len = len;

    free_list_remove( start );
    for (i = start + len; i < NUM_CHUNKS; i++)
    {
        if ( the_chunks[i].free )
        {
            free_list_add( i );
            // FIXME: this is not very efficient
            the_chunks[i].len = sizeof_cont_chuncks(i) / CHUNK_SIZE;
            break;
        }
    }
}

static void unmark_chunks(int start, int len)
{
    int i;

    /*assert( the_chunks[start].free == 0 );*/
    /*assert( the_chunks[start].len != 0 );*/

    for (i = 0; i < len; i ++)
    {
        the_chunks[start+i].free = 1;
        the_chunks[start+i].len = 0;
    }
    the_chunks[start].len = len;
    free_list_add( start );
}

static size_t sizeof_cont_chuncks(int ch)
{
    size_t size = 0;

    assert( ch < NUM_CHUNKS );

    while (the_chunks[ch++].free)
        size += CHUNK_SIZE;

    return size;
}

static int find_chunks(size_t size)
{
    int head = -1;
    int i;

    for (i = 0; i < MAX_FREE_BLOCKS; i++)
    {
        if ( (free_list[i] >= 0) &&
                (the_chunks[free_list[i]].len * CHUNK_SIZE >= size) )
        {
            head = free_list[i];
            /*printf("Found chunk: %d.\n", head);*/
            break;
        }
    }

    // FIXME: when the above failed, should try to merge freed chunks

    return head;
}

#ifdef DEBUG
static void check_if_clean(void)
{
    int i;

    for ( i = 0; i < NUM_CHUNKS; i++ )
    {
        assert ( the_chunks[i].free == 1 );
    }
}
#endif

/* TESTS */
int test_1(void)
{
    int i;
    size_t s;
    void *ptr[10];

    memset( ptr, 0, 10 );
    for (i = 0; i < 10; i++)
    {
        s = rand() % (20*4096);
        ptr[i] = mmalloc( s );
        if ( ptr[i] == NULL )
        {
            printf("Failed to allcoate %ld bytes.\n", s);
        }
        else
        {
            printf("Allocated %ld bytes.\n", s);
            memset(ptr[i], i, s);
        }
    }

    return 0;
}

int test_2(void)
{
    int i;
    size_t s[10];
    void *ptr[10];

    printf("+++++++++++ TEST_2 ++++++++++++++\n");

    memset( ptr, 0, 10 );
    for (i = 0; i < 10; i++)
    {
        s[i] = rand() % (2*4096);
        ptr[i] = mmalloc( s[i] );
        if ( ptr[i] == NULL )
        {
            printf("Failed to allcoate %ld bytes.\n", s[i]);
        }
        else
        {
            printf("Allocated %ld bytes @ %p.\n", s[i], ptr[i]);
            memset(ptr[i], i, s[i]);
        }
    }
    for (i = 0; i < 10; i++)
    {
        memset(ptr[i], 0, s[i]);
        printf("Freeing %p.\n", ptr[i]);
        ffree(ptr[i]);
    }

    return 0;
}

void check_data(void **ptr, size_t *size, int n)
{
    int i, j;
    unsigned char *p;

    for (i = 0; i < n; i++)
    {
        p = (unsigned char *)(ptr[i]);
        if ( p == 0 )
            continue;
        for (j = 0; j < size[i]; j++)
        {
            assert( p[j] == i );
        }
    }
}

int test_3(void)
{
#define N 100
    int i, j;
    size_t s[N];
    int alloc;
    void *ptr[N];

    printf("+++++++++++ TEST_3 ++++++++++++++\n");
    for (i = 0; i < N; i++)
    {
        ptr[i] = NULL;
    }

    for (i = 0; i < N; i++)
    {
        alloc = (rand() % 10) - 5;
        if ( alloc >= 0 )
        {
            s[i] = rand() % (2*4096);
            ptr[i] = mmalloc( s[i] );
            if ( ptr[i] == NULL )
            {
                printf("Failed to allcoate %ld bytes.\n", s[i]);
            }
            else
            {
                printf("Allocated %ld bytes @ %p.\n", s[i], ptr[i]);
                memset(ptr[i], i, s[i]);
            }
            check_data( ptr, s, N );
        }
        else
        {
            for (j = 0; j < N; j++)
            {
                if ( ptr[j] != NULL )
                {
                    memset(ptr[j], 0, s[j]);
                    printf("Freeing %p.\n", ptr[j]);
                    ffree(ptr[j]);
                    ptr[j] = NULL;
                }
            }
            check_data( ptr, s, N );
        }
    }

    for (i = 0; i < N; i++)
    {
        if ( ptr[i] != NULL )
        {
            memset(ptr[i], 0, s[i]);
            printf("Freeing %p.\n", ptr[i]);
            ffree(ptr[i]);
            ptr[i] = NULL;
        }
    }

    return 0;
}

int main(void)
{
    init();
    /*test_1();*/
    test_2();
    check_if_clean();
    test_3();
    check_if_clean();
    printf("All done.\n");

    return 0;
}
