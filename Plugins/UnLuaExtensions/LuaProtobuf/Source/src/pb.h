#ifndef pb_h
#define pb_h

#include "lua.hpp"

#ifndef PB_NS_BEGIN
# ifdef __cplusplus
#   define PB_NS_BEGIN extern "C" {
#   define PB_NS_END   }
# else
#   define PB_NS_BEGIN
#   define PB_NS_END
# endif
#endif /* PB_NS_BEGIN */

#ifndef PB_STATIC
# if __GNUC__
#   define PB_STATIC static __attribute((unused))
# else
#   define PB_STATIC static
# endif
#endif

#ifdef PB_STATIC_API
# ifndef PB_IMPLEMENTATION
#  define PB_IMPLEMENTATION
# endif
# define PB_API PB_STATIC
#endif

#if !defined(PB_API) && defined(_WIN32)
# ifdef PB_IMPLEMENTATION
#  define PB_API __declspec(dllexport)
# else
#  define PB_API __declspec(dllimport)
# endif
#endif

#ifndef PB_API
# define PB_API extern
#endif

#if defined(_MSC_VER) || defined(__UNIXOS2__) || defined(__SOL64__)
typedef unsigned char      uint8_t;
typedef signed   char       int8_t;
typedef unsigned short     uint16_t;
typedef signed   short      int16_t;
typedef unsigned int       uint32_t;
typedef signed   int        int32_t;
typedef unsigned long long uint64_t;
typedef signed   long long  int64_t;

#ifndef INT64_MIN
# define INT64_MIN LLONG_MIN
#endif

#ifndef INT64_MAX
# define INT64_MAX LLONG_MAX
#endif

#elif defined(__SCO__) || defined(__USLC__) || defined(__MINGW32__)
# include <stdint.h>
#else
# include <inttypes.h>
# if (defined(__sun__) || defined(__digital__))
#   if defined(__STDC__) && (defined(__arch64__) || defined(_LP64))
typedef unsigned long int  uint64_t;
typedef signed   long int   int64_t;
#   else
typedef unsigned long long uint64_t;
typedef signed   long long  int64_t;
#   endif /* LP64 */
# endif /* __sun__ || __digital__ */
#endif

#include <stddef.h>
#include <limits.h>

PB_NS_BEGIN


/* types */

#define PB_WIRETYPES(X)   /* X(id,    name,    fmt) */\
    X(VARINT, "varint", 'v') X(64BIT, "64bit", 'q') X(BYTES, "bytes", 's') \
    X(GSTART, "gstart", '!') X(GEND,  "gend",  '!') X(32BIT, "32bit", 'd') \

#define PB_TYPES(X)          /* X(name,     type,     fmt) */\
    X(double,   double,   'F')  X(float,    float,    'f') \
    X(int64,    int64_t,  'I')  X(uint64,   uint64_t, 'U') \
    X(int32,    int32_t,  'i')  X(fixed64,  uint64_t, 'X') \
    X(fixed32,  uint32_t, 'x')  X(bool,     int,      'b') \
    X(string,   pb_Slice, 't')  X(group,    pb_Slice, 'g') \
    X(message,  pb_Slice, 'S')  X(bytes,    pb_Slice, 's') \
    X(uint32,   uint32_t, 'u')  X(enum,     int32_t,  'v') \
    X(sfixed32, int32_t,  'y')  X(sfixed64, int64_t,  'Y') \
    X(sint32,   int32_t,  'j')  X(sint64,   int64_t,  'J') \

typedef enum pb_WireType {
#define X(id,name,fmt) PB_T##id,
    PB_WIRETYPES(X)
#undef  X
    PB_TWIRECOUNT
} pb_WireType;

typedef enum pb_FieldType {
    PB_TNONE,
#define X(name,type,fmt) PB_T##name,
    PB_TYPES(X)
#undef  X
    PB_TYPECOUNT
} pb_FieldType;


/* conversions */

PB_API uint64_t pb_expandsig     (uint32_t v);
PB_API uint32_t pb_encode_sint32 ( int32_t v);
PB_API  int32_t pb_decode_sint32 (uint32_t v);
PB_API uint64_t pb_encode_sint64 ( int64_t v);
PB_API  int64_t pb_decode_sint64 (uint64_t v);
PB_API uint32_t pb_encode_float  (float    v);
PB_API float    pb_decode_float  (uint32_t v);
PB_API uint64_t pb_encode_double (double   v);
PB_API double   pb_decode_double (uint64_t v);


/* decode */

typedef struct pb_Slice { const char *p, *start, *end; } pb_Slice;
#define pb_gettype(v)      ((v) &  7)
#define pb_gettag(v)       ((v) >> 3)
#define pb_pair(tag,type)  ((tag) << 3 | ((type) & 7))

PB_API pb_Slice pb_slice  (const char *p);
PB_API pb_Slice pb_lslice (const char *p, size_t len);

PB_API size_t pb_pos (const pb_Slice s);
PB_API size_t pb_len (const pb_Slice s);

PB_API size_t pb_readvarint32 (pb_Slice *s, uint32_t *pv);
PB_API size_t pb_readvarint64 (pb_Slice *s, uint64_t *pv);
PB_API size_t pb_readfixed32  (pb_Slice *s, uint32_t *pv);
PB_API size_t pb_readfixed64  (pb_Slice *s, uint64_t *pv);

PB_API size_t pb_readslice (pb_Slice *s, size_t len, pb_Slice *pv);
PB_API size_t pb_readbytes (pb_Slice *s, pb_Slice *pv);
PB_API size_t pb_readgroup (pb_Slice *s, uint32_t tag, pb_Slice *pv);

PB_API size_t pb_skipvarint (pb_Slice *s);
PB_API size_t pb_skipbytes  (pb_Slice *s);
PB_API size_t pb_skipslice  (pb_Slice *s, size_t len);
PB_API size_t pb_skipvalue  (pb_Slice *s, uint32_t tag);

PB_API const char *pb_wtypename (int wiretype, const char *def);
PB_API const char *pb_typename  (int type,     const char *def);

PB_API int pb_typebyname  (const char *name, int def);
PB_API int pb_wtypebyname (const char *name, int def);
PB_API int pb_wtypebytype (int type);


/* encode */

#define PB_SSO_SIZE (sizeof(pb_HeapBuffer))

typedef struct pb_HeapBuffer {
    unsigned capacity;
    char    *buff;
} pb_HeapBuffer;

typedef struct pb_Buffer {
    unsigned size : sizeof(unsigned)*CHAR_BIT - 1;
    unsigned heap : 1;
    union {
        char buff[PB_SSO_SIZE];
        pb_HeapBuffer h;
    } u;
} pb_Buffer;

#define pb_onheap(b)     ((b)->heap)
#define pb_bufflen(b)    ((b)->size)
#define pb_buffer(b)     (pb_onheap(b) ? (b)->u.h.buff : (b)->u.buff)
#define pb_addsize(b,sz) ((void)((b)->size += (unsigned)(sz)))

PB_API void  pb_initbuffer   (pb_Buffer *b);
PB_API void  pb_resetbuffer  (pb_Buffer *b);
PB_API char *pb_prepbuffsize (pb_Buffer *b, size_t len);

PB_API pb_Slice pb_result (const pb_Buffer *b);

PB_API size_t pb_addvarint32 (pb_Buffer *b, uint32_t v);
PB_API size_t pb_addvarint64 (pb_Buffer *b, uint64_t v);
PB_API size_t pb_addfixed32  (pb_Buffer *b, uint32_t v);
PB_API size_t pb_addfixed64  (pb_Buffer *b, uint64_t v);

PB_API size_t pb_addslice  (pb_Buffer *b, pb_Slice s);
PB_API size_t pb_addbytes  (pb_Buffer *b, pb_Slice s);
PB_API size_t pb_addlength (pb_Buffer *b, size_t len);


/* type info database state and name table */

typedef struct pb_State pb_State;
typedef struct pb_Name  pb_Name;
typedef struct pb_Cache pb_Cache;

PB_API void pb_init (pb_State *S);
PB_API void pb_free (pb_State *S);

PB_API pb_Name *pb_newname (pb_State *S, pb_Slice s, pb_Cache *cache);
PB_API void     pb_delname (pb_State *S, pb_Name *name);
PB_API pb_Name *pb_usename (pb_Name *name);

PB_API const pb_Name *pb_name (const pb_State *S, pb_Slice s, pb_Cache *cache);


/* type info */

typedef struct pb_Type  pb_Type;
typedef struct pb_Field pb_Field;

#define PB_OK     0
#define PB_ERROR  1
#define PB_ENOMEM 2

PB_API int pb_load (pb_State *S, pb_Slice *s);

