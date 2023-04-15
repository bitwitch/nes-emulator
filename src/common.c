#define array_count(a) sizeof(a)/sizeof(*(a))

void *xmalloc(size_t size) {
    void *ptr = malloc(size);
    if (ptr == NULL) {
        perror("malloc");
        exit(1);
    }
    return ptr;
}

void *xcalloc(size_t num_items, size_t item_size) {
    void *ptr = calloc(num_items, item_size);
    if (ptr == NULL) {
        perror("calloc");
        exit(1);
    }
    return ptr;
}

void *xrealloc(void *ptr, size_t size) {
    void *result = realloc(ptr, size);
    if (result == NULL) {
        perror("recalloc");
        exit(1);
    }
    return result;
}

void fatal(char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    printf("FATAL: ");
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
    exit(1);
}


// dynamic array or "stretchy buffers", a la sean barrett
// ---------------------------------------------------------------------------

typedef struct {
	size_t len;
	size_t cap;
	char buf[]; // flexible array member
} DA_Header;

#define MAX(x, y) ((x) >= (y) ? (x) : (y))

// get the metadata of the array which is stored before the actual buffer in memory
#define da__header(b) ((DA_Header*)((char*)b - offsetof(DA_Header, buf)))
// checks if n new elements will fit in the array
#define da__fits(b, n) (da_lenu(b) + (n) <= da_cap(b)) 
// if n new elements will not fit in the array, grow the array by reallocating 
#define da__fit(b, n) (da__fits(b, n) ? 0 : ((b) = da__grow((b), da_lenu(b) + (n), sizeof(*(b)))))

#define BUF(x) x // annotates that x is a stretchy buffer
#define da_len(b)  ((b) ? (int32_t)da__header(b)->len : 0)
#define da_lenu(b) ((b) ?          da__header(b)->len : 0)
#define da_set_len(b, l) da__header(b)->len = (l)
#define da_cap(b) ((b) ? da__header(b)->cap : 0)
#define da_end(b) ((b) + da_lenu(b))
#define da_push(b, ...) (da__fit(b, 1), (b)[da__header(b)->len++] = (__VA_ARGS__))
#define da_free(b) ((b) ? (free(da__header(b)), (b) = NULL) : 0)
#define da_printf(b, fmt, ...) ((b) = da__printf((b), (fmt), __VA_ARGS__))

void *da__grow(void *buf, size_t new_len, size_t elem_size) {
	size_t new_cap = MAX(1 + 2*da_cap(buf), new_len);
	assert(new_len <= new_cap);
	size_t new_size = offsetof(DA_Header, buf) + new_cap*elem_size;

	DA_Header *new_header;
	if (buf) {
		new_header = xrealloc(da__header(buf), new_size);
	} else {
		new_header = xmalloc(new_size);
		new_header->len = 0;
	}
	new_header->cap = new_cap;
	return new_header->buf;
}

char *da__printf(char *buf, char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int add_size = 1 + vsnprintf(NULL, 0, fmt, args);
    va_end(args);

	int cur_len = da_len(buf);

	da__fit(buf, add_size);

	char *start = cur_len ? buf + cur_len - 1 : buf;
    va_start(args, fmt);
    vsnprintf(start, add_size, fmt, args);
    va_end(args);

	// if appending to a string that is already null terminated, we clobber the
	// original null terminator so we need to subtract 1
	da__header(buf)->len += cur_len ? add_size - 1 : add_size;

	return buf;
}


// Arena Allocator
// ---------------------------------------------------------------------------
#define ARENA_BLOCK_SIZE 65536

typedef struct {
    char *ptr;
    char *end;
    BUF(char **blocks);
} Arena;

void arena_grow(Arena *arena, size_t min_size) {
    size_t size = MAX(ARENA_BLOCK_SIZE, min_size);
    arena->ptr = xmalloc(size);
    arena->end = arena->ptr + size;
    da_push(arena->blocks, arena->ptr);
}

void *arena_alloc(Arena *arena, size_t size) {
    if (arena->ptr + size > arena->end) {
        arena_grow(arena, size); 
    }
    void *ptr = arena->ptr;
    arena->ptr += size;
    return ptr;
}

void *arena_alloc_zeroed(Arena *arena, size_t size) {
    void *ptr = arena_alloc(arena, size);
    memset(ptr, 0, size);
    return ptr;
}

void arena_free(Arena *arena) {
    for (int i=0; i<da_len(arena->blocks); ++i) {
        free(arena->blocks[i]);
    }
    da_free(arena->blocks);
}

char *arena_get_pos(Arena *arena) {
	return arena->ptr;
}

// FIXME(shaw): I think there is subtle bug here to be aware of, if you were to set
// pos to a previous block, then the arena end ptr will be wrong and the current block
// will become unreachable for future arena allocations.
// since I am just using the arena for relatively small allocations before popping 
// i should never really hit this issue, but it is still not correct
void arena_set_pos(Arena *arena, char *pos) {
	arena->ptr = pos;
}

void *arena_memdup(Arena *arena, void *src, size_t size) {
    if (size == 0) return NULL;
    void *new_mem = arena_alloc(arena, size);
    memcpy(new_mem, src, size);
    return new_mem;
}
