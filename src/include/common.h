#ifndef COMMON_H
#define COMMON_H

typedef char i8;
typedef short i16;
typedef int i32;
typedef long long i64;
typedef long isz;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef unsigned long usz;

typedef void *ptr;
typedef const void *cptr;

// Initial capacity of a dynamic array
#define DA_INIT_CAP 256

// Append an item to a dynamic array
#define da_append(da, item)                                                    \
    do {                                                                       \
        if ((da)->count >= (da)->capacity) {                                   \
            (da)->capacity =                                                   \
                (da)->capacity == 0 ? DA_INIT_CAP : (da)->capacity * 2;        \
            (da)->items =                                                      \
                realloc((da)->items, (da)->capacity * sizeof(*(da)->items));   \
            assert((da)->items != NULL && "Buy more RAM lol");                 \
        }                                                                      \
                                                                               \
        (da)->items[(da)->count++] = (item);                                   \
    } while (0)
#define da_pop(da) ((da)->items[--(da)->count])

#define array_t(T)                                                             \
    struct {                                                                   \
        T *items;                                                              \
        usz count;                                                             \
        usz capacity;                                                          \
    }

#define option_t(T) T *

#endif // !COMMON_H