PB_API pb_Type  *pb_newtype  (pb_State *S, pb_Name *tname);
PB_API void      pb_deltype  (pb_State *S, pb_Type *t);
PB_API pb_Field *pb_newfield (pb_State *S, pb_Type *t, pb_Name *fname, int32_t number);
PB_API void      pb_delfield (pb_State *S, pb_Type *t, pb_Field *f);

PB_API const pb_Type  *pb_type  (const pb_State *S, const pb_Name *tname);
PB_API const pb_Field *pb_fname (const pb_Type *t,  const pb_Name *tname);
PB_API const pb_Field *pb_field (const pb_Type *t,  int32_t number);

PB_API const pb_Name *pb_oneofname (const pb_Type *t, int oneof_index);

PB_API int pb_nexttype  (const pb_State *S, const pb_Type **ptype);
PB_API int pb_nextfield (const pb_Type *t, const pb_Field **pfield);


/* util: memory pool */

#define PB_POOLSIZE 4096

typedef struct pb_Pool {
    void  *pages;
    void  *freed;
    size_t obj_size;
} pb_Pool;

PB_API void pb_initpool (pb_Pool *pool, size_t obj_size);
PB_API void pb_freepool (pb_Pool *pool);

PB_API void *pb_poolalloc (pb_Pool *pool);
PB_API void  pb_poolfree  (pb_Pool *pool, void *obj);

/* util: hash table */

typedef struct pb_Table pb_Table;
typedef struct pb_Entry pb_Entry;
typedef ptrdiff_t pb_Key;

PB_API void pb_inittable (pb_Table *t, size_t entrysize);
PB_API void pb_freetable (pb_Table *t);

PB_API size_t pb_resizetable (pb_Table *t, size_t size);

PB_API pb_Entry *pb_gettable (const pb_Table *t, pb_Key key);
PB_API pb_Entry *pb_settable (pb_Table *t, pb_Key key);

PB_API int pb_nextentry (const pb_Table *t, const pb_Entry **pentry);

struct pb_Table {
    unsigned  size;
    unsigned  lastfree;
    unsigned  entry_size : sizeof(unsigned)*CHAR_BIT - 1;
    unsigned  has_zero   : 1;
    pb_Entry *hash;
};

struct pb_Entry {
    ptrdiff_t next;
    pb_Key    key;
};


/* fields */

#define PB_CACHE_SIZE  (53)

typedef struct pb_NameEntry {
    struct pb_NameEntry *next;
    unsigned hash     : 32;
    unsigned length   : 16;
    unsigned refcount : 16;
} pb_NameEntry;

typedef struct pb_NameTable {
    size_t         size;
    size_t         count;
    pb_NameEntry **hash;
} pb_NameTable;

typedef struct pb_CacheSlot {
    const char *name;
    unsigned    hash;
} pb_CacheSlot;

struct pb_Cache {
    pb_CacheSlot slots[PB_CACHE_SIZE][2];
    unsigned     hash;
};

struct pb_State {
    pb_NameTable nametable;
    pb_Table     types;
    pb_Pool      typepool;
    pb_Pool      fieldpool;
};

struct pb_Field {
    pb_Name *name;
    pb_Type *type;
    pb_Name *default_value;
    int32_t  number;
    int32_t  sort_index;
    unsigned oneof_idx : 24;
    unsigned type_id   : 5; /* PB_T* enum */
    unsigned repeated  : 1;
    unsigned packed    : 1;
    unsigned scalar    : 1;
};

struct pb_Type {
    pb_Name    *name;
    const char *basename;
    pb_Field **field_sort;
    pb_Table field_tags;
    pb_Table field_names;
    pb_Table oneof_index;
    unsigned oneof_count; /* extra field count from oneof entries */
    unsigned oneof_field; /* extra field in oneof declarations */
    unsigned field_count : 28;
    unsigned is_enum   : 1;
    unsigned is_map    : 1;
    unsigned is_proto3 : 1;
    unsigned is_dead   : 1;
};


PB_NS_END

#endif /* pb_h */


#if defined(PB_IMPLEMENTATION) && !defined(pb_implemented)
#define pb_implemented

#define PB_MAX_SIZET          ((unsigned)~0 - 100)
#define PB_MAX_HASHSIZE       ((unsigned)~0 - 100)
#define PB_MIN_STRTABLE_SIZE  16
#define PB_MIN_HASHTABLE_SIZE 8
#define PB_HASHLIMIT          5

#include <assert.h>
#include <stdlib.h>
#include <string.h>

PB_NS_BEGIN


/* conversions */

PB_API uint32_t pb_encode_sint32(int32_t value)
{ return ((uint32_t)value << 1) ^ -(value < 0); }

PB_API int32_t pb_decode_sint32(uint32_t value)
{ return (value >> 1) ^ -(int32_t)(value & 1); }

PB_API uint64_t pb_encode_sint64(int64_t value)
{ return ((uint64_t)value << 1) ^ -(value < 0); }

PB_API int64_t pb_decode_sint64(uint64_t value)
{ return (value >> 1) ^ -(int64_t)(value & 1); }

PB_API uint64_t pb_expandsig(uint32_t value)
{ uint64_t m = (uint64_t)1U << 31; return (value ^ m) - m; }

PB_API uint32_t pb_encode_float(float value)
{ union { uint32_t u32; float f; } u; u.f = value; return u.u32; }

PB_API float pb_decode_float(uint32_t value)
{ union { uint32_t u32; float f; } u; u.u32 = value; return u.f; }

PB_API uint64_t pb_encode_double(double value)
{ union { uint64_t u64; double d; } u; u.d = value; return u.u64; }

PB_API double pb_decode_double(uint64_t value)
{ union { uint64_t u64; double d; } u; u.u64 = value; return u.d; }


/* decode */

PB_API pb_Slice pb_slice(const char *s)
{ return s ? pb_lslice(s, strlen(s)) : pb_lslice(NULL, 0); }

PB_API size_t pb_pos(const pb_Slice s) { return s.p - s.start; }
PB_API size_t pb_len(const pb_Slice s) { return s.end - s.p; }

static size_t pb_readvarint_slow(pb_Slice *s, uint64_t *pv) {
    const char *p = s->p;
    uint64_t n = 0;
    int i = 0;
    while (s->p < s->end && i < 10) {
        int b = *s->p++;
        n |= ((uint64_t)b & 0x7F) << (7*i++);
        if ((b & 0x80) == 0) {
            *pv = n;
            return i;
        }
    }
    s->p = p;
    return 0;
}

static size_t pb_readvarint32_fallback(pb_Slice *s, uint32_t *pv) {
    const uint8_t *p = (const uint8_t*)s->p, *o = p;
    uint32_t b, n;
    for (;;) {
        n = *p++ - 0x80, n += (b = *p++) <<  7; if (!(b & 0x80)) break;
        n -= 0x80 <<  7, n += (b = *p++) << 14; if (!(b & 0x80)) break;
        n -= 0x80 << 14, n += (b = *p++) << 21; if (!(b & 0x80)) break;
        n -= 0x80 << 21, n += (b = *p++) << 28; if (!(b & 0x80)) break;
        /* n -= 0x80 << 28; */
        if (!(*p++ & 0x80)) break;
        if (!(*p++ & 0x80)) break;
        if (!(*p++ & 0x80)) break;
        if (!(*p++ & 0x80)) break;
        if (!(*p++ & 0x80)) break;
        return 0;
    }
    *pv = n;
    s->p = (const char*)p;
    return p - o;
}

static size_t pb_readvarint64_fallback(pb_Slice *s, uint64_t *pv) {
    const uint8_t *p = (const uint8_t*)s->p, *o = p;
    uint32_t b, n1, n2 = 0, n3 = 0;
    for (;;) {
        n1 = *p++ - 0x80, n1 += (b = *p++) <<  7; if (!(b & 0x80)) break;
        n1 -= 0x80 <<  7, n1 += (b = *p++) << 14; if (!(b & 0x80)) break;
        n1 -= 0x80 << 14, n1 += (b = *p++) << 21; if (!(b & 0x80)) break;
        n1 -= 0x80 << 21, n2 += (b = *p++)      ; if (!(b & 0x80)) break;
        n2 -= 0x80      , n2 += (b = *p++) <<  7; if (!(b & 0x80)) break;
        n2 -= 0x80 <<  7, n2 += (b = *p++) << 14; if (!(b & 0x80)) break;
        n2 -= 0x80 << 14, n2 += (b = *p++) << 21; if (!(b & 0x80)) break;
        n2 -= 0x80 << 21, n3 += (b = *p++)      ; if (!(b & 0x80)) break;
        n3 -= 0x80      , n3 += (b = *p++) <<  7; if (!(b & 0x80)) break;
        return 0;
    }
    *pv = n1 | ((uint64_t)n2 << 28) | ((uint64_t)n3 << 56);
    s->p = (const char*)p;
    return p - o;
}

