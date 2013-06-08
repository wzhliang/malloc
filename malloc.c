#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

typedef struct chunk_tag
{
    unsigned char *addr;
    int free;
    int index;
    int len; // number of chunks when it's allocated
} chunk_t;


#define CHUNK_SIZE (2*1024)
#define NUM_CHUNKS (2*1024)
#define HEAP_SIZE (CHUNK_SIZE*NUM_CHUNKS)
#define MAX_FREE_BLOCKS 1024

/*#define reset_chunk(b) (b)->addr = 0; (b)->free = 1;*/

unsigned char _the_heap[HEAP_SIZE];
unsigned char *the_heap = _the_heap;
chunk_t the_chunks[NUM_CHUNKS];
int free_list[MAX_FREE_BLOCKS]; // -1 means it's not occupied. other value stores the index of chunk that's free

static void mark_chunks(int start, int len);
static size_t sizeof_cont_chunkc(int ch);
static int find_chunks(size_t size);
static void free_list_add(int idx);
static void free_list_remove(int idx);
static void unmark_chunks(int start);

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
    free_list[0] = 0;
}

void *mmaloc(size_t size)
{
    int ch;

    /* FIXME: this is probably not right */
    if ( size == 0 )
        size = 1;

    ch = find_chunks(size);
    if ( ch < 0 )
    {
        return NULL;
    }

    mark_chunks( ch, size / CHUNK_SIZE + 1);

    return (void*)(the_chunks[ch].addr);
}

void free(void *ptr)
{
    unsigned char *p = ptr;
    int ch;

    if ( p == NULL || p < the_heap )
        return;

    if ( ((p - the_heap) % CHUNK_SIZE)!= 0 )
        return;

    ch = (p - the_heap) / CHUNK_SIZE;
    unmark_chunks( ch );
}

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

static void mark_chunks(int start, int len)
{
    int i;

    assert( start < MAX_FREE_BLOCKS );

    for ( i = 0; i < len; i++ )
    {
        assert( the_chunks[start+i].free == 1 );
        the_chunks[start+i].free = 0;
    }
    the_chunks[start].len = len;

    free_list_remove(start);
    free_list_add(start+len);
}

// non-free chunk knows itself how long it is
static void unmark_chunks(int start)
{
    int i;

    assert( the_chunks[start].free == 0 );
    assert( the_chunks[start].len != 0 );

    for ( i = 0; i < the_chunks[start].len; i ++ )
    {
        the_chunks[start+i].free = 1;
    }
    free_list_remove( start );
}

static size_t sizeof_cont_chunkc(int ch)
{
    size_t size = 0;

    assert( ch < NUM_CHUNKS );

    while ( the_chunks[ch++].free )
        size += CHUNK_SIZE;

    return size;
}

static int find_chunks(size_t size)
{
    int head = -1;
    int i;

    for ( i = 0; i < MAX_FREE_BLOCKS; i ++ )
    {
        if ( free_list[i] >= 0 && sizeof_cont_chunkc( free_list[i] ) >= size )
        {
            head = free_list[i];
            printf("Found chunk: %d.\n", head);
            break;
        }
    }

    return head;
}

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
        ptr[i] = mmaloc( s );
        if ( ptr[i] == NULL )
        {
            printf("Failed to allcoate %d bytes.\n", s);
        }
        else
        {
            printf("Allocated %d bytes.\n", s);
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

    memset( ptr, 0, 10 );
    for (i = 0; i < 10; i++)
    {
        s[i] = rand() % (2*4096);
        ptr[i] = mmaloc( s[i] );
        if ( ptr[i] == NULL )
        {
            printf("Failed to allcoate %d bytes.\n", s[i]);
        }
        else
        {
            printf("Allocated %d bytes.\n", s[i]);
            memset(ptr[i], i, s[i]);
        }
    }
    for (i = 0; i < 10; i++)
    {
        memset(ptr[i], 0, s[i]);
        printf("Freeing %p.\n", ptr[i]);
        free(ptr[i]);
    }

    return 0;
}

int main(void)
{
    init();
    /*test_1();*/
    test_2();
    printf("All done.\n");

    return 0;
}
