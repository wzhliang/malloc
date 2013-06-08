#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

typedef struct chunk_tag
{
    void *start;
    int free;
    int index;
} chunk_t;


#define CHUNK_SIZE (2*1024)
#define NUM_CHUNKS (2*1024)
#define HEAP_SIZE (CHUNK_SIZE*NUM_CHUNKS)
#define MAX_FREE_BLOCKS 1024

#define reset_chunk(b) (b)->start = 0; (b)->free = 1;

void *the_heap[HEAP_SIZE];
chunk_t the_chunks[NUM_CHUNKS];

static void mark_chunks(int start, int len);
static size_t sizeof_cont_chunkc(int ch);
static int find_chunks(size_t size);

void init(void)
{
    int i;

    memset(the_heap, HEAP_SIZE, 0);

    for (i = 0; i < MAX_FREE_BLOCKS; i++)
    {
        the_chunks[i].start = the_heap + i* CHUNK_SIZE;
        the_chunks[i].free = 1;
        the_chunks[i].index = i;

        reset_chunk(the_chunks+i);
    }
}

void *mmaloc(size_t size)
{
    int ch;

    ch = find_chunks(size);
    if ( ch < 0 )
    {
        return NULL;
    }

    mark_chunks( ch, size / CHUNK_SIZE + 1);

    return the_chunks[ch].start;
}

void free(void *ptr)
{
}

static void mark_chunks(int start, int len)
{
    int i;

    assert( start < MAX_FREE_BLOCKS );

    for ( i = 0; i < len; i++ )
    {
        assert( the_chunks[i].free == 1 );
        the_chunks[i].free = 0;
    }
}

static size_t sizeof_cont_chunkc(int ch)
{
    int i;
    size_t size = 0;

    assert( ch < NUM_CHUNKS );

    while ( the_chunks[ch++].free )
        size += CHUNK_SIZE;

    return size;
}

static int find_chunks(size_t size)
{
    int i;

    for ( i = 0; i < MAX_FREE_BLOCKS; i ++ )
    {
        if ( sizeof_cont_chunkc( i ) >= size )
        {
            return i;
        }
    }

    return -1;
}

/* TESTS */
int test_1(void)
{
    int i;
    size_t s;
    void *ptr[10];

    for (i = 0; i < 10; i++)
    {
        s = rand() % 4096;
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

int main(void)
{
    init();
    test_1();
    printf("All done.\n");

    return 0;
}