PB_API pb_Slice pb_lslice(const char *s, size_t len) {
    pb_Slice slice;
    slice.start = slice.p = s;
    slice.end = s + len;
    return slice;
}

PB_API size_t pb_readvarint32(pb_Slice *s, uint32_t *pv) {
    uint64_t u64;
    size_t ret;
    if (s->p >= s->end)  return 0;
    if (!(*s->p & 0x80)) return *pv = *s->p++, 1;
    if (pb_len(*s) >= 10 || !(s->end[-1] & 0x80))
        return pb_readvarint32_fallback(s, pv);
    if ((ret = pb_readvarint_slow(s, &u64)) != 0)
        *pv = (uint32_t)u64;
    return ret;
}

PB_API size_t pb_readvarint64(pb_Slice *s, uint64_t *pv) {
    if (s->p >= s->end)  return 0;
    if (!(*s->p & 0x80)) return *pv = *s->p++, 1;
    if (pb_len(*s) >= 10 || !(s->end[-1] & 0x80))
        return pb_readvarint64_fallback(s, pv);
    return pb_readvarint_slow(s, pv);
}

PB_API size_t pb_readfixed32(pb_Slice *s, uint32_t *pv) {
    int i;
    uint32_t n = 0;
    if (s->p + 4 > s->end)
        return 0;
    for (i = 3; i >= 0; --i) {
        n <<= 8;
        n |= s->p[i] & 0xFF;
    }
    s->p += 4;
    *pv = n;
    return 4;
}

PB_API size_t pb_readfixed64(pb_Slice *s, uint64_t *pv) {
    int i;
    uint64_t n = 0;
    if (s->p + 8 > s->end)
        return 0;
    for (i = 7; i >= 0; --i) {
        n <<= 8;
        n |= s->p[i] & 0xFF;
    }
    s->p += 8;
    *pv = n;
    return 8;
}

PB_API size_t pb_readslice(pb_Slice *s, size_t len, pb_Slice *pv) {
    if (pb_len(*s) < len)
        return 0;
    pv->start = s->start;
    pv->p     = s->p;
    pv->end   = s->p + len;
    s->p = pv->end;
    return len;
}

PB_API size_t pb_readbytes(pb_Slice *s, pb_Slice *pv) {
    const char *p = s->p;
    uint64_t len;
    if (pb_readvarint64(s, &len) == 0 || pb_len(*s) < len) {
        s->p = p;
        return 0;
    }
    pv->start = s->start;
    pv->p   = s->p;
    pv->end = s->p + len;
    s->p = pv->end;
    return s->p - p;
}

PB_API size_t pb_readgroup(pb_Slice *s, uint32_t tag, pb_Slice *pv) {
    const char *p = s->p;
    uint32_t newtag = 0;
    size_t count;
    assert(pb_gettype(tag) == PB_TGSTART);
    while ((count = pb_readvarint32(s, &newtag)) != 0) {
        if (pb_gettype(newtag) == PB_TGEND) {
            if (pb_gettag(newtag) != pb_gettag(tag))
                break;
            pv->start = s->start;
            pv->p = p;
            pv->end = s->p - count;
            return s->p - p;
        }
        if (pb_skipvalue(s, newtag) == 0) break;
    }
    s->p = p;
    return 0;
}

PB_API size_t pb_skipvalue(pb_Slice *s, uint32_t tag) {
    const char *p = s->p;
    size_t ret = 0;
    pb_Slice data;
    switch (pb_gettype(tag)) {
    default: break;
    case PB_TVARINT: ret = pb_skipvarint(s); break;
    case PB_T64BIT:  ret = pb_skipslice(s, 8); break;
    case PB_TBYTES:  ret = pb_skipbytes(s); break;
    case PB_T32BIT:  ret = pb_skipslice(s, 4); break;
    case PB_TGSTART: ret = pb_readgroup(s, tag, &data); break;
    }
    if (!ret) s->p = p;
    return ret;
}

PB_API size_t pb_skipvarint(pb_Slice *s) {
    const char *p = s->p, *op = p;
    while (p < s->end && (*p & 0x80) != 0) ++p;
    if (p >= s->end) return 0;
    s->p = ++p;
    return p - op;
}

PB_API size_t pb_skipbytes(pb_Slice *s) {
    const char *p = s->p;
    uint64_t var;
    if (!pb_readvarint64(s, &var)) return 0;
    if (pb_len(*s) < var) {
        s->p = p;
        return 0;
    }
    s->p += var;
    return s->p - p;
}

PB_API size_t pb_skipslice(pb_Slice *s, size_t len) {
    if (s->p + len > s->end) return 0;
    s->p += len;
    return len;
}

PB_API int pb_wtypebytype(int type) {
    switch (type) {
    case PB_Tdouble:    return PB_T64BIT;
    case PB_Tfloat:     return PB_T32BIT;
    case PB_Tint64:     return PB_TVARINT;
    case PB_Tuint64:    return PB_TVARINT;
    case PB_Tint32:     return PB_TVARINT;
    case PB_Tfixed64:   return PB_T64BIT;
    case PB_Tfixed32:   return PB_T32BIT;
    case PB_Tbool:      return PB_TVARINT;
    case PB_Tstring:    return PB_TBYTES;
    case PB_Tmessage:   return PB_TBYTES;
    case PB_Tbytes:     return PB_TBYTES;
    case PB_Tuint32:    return PB_TVARINT;
    case PB_Tenum:      return PB_TVARINT;
    case PB_Tsfixed32:  return PB_T32BIT;
    case PB_Tsfixed64:  return PB_T64BIT;
    case PB_Tsint32:    return PB_TVARINT;
    case PB_Tsint64:    return PB_TVARINT;
    default:            return PB_TWIRECOUNT;
    }
}

PB_API const char *pb_wtypename(int wiretype, const char *def) {
    switch (wiretype) {
#define X(id,name,fmt) case PB_T##id: return name;
        PB_WIRETYPES(X)
#undef  X
    default: return def ? def : "<unknown>";
    }
}

PB_API const char *pb_typename(int type, const char *def) {
    switch (type) {
#define X(name,type,fmt) case PB_T##name: return #name;
        PB_TYPES(X)
#undef  X
    default: return def ? def : "<unknown>";
    }
}

PB_API int pb_typebyname(const char *name, int def) {
    static struct entry { const char *name; int value; } names[] = {
#define X(name,type,fmt) { #name, PB_T##name },
        PB_TYPES(X)
#undef  X
        { NULL, 0 }
    };
    struct entry *p;
    for (p = names; p->name != NULL; ++p)
        if (strcmp(p->name, name) == 0)
            return p->value;
    return def;
}

PB_API int pb_wtypebyname(const char *name, int def) {
    static struct entry { const char *name; int value; } names[] = {
#define X(id,name,fmt) { name, PB_T##id },
        PB_WIRETYPES(X)
#undef  X
        { NULL, 0 }
    };
    struct entry *p;
    for (p = names; p->name != NULL; ++p)
        if (strcmp(p->name, name) == 0)
            return p->value;
    return def;
}


/* encode */

PB_API pb_Slice pb_result(const pb_Buffer *b)
{ pb_Slice slice = pb_lslice(pb_buffer(b), b->size); return slice; }

PB_API void pb_initbuffer(pb_Buffer *b)
{ memset(b, 0, sizeof(pb_Buffer)); }

PB_API void pb_resetbuffer(pb_Buffer *b)
{ if (pb_onheap(b)) free(b->u.h.buff); pb_initbuffer(b); }

static int pb_write32(char *buff, uint32_t n) {
    int p, c = 0;
    do {
        p = n & 0x7F; if ((n >>= 7) == 0) break; *buff++ = p | 0x80, ++c;
        p = n & 0x7F; if ((n >>= 7) == 0) break; *buff++ = p | 0x80, ++c;
        p = n & 0x7F; if ((n >>= 7) == 0) break; *buff++ = p | 0x80, ++c;
        p = n & 0x7F; if ((n >>= 7) == 0) break; *buff++ = p | 0x80, ++c;
        p = n;
    } while (0);
    return *buff++ = p, ++c;
}

static int pb_write64(char *buff, uint64_t n) {
    int p, c = 0;
    do {
        p = n & 0x7F; if ((n >>= 7) == 0) break; *buff++ = p | 0x80, ++c;
        p = n & 0x7F; if ((n >>= 7) == 0) break; *buff++ = p | 0x80, ++c;
        p = n & 0x7F; if ((n >>= 7) == 0) break; *buff++ = p | 0x80, ++c;
        p = n & 0x7F; if ((n >>= 7) == 0) break; *buff++ = p | 0x80, ++c;
        p = n & 0x7F; if ((n >>= 7) == 0) break; *buff++ = p | 0x80, ++c;
        p = n & 0x7F; if ((n >>= 7) == 0) break; *buff++ = p | 0x80, ++c;
        p = n & 0x7F; if ((n >>= 7) == 0) break; *buff++ = p | 0x80, ++c;
        p = n & 0x7F; if ((n >>= 7) == 0) break; *buff++ = p | 0x80, ++c;
        p = n & 0x7F; if ((n >>= 7) == 0) break; *buff++ = p | 0x80, ++c;
        p = n & 0x7F;
    } while (0);
    return *buff++ = p, ++c;
}

PB_API char *pb_prepbuffsize(pb_Buffer *b, size_t len) {
    size_t capacity = pb_onheap(b) ? b->u.h.capacity : PB_SSO_SIZE;
    if (b->size + len > capacity) {
        char *newp, *oldp = pb_onheap(b) ? b->u.h.buff : NULL;
        size_t expected = b->size + len;
        size_t newsize  = PB_SSO_SIZE;
        while (newsize < PB_MAX_SIZET/2 && newsize < expected)
            newsize += newsize >> 1;
        if (newsize < expected) return NULL;
        if ((newp = (char*)realloc(oldp, newsize)) == NULL) return NULL;
        if (!pb_onheap(b)) memcpy(newp, pb_buffer(b), b->size);
        b->heap         = 1;
        b->u.h.buff     = newp;
        b->u.h.capacity = (unsigned)newsize;
    }
    return &pb_buffer(b)[b->size];
}

PB_API size_t pb_addslice(pb_Buffer *b, pb_Slice s) {
    size_t len = pb_len(s);
    char *buff = pb_prepbuffsize(b, len);
    if (buff == NULL) return 0;
    memcpy(buff, s.p, len);
    pb_addsize(b, len);
    return len;
}

PB_API size_t pb_addlength(pb_Buffer *b, size_t len) {
    char buff[10], *s;
    size_t bl, ml;
    if ((bl = pb_bufflen(b)) < len)
        return 0;
    ml = pb_write64(buff, bl - len);
    if (pb_prepbuffsize(b, ml) == NULL) return 0;
    s = pb_buffer(b) + len;
    memmove(s+ml, s, bl - len);
    memcpy(s, buff, ml);
    pb_addsize(b, ml);
    return ml;
}

PB_API size_t pb_addbytes(pb_Buffer *b, pb_Slice s) {
    size_t ret, len = pb_len(s);
    if (pb_prepbuffsize(b, len+5) == NULL) return 0;
    ret = pb_addvarint32(b, (uint32_t)len);
    return ret + pb_addslice(b, s);
}

PB_API size_t pb_addvarint32(pb_Buffer *b, uint32_t n) {
    char *buff = pb_prepbuffsize(b, 5);
    size_t l;
    if (buff == NULL) return 0;
    pb_addsize(b, l = pb_write32(buff, n));
    return l;
}

PB_API size_t pb_addvarint64(pb_Buffer *b, uint64_t n) {
    char *buff = pb_prepbuffsize(b, 10);
    size_t l;
    if (buff == NULL) return 0;
    pb_addsize(b, l = pb_write64(buff, n));
    return l;
}

PB_API size_t pb_addfixed32(pb_Buffer *b, uint32_t n) {
    char *ch = pb_prepbuffsize(b, 4);
    if (ch == NULL) return 0;
    *ch++ = n & 0xFF; n >>= 8;
    *ch++ = n & 0xFF; n >>= 8;
    *ch++ = n & 0xFF; n >>= 8;
    *ch   = n & 0xFF;
    pb_addsize(b, 4);
    return 4;
}

PB_API size_t pb_addfixed64(pb_Buffer *b, uint64_t n) {
    char *ch = pb_prepbuffsize(b, 8);
    if (ch == NULL) return 0;
    *ch++ = n & 0xFF; n >>= 8;
    *ch++ = n & 0xFF; n >>= 8;
    *ch++ = n & 0xFF; n >>= 8;
    *ch++ = n & 0xFF; n >>= 8;
    *ch++ = n & 0xFF; n >>= 8;
    *ch++ = n & 0xFF; n >>= 8;
    *ch++ = n & 0xFF; n >>= 8;
    *ch   = n & 0xFF;
    pb_addsize(b, 8);
    return 8;
}


/* memory pool */

PB_API void pb_initpool(pb_Pool *pool, size_t obj_size) {
    memset(pool, 0, sizeof(pb_Pool));
    pool->obj_size = obj_size;
    assert(obj_size > sizeof(void*) && obj_size < PB_POOLSIZE/4);
}

PB_API void pb_freepool(pb_Pool *pool) {
    void *page = pool->pages;
    while (page) {
        void *next = *(void**)((char*)page + PB_POOLSIZE - sizeof(void*));
        free(page);
        page = next;
    }
    pb_initpool(pool, pool->obj_size);
}

PB_API void *pb_poolalloc(pb_Pool *pool) {
    void *obj = pool->freed;
    if (obj == NULL) {
        size_t objsize = pool->obj_size, offset;
        void *newpage = malloc(PB_POOLSIZE);
        if (newpage == NULL) return NULL;
        offset = ((PB_POOLSIZE - sizeof(void*)) / objsize - 1) * objsize;
        for (; offset > 0; offset -= objsize) {
            void **entry = (void**)((char*)newpage + offset);
            *entry = pool->freed, pool->freed = (void*)entry;
        }
        *(void**)((char*)newpage + PB_POOLSIZE - sizeof(void*)) = pool->pages;
        pool->pages = newpage;
        return newpage;
    }
    pool->freed = *(void**)obj;
    return obj;
}

PB_API void pb_poolfree(pb_Pool *pool, void *obj)
{ *(void**)obj = pool->freed, pool->freed = obj; }


/* hash table */

#define pbT_offset(a,b) ((char*)(a) - (char*)(b))
#define pbT_index(a,b)  ((pb_Entry*)((char*)(a) + (b)))

PB_API void pb_inittable(pb_Table *t, size_t entrysize)
{ memset(t, 0, sizeof(pb_Table)), t->entry_size = (unsigned)entrysize; }

PB_API void pb_freetable(pb_Table *t)
{ free(t->hash); pb_inittable(t, t->entry_size); }

static pb_Entry *pbT_hash(const pb_Table *t, pb_Key key) {
    size_t h = ((size_t)key*2654435761U)&(t->size-1);
    if (key && h == 0) h = 1;
    return pbT_index(t->hash, h*t->entry_size);
}

static pb_Entry *pbT_newkey(pb_Table *t, pb_Key key) {
    pb_Entry *mp, *on, *next, *f = NULL;
    if (t->size == 0 && pb_resizetable(t, (size_t)t->size*2) == 0) return NULL;
    if (key == 0) {
        mp = t->hash;
        t->has_zero = 1;
    } else if ((mp = pbT_hash(t, key))->key != 0) {
        while (t->lastfree > t->entry_size) {
            pb_Entry *cur = pbT_index(t->hash, t->lastfree -= t->entry_size);
            if (cur->key == 0 && cur->next == 0) { f = cur; break; }
        }
        if (f == NULL) return pb_resizetable(t, (size_t)t->size*2u) ?
            pbT_newkey(t, key) : NULL;
        if ((on = pbT_hash(t, mp->key)) != mp) {
            while ((next = pbT_index(on, on->next)) != mp) on = next;
            on->next = pbT_offset(f, on);
            memcpy(f, mp, t->entry_size);
            if (mp->next != 0) f->next += pbT_offset(mp, f), mp->next = 0;
        } else {
            if (mp->next != 0) f->next = pbT_offset(mp, f) + mp->next;
            else assert(f->next == 0);
            mp->next = pbT_offset(f, mp);
            mp = f;
        }
    }
    mp->key = key;
    if (t->entry_size != sizeof(pb_Entry))
        memset(mp+1, 0, t->entry_size - sizeof(pb_Entry));
    return mp;
}

PB_API size_t pb_resizetable(pb_Table *t, size_t size) {
    pb_Table nt = *t;
    unsigned i, rawsize = t->size*t->entry_size;
    unsigned newsize = PB_MIN_HASHTABLE_SIZE;
    while (newsize < PB_MAX_HASHSIZE/t->entry_size && newsize < size)
        newsize <<= 1;
    if (newsize < size) return 0;
    nt.size     = newsize;
    nt.lastfree = nt.entry_size * newsize;
    nt.hash     = (pb_Entry*)malloc(nt.lastfree);
    if (nt.hash == NULL) return 0;
    memset(nt.hash, 0, nt.lastfree);
    for (i = 0; i < rawsize; i += t->entry_size) {
        pb_Entry *olde = (pb_Entry*)((char*)t->hash + i);
        pb_Entry *newe = pbT_newkey(&nt, olde->key);
        if (nt.entry_size > sizeof(pb_Entry))
            memcpy(newe+1, olde+1, nt.entry_size - sizeof(pb_Entry));
    }
    free(t->hash);
    *t = nt;
    return newsize;
}

PB_API pb_Entry *pb_gettable(const pb_Table *t, pb_Key key) {
    pb_Entry *entry;
    if (t == NULL || t->size == 0)
        return NULL;
    if (key == 0)
        return t->has_zero ? t->hash : NULL;
    for (entry = pbT_hash(t, key);
            entry->key != key;
            entry = pbT_index(entry, entry->next))
        if (entry->next == 0) return NULL;
    return entry;
}

PB_API pb_Entry *pb_settable(pb_Table *t, pb_Key key) {
    pb_Entry *entry;
    if ((entry = pb_gettable(t, key)) != NULL)
        return entry;
    return pbT_newkey(t, key);
}

PB_API int pb_nextentry(const pb_Table *t, const pb_Entry **pentry) {
    size_t i = *pentry ? pbT_offset(*pentry, t->hash) : 0;
    size_t size = (size_t)t->size*t->entry_size;
    if (*pentry == NULL && t->has_zero) {
        *pentry = t->hash;
        return 1;
    }
    while (i += t->entry_size, i < size) {
        pb_Entry *entry = pbT_index(t->hash, i);
        if (entry->key != 0) {
            *pentry = entry;
            return 1;
        }
    }
    *pentry = NULL;
    return 0;
}


/* name table */

static void pbN_init(pb_State *S)
{ memset(&S->nametable, 0, sizeof(pb_NameTable)); }

PB_API pb_Name *pb_usename(pb_Name *name)
{ if (name != NULL) ++((pb_NameEntry*)name-1)->refcount; return name; }

static void pbN_free(pb_State *S) {
    pb_NameTable *nt = &S->nametable;
    size_t i;
    for (i = 0; i < nt->size; ++i) {
        pb_NameEntry *ne = nt->hash[i];
        while (ne != NULL) {
            pb_NameEntry *next = ne->next;
            free(ne);
            ne = next;
        }
    }
    free(nt->hash);
    pbN_init(S);
}

static unsigned pbN_calchash(pb_Slice s) {
    size_t len = pb_len(s);
    unsigned h = (unsigned)len;
    size_t step = (len >> PB_HASHLIMIT) + 1;
    for (; len >= step; len -= step)
        h ^= ((h<<5) + (h>>2) + (unsigned char)(s.p[len - 1]));
    return h;
}

static size_t pbN_resize(pb_State *S, size_t size) {
    pb_NameTable *nt = &S->nametable;
    pb_NameEntry **hash;
    size_t i, newsize = PB_MIN_STRTABLE_SIZE;
    while (newsize < PB_MAX_HASHSIZE/sizeof(pb_NameEntry*) && newsize < size)
        newsize <<= 1;
    if (newsize < size) return 0;
    hash = (pb_NameEntry**)malloc(newsize * sizeof(pb_NameEntry*));
    if (hash == NULL) return 0;
    memset(hash, 0, newsize * sizeof(pb_NameEntry*));
    for (i = 0; i < nt->size; ++i) {
        pb_NameEntry *entry = nt->hash[i];
        while (entry != NULL) {
            pb_NameEntry *next = entry->next;
            pb_NameEntry **newh = &hash[entry->hash & (newsize - 1)];
            entry->next = *newh, *newh = entry;
            entry = next;
        }
    }
    free(nt->hash);
    nt->hash = hash;
    nt->size = newsize;
    return newsize;
}

static pb_NameEntry *pbN_newname(pb_State *S, pb_Slice s, unsigned hash) {
    pb_NameTable *nt = &S->nametable;
    pb_NameEntry **list, *newobj;
    size_t len = pb_len(s);
    if (nt->count >= nt->size && !pbN_resize(S, nt->size * 2)) return NULL;
    list = &nt->hash[hash & (nt->size - 1)];
    newobj = (pb_NameEntry*)malloc(sizeof(pb_NameEntry) + len + 1);
    if (newobj == NULL) return NULL;
    newobj->next     = *list;
    newobj->length   = (unsigned)len;
    newobj->refcount = 0;
    newobj->hash     = hash;
    memcpy(newobj+1, s.p, len);
    ((char*)(newobj+1))[len] = '\0';
    *list = newobj;
    ++nt->count;
    return newobj;
}

static void pbN_delname(pb_State *S, pb_NameEntry *name) {
    pb_NameTable *nt = &S->nametable;
    pb_NameEntry **list = &nt->hash[name->hash & (nt->size - 1)];
    while (*list != NULL) {
        if (*list != name)
            list = &(*list)->next;
        else {
            *list = (*list)->next;
            --nt->count;
            free(name);
            break;
        }
    }
}

static pb_NameEntry *pbN_getname(const pb_State *S, pb_Slice s, unsigned hash) {
    const pb_NameTable *nt = &S->nametable;
    size_t len = pb_len(s);
    if (nt->hash) {
        pb_NameEntry *entry = nt->hash[hash & (nt->size - 1)];
        for (; entry != NULL; entry = entry->next)
            if (entry->hash == hash && entry->length == len
                    && memcmp(s.p, entry + 1, len) == 0)
                return entry;
    }
    return NULL;
}

PB_API void pb_delname(pb_State *S, pb_Name *name) {
    if (name != NULL) {
        pb_NameEntry *ne = (pb_NameEntry*)name - 1;
        if (ne->refcount <= 1) { pbN_delname(S, ne); return; }
        --ne->refcount;
    }
}

PB_API pb_Name *pb_newname(pb_State *S, pb_Slice s, pb_Cache *cache) {
    pb_NameEntry *entry;
    if (s.p == NULL) return NULL;
    (void)cache;
    assert(cache == NULL);
    /* if (cache == NULL) */{
        unsigned hash = pbN_calchash(s);
        entry = pbN_getname(S, s, hash);
        if (entry == NULL) entry = pbN_newname(S, s, hash);
    }/* else {
        pb_Name *name = (pb_Name*)pb_name(S, s, cache);
        if (name) return pb_usename(name);
        entry = pbN_newname(S, s, cache->hash);
    }*/
    return entry ? pb_usename((pb_Name*)(entry + 1)) : NULL;
}

PB_API const pb_Name *pb_name(const pb_State *S, pb_Slice s, pb_Cache *cache) {
    pb_NameEntry *entry = NULL;
    pb_CacheSlot *slot;
    if (s.p == NULL) return NULL;
    if (cache == NULL)
        entry = pbN_getname(S, s, pbN_calchash(s));
    else {
        slot = cache->slots[((uintptr_t)s.p*2654435761U)%PB_CACHE_SIZE];
        if (slot[0].name == s.p)
            entry = pbN_getname(S, s, cache->hash = slot[0].hash);
        else if (slot[1].name == s.p)
            entry = pbN_getname(S, s, cache->hash = (++slot)[0].hash);
        else
            slot[1] = slot[0], slot[0].name = s.p;
        if (entry == NULL) {
            cache->hash = slot[0].hash = pbN_calchash(s);
            entry = pbN_getname(S, s, slot[0].hash);
        }
    }
    return entry ? (pb_Name*)(entry + 1) : NULL;
}


/* state */

typedef struct pb_TypeEntry { pb_Entry entry; pb_Type *value; } pb_TypeEntry;
typedef struct pb_FieldEntry { pb_Entry entry; pb_Field *value; } pb_FieldEntry;

typedef struct pb_OneofEntry {
    pb_Entry entry;
    pb_Name *name;
    unsigned index;
} pb_OneofEntry;

PB_API void pb_init(pb_State *S) {
    memset(S, 0, sizeof(pb_State));
    S->types.entry_size = sizeof(pb_TypeEntry);
    pb_initpool(&S->typepool, sizeof(pb_Type));
    pb_initpool(&S->fieldpool, sizeof(pb_Field));
}

PB_API void pb_free(pb_State *S) {
    const pb_TypeEntry *te = NULL;
    if (S == NULL) return;
    while (pb_nextentry(&S->types, (const pb_Entry**)&te))
        if (te->value != NULL) pb_deltype(S, te->value);
    pb_freetable(&S->types);
    pb_freepool(&S->typepool);
    pb_freepool(&S->fieldpool);
    pbN_free(S);
}

PB_API const pb_Type *pb_type(const pb_State *S, const pb_Name *tname) {
    pb_TypeEntry *te = NULL;
    if (S != NULL && tname != NULL)
        te = (pb_TypeEntry*)pb_gettable(&S->types, (pb_Key)tname);
    return te && !te->value->is_dead ? te->value : NULL;
}

PB_API const pb_Field *pb_fname(const pb_Type *t, const pb_Name *name) {
    pb_FieldEntry *fe = NULL;
    if (t != NULL && name != NULL)
        fe = (pb_FieldEntry*)pb_gettable(&t->field_names, (pb_Key)name);
    return fe ? fe->value : NULL;
}

PB_API const pb_Field *pb_field(const pb_Type *t, int32_t number) {
    pb_FieldEntry *fe = NULL;
    if (t != NULL) fe = (pb_FieldEntry*)pb_gettable(&t->field_tags, number);
    return fe ? fe->value : NULL;
}


static int comp_field(const void* a, const void* b) {
    return (*(const pb_Field**)a)->number - (*(const pb_Field**)b)->number;
}

PB_API pb_Field** pb_sortfield(pb_Type* t) {
    if (!t->field_sort && t->field_count) {
        int index = 0;
        unsigned int i = 0;
        const pb_Field* f = NULL;
        pb_Field** list = (pb_Field**)malloc(sizeof(pb_Field*) * t->field_count);

        assert(list);
        while (pb_nextfield(t, &f)) {
            list[index++] = (pb_Field*)f;
        }

        qsort(list, index, sizeof(pb_Field*), comp_field);
        for (i = 0; i < t->field_count; i++) {
            list[i]->sort_index = i + 1;
        }
        t->field_sort = list;
    }

    return t->field_sort;
}

PB_API const pb_Name *pb_oneofname(const pb_Type *t, int idx) {
    pb_OneofEntry *oe = NULL;
    if (t != NULL) oe = (pb_OneofEntry*)pb_gettable(&t->oneof_index, idx);
    return oe ? oe->name : NULL;
}

PB_API int pb_nexttype(const pb_State *S, const pb_Type **ptype) {
    const pb_TypeEntry *e = NULL;
    if (S != NULL) {
        if (*ptype != NULL)
            e = (pb_TypeEntry*)pb_gettable(&S->types, (pb_Key)(*ptype)->name);
        while (pb_nextentry(&S->types, (const pb_Entry**)&e))
            if ((*ptype = e->value) != NULL && !(*ptype)->is_dead)
                return 1;
    }
    *ptype = NULL;
    return 0;
}

PB_API int pb_nextfield(const pb_Type *t, const pb_Field **pfield) {
    const pb_FieldEntry *e = NULL;
    if (t != NULL) {
        if (*pfield != NULL)
            e = (pb_FieldEntry*)pb_gettable(&t->field_tags, (*pfield)->number);
        while (pb_nextentry(&t->field_tags, (const pb_Entry**)&e))
            if ((*pfield = e->value) != NULL)
                return 1;
    }
    *pfield = NULL;
    return 0;
}


/* new type/field */

static const char *pbT_basename(const char *tname) {
    const char *end = tname + strlen(tname);
    while (tname < end && *--end != '.')
        ;
    return *end != '.' ? end : end + 1;
}

static void pbT_inittype(pb_Type *t) {
    memset(t, 0, sizeof(pb_Type));
    pb_inittable(&t->field_names, sizeof(pb_FieldEntry));
    pb_inittable(&t->field_tags, sizeof(pb_FieldEntry));
    pb_inittable(&t->oneof_index, sizeof(pb_OneofEntry));
}

static void pbT_freefield(pb_State *S, pb_Field *f) {
    pb_delname(S, f->default_value);
    pb_delname(S, f->name);
    pb_poolfree(&S->fieldpool, f);
}

PB_API pb_Type *pb_newtype(pb_State *S, pb_Name *tname) {
    pb_TypeEntry *te;
    pb_Type *t;
    if (tname == NULL) return NULL;
    te = (pb_TypeEntry*)pb_settable(&S->types, (pb_Key)tname);
    if (te == NULL) return NULL;
    if ((t = te->value) != NULL) return t->is_dead = 0, t;
    if (!(t = (pb_Type*)pb_poolalloc(&S->typepool))) return NULL;
    pbT_inittype(t);
    t->name = tname;
    t->basename = pbT_basename((const char*)tname);
    return te->value = t;
}

PB_API void pb_delsort(pb_Type *t) {
    if (t->field_sort) {
        free(t->field_sort);
        t->field_sort = NULL;
    }
}

PB_API void pb_deltype(pb_State *S, pb_Type *t) {
    pb_FieldEntry *nf = NULL;
    pb_OneofEntry *ne = NULL;
    if (S == NULL || t == NULL) return;
    while (pb_nextentry(&t->field_names, (const pb_Entry**)&nf)) {
        if (nf->value != NULL) {
            pb_FieldEntry *of = (pb_FieldEntry*)pb_gettable(
                    &t->field_tags, nf->value->number);
            if (of && of->value == nf->value)
                of->entry.key = 0, of->value = NULL;
            pbT_freefield(S, nf->value);
        }
    }
    while (pb_nextentry(&t->field_tags, (const pb_Entry**)&nf))
        if (nf->value != NULL) pbT_freefield(S, nf->value);
    while (pb_nextentry(&t->oneof_index, (const pb_Entry**)&ne))
        pb_delname(S, ne->name);
    pb_freetable(&t->field_tags);
    pb_freetable(&t->field_names);
    pb_freetable(&t->oneof_index);
    t->oneof_field = 0, t->field_count = 0;
    t->is_dead = 1;
    pb_delsort(t);
    /*pb_delname(S, t->name); */
    /*pb_poolfree(&S->typepool, t); */
}

PB_API pb_Field *pb_newfield(pb_State *S, pb_Type *t, pb_Name *fname, int32_t number) {
    pb_FieldEntry *nf, *tf;
    pb_Field *f;
    if (fname == NULL) return NULL;
    nf = (pb_FieldEntry*)pb_settable(&t->field_names, (pb_Key)fname);
    tf = (pb_FieldEntry*)pb_settable(&t->field_tags, number);
    if (nf == NULL || tf == NULL) return NULL;
    if ((f = nf->value) != NULL && tf->value == f) {
        pb_delname(S, f->default_value);
        f->default_value = NULL;
        return f;
    }
    if (!(f = (pb_Field*)pb_poolalloc(&S->fieldpool))) return NULL;
    memset(f, 0, sizeof(pb_Field));
    f->name   = fname;
    f->type   = t;
    f->number = number;
    if (nf->value && pb_field(t, nf->value->number) != nf->value)
        pbT_freefield(S, nf->value), --t->field_count;
    if (tf->value && pb_fname(t, tf->value->name) != tf->value)
        pbT_freefield(S, tf->value), --t->field_count;
    ++t->field_count;
    pb_delsort(t);
    return nf->value = tf->value = f;
}

PB_API void pb_delfield(pb_State *S, pb_Type *t, pb_Field *f) {
    pb_FieldEntry *nf, *tf;
    int count = 0;
    if (S == NULL || t == NULL || f == NULL) return;
    nf = (pb_FieldEntry*)pb_gettable(&t->field_names, (pb_Key)f->name);
    tf = (pb_FieldEntry*)pb_gettable(&t->field_tags, (pb_Key)f->number);
    if (nf && nf->value == f) nf->entry.key = 0, nf->value = NULL, ++count;
    if (tf && tf->value == f) tf->entry.key = 0, tf->value = NULL, ++count;
    if (count) pbT_freefield(S, f), --t->field_count;
    pb_delsort(t);
}


/* .pb proto loader */

typedef struct pb_Loader         pb_Loader;
typedef struct pbL_FieldInfo     pbL_FieldInfo;
typedef struct pbL_EnumValueInfo pbL_EnumValueInfo;
typedef struct pbL_EnumInfo      pbL_EnumInfo;
typedef struct pbL_TypeInfo      pbL_TypeInfo;
typedef struct pbL_FileInfo      pbL_FileInfo;

#define pbC(e)  do { int r = (e); if (r != PB_OK) return r; } while (0)
#define pbCM(e) do { if ((e) == NULL) return PB_ENOMEM; } while (0)
#define pbCE(e) do { if ((e) == NULL) return PB_ERROR;  } while (0)

typedef struct pb_ArrayHeader {
    unsigned count;
    unsigned capacity;
} pb_ArrayHeader;

#define pbL_rawh(A)   ((pb_ArrayHeader*)(A) - 1)
#define pbL_delete(A) ((A) ? (void)free(pbL_rawh(A)) : (void)0)
#define pbL_count(A)  ((A) ? pbL_rawh(A)->count    : 0)
#define pbL_add(A)    (pbL_grow((void**)&(A),sizeof(*(A)))==PB_OK ?\
                       &(A)[pbL_rawh(A)->count++] : NULL)

struct pb_Loader {
    pb_Slice  s;
    pb_Buffer b;
    int       is_proto3;
};

/* parsers */

struct pbL_EnumValueInfo {
    pb_Slice name;
    int32_t  number;
};

struct pbL_EnumInfo {
    pb_Slice           name;
    pbL_EnumValueInfo *value;
};

struct pbL_FieldInfo {
    pb_Slice name;
    pb_Slice type_name;
    pb_Slice extendee;
    pb_Slice default_value;
    int32_t  number;
    int32_t  label;
    int32_t  type;
    int32_t  oneof_index;
    int32_t  packed;
};

struct pbL_TypeInfo {
    pb_Slice       name;
    int32_t        is_map;
    pbL_FieldInfo *field;
    pbL_FieldInfo *extension;
    pbL_EnumInfo  *enum_type;
    pbL_TypeInfo  *nested_type;
    pb_Slice      *oneof_decl;
};

struct pbL_FileInfo {
    pb_Slice       package;
    pb_Slice       syntax;
    pbL_EnumInfo  *enum_type;
    pbL_TypeInfo  *message_type;
    pbL_FieldInfo *extension;
};

static int pbL_readbytes(pb_Loader *L, pb_Slice *pv)
{ return pb_readbytes(&L->s, pv) == 0 ? PB_ERROR : PB_OK; }

static int pbL_beginmsg(pb_Loader *L, pb_Slice *pv)
{ pb_Slice v; pbC(pbL_readbytes(L, &v)); *pv = L->s, L->s = v; return PB_OK; }

static void pbL_endmsg(pb_Loader *L, pb_Slice *pv)
{ L->s = *pv; }

static int pbL_grow(void **pp, size_t objs) {
    pb_ArrayHeader *nh, *h = *pp ? pbL_rawh(*pp) : NULL;
    if (h == NULL || h->capacity <= h->count) {
        size_t used = (h ? h->count : 0);
        size_t size = used + 4, nsize = size + (size >> 1);
        nh = nsize < size ? NULL :
            (pb_ArrayHeader*)realloc(h, sizeof(pb_ArrayHeader)+nsize*objs);
        if (nh == NULL) return PB_ENOMEM;
        nh->count    = (unsigned)used;
        nh->capacity = (unsigned)nsize;
        *pp = nh + 1;
        memset((char*)*pp + used*objs, 0, (nsize - used)*objs);
    }
    return PB_OK;
}

static int pbL_readint32(pb_Loader *L, int32_t *pv) {
    uint32_t v;
    if (pb_readvarint32(&L->s, &v) == 0) return PB_ERROR;
    *pv = (int32_t)v;
    return PB_OK;
}

static int pbL_FieldOptions(pb_Loader *L, pbL_FieldInfo *info) {
    pb_Slice s;
    uint32_t tag;
    pbC(pbL_beginmsg(L, &s));
    while (pb_readvarint32(&L->s, &tag)) {
        switch (tag) {
        case pb_pair(2, PB_TVARINT): /* bool packed */
            pbC(pbL_readint32(L, &info->packed)); break;
        default: if (pb_skipvalue(&L->s, tag) == 0) return PB_ERROR;
        }
    }
    pbL_endmsg(L, &s);
    return PB_OK;
}

static int pbL_FieldDescriptorProto(pb_Loader *L, pbL_FieldInfo *info) {
    pb_Slice s;
    uint32_t tag;
    pbCM(info); pbC(pbL_beginmsg(L, &s));
    info->packed = -1;
    while (pb_readvarint32(&L->s, &tag)) {
        switch (tag) {
        case pb_pair(1, PB_TBYTES): /* string name */
            pbC(pbL_readbytes(L, &info->name)); break;
        case pb_pair(3, PB_TVARINT): /* int32 number */
            pbC(pbL_readint32(L, &info->number)); break;
        case pb_pair(4, PB_TVARINT): /* Label label */
            pbC(pbL_readint32(L, &info->label)); break;
        case pb_pair(5, PB_TVARINT): /* Type type */
            pbC(pbL_readint32(L, &info->type)); break;
        case pb_pair(6, PB_TBYTES): /* string type_name */
            pbC(pbL_readbytes(L, &info->type_name)); break;
        case pb_pair(2, PB_TBYTES): /* string extendee */
            pbC(pbL_readbytes(L, &info->extendee)); break;
        case pb_pair(7, PB_TBYTES): /* string default_value */
            pbC(pbL_readbytes(L, &info->default_value)); break;
        case pb_pair(8, PB_TBYTES): /* FieldOptions options */
            pbC(pbL_FieldOptions(L, info)); break;
        case pb_pair(9, PB_TVARINT): /* int32 oneof_index */
            pbC(pbL_readint32(L, &info->oneof_index));
            ++info->oneof_index; break;
        default: if (pb_skipvalue(&L->s, tag) == 0) return PB_ERROR;
        }
    }
    pbL_endmsg(L, &s);
    return PB_OK;
}

static int pbL_EnumValueDescriptorProto(pb_Loader *L, pbL_EnumValueInfo *info) {
    pb_Slice s;
    uint32_t tag;
    pbCM(info); pbC(pbL_beginmsg(L, &s));
    while (pb_readvarint32(&L->s, &tag)) {
        switch (tag) {
        case pb_pair(1, PB_TBYTES): /* string name */
            pbC(pbL_readbytes(L, &info->name)); break;
        case pb_pair(2, PB_TVARINT): /* int32 number */
            pbC(pbL_readint32(L, &info->number)); break;
        default: if (pb_skipvalue(&L->s, tag) == 0) return PB_ERROR;
        }
    }
    pbL_endmsg(L, &s);
    return PB_OK;
}

static int pbL_EnumDescriptorProto(pb_Loader *L, pbL_EnumInfo *info) {
    pb_Slice s;
    uint32_t tag;
    pbCM(info); pbC(pbL_beginmsg(L, &s));
    while (pb_readvarint32(&L->s, &tag)) {
        switch (tag) {
        case pb_pair(1, PB_TBYTES): /* string name */
            pbC(pbL_readbytes(L, &info->name)); break;
        case pb_pair(2, PB_TBYTES): /* EnumValueDescriptorProto value */
            pbC(pbL_EnumValueDescriptorProto(L, pbL_add(info->value))); break;
        default: if (pb_skipvalue(&L->s, tag) == 0) return PB_ERROR;
        }
    }
    pbL_endmsg(L, &s);
    return PB_OK;
}

static int pbL_MessageOptions(pb_Loader *L, pbL_TypeInfo *info) {
    pb_Slice s;
    uint32_t tag;
    pbCM(info); pbC(pbL_beginmsg(L, &s));
    while (pb_readvarint32(&L->s, &tag)) {
        switch (tag) {
        case pb_pair(7, PB_TVARINT): /* bool map_entry */
            pbC(pbL_readint32(L, &info->is_map)); break;
        default: if (pb_skipvalue(&L->s, tag) == 0) return PB_ERROR;
        }
    }
    pbL_endmsg(L, &s);
    return PB_OK;
}

static int pbL_OneofDescriptorProto(pb_Loader *L, pbL_TypeInfo *info) {
    pb_Slice s;
    uint32_t tag;
    pbCM(info); pbC(pbL_beginmsg(L, &s));
    while (pb_readvarint32(&L->s, &tag)) {
        switch (tag) {
        case pb_pair(1, PB_TBYTES): /* string name */
            pbC(pbL_readbytes(L, pbL_add(info->oneof_decl))); break;
        default: if (pb_skipvalue(&L->s, tag) == 0) return PB_ERROR;
        }
    }
    pbL_endmsg(L, &s);
    return PB_OK;
}

static int pbL_DescriptorProto(pb_Loader *L, pbL_TypeInfo *info) {
    pb_Slice s;
    uint32_t tag;
    pbCM(info); pbC(pbL_beginmsg(L, &s));
    while (pb_readvarint32(&L->s, &tag)) {
        switch (tag) {
        case pb_pair(1, PB_TBYTES): /* string name */
            pbC(pbL_readbytes(L, &info->name)); break;
        case pb_pair(2, PB_TBYTES): /* FieldDescriptorProto field */
            pbC(pbL_FieldDescriptorProto(L, pbL_add(info->field))); break;
        case pb_pair(6, PB_TBYTES): /* FieldDescriptorProto extension */
            pbC(pbL_FieldDescriptorProto(L, pbL_add(info->extension))); break;
        case pb_pair(3, PB_TBYTES): /* DescriptorProto nested_type */
            pbC(pbL_DescriptorProto(L, pbL_add(info->nested_type))); break;
        case pb_pair(4, PB_TBYTES): /* EnumDescriptorProto enum_type */
            pbC(pbL_EnumDescriptorProto(L, pbL_add(info->enum_type))); break;
        case pb_pair(8, PB_TBYTES): /* OneofDescriptorProto oneof_decl */
            pbC(pbL_OneofDescriptorProto(L, info)); break;
        case pb_pair(7, PB_TBYTES): /* MessageOptions options */
            pbC(pbL_MessageOptions(L, info)); break;
        default: if (pb_skipvalue(&L->s, tag) == 0) return PB_ERROR;
        }
    }
    pbL_endmsg(L, &s);
    return PB_OK;
}

static int pbL_FileDescriptorProto(pb_Loader *L, pbL_FileInfo *info) {
    pb_Slice s;
    uint32_t tag;
    pbCM(info); pbC(pbL_beginmsg(L, &s));
    while (pb_readvarint32(&L->s, &tag)) {
        switch (tag) {
        case pb_pair(2, PB_TBYTES): /* string package */
            pbC(pbL_readbytes(L, &info->package)); break;
        case pb_pair(4, PB_TBYTES): /* DescriptorProto message_type */
            pbC(pbL_DescriptorProto(L, pbL_add(info->message_type))); break;
        case pb_pair(5, PB_TBYTES): /* EnumDescriptorProto enum_type */
            pbC(pbL_EnumDescriptorProto(L, pbL_add(info->enum_type))); break;
        case pb_pair(7, PB_TBYTES): /* FieldDescriptorProto extension */
            pbC(pbL_FieldDescriptorProto(L, pbL_add(info->extension))); break;
        case pb_pair(12, PB_TBYTES): /* string syntax */
            pbC(pbL_readbytes(L, &info->syntax)); break;
        default: if (pb_skipvalue(&L->s, tag) == 0) return PB_ERROR;
        }
    }
    pbL_endmsg(L, &s);
    return PB_OK;
}

static int pbL_FileDescriptorSet(pb_Loader *L, pbL_FileInfo **pfiles) {
    uint32_t tag;
    while (pb_readvarint32(&L->s, &tag)) {
        switch (tag) {
        case pb_pair(1, PB_TBYTES): /* FileDescriptorProto file */
            pbC(pbL_FileDescriptorProto(L, pbL_add(*pfiles))); break;
        default: if (pb_skipvalue(&L->s, tag) == 0) return PB_ERROR;
        }
    }
    return PB_OK;
}

/* loader */

static void pbL_delTypeInfo(pbL_TypeInfo *info) {
    size_t i, count;
    for (i = 0, count = pbL_count(info->nested_type); i < count; ++i)
        pbL_delTypeInfo(&info->nested_type[i]);
    for (i = 0, count = pbL_count(info->enum_type); i < count; ++i)
        pbL_delete(info->enum_type[i].value);
    pbL_delete(info->nested_type);
    pbL_delete(info->enum_type);
    pbL_delete(info->field);
    pbL_delete(info->extension);
    pbL_delete(info->oneof_decl);
}

static void pbL_delFileInfo(pbL_FileInfo *files) {
    size_t i, count, j, jcount;
    for (i = 0, count = pbL_count(files); i < count; ++i) {
        for (j = 0, jcount = pbL_count(files[i].message_type); j < jcount; ++j)
            pbL_delTypeInfo(&files[i].message_type[j]);
        for (j = 0, jcount = pbL_count(files[i].enum_type); j < jcount; ++j)
            pbL_delete(files[i].enum_type[j].value);
        pbL_delete(files[i].message_type);
        pbL_delete(files[i].enum_type);
        pbL_delete(files[i].extension);
    }
    pbL_delete(files);
}

static int pbL_prefixname(pb_State *S, pb_Slice s, size_t *ps, pb_Loader *L, pb_Name **out) {
    char *buff;
    *ps = pb_bufflen(&L->b);
    pbCM(buff = pb_prepbuffsize(&L->b, pb_len(s) + 1));
    *buff = '.'; pb_addsize(&L->b, 1);
    if (pb_addslice(&L->b, s) == 0) return PB_ENOMEM;
    if (out) *out = pb_newname(S, pb_result(&L->b), NULL);
    return PB_OK;
}

static int pbL_loadEnum(pb_State *S, pbL_EnumInfo *info, pb_Loader *L) {
    size_t i, count, curr;
    pb_Name *name;
    pb_Type *t;
    pbC(pbL_prefixname(S, info->name, &curr, L, &name));
    pbCM(t = pb_newtype(S, name));
    t->is_enum = 1;
    for (i = 0, count = pbL_count(info->value); i < count; ++i) {
        pbL_EnumValueInfo *ev = &info->value[i];
        pbCE(pb_newfield(S, t, pb_newname(S, ev->name, NULL), ev->number));
    }
    L->b.size = (unsigned)curr;
    return PB_OK;
}

static int pbL_loadField(pb_State *S, pbL_FieldInfo *info, pb_Loader *L, pb_Type *t) {
    pb_Type  *ft = NULL;
    pb_Field *f;
    if (info->type == PB_Tmessage || info->type == PB_Tenum)
        pbCE(ft = pb_newtype(S, pb_newname(S, info->type_name, NULL)));
    if (t == NULL)
        pbCE(t = pb_newtype(S, pb_newname(S, info->extendee, NULL)));
    pbCE(f = pb_newfield(S, t, pb_newname(S, info->name, NULL), info->number));
    f->default_value = pb_newname(S, info->default_value, NULL);
    f->type      = ft;
    if ((f->oneof_idx = info->oneof_index)) ++t->oneof_field;
    f->type_id   = info->type;
    f->repeated  = info->label == 3; /* repeated */
    f->packed    = info->packed >= 0 ? info->packed : L->is_proto3 && f->repeated;
    if (f->type_id >= 9 && f->type_id <= 12) f->packed = 0;
    f->scalar = (f->type == NULL);
    return PB_OK;
}

static int pbL_loadType(pb_State *S, pbL_TypeInfo *info, pb_Loader *L) {
    size_t i, count, curr;
    pb_Name *name;
    pb_Type *t;
    pbC(pbL_prefixname(S, info->name, &curr, L, &name));
    pbCM(t = pb_newtype(S, name));
    t->is_map = info->is_map, t->is_proto3 = L->is_proto3;
    for (i = 0, count = pbL_count(info->oneof_decl); i < count; ++i) {
        pb_OneofEntry *e = (pb_OneofEntry*)pb_settable(&t->oneof_index, i+1);
        pbCM(e); pbCE(e->name = pb_newname(S, info->oneof_decl[i], NULL));
        e->index = (int)i+1;
    }
    for (i = 0, count = pbL_count(info->field); i < count; ++i)
        pbC(pbL_loadField(S, &info->field[i], L, t));
    for (i = 0, count = pbL_count(info->extension); i < count; ++i)
        pbC(pbL_loadField(S, &info->extension[i], L, NULL));
    for (i = 0, count = pbL_count(info->enum_type); i < count; ++i)
        pbC(pbL_loadEnum(S, &info->enum_type[i], L));
    for (i = 0, count = pbL_count(info->nested_type); i < count; ++i)
        pbC(pbL_loadType(S, &info->nested_type[i], L));
    t->oneof_count = pbL_count(info->oneof_decl);
    L->b.size = (unsigned)curr;
    return PB_OK;
}

static int pbL_loadFile(pb_State *S, pbL_FileInfo *info, pb_Loader *L) {
    size_t i, count, j, jcount, curr = 0;
    pb_Name *syntax;
    pbCM(syntax = pb_newname(S, pb_slice("proto3"), NULL));
    for (i = 0, count = pbL_count(info); i < count; ++i) {
        if (info[i].package.p)
            pbC(pbL_prefixname(S, info[i].package, &curr, L, NULL));
        L->is_proto3 = (pb_name(S, info[i].syntax, NULL) == syntax);
        for (j = 0, jcount = pbL_count(info[i].enum_type); j < jcount; ++j)
            pbC(pbL_loadEnum(S, &info[i].enum_type[j], L));
        for (j = 0, jcount = pbL_count(info[i].message_type); j < jcount; ++j)
            pbC(pbL_loadType(S, &info[i].message_type[j], L));
        for (j = 0, jcount = pbL_count(info[i].extension); j < jcount; ++j)
            pbC(pbL_loadField(S, &info[i].extension[j], L, NULL));
        L->b.size = (unsigned)curr;
    }
    return PB_OK;
}

PB_API int pb_load(pb_State *S, pb_Slice *s) {
    pbL_FileInfo *files = NULL;
    pb_Loader L;
    int r;
    pb_initbuffer(&L.b);
    L.s         = *s;
    L.is_proto3 = 0;
    if ((r = pbL_FileDescriptorSet(&L, &files)) == PB_OK)
        r = pbL_loadFile(S, files, &L);
    pbL_delFileInfo(files);
    pb_resetbuffer(&L.b);
    s->p = L.s.p;
    return r;
}


PB_NS_END

#endif /* PB_IMPLEMENTATION */

/* cc: flags+='-shared -DPB_IMPLEMENTATION -xc' output='pb.so' */

