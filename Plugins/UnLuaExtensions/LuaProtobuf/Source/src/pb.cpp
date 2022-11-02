#ifdef _MSC_VER
# define _CRT_SECURE_NO_WARNINGS
# define _CRT_NONSTDC_NO_WARNINGS
# pragma warning(disable: 4244) /* int -> char */
# pragma warning(disable: 4706) /* = in if condition */
# pragma warning(disable: 4709) /* comma in array index */
# pragma warning(disable: 4127) /* const in if condition */
#endif

#define PB_STATIC_API
#include "pb.h"

PB_NS_BEGIN

#include <stdio.h>
#include <errno.h>


/* Lua util routines */

#define PB_STATE     "pb.State"
#define PB_BUFFER    "pb.Buffer"
#define PB_SLICE     "pb.Slice"

#define check_buffer(L,idx) ((pb_Buffer*)luaL_checkudata(L,idx,PB_BUFFER))
#define test_buffer(L,idx)  ((pb_Buffer*)luaL_testudata(L,idx,PB_BUFFER))
#define check_slice(L,idx)  ((pb_Slice*)luaL_checkudata(L,idx,PB_SLICE))
#define test_slice(L,idx)   ((pb_Slice*)luaL_testudata(L,idx,PB_SLICE))
#define push_slice(L,s)     lua_pushlstring((L), (s).p, pb_len((s)))
#define return_self(L) { return lua_settop(L, 1), 1; }

static int lpb_relindex(int idx, int offset) {
    return idx < 0 && idx > LUA_REGISTRYINDEX ? idx - offset : idx;
}

#if LUA_VERSION_NUM < 502
#include <assert.h>

# define LUA_OK        0
# define lua_rawlen    lua_objlen
# define luaL_setfuncs(L,l,n) (assert(n==0), luaL_register(L,NULL,l))
# define luaL_setmetatable(L, name) \
    (luaL_getmetatable((L), (name)), lua_setmetatable(L, -2))

static void lua_rawgetp(lua_State *L, int idx, const void *p) {
    lua_pushlightuserdata(L, (void*)p);
    lua_rawget(L, lpb_relindex(idx, 1));
}

static void lua_rawsetp(lua_State *L, int idx, const void *p) {
    lua_pushlightuserdata(L, (void*)p);
    lua_insert(L, -2);
    lua_rawset(L, lpb_relindex(idx, 1));
}

#ifndef luaL_newlib /* not LuaJIT 2.1 */
#define luaL_newlib(L,l) (lua_newtable(L), luaL_register(L,NULL,l))

static lua_Integer lua_tointegerx(lua_State *L, int idx, int *isint) {
    lua_Integer i = lua_tointeger(L, idx);
    if (isint) *isint = (i != 0 || lua_type(L, idx) == LUA_TNUMBER);
    return i;
}

static lua_Number lua_tonumberx(lua_State *L, int idx, int *isnum) {
    lua_Number i = lua_tonumber(L, idx);
    if (isnum) *isnum = (i != 0 || lua_type(L, idx) == LUA_TNUMBER);
    return i;
}

static void *luaL_testudata(lua_State *L, int idx, const char *type) {
    void *p = lua_touserdata(L, idx);
    if (p != NULL && lua_getmetatable(L, idx)) {
        lua_getfield(L, LUA_REGISTRYINDEX, type);
        if (!lua_rawequal(L, -2, -1))
            p = NULL;
        lua_pop(L, 2);
        return p;
    }
    return NULL;
}

#endif

#ifdef LUAI_BITSINT /* not LuaJIT */
#include <errno.h>

static int luaL_fileresult(lua_State *L, int stat, const char *fname) {
    int en = errno;
    if (stat) return lua_pushboolean(L, 1), 1;
    lua_pushnil(L);
    lua_pushfstring(L, "%s: %s", fname, strerror(en));
    /*if (fname) lua_pushfstring(L, "%s: %s", fname, strerror(en));
      else       lua_pushstring(L, strerror(en));*//* NOT USED */
    lua_pushinteger(L, en);
    return 3;
}

#endif /* not LuaJIT */

#endif

#if LUA_VERSION_NUM >= 503
# define lua53_getfield lua_getfield
# define lua53_rawgeti  lua_rawgeti
# define lua53_rawgetp  lua_rawgetp
#else /* not Lua 5.3 */
static int lua53_getfield(lua_State *L, int idx, const char *field)
{ return lua_getfield(L, idx, field), lua_type(L, -1); }
static int lua53_rawgeti(lua_State *L, int idx, lua_Integer i)
{ return lua_rawgeti(L, idx, i), lua_type(L, -1); }
static int lua53_rawgetp(lua_State *L, int idx, const void *p)
{ return lua_rawgetp(L, idx, p), lua_type(L, -1); }
#endif


/* protobuf global state */

#define lpbS_state(LS)    ((LS)->state)
#define lpb_name(LS,s)   pb_name(lpbS_state(LS), (s), &(LS)->cache)

static const pb_State *global_state = NULL;
static const char state_name[] = PB_STATE;

enum lpb_Int64Mode { LPB_NUMBER, LPB_STRING, LPB_HEXSTRING };
enum lpb_EncodeMode   { LPB_DEFDEF, LPB_COPYDEF, LPB_METADEF, LPB_NODEF };

typedef struct lpb_State {
    const pb_State *state;
    pb_State  local;
    pb_Cache  cache;
    pb_Buffer buffer;
    int defs_index;
    int enc_hooks_index;
    int dec_hooks_index;
    unsigned use_dec_hooks : 1;
    unsigned use_enc_hooks : 1;
    unsigned enum_as_value : 1;
    unsigned encode_mode   : 2; /* lpb_EncodeMode */
    unsigned int64_mode    : 2; /* lpb_Int64Mode */
    unsigned encode_default_values  : 1;
    unsigned decode_default_array   : 1;
    unsigned decode_default_message : 1;
    unsigned encode_order  : 1;
} lpb_State;

static int lpb_reftable(lua_State *L, int ref) {
    if (ref != LUA_NOREF) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
        return ref;
    } else {
        lua_newtable(L);
        lua_pushvalue(L, -1);
        return luaL_ref(L, LUA_REGISTRYINDEX);
    }
}

static void lpb_pushdeftable(lua_State *L, lpb_State *LS)
{ LS->defs_index = lpb_reftable(L, LS->defs_index); }

static void lpb_pushenchooktable(lua_State *L, lpb_State *LS)
{ LS->enc_hooks_index = lpb_reftable(L, LS->enc_hooks_index); }

static void lpb_pushdechooktable(lua_State *L, lpb_State *LS)
{ LS->dec_hooks_index = lpb_reftable(L, LS->dec_hooks_index); }

static int Lpb_delete(lua_State *L) {
    lpb_State *LS = (lpb_State*)luaL_testudata(L, 1, PB_STATE);
    if (LS != NULL) {
        const pb_State *GS = global_state;
        pb_free(&LS->local);
        if (&LS->local == GS)
            global_state = NULL;
        LS->state = NULL;
        pb_resetbuffer(&LS->buffer);
        luaL_unref(L, LUA_REGISTRYINDEX, LS->defs_index);
        luaL_unref(L, LUA_REGISTRYINDEX, LS->enc_hooks_index);
        luaL_unref(L, LUA_REGISTRYINDEX, LS->dec_hooks_index);
    }
    return 0;
}

LUALIB_API lpb_State *lpb_lstate(lua_State *L) {
    lpb_State *LS;
    if (lua53_rawgetp(L, LUA_REGISTRYINDEX, state_name) == LUA_TUSERDATA) {
        LS = (lpb_State*)lua_touserdata(L, -1);
        lua_pop(L, 1);
    } else {
        lua_pop(L, 1);
        LS = (lpb_State*)lua_newuserdata(L, sizeof(lpb_State));
        memset(LS, 0, sizeof(lpb_State));
        LS->defs_index = LUA_NOREF;
        LS->enc_hooks_index = LUA_NOREF;
        LS->dec_hooks_index = LUA_NOREF;
        LS->state = &LS->local;
        pb_init(&LS->local);
        pb_initbuffer(&LS->buffer);
        luaL_setmetatable(L, PB_STATE);
        lua_rawsetp(L, LUA_REGISTRYINDEX, state_name);
    }
    return LS;
}

static int Lpb_state(lua_State *L) {
    int top = lua_gettop(L);
    lpb_lstate(L);
    lua_rawgetp(L, LUA_REGISTRYINDEX, state_name);
    if (top != 0) {
        if (lua_isnil(L, 1))
            lua_pushnil(L);
        else {
            luaL_checkudata(L, 1, PB_STATE);
            lua_pushvalue(L, 1);
        }
        lua_rawsetp(L, LUA_REGISTRYINDEX, state_name);
    }
    return 1;
}


/* protobuf util routines */

static void lpb_addlength(lua_State *L, pb_Buffer *b, size_t len)
{ if (pb_addlength(b, len) == 0) luaL_error(L, "encode bytes fail"); }

static int typeerror(lua_State *L, int idx, const char *type) {
    lua_pushfstring(L, "%s expected, got %s", type, luaL_typename(L, idx));
    return luaL_argerror(L, idx, lua_tostring(L, -1));
}

static lua_Integer posrelat(lua_Integer pos, size_t len) {
    if (pos >= 0) return pos;
    else if (0u - (size_t)pos > len) return 0;
    else return (lua_Integer)len + pos + 1;
}

static lua_Integer rangerelat(lua_State *L, int idx, lua_Integer r[2], size_t len) {
    r[0] = posrelat(luaL_optinteger(L, idx, 1), len);
    r[1] = posrelat(luaL_optinteger(L, idx+1, len), len);
    if (r[0] < 1) r[0] = 1;
    if (r[1] > (lua_Integer)len) r[1] = len;
    return r[0] <= r[1] ? r[1] - r[0] + 1 : 0;
}

static int argcheck(lua_State *L, int cond, int idx, const char *fmt, ...) {
    if (!cond) {
        va_list l;
        va_start(l, fmt);
        lua_pushvfstring(L, fmt, l);
        va_end(l);
        return luaL_argerror(L, idx, lua_tostring(L, -1));
    }
    return 1;
}

static pb_Slice lpb_toslice(lua_State *L, int idx) {
    int type = lua_type(L, idx);
    if (type == LUA_TSTRING) {
        size_t len;
        const char *s = lua_tolstring(L, idx, &len);
        return pb_lslice(s, len);
    } else if (type == LUA_TUSERDATA) {
        pb_Buffer *buffer;
        pb_Slice *s;
        if ((buffer = test_buffer(L, idx)) != NULL)
            return pb_result(buffer);
        else if ((s = test_slice(L, idx)) != NULL)
            return *s;
    }
    return pb_slice(NULL);
}

LUALIB_API pb_Slice lpb_checkslice(lua_State *L, int idx) {
    pb_Slice ret = lpb_toslice(L, idx);
    if (ret.p == NULL) typeerror(L, idx, "string/buffer/slice");
    return ret;
}

static void lpb_readbytes(lua_State *L, pb_Slice *s, pb_Slice *pv) {
    uint64_t len = 0;
    if (pb_readvarint64(s, &len) == 0 || len > PB_MAX_SIZET)
        luaL_error(L, "invalid bytes length: %d (at offset %d)",
                (int)len, pb_pos(*s)+1);
    if (pb_readslice(s, (size_t)len, pv) == 0 && len != 0)
        luaL_error(L, "unfinished bytes (len %d at offset %d)",
                (int)len, pb_pos(*s)+1);
}

static int lpb_hexchar(char ch) {
    switch (ch) {
    case '0': return 0;
    case '1': return 1; case '2': return 2; case '3': return 3;
    case '4': return 4; case '5': return 5; case '6': return 6;
    case '7': return 7; case '8': return 8; case '9': return 9;
    case 'a': case 'A': return 10; case 'b': case 'B': return 11;
    case 'c': case 'C': return 12; case 'd': case 'D': return 13;
    case 'e': case 'E': return 14; case 'f': case 'F': return 15;
    }
    return -1;
}

static uint64_t lpb_tointegerx(lua_State *L, int idx, int *isint) {
    int neg = 0;
    const char *s, *os;
#if LUA_VERSION_NUM >= 503
    uint64_t v = (uint64_t)lua_tointegerx(L, idx, isint);
    if (*isint) return v;
#else
    uint64_t v = 0;
    lua_Number nv = lua_tonumberx(L, idx, isint);
    if (*isint) {
        if (nv < (lua_Number)INT64_MIN || nv > (lua_Number)INT64_MAX)
            luaL_error(L, "number has no integer representation");
        return (uint64_t)(int64_t)nv;
    }
#endif
    if ((os = s = lua_tostring(L, idx)) == NULL) return 0;
    while (*s == '#' || *s == '+' || *s == '-')
        neg = (*s == '-') ^ neg, ++s;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        for (s += 2; *s != '\0'; ++s) {
            int n = lpb_hexchar(*s);
            if (n < 0) break;
            v = v << 4 | n;
        }
    } else {
        for (; *s != '\0'; ++s) {
            int n = lpb_hexchar(*s);
            if (n < 0 || n > 10) break;
            v = v * 10 + n;
        }
    }
    if (!(*isint = *s == '\0')) return 0;
    return neg ? ~v + 1 : v;
}

static uint64_t lpb_checkinteger(lua_State *L, int idx) {
    int isint;
    uint64_t v = lpb_tointegerx(L, idx, &isint);
    if (!isint) {
        if (lua_type(L, idx) == LUA_TSTRING)
            luaL_error(L, "integer format error: '%s'", lua_tostring(L, idx));
        typeerror(L, idx, "number/string");
    }
    return v;
}

static void lpb_pushinteger(lua_State *L, int64_t n, int u, int mode) {
    if (mode != LPB_NUMBER && ((u && n < 0) || n < INT_MIN || n > UINT_MAX)) {
        char buff[32], *p = buff + sizeof(buff) - 1;
        int neg = !u && n < 0;
        uint64_t un = !u && neg ? ~(uint64_t)n + 1 : (uint64_t)n;
        if (mode == LPB_STRING) {
            for (*p = '\0'; un > 0; un /= 10)
                *--p = "0123456789"[un % 10];
        } else if (mode == LPB_HEXSTRING) {
            for (*p = '\0'; un > 0; un >>= 4)
                *--p = "0123456789ABCDEF"[un & 0xF];
            *--p = 'x', *--p = '0';
        }
        if (neg) *--p = '-';
        *--p = '#';
        lua_pushstring(L, p);
    } else if (LUA_VERSION_NUM >= 503 && sizeof(lua_Integer) >= 8)
        lua_pushinteger(L, (lua_Integer)n);
    else
        lua_pushnumber(L, (lua_Number)n);
}

typedef union lpb_Value {
    pb_Slice    s[1];
    uint32_t    u32;
    uint64_t    u64;
    lua_Integer lint;
    lua_Number  lnum;
} lpb_Value;

static int lpb_addtype(lua_State *L, pb_Buffer *b, int idx, int type, size_t *plen) {
    int ret = 0, expected = LUA_TNUMBER;
    lpb_Value v;
    size_t len = 0;
    switch (type) {
    case PB_Tbool:
        len = pb_addvarint32(b, ret = lua_toboolean(L, idx));
        if (ret) len = 0;
        ret = 1;
        break;
    case PB_Tdouble:
        v.lnum = lua_tonumberx(L, idx, &ret);
        if (ret) len = pb_addfixed64(b, pb_encode_double((double)v.lnum));
        if (v.lnum != 0.0) len = 0;
        break;
    case PB_Tfloat:
        v.lnum = lua_tonumberx(L, idx, &ret);
        if (ret) len = pb_addfixed32(b, pb_encode_float((float)v.lnum));
        if (v.lnum != 0.0) len = 0;
        break;
    case PB_Tfixed32:
        v.u64 = lpb_tointegerx(L, idx, &ret);
        if (ret) len = pb_addfixed32(b, v.u32);
        if (v.u64 != 0) len = 0;
        break;
    case PB_Tsfixed32:
        v.u64 = lpb_tointegerx(L, idx, &ret);
        if (ret) len = pb_addfixed32(b, v.u32);
        if (v.u64 != 0) len = 0;
        break;
    case PB_Tint32:
        v.u64 = lpb_tointegerx(L, idx, &ret);
        if (ret) len = pb_addvarint64(b, pb_expandsig((uint32_t)v.u64));
        if (v.u64 != 0) len = 0;
        break;
    case PB_Tuint32:
        v.u64 = lpb_tointegerx(L, idx, &ret);
        if (ret) len = pb_addvarint32(b, v.u32);
        if (v.u64 != 0) len = 0;
        break;
    case PB_Tsint32:
        v.u64 = lpb_tointegerx(L, idx, &ret);
        if (ret) len = pb_addvarint32(b, pb_encode_sint32(v.u32));
        if (v.u64 != 0) len = 0;
        break;
    case PB_Tfixed64:
        v.u64 = lpb_tointegerx(L, idx, &ret);
        if (ret) len = pb_addfixed64(b, v.u64);
        if (v.u64 != 0) len = 0;
        break;
    case PB_Tsfixed64:
        v.u64 = lpb_tointegerx(L, idx, &ret);
        if (ret) len = pb_addfixed64(b, v.u64);
        if (v.u64 != 0) len = 0;
        break;
    case PB_Tint64: case PB_Tuint64:
        v.u64 = lpb_tointegerx(L, idx, &ret);
        if (ret) len = pb_addvarint64(b, v.u64);
        if (v.u64 != 0) len = 0;
        break;
    case PB_Tsint64:
        v.u64 = lpb_tointegerx(L, idx, &ret);
        if (ret) len = pb_addvarint64(b, pb_encode_sint64(v.u64));
        if (v.u64 != 0) len = 0;
        break;
    case PB_Tbytes: case PB_Tstring:
        *v.s = lpb_toslice(L, idx);
        if ((ret = (v.s->p != NULL))) len = pb_addbytes(b, *v.s);
        if (pb_len(*v.s) != 0) len = 0;
        expected = LUA_TSTRING;
        break;
    default:
        lua_pushfstring(L, "unknown type %s", pb_typename(type, "<unknown>"));
        if (idx > 0) argcheck(L, 0, idx, lua_tostring(L, -1));
        lua_error(L);
    }
    if (plen) *plen = len;
    return ret ? 0 : expected;
}

static void lpb_readtype(lua_State *L, lpb_State *LS, int type, pb_Slice *s) {
    lpb_Value v;
    switch (type) {
#define pushinteger(n,u) lpb_pushinteger((L), (n), (u), LS->int64_mode)
    case PB_Tbool:  case PB_Tenum:
    case PB_Tint32: case PB_Tuint32: case PB_Tsint32:
    case PB_Tint64: case PB_Tuint64: case PB_Tsint64:
        if (pb_readvarint64(s, &v.u64) == 0)
            luaL_error(L, "invalid varint value at offset %d", pb_pos(*s)+1);
        switch (type) {
        case PB_Tbool:   lua_pushboolean(L, v.u64 != 0); break;
         /*case PB_Tenum:   pushinteger(v.u64); break; [> NOT REACHED <]*/
        case PB_Tint32:  pushinteger((int32_t)v.u64, 0); break;
        case PB_Tuint32: pushinteger((uint32_t)v.u64, 1); break;
        case PB_Tsint32: pushinteger(pb_decode_sint32((uint32_t)v.u64), 0); break;
        case PB_Tint64:  pushinteger((int64_t)v.u64, 0); break;
        case PB_Tuint64: pushinteger((uint64_t)v.u64, 1); break;
        case PB_Tsint64: pushinteger(pb_decode_sint64(v.u64), 0); break;
        }
        break;
    case PB_Tfloat:
    case PB_Tfixed32:
    case PB_Tsfixed32:
        if (pb_readfixed32(s, &v.u32) == 0)
            luaL_error(L, "invalid fixed32 value at offset %d", pb_pos(*s)+1);
        switch (type) {
        case PB_Tfloat:    lua_pushnumber(L, pb_decode_float(v.u32)); break;
        case PB_Tfixed32:  pushinteger(v.u32, 1); break;
        case PB_Tsfixed32: pushinteger((int32_t)v.u32, 0); break;
        }
        break;
    case PB_Tdouble:
    case PB_Tfixed64:
    case PB_Tsfixed64:
        if (pb_readfixed64(s, &v.u64) == 0)
            luaL_error(L, "invalid fixed64 value at offset %d", pb_pos(*s)+1);
        switch (type) {
        case PB_Tdouble:   lua_pushnumber(L, pb_decode_double(v.u64)); break;
        case PB_Tfixed64:  pushinteger(v.u64, 1); break;
        case PB_Tsfixed64: pushinteger((int64_t)v.u64, 0); break;
        }
        break;
    case PB_Tbytes:
    case PB_Tstring:
    case PB_Tmessage:
        lpb_readbytes(L, s, v.s);
        push_slice(L, *v.s);
        break;
    default:
        luaL_error(L, "unknown type %s (%d)", pb_typename(type, NULL), type);
    }
}


/* io routines */

#ifdef _WIN32
# include <io.h>
# include <fcntl.h>
#else
# define setmode(a,b)  ((void)0)
#endif

static int io_read(lua_State *L) {
    FILE *fp = (FILE*)lua_touserdata(L, 1);
    size_t nr;
    luaL_Buffer b;
    luaL_buffinit(L, &b);
    do {  /* read file in chunks of LUAL_BUFFERSIZE bytes */
        char *p = luaL_prepbuffer(&b);
        nr = fread(p, sizeof(char), LUAL_BUFFERSIZE, fp);
        luaL_addsize(&b, nr);
    } while (nr == LUAL_BUFFERSIZE);
    luaL_pushresult(&b);  /* close buffer */
    return 1;
}

static int io_write(lua_State *L, FILE *f, int idx) {
    int nargs = lua_gettop(L) - idx + 1;
    int status = 1;
    for (; nargs--; idx++) {
        pb_Slice s = lpb_checkslice(L, idx);
        size_t l = pb_len(s);
        status = status && (fwrite(s.p, sizeof(char), l, f) == l);
    }
    return status ? 1 : luaL_fileresult(L, 0, NULL);
}

static int Lio_read(lua_State *L) {
    const char *fname = luaL_optstring(L, 1, NULL);
    FILE *fp = stdin;
    int ret;
    if (fname == NULL)
        (void)setmode(fileno(stdin), O_BINARY);
    else if ((fp = fopen(fname, "rb")) == NULL)
        return luaL_fileresult(L, 0, fname);
    lua_pushcfunction(L, io_read);
    lua_pushlightuserdata(L, fp);
    ret = lua_pcall(L, 1, 1, 0);
    if (fp != stdin) fclose(fp);
    else (void)setmode(fileno(stdin), O_TEXT);
    if (ret != LUA_OK) { lua_pushnil(L); lua_insert(L, -2); return 2; }
    return 1;
}

static int Lio_write(lua_State *L) {
    int res;
    (void)setmode(fileno(stdout), O_BINARY);
    res = io_write(L, stdout, 1);
    fflush(stdout);
    (void)setmode(fileno(stdout), O_TEXT);
    return res;
}

static int Lio_dump(lua_State *L) {
    int res;
    const char *fname = luaL_checkstring(L, 1);
    FILE *fp = fopen(fname, "wb");
    if (fp == NULL) return luaL_fileresult(L, 0, fname);
    res = io_write(L, fp, 2);
    fclose(fp);
    return res;
}

LUALIB_API int luaopen_pb_io(lua_State *L) {
    luaL_Reg libs[] = {
#define ENTRY(name) { #name, Lio_##name }
        ENTRY(read),
        ENTRY(write),
        ENTRY(dump),
#undef  ENTRY
        { NULL, NULL }
    };
    luaL_newlib(L, libs);
    return 1;
}


/* protobuf integer conversion */

static int Lconv_encode_int32(lua_State *L) {
    uint64_t v = pb_expandsig((int32_t)lpb_checkinteger(L, 1));
    return lpb_pushinteger(L, v, 1, lpb_lstate(L)->int64_mode), 1;
}

static int Lconv_encode_uint32(lua_State *L) {
    return lpb_pushinteger(L, (uint32_t)lpb_checkinteger(L, 1),
            1, lpb_lstate(L)->int64_mode), 1;
}

static int Lconv_encode_sint32(lua_State *L) {
    return lpb_pushinteger(L, pb_encode_sint32((int32_t)lpb_checkinteger(L, 1)),
            1, lpb_lstate(L)->int64_mode), 1;
}

static int Lconv_decode_sint32(lua_State *L) {
    return lpb_pushinteger(L, pb_decode_sint32((uint32_t)lpb_checkinteger(L, 1)),
            0, lpb_lstate(L)->int64_mode), 1;
}

static int Lconv_encode_sint64(lua_State *L) {
    return lpb_pushinteger(L, pb_encode_sint64(lpb_checkinteger(L, 1)),
            1, lpb_lstate(L)->int64_mode), 1;
}

static int Lconv_decode_sint64(lua_State *L) {
    return lpb_pushinteger(L, pb_decode_sint64(lpb_checkinteger(L, 1)),
            0, lpb_lstate(L)->int64_mode), 1;
}

static int Lconv_encode_float(lua_State *L) {
    return lpb_pushinteger(L, pb_encode_float((float)luaL_checknumber(L, 1)),
            1, lpb_lstate(L)->int64_mode), 1;
}

static int Lconv_decode_float(lua_State *L) {
    return lua_pushnumber(L,
            pb_decode_float((uint32_t)lpb_checkinteger(L, 1))), 1;
}

static int Lconv_encode_double(lua_State *L) {
    return lpb_pushinteger(L, pb_encode_double(luaL_checknumber(L, 1)),
            1, lpb_lstate(L)->int64_mode), 1;
}

static int Lconv_decode_double(lua_State *L) {
    return lua_pushnumber(L, pb_decode_double(lpb_checkinteger(L, 1))), 1;
}

LUALIB_API int luaopen_pb_conv(lua_State *L) {
    luaL_Reg libs[] = {
        { "decode_uint32", Lconv_encode_uint32 },
        { "decode_int32", Lconv_encode_int32 },
#define ENTRY(name) { #name, Lconv_##name }
        ENTRY(encode_int32),
        ENTRY(encode_uint32),
        ENTRY(encode_sint32),
        ENTRY(encode_sint64),
        ENTRY(decode_sint32),
        ENTRY(decode_sint64),
        ENTRY(decode_float),
        ENTRY(decode_double),
        ENTRY(encode_float),
        ENTRY(encode_double),
#undef  ENTRY
        { NULL, NULL }
    };
    luaL_newlib(L, libs);
    return 1;
}


/* protobuf encode routine */

static int lpb_typefmt(int fmt) {
    switch (fmt) {
#define X(name,type,fmt) case fmt: return PB_T##name;
        PB_TYPES(X)
#undef  X
    }
    return -1;
}

static int lpb_packfmt(lua_State *L, int idx, pb_Buffer *b, const char **pfmt, int level) {
    const char *fmt = *pfmt;
    int type, ltype;
    size_t len;
    argcheck(L, level <= 100, 1, "format level overflow");
    for (; *fmt != '\0'; ++fmt) {
        switch (*fmt) {
        case 'v': pb_addvarint64(b, (uint64_t)lpb_checkinteger(L, idx++)); break;
        case 'd': pb_addfixed32(b, (uint32_t)lpb_checkinteger(L, idx++)); break;
        case 'q': pb_addfixed64(b, (uint64_t)lpb_checkinteger(L, idx++)); break;
        case 'c': pb_addslice(b, lpb_checkslice(L, idx++)); break;
        case 's': pb_addbytes(b, lpb_checkslice(L, idx++)); break;
        case '#': lpb_addlength(L, b, (size_t)lpb_checkinteger(L, idx++)); break;
        case '(':
            len = pb_bufflen(b);
            ++fmt;
            idx = lpb_packfmt(L, idx, b, &fmt, level+1);
            lpb_addlength(L, b, len);
            break;
        case ')':
            if (level == 0) luaL_argerror(L, 1, "unexpected ')' in format");
            *pfmt = fmt;
            return idx;
        case '\0':
        default:
            argcheck(L, (type = lpb_typefmt(*fmt)) >= 0,
                    1, "invalid formater: '%c'", *fmt);
            ltype = lpb_addtype(L, b, idx, type, NULL);
            argcheck(L, ltype == 0, idx, "%s expected for type '%s', got %s",
                    lua_typename(L, ltype), pb_typename(type, "<unknown>"),
                    luaL_typename(L, idx));
            ++idx;
        }
    }
    if (level != 0) luaL_argerror(L, 2, "unmatch '(' in format");
    *pfmt = fmt;
    return idx;
}

static int Lpb_tohex(lua_State *L) {
    pb_Slice s = lpb_checkslice(L, 1);
    const char *hexa = "0123456789ABCDEF";
    char hex[4] = "XX ";
    lua_Integer r[2] = { 1, -1 };
    luaL_Buffer lb;
    rangerelat(L, 2, r, pb_len(s));
    luaL_buffinit(L, &lb);
    for (; r[0] <= r[1]; ++r[0]) {
        unsigned int ch = s.p[r[0]-1];
        hex[0] = hexa[(ch>>4)&0xF];
        hex[1] = hexa[(ch   )&0xF];
        if (r[0] == r[1]) hex[2] = '\0';
        luaL_addstring(&lb, hex);
    }
    luaL_pushresult(&lb);
    return 1;
}

static int Lpb_fromhex(lua_State *L) {
    pb_Slice s = lpb_checkslice(L, 1);
    lua_Integer r[2] = { 1, -1 };
    luaL_Buffer lb;
    int curr = 0, idx = 0, num;
    rangerelat(L, 2, r, pb_len(s));
    luaL_buffinit(L, &lb);
    for (; r[0] <= r[1]; ++r[0]) {
        switch (num = s.p[r[0]-1]) {
        case '0': case '1': case '2': case '3':
        case '4': case '5': case '6': case '7':
        case '8': case '9': num -= '0'; break;
        case 'A': case 'a': num  =  10; break;
        case 'B': case 'b': num  =  11; break;
        case 'C': case 'c': num  =  12; break;
        case 'D': case 'd': num  =  13; break;
        case 'E': case 'e': num  =  14; break;
        case 'F': case 'f': num  =  15; break;
        default: continue;
        }
        curr = curr<<4 | num;
        if (++idx % 2 == 0) luaL_addchar(&lb, curr), curr = 0;
    }
    luaL_pushresult(&lb);
    return 1;
}

static int Lpb_result(lua_State *L) {
    pb_Slice s = lpb_checkslice(L, 1);
    lua_Integer r[2] = {1, -1}, range = rangerelat(L, 2, r, pb_len(s));
    lua_pushlstring(L, s.p+r[0]-1, (size_t)range);
    return 1;
}

static int Lbuf_new(lua_State *L) {
    int i, top = lua_gettop(L);
    pb_Buffer *buf = (pb_Buffer*)lua_newuserdata(L, sizeof(pb_Buffer));
    pb_initbuffer(buf);
    luaL_setmetatable(L, PB_BUFFER);
    for (i = 1; i <= top; ++i)
        pb_addslice(buf, lpb_checkslice(L, i));
    return 1;
}

static int Lbuf_delete(lua_State *L) {
    pb_Buffer *buf = test_buffer(L, 1);
    if (buf) pb_resetbuffer(buf);
    return 0;
}

static int Lbuf_libcall(lua_State *L) {
    int i, top = lua_gettop(L);
    pb_Buffer *buf = (pb_Buffer*)lua_newuserdata(L, sizeof(pb_Buffer));
    pb_initbuffer(buf);
    luaL_setmetatable(L, PB_BUFFER);
    for (i = 2; i <= top; ++i)
        pb_addslice(buf, lpb_checkslice(L, i));
    return 1;
}

static int Lbuf_tostring(lua_State *L) {
    pb_Buffer *buf = check_buffer(L, 1);
    lua_pushfstring(L, "pb.Buffer: %p", buf);
    return 1;
}

static int Lbuf_reset(lua_State *L) {
    pb_Buffer *buf = check_buffer(L, 1);
    int i, top = lua_gettop(L);
    pb_bufflen(buf) = 0;
    for (i = 2; i <= top; ++i)
        pb_addslice(buf, lpb_checkslice(L, i));
    return_self(L);
}

static int Lbuf_len(lua_State *L) {
    pb_Buffer *buf = check_buffer(L, 1);
    lua_pushinteger(L, (lua_Integer)buf->size);
    return 1;
}

static int Lbuf_pack(lua_State *L) {
    pb_Buffer b, *pb = test_buffer(L, 1);
    int idx = 1 + (pb != NULL);
    const char *fmt = luaL_checkstring(L, idx++);
    if (pb == NULL) pb_initbuffer(pb = &b);
    lpb_packfmt(L, idx, pb, &fmt, 0);
    if (pb != &b)
        lua_settop(L, 1);
    else {
        pb_Slice ret = pb_result(pb);
        push_slice(L, ret);
        pb_resetbuffer(pb);
    }
    return 1;
}

LUALIB_API int luaopen_pb_buffer(lua_State *L) {
    luaL_Reg libs[] = {
        { "__tostring", Lbuf_tostring },
        { "__len",      Lbuf_len },
        { "__gc",       Lbuf_delete },
        { "delete",     Lbuf_delete },
        { "tohex",      Lpb_tohex },
        { "fromhex",    Lpb_fromhex },
        { "result",     Lpb_result },
#define ENTRY(name) { #name, Lbuf_##name }
        ENTRY(new),
        ENTRY(reset),
        ENTRY(pack),
#undef  ENTRY
        { NULL, NULL }
    };
    if (luaL_newmetatable(L, PB_BUFFER)) {
        luaL_setfuncs(L, libs, 0);
        lua_pushvalue(L, -1);
        lua_setfield(L, -2, "__index");
        lua_createtable(L, 0, 1);
        lua_pushcfunction(L, Lbuf_libcall);
        lua_setfield(L, -2, "__call");
        lua_setmetatable(L, -2);
    }
    return 1;
}


/* protobuf decode routine */

#define LPB_INITSTACKLEN 2

typedef struct lpb_Slice {
    pb_Slice  curr;
    pb_Slice *buff;
    size_t       used;
    size_t       size;
    pb_Slice  init_buff[LPB_INITSTACKLEN];
} lpb_Slice;

static void lpb_resetslice(lua_State *L, lpb_Slice *s, size_t size) {
    if (size == sizeof(lpb_Slice)) {
        if (s->buff != s->init_buff) free(s->buff);
        memset(s, 0, sizeof(lpb_Slice));
        s->buff = s->init_buff;
        s->size = LPB_INITSTACKLEN;
    }
    lua_pushnil(L);
    lua_rawsetp(L, LUA_REGISTRYINDEX, s);
}

static pb_Slice lpb_checkview(lua_State *L, int idx, pb_Slice *ps) {
    pb_Slice src = lpb_checkslice(L, idx);
    lua_Integer r[2] = {1, -1}, range = rangerelat(L, idx+1, r, pb_len(src));
    pb_Slice ret;
    if (ps) *ps = src, ps->start = src.p;
    ret.p     = src.p + r[0] - 1;
    ret.end   = ret.p + range;
    ret.start = src.p;
    return ret;
}

static void lpb_enterview(lua_State *L, lpb_Slice *s, pb_Slice view) {
    if (s->used >= s->size) {
        size_t newsize = s->size * 2;
        pb_Slice *oldp = s->buff != s->init_buff ? s->buff : NULL;
        pb_Slice *newp = (pb_Slice*)realloc(oldp, newsize*sizeof(pb_Slice));
        if (newp == NULL) { luaL_error(L, "out of memory"); return; }
        if (oldp == NULL) memcpy(newp, s->buff, s->used*sizeof(pb_Slice));
        s->buff = newp;
        s->size = newsize;
    }
    s->buff[s->used++] = s->curr;
    s->curr = view;
}

static void lpb_initslice(lua_State *L, int idx, lpb_Slice *s, size_t size) {
    if (size == sizeof(lpb_Slice)) {
        memset(s, 0, sizeof(lpb_Slice));
        s->buff = s->init_buff;
        s->size = LPB_INITSTACKLEN;
    }
    if (!lua_isnoneornil(L, idx)) {
        pb_Slice base, view = lpb_checkview(L, idx, &base);
        s->curr = base;
        if (size == sizeof(lpb_Slice)) lpb_enterview(L, s, view);
        lua_pushvalue(L, idx);
        lua_rawsetp(L, LUA_REGISTRYINDEX, s);
    }
}

static int lpb_unpackscalar(lua_State *L, int *pidx, int top, int fmt, pb_Slice *s) {
    unsigned mode = lpb_lstate(L)->int64_mode;
    lpb_Value v;
    switch (fmt) {
    case 'v':
        if (pb_readvarint64(s, &v.u64) == 0)
            luaL_error(L, "invalid varint value at offset %d", pb_pos(*s)+1);
        lpb_pushinteger(L, v.u64, 1, mode);
        break;
    case 'd':
        if (pb_readfixed32(s, &v.u32) == 0)
            luaL_error(L, "invalid fixed32 value at offset %d", pb_pos(*s)+1);
        lpb_pushinteger(L, v.u32, 1, mode);
        break;
    case 'q':
        if (pb_readfixed64(s, &v.u64) == 0)
            luaL_error(L, "invalid fixed64 value at offset %d", pb_pos(*s)+1);
        lpb_pushinteger(L, v.u64, 1, mode);
        break;
    case 's':
        if (pb_readbytes(s, v.s) == 0)
            luaL_error(L, "invalid bytes value at offset %d", pb_pos(*s)+1);
        push_slice(L, *v.s);
        break;
    case 'c':
        argcheck(L, *pidx <= top, 1, "format argument exceed");
        v.lint = luaL_checkinteger(L, (*pidx)++);
        if (pb_readslice(s, (size_t)v.lint, v.s) == 0)
            luaL_error(L, "invalid sub string at offset %d", pb_pos(*s)+1);
        push_slice(L, *v.s);
        break;
    default:
        return 0;
    }
    return 1;
}

static int lpb_unpackloc(lua_State *L, int *pidx, int top, int fmt, pb_Slice *s, int *prets) {
    lua_Integer li;
    size_t len = s->end - s->start;
    switch (fmt) {
    case '@':
        lua_pushinteger(L, pb_pos(*s)+1);
        ++*prets;
        break;

    case '*': case '+':
        argcheck(L, *pidx <= top, 1, "format argument exceed");
        if (fmt == '*')
            li = posrelat(luaL_checkinteger(L, (*pidx)++), len);
        else
            li = pb_pos(*s) + luaL_checkinteger(L, (*pidx)++) + 1;
        if (li == 0) li = 1;
        if (li > (lua_Integer)len) li = (lua_Integer)len + 1;
        s->p = s->start + li - 1;
        break;

    default:
        return 0;
    }
    return 1;
}

static int lpb_unpackfmt(lua_State *L, int idx, const char *fmt, pb_Slice *s) {
    int rets = 0, top = lua_gettop(L), type;
    for (; *fmt != '\0'; ++fmt) {
        if (lpb_unpackloc(L, &idx, top, *fmt, s, &rets))
            continue;
        if (s->p >= s->end) return lua_pushnil(L), rets + 1;
        luaL_checkstack(L, 1, "too many values");
        if (!lpb_unpackscalar(L, &idx, top, *fmt, s)) {
            argcheck(L, (type = lpb_typefmt(*fmt)) >= 0,
                    1, "invalid formater: '%c'", *fmt);
            lpb_readtype(L, lpb_lstate(L), type, s);
        }
        ++rets;
    }
    return rets;
}

static lpb_Slice *check_lslice(lua_State *L, int idx) {
    pb_Slice *s = check_slice(L, idx);
    argcheck(L, lua_rawlen(L, 1) == sizeof(lpb_Slice),
            idx, "unsupport operation for raw mode slice");
    return (lpb_Slice*)s;
}

static int Lslice_new(lua_State *L) {
    lpb_Slice *s;
    lua_settop(L, 3);
    s = (lpb_Slice*)lua_newuserdata(L, sizeof(lpb_Slice));
    lpb_initslice(L, 1, s, sizeof(lpb_Slice));
    if (s->curr.p == NULL) s->curr = pb_lslice("", 0);
    luaL_setmetatable(L, PB_SLICE);
    return 1;
}

static int Lslice_libcall(lua_State *L) {
    lpb_Slice *s;
    lua_settop(L, 4);
    s = (lpb_Slice*)lua_newuserdata(L, sizeof(lpb_Slice));
    lpb_initslice(L, 2, s, sizeof(lpb_Slice));
    luaL_setmetatable(L, PB_SLICE);
    return 1;
}

static int Lslice_reset(lua_State *L) {
    lpb_Slice *s = (lpb_Slice*)check_slice(L, 1);
    size_t size = lua_rawlen(L, 1);
    lpb_resetslice(L, s, size);
    if (!lua_isnoneornil(L, 2))
        lpb_initslice(L, 2, s, size);
    return_self(L);
}

static int Lslice_tostring(lua_State *L) {
    pb_Slice *s = check_slice(L, 1);
    lua_pushfstring(L, "pb.Slice: %p%s", s,
            lua_rawlen(L, 1) == sizeof(lpb_Slice) ? "" : " (raw)");
    return 1;
}

static int Lslice_len(lua_State *L) {
    pb_Slice *s = check_slice(L, 1);
    lua_pushinteger(L, (lua_Integer)pb_len(*s));
    lua_pushinteger(L, (lua_Integer)pb_pos(*s)+1);
    return 2;
}

static int Lslice_unpack(lua_State *L) {
    pb_Slice view, *s = test_slice(L, 1);
    const char *fmt = luaL_checkstring(L, 2);
    if (s == NULL) view = lpb_checkslice(L, 1), s = &view;
    return lpb_unpackfmt(L, 3, fmt, s);
}

static int Lslice_level(lua_State *L) {
    lpb_Slice *s = check_lslice(L, 1);
    if (!lua_isnoneornil(L, 2)) {
        pb_Slice *se;
        lua_Integer level = posrelat(luaL_checkinteger(L, 2), s->used);
        if (level > (lua_Integer)s->used)
            return 0;
        else if (level == (lua_Integer)s->used)
            se = &s->curr;
        else
            se = &s->buff[level];
        lua_pushinteger(L, (lua_Integer)(se->p     - s->buff[0].start) + 1);
        lua_pushinteger(L, (lua_Integer)(se->start - s->buff[0].start) + 1);
        lua_pushinteger(L, (lua_Integer)(se->end   - s->buff[0].start));
        return 3;
    }
    lua_pushinteger(L, s->used);
    return 1;
}

static int Lslice_enter(lua_State *L) {
    lpb_Slice *s = check_lslice(L, 1);
    pb_Slice view;
    if (lua_isnoneornil(L, 2)) {
        argcheck(L, pb_readbytes(&s->curr, &view) != 0,
            1, "bytes wireformat expected at offset %d", pb_pos(s->curr)+1);
        view.start = view.p;
        lpb_enterview(L, s, view);
    } else {
        lua_Integer r[] = {1, -1};
        lua_Integer range = rangerelat(L, 2, r, pb_len(s->curr));
        view.p     = s->curr.start + r[0] - 1;
        view.end   = view.p + range;
        view.start = s->curr.p;
        lpb_enterview(L, s, view);
    }
    return_self(L);
}

static int Lslice_leave(lua_State *L) {
    lpb_Slice *s = check_lslice(L, 1);
    lua_Integer count = posrelat(luaL_optinteger(L, 2, 1), s->used);
    if (count > (lua_Integer)s->used)
        argcheck(L, 0, 2, "level (%d) exceed max level %d",
                (int)count, (int)s->used);
    else if (count == (lua_Integer)s->used) {
        s->curr = s->buff[0];
        s->used = 1;
    } else {
        s->used -= (size_t)count;
        s->curr = s->buff[s->used];
    }
    lua_settop(L, 1);
    lua_pushinteger(L, s->used);
    return 2;
}

LUALIB_API int lpb_newslice(lua_State *L, const char *s, size_t len) {
    pb_Slice *ls = (pb_Slice*)lua_newuserdata(L, sizeof(pb_Slice));
    *ls = pb_lslice(s, len);
    luaL_setmetatable(L, PB_SLICE);
    return 1;
}

LUALIB_API int luaopen_pb_slice(lua_State *L) {
    luaL_Reg libs[] = {
        { "__tostring", Lslice_tostring },
        { "__len",      Lslice_len   },
        { "__gc",       Lslice_reset },
        { "delete",     Lslice_reset },
        { "tohex",      Lpb_tohex   },
        { "fromhex",    Lpb_fromhex   },
        { "result",     Lpb_result  },
#define ENTRY(name) { #name, Lslice_##name }
        ENTRY(new),
        ENTRY(reset),
        ENTRY(level),
        ENTRY(enter),
        ENTRY(leave),
        ENTRY(unpack),
#undef  ENTRY
        { NULL, NULL }
    };
    if (luaL_newmetatable(L, PB_SLICE)) {
        luaL_setfuncs(L, libs, 0);
        lua_pushvalue(L, -1);
        lua_setfield(L, -2, "__index");
        lua_createtable(L, 0, 1);
        lua_pushcfunction(L, Lslice_libcall);
        lua_setfield(L, -2, "__call");
        lua_setmetatable(L, -2);
    }
    return 1;
}


/* high level typeinfo/encode/decode routines */

typedef enum {USE_FIELD = 1, USE_REPEAT = 2, USE_MESSAGE = 4} lpb_DefFlags;

static void lpb_pushtypetable(lua_State *L, lpb_State *LS, const pb_Type *t);

static void lpb_newmsgtable(lua_State *L, const pb_Type *t)
{ lua_createtable(L, 0, t->field_count - t->oneof_field + t->oneof_count*2); }

LUALIB_API const pb_Type *lpb_type(lpb_State *LS, pb_Slice s) {
    const pb_Type *t;
    if (s.p == NULL || *s.p == '.')
        t = pb_type(lpbS_state(LS), lpb_name(LS, s));
    else {
        pb_Buffer b;
        pb_initbuffer(&b);
        *pb_prepbuffsize(&b, 1) = '.';
        pb_addsize(&b, 1);
        pb_addslice(&b, s);
        t = pb_type(lpbS_state(LS), pb_name(lpbS_state(LS),pb_result(&b),NULL));
        pb_resetbuffer(&b);
    }
    return t;
}

static const pb_Field *lpb_field(lua_State *L, int idx, const pb_Type *t) {
    lpb_State *LS = lpb_lstate(L);
    int isint, number = (int)lua_tointegerx(L, idx, &isint);
    if (isint) return pb_field(t, number);
    return pb_fname(t, lpb_name(LS, lpb_checkslice(L, idx)));
}

static int Lpb_load(lua_State *L) {
    lpb_State *LS = lpb_lstate(L);
    pb_Slice s = lpb_checkslice(L, 1);
    int r = pb_load(&LS->local, &s);
    if (r == PB_OK) global_state = &LS->local;
    lua_pushboolean(L, r == PB_OK);
    lua_pushinteger(L, pb_pos(s)+1);
    return 2;
}

static int Lpb_loadfile(lua_State *L) {
    lpb_State *LS = lpb_lstate(L);
    const char *filename = luaL_checkstring(L, 1);
    size_t size;
    pb_Buffer b;
    pb_Slice s;
    int ret;
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL)
        return luaL_fileresult(L, 0, filename);
    pb_initbuffer(&b);
    do {
        char *d = pb_prepbuffsize(&b, BUFSIZ);
        if (d == NULL) return fclose(fp), luaL_error(L, "out of memory");
        size = fread(d, 1, BUFSIZ, fp);
        pb_addsize(&b, size);
    } while (size == BUFSIZ);
    fclose(fp);
    s = pb_result(&b);
    ret = pb_load(&LS->local, &s);
    if (ret == PB_OK) global_state = &LS->local;
    pb_resetbuffer(&b);
    lua_pushboolean(L, ret == PB_OK);
    lua_pushinteger(L, pb_pos(s)+1);
    return 2;
}

static int lpb_pushtype(lua_State *L, const pb_Type *t) {
    if (t == NULL) return 0;
    lua_pushstring(L, (const char*)t->name);
    lua_pushstring(L, (const char*)t->basename);
    lua_pushstring(L, t->is_map ? "map" : t->is_enum ? "enum" : "message");
    return 3;
}

static int lpb_pushfield(lua_State *L, const pb_Type *t, const pb_Field *f) {
    if (f == NULL) return 0;
    lua_pushstring(L, (const char*)f->name);
    lua_pushinteger(L, f->number);
    lua_pushstring(L, f->type ?
            (const char*)f->type->name :
            pb_typename(f->type_id, "<unknown>"));
    lua_pushstring(L, (const char*)f->default_value);
    lua_pushstring(L, f->repeated ?
            (f->packed ? "packed" : "repeated") :
            "optional");
    if (f->oneof_idx > 0) {
        lua_pushstring(L, (const char*)pb_oneofname(t, f->oneof_idx));
        lua_pushinteger(L, f->oneof_idx-1);
        return 7;
    }
    return 5;
}

static int Lpb_typesiter(lua_State *L) {
    lpb_State *LS = lpb_lstate(L);
    const pb_Type *t = lpb_type(LS, lpb_toslice(L, 2));
    if ((t == NULL && !lua_isnoneornil(L, 2)))
        return 0;
    pb_nexttype(lpbS_state(LS), &t);
    return lpb_pushtype(L, t);
}

static int Lpb_types(lua_State *L) {
    lua_pushcfunction(L, Lpb_typesiter);
    lua_pushnil(L);
    lua_pushnil(L);
    return 3;
}

static int Lpb_fieldsiter(lua_State *L) {
    lpb_State *LS = lpb_lstate(L);
    const pb_Type *t = lpb_type(LS, lpb_checkslice(L, 1));
    const pb_Field *f = pb_fname(t, lpb_name(LS, lpb_toslice(L, 2)));
    if ((f == NULL && !lua_isnoneornil(L, 2)) || !pb_nextfield(t, &f))
        return 0;
    return lpb_pushfield(L, t, f);
}

static int Lpb_fields(lua_State *L) {
    lua_pushcfunction(L, Lpb_fieldsiter);
    lua_pushvalue(L, 1);
    lua_pushnil(L);
    return 3;
}

static int Lpb_type(lua_State *L) {
    lpb_State *LS = lpb_lstate(L);
    const pb_Type *t = lpb_type(LS, lpb_checkslice(L, 1));
    if (t == NULL || t->is_dead)
        return 0;
    return lpb_pushtype(L, t);
}

static int Lpb_field(lua_State *L) {
    lpb_State *LS = lpb_lstate(L);
    const pb_Type *t = lpb_type(LS, lpb_checkslice(L, 1));
    return lpb_pushfield(L, t, lpb_field(L, 2, t));
}

static int Lpb_enum(lua_State *L) {
    lpb_State *LS = lpb_lstate(L);
    const pb_Type *t = lpb_type(LS, lpb_checkslice(L, 1));
    const pb_Field *f = lpb_field(L, 2, t);
    if (f == NULL) return 0;
    if (lua_type(L, 2) == LUA_TNUMBER)
        lua_pushstring(L, (const char*)f->name);
    else
        lpb_pushinteger(L, f->number, 1, LS->int64_mode);
    return 1;
}

static int lpb_pushdeffield(lua_State *L, lpb_State *LS, const pb_Field *f, int is_proto3) {
    int ret = 0, u = 0;
    const pb_Type *type;
    char *end;
    if (f == NULL) return 0;
    switch (f->type_id) {
    case PB_Tenum:
        if ((type = f ? f->type : NULL) == NULL) return 0;
        if ((f = pb_fname(type, f->default_value)) != NULL)
            ret = LS->enum_as_value ?
                (lpb_pushinteger(L, f->number, 1, LS->int64_mode), 1) :
                (lua_pushstring(L, (const char*)f->name), 1);
        else if (is_proto3)
            ret = (f = pb_field(type, 0)) == NULL || LS->enum_as_value ?
                (lua_pushinteger(L, 0), 1) :
                (lua_pushstring(L, (const char*)f->name), 1);
        break;
    case PB_Tmessage:
        ret = (lpb_pushtypetable(L, LS, f->type), 1);
        break;
    case PB_Tbytes: case PB_Tstring:
        if (f->default_value)
            ret = (lua_pushstring(L, (const char*)f->default_value), 1);
        else if (is_proto3) ret = (lua_pushliteral(L, ""), 1);
        break;
    case PB_Tbool:
        if (f->default_value) {
            if (f->default_value == lpb_name(LS, pb_slice("true")))
                ret = (lua_pushboolean(L, 1), 1);
            else if (f->default_value == lpb_name(LS, pb_slice("false")))
                ret = (lua_pushboolean(L, 0), 1);
        } else if (is_proto3) ret = (lua_pushboolean(L, 0), 1);
        break;
    case PB_Tdouble: case PB_Tfloat:
        if (f->default_value) {
            lua_Number ln = (lua_Number)strtod((const char*)f->default_value, &end);
            if ((const char*)f->default_value == end) return 0;
            ret = (lua_pushnumber(L, ln), 1);
        } else if (is_proto3) ret = (lua_pushnumber(L, 0.0), 1);
        break;

    case PB_Tuint64: case PB_Tfixed64: case PB_Tfixed32: case PB_Tuint32:
        u = 1;
        /* FALLTHROUGH */
    default:
        if (f->default_value) {
            lua_Integer li = (lua_Integer)strtol((const char*)f->default_value, &end, 10);
            if ((const char*)f->default_value == end) return 0;
            ret = (lpb_pushinteger(L, li, u, LS->int64_mode), 1);
        } else if (is_proto3) ret = (lua_pushinteger(L, 0), 1);
    }
    return ret;
}

static void lpb_setdeffields(lua_State *L, lpb_State *LS, const pb_Type *t, lpb_DefFlags flags) {
    const pb_Field *f = NULL;
    while (pb_nextfield(t, &f)) {
        int has_field = f->repeated ?
            (flags & USE_REPEAT) && (t->is_proto3 || LS->decode_default_array)
            && (lua_newtable(L), 1) :
            !f->oneof_idx && (f->type_id != PB_Tmessage ?
                    (flags & USE_FIELD) :
                    (flags & USE_MESSAGE) && LS->decode_default_message)
            && lpb_pushdeffield(L, LS, f, t->is_proto3);
        if (has_field) lua_setfield(L, -2, (const char*)f->name);
    }
}

static void lpb_pushdefmeta(lua_State *L, lpb_State *LS, const pb_Type *t) {
    lpb_pushdeftable(L, LS);
    if (lua53_rawgetp(L, -1, t) != LUA_TTABLE) {
        lua_pop(L, 1);
        lpb_newmsgtable(L, t);
        lpb_setdeffields(L, LS, t, USE_FIELD);
        lua_pushvalue(L, -1);
        lua_setfield(L, -2, "__index");
        lua_pushvalue(L, -1);
        lua_rawsetp(L, -3, t);
    }
    lua_remove(L, -2);
}

static void lpb_cleardefmeta(lua_State *L, lpb_State *LS, const pb_Type *t) {
    lpb_pushdeftable(L, LS);
    lua_pushnil(L);
    lua_rawsetp(L, -2, t);
    lua_pop(L, 1);
}

static int Lpb_defaults(lua_State *L) {
    lpb_State *LS = lpb_lstate(L);
    const pb_Type *t = lpb_type(LS, lpb_checkslice(L, 1));
    int clear = lua_toboolean(L, 2);
    if (t == NULL) luaL_argerror(L, 1, "type not found");
    lpb_pushdefmeta(L, LS, t);
    if (clear) lpb_cleardefmeta(L, LS, t);
    return 1;
}

static int Lpb_hook(lua_State *L) {
    lpb_State *LS = lpb_lstate(L);
    const pb_Type *t = lpb_type(LS, lpb_checkslice(L, 1));
    int type = lua_type(L, 2);
    if (t == NULL) luaL_argerror(L, 1, "type not found");
    if (type != LUA_TNONE && type != LUA_TNIL && type != LUA_TFUNCTION)
        typeerror(L, 2, "function");
    lua_settop(L, 2);
    lpb_pushdechooktable(L, LS);
    lua_rawgetp(L, 3, t);
    if (type != LUA_TNONE) {
        lua_pushvalue(L, 2);
        lua_rawsetp(L, 3, t);
    }
    return 1;
}

static int Lpb_encode_hook(lua_State *L) {
    lpb_State *LS = lpb_lstate(L);
    const pb_Type *t = lpb_type(LS, lpb_checkslice(L, 1));
    int type = lua_type(L, 2);
    if (t == NULL) luaL_argerror(L, 1, "type not found");
    if (type != LUA_TNONE && type != LUA_TNIL && type != LUA_TFUNCTION)
        typeerror(L, 2, "function");
    lua_settop(L, 2);
    lpb_pushenchooktable(L, LS);
    lua_rawgetp(L, 3, t);
    if (type != LUA_TNONE) {
        lua_pushvalue(L, 2);
        lua_rawsetp(L, 3, t);
    }
    return 1;
}

static int Lpb_clear(lua_State *L) {
    lpb_State *LS = lpb_lstate(L);
    pb_State *S = (pb_State*)LS->state;
    pb_Type *t;
    if (lua_isnoneornil(L, 1)) {
        pb_free(&LS->local), pb_init(&LS->local);
        luaL_unref(L, LUA_REGISTRYINDEX, LS->defs_index);
        LS->defs_index = LUA_NOREF;
        luaL_unref(L, LUA_REGISTRYINDEX, LS->enc_hooks_index);
        LS->enc_hooks_index = LUA_NOREF;
        luaL_unref(L, LUA_REGISTRYINDEX, LS->dec_hooks_index);
        LS->dec_hooks_index = LUA_NOREF;
        return 0;
    }
    LS->state = &LS->local;
    t = (pb_Type*)lpb_type(LS, lpb_checkslice(L, 1));
    if (lua_isnoneornil(L, 2)) pb_deltype(&LS->local, t);
    else pb_delfield(&LS->local, t, (pb_Field*)lpb_field(L, 2, t));
    LS->state = S;
    lpb_cleardefmeta(L, LS, t);
    return 0;
}

static int Lpb_typefmt(lua_State *L) {
    pb_Slice s = lpb_checkslice(L, 1);
    const char *r = NULL;
    char buf[2] = {0};
    int type;
    if (pb_len(s) == 1)
        r = pb_typename(type = lpb_typefmt(*s.p), "!");
    else if (lpb_type(lpb_lstate(L), s))
        r = "message", type = PB_TBYTES;
    else if ((type = pb_typebyname(s.p, PB_Tmessage)) != PB_Tmessage) {
        switch (type) {
#define X(name,type,fmt) case PB_T##name: buf[0] = fmt, r = buf; break;
            PB_TYPES(X)
#undef  X
        }
        type = pb_wtypebytype(type);
    } else if ((type = pb_wtypebyname(s.p, PB_Tmessage)) != PB_Tmessage) {
        switch (type) {
#define X(id,name,fmt) case PB_T##id: buf[0] = fmt, r = buf; break;
            PB_WIRETYPES(X)
#undef  X
        }
    }
    lua_pushstring(L, r ? r : "!");
    lua_pushinteger(L, type);
    return 2;
}


/* protobuf encode */

typedef struct lpb_Env {
    lua_State *L;
    lpb_State *LS;
    pb_Buffer *b;
    pb_Slice *s;
} lpb_Env;

static void lpbE_encode (lpb_Env *e, const pb_Type *t, int idx);

static void lpb_checktable(lua_State *L, const pb_Field *f, int idx) {
    argcheck(L, lua_istable(L, idx),
            2, "table expected at field '%s', got %s",
            (const char*)f->name, luaL_typename(L, idx));
}

static void lpb_useenchooks(lua_State *L, lpb_State *LS, const pb_Type *t) {
    lpb_pushenchooktable(L, LS);
    if (lua53_rawgetp(L, -1, t) != LUA_TNIL) {
        lua_pushvalue(L, -3);
        lua_call(L, 1, 1);
        if (!lua_isnil(L, -1)) {
            lua_pushvalue(L, -1);
            lua_replace(L, -4);
        }
    }
    lua_pop(L, 2);
}

static void lpbE_enum(lpb_Env *e, const pb_Field *f, int idx) {
    lua_State *L = e->L;
    pb_Buffer *b = e->b;
    const pb_Field *ev;
    int type = lua_type(L, idx);
    if (type == LUA_TNUMBER)
        pb_addvarint64(b, (uint64_t)lua_tonumber(L, idx));
    else if ((ev = pb_fname(f->type,
                    lpb_name(e->LS, lpb_toslice(L, idx)))) != NULL)
        pb_addvarint32(b, ev->number);
    else if (type != LUA_TSTRING)
        argcheck(L, 0, 2, "number/string expected at field '%s', got %s",
                (const char*)f->name, luaL_typename(L, idx));
    else {
        uint64_t v = lpb_tointegerx(L, idx, &type);
        if (!type)
            argcheck(L, 0, 2, "can not encode unknown enum '%s' at field '%s'",
                    lua_tostring(L, -1), (const char*)f->name);
        pb_addvarint64(b, v);
    }
}

static void lpbE_field(lpb_Env *e, const pb_Field *f, size_t *plen, int idx) {
    lua_State *L = e->L;
    pb_Buffer *b = e->b;
    size_t len;
    int ltype;
    if (plen) *plen = 0;
    switch (f->type_id) {
    case PB_Tenum:
        if (e->LS->use_enc_hooks) lpb_useenchooks(L, e->LS, f->type);
        lpbE_enum(e, f, idx);
        break;

    case PB_Tmessage:
        if (e->LS->use_enc_hooks) lpb_useenchooks(L, e->LS, f->type);
        lpb_checktable(L, f, idx);
        len = pb_bufflen(b);
        lpbE_encode(e, f->type, idx);
        lpb_addlength(L, b, len);
        break;

    default:
        ltype = lpb_addtype(L, b, idx, f->type_id, plen);
        argcheck(L, ltype == 0,
                2, "%s expected for field '%s', got %s",
                lua_typename(L, ltype),
                (const char*)f->name, luaL_typename(L, idx));
    }
}

static void lpbE_tagfield(lpb_Env *e, const pb_Field *f, int ignorezero, int idx) {
    size_t hlen = pb_addvarint32(e->b,
            pb_pair(f->number, pb_wtypebytype(f->type_id)));
    size_t ignoredlen;
    lpbE_field(e, f, &ignoredlen, idx);
    if (!e->LS->encode_default_values && ignoredlen != 0 && ignorezero)
        e->b->size -= (unsigned)(ignoredlen + hlen);
}

static void lpbE_map(lpb_Env *e, const pb_Field *f, int idx) {
    lua_State *L = e->L;
    const pb_Field *kf = pb_field(f->type, 1);
    const pb_Field *vf = pb_field(f->type, 2);
    if (kf == NULL || vf == NULL) return;
    lpb_checktable(L, f, idx);
    lua_pushnil(L);
    while (lua_next(L, lpb_relindex(idx, 1))) {
        size_t len;
        pb_addvarint32(e->b, pb_pair(f->number, PB_TBYTES));
        len = pb_bufflen(e->b);
        lpbE_tagfield(e, kf, 1, -2);
        lpbE_tagfield(e, vf, 1, -1);
        lpb_addlength(L, e->b, len);

        lua_pop(L, 1);
    }
}

static void lpbE_repeated(lpb_Env *e, const pb_Field *f, int idx) {
    lua_State *L = e->L;
    pb_Buffer *b = e->b;
    int i;
    lpb_checktable(L, f, idx);

    if (f->packed) {
        unsigned len, bufflen = pb_bufflen(b);
        pb_addvarint32(b, pb_pair(f->number, PB_TBYTES));
        len = pb_bufflen(b);
        for (i = 1; lua53_rawgeti(L, idx, i) != LUA_TNIL; ++i) {
            lpbE_field(e, f, NULL, -1);
            lua_pop(L, 1);
        }
        if (i == 1 && !e->LS->encode_default_values)
            pb_bufflen(b) = bufflen;
        else
            lpb_addlength(L, b, len);
    } else {
        for (i = 1; lua53_rawgeti(L, idx, i) != LUA_TNIL; ++i) {
            lpbE_tagfield(e, f, 0, -1);
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);
}

static void lpb_encode_onefield(lpb_Env *e, const pb_Type *t, const pb_Field *f, int idx) {
    if (f->type && f->type->is_map)
        lpbE_map(e, f, idx);
    else if (f->repeated)
        lpbE_repeated(e, f, idx);
    else if (!f->type || !f->type->is_dead)
        lpbE_tagfield(e, f, t->is_proto3 && !f->oneof_idx, idx);
}

static void lpbE_encode(lpb_Env *e, const pb_Type *t, int idx) {
    lua_State *L = e->L;
    luaL_checkstack(L, 3, "message too many levels");
    if (e->LS->encode_order) {
        const pb_Field *f = NULL;
        while (pb_nextfield(t, &f)) {
            if (lua53_getfield(L, idx, (const char*)f->name) != LUA_TNIL)
                lpb_encode_onefield(e, t, f, -1);
            lua_pop(L, 1);
        }
    } else {
        lua_pushnil(L);
        while (lua_next(L, lpb_relindex(idx, 1))) {
            if (lua_type(L, -2) == LUA_TSTRING) {
                const pb_Field *f =
                    pb_fname(t, lpb_name(e->LS, lpb_toslice(L, -2)));
                if (f != NULL) lpb_encode_onefield(e, t, f, -1);
            }
            lua_pop(L, 1);
        }
    }
}

static int Lpb_encode(lua_State *L) {
    lpb_State *LS = lpb_lstate(L);
    const pb_Type *t = lpb_type(LS, lpb_checkslice(L, 1));
    lpb_Env e;
    argcheck(L, t!=NULL, 1, "type '%s' does not exists", lua_tostring(L, 1));
    luaL_checktype(L, 2, LUA_TTABLE);
    e.L = L, e.LS = LS, e.b = test_buffer(L, 3);
    if (e.b == NULL) pb_resetbuffer(e.b = &LS->buffer);
    lua_pushvalue(L, 2);
    if (e.LS->use_enc_hooks) lpb_useenchooks(L, e.LS, t);
    lpbE_encode(&e, t, -1);
    if (e.b != &LS->buffer)
        lua_settop(L, 3);
    else {
        lua_pushlstring(L, pb_buffer(e.b), pb_bufflen(e.b));
        pb_resetbuffer(e.b);
    }
    return 1;
}

static int lpbE_pack(lpb_Env* e, const pb_Type* t, int idx) {
    unsigned i;
    lua_State* L = e->L;
    pb_Field** f = pb_sortfield((pb_Type*)t);
    for (i = 0; i < t->field_count; i++) {
        int index = idx + i;
        if (!lua_isnoneornil(L, index)) {
            lpb_encode_onefield(e, t, f[i], index);
        }
    }
    return 0;
}

static int Lpb_pack(lua_State* L) {
    lpb_State* LS = lpb_lstate(L);
    const pb_Type* t = lpb_type(LS, lpb_checkslice(L, 1));
    lpb_Env e;
    int idx = 3;
    e.L = L, e.LS = LS, e.b = test_buffer(L, 2);
    if (e.b == NULL) {
        idx = 2;
        pb_resetbuffer(e.b = &LS->buffer);
    }
    lpbE_pack(&e, t, idx);
    if (e.b != &LS->buffer)
        lua_settop(L, 3);
    else {
        lua_pushlstring(L, pb_buffer(e.b), pb_bufflen(e.b));
        pb_resetbuffer(e.b);
    }
    return 1;
}

/* protobuf decode */

#define lpb_withinput(e,ns,stmt) ((e)->s = (ns), (stmt), (e)->s = s)

static int lpbD_message(lpb_Env *e, const pb_Type *t);

static void lpb_usedechooks(lua_State *L, lpb_State *LS, const pb_Type *t) {
    lpb_pushdechooktable(L, LS);
    if (lua53_rawgetp(L, -1, t) != LUA_TNIL) {
        lua_pushvalue(L, -3);
        lua_call(L, 1, 1);
        if (!lua_isnil(L, -1)) {
            lua_pushvalue(L, -1);
            lua_replace(L, -4);
        }
    }
    lua_pop(L, 2);
}

static void lpb_pushtypetable(lua_State *L, lpb_State *LS, const pb_Type *t) {
    int mode = LS->encode_mode;
    luaL_checkstack(L, 2, "too many levels");
    lpb_newmsgtable(L, t);
    switch (t->is_proto3 && mode == LPB_DEFDEF ? LPB_COPYDEF : mode) {
    case LPB_COPYDEF:
        lpb_setdeffields(L, LS, t, (lpb_DefFlags)(USE_FIELD|USE_REPEAT|USE_MESSAGE));
        break;
    case LPB_METADEF:
        lpb_setdeffields(L, LS, t, (lpb_DefFlags)(USE_REPEAT|USE_MESSAGE));
        lpb_pushdefmeta(L, LS, t);
        lua_setmetatable(L, -2);
        break;
    default:
        if (LS->decode_default_array || LS->decode_default_message)
            lpb_setdeffields(L, LS, t, (lpb_DefFlags)(USE_REPEAT|USE_MESSAGE));
        break;
    }
}

static void lpb_fetchtable(lpb_Env *e, const pb_Field *f) {
    lua_State *L = e->L;
    if (lua53_getfield(L, -1, (const char*)f->name) == LUA_TNIL) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setfield(L, -3, (const char*)f->name);
    }
}

static void lpbD_rawfield(lpb_Env *e, const pb_Field *f) {
    lua_State *L = e->L;
    pb_Slice sv, *s = e->s;
    const pb_Field *ev = NULL;
    uint64_t u64;
    switch (f->type_id) {
    case PB_Tenum:
        if (pb_readvarint64(s, &u64) == 0)
            luaL_error(L, "invalid varint value at offset %d", pb_pos(*s)+1);
        if (!lpb_lstate(L)->enum_as_value)
            ev = pb_field(f->type, (int32_t)u64);
        if (ev) lua_pushstring(L, (const char*)ev->name);
        else lpb_pushinteger(L, (lua_Integer)u64, 1, lpb_lstate(L)->int64_mode);
        if (e->LS->use_dec_hooks) lpb_usedechooks(L, e->LS, f->type);
        break;

    case PB_Tmessage:
        lpb_readbytes(L, s, &sv);
        if (f->type == NULL || f->type->is_dead)
            lua_pushnil(L);
        else {
            lpb_pushtypetable(L, e->LS, f->type);
            lpb_withinput(e, &sv, lpbD_message(e, f->type));
        }
        break;

    default:
        lpb_readtype(L, e->LS, f->type_id, s);
    }
}

static void lpbD_checktype(lpb_Env *e, const pb_Field *f, uint32_t tag) {
    if (pb_wtypebytype(f->type_id) == (int)pb_gettype(tag)) return;
    luaL_error(e->L,
            "type mismatch for %s%sfield '%s' at offset %d, "
            "%s expected for type %s, got %s",
            f->packed ? "packed " : "", f->repeated ? "repeated " : "",
            (const char*)f->name,
            pb_pos(*e->s)+1,
            pb_wtypename(pb_wtypebytype(f->type_id), NULL),
            pb_typename(f->type_id, NULL),
            pb_wtypename(pb_gettype(tag), NULL));
}

static void lpbD_field(lpb_Env *e, const pb_Field *f, uint32_t tag) {
    lpbD_checktype(e, f, tag);
    lpbD_rawfield(e, f);
}

static void lpbD_map(lpb_Env *e, const pb_Field *f) {
    lua_State *L = e->L;
    pb_Slice p, *s = e->s;
    int mask = 0, top = lua_gettop(L);
    uint32_t tag;
    lpb_readbytes(L, s, &p);
    if (f->type == NULL) return;
    lua_pushnil(L);
    lua_pushnil(L);
    while (pb_readvarint32(&p, &tag)) {
        int n = pb_gettag(tag);
        if (n == 1 || n == 2) {
            mask |= n;
            lpb_withinput(e, &p, lpbD_field(e, pb_field(f->type, n), tag));
            lua_replace(L, top+n);
        }
    }
    if (!(mask & 1) && lpb_pushdeffield(L, e->LS, pb_field(f->type, 1), 1))
        lua_replace(L, top + 1), mask |= 1;
    if (!(mask & 2) && lpb_pushdeffield(L, e->LS, pb_field(f->type, 2), 1))
        lua_replace(L, top + 2), mask |= 2;
    if (mask == 3) lua_rawset(L, -3); else lua_pop(L, 2);
}

static void lpbD_repeated(lpb_Env *e, const pb_Field *f, uint32_t tag) {
    lua_State *L = e->L;
    if (pb_gettype(tag) != PB_TBYTES
            || (!f->packed && pb_wtypebytype(f->type_id) == PB_TBYTES)) {
        lpbD_field(e, f, tag);
        lua_rawseti(L, -2, (lua_Integer)lua_rawlen(L, -2) + 1);
    } else {
        int len = (int)lua_rawlen(L, -1);
        pb_Slice p, *s = e->s;
        lpb_readbytes(L, s, &p);
        while (p.p < p.end) {
            lpb_withinput(e, &p, lpbD_rawfield(e, f));
            lua_rawseti(L, -2, ++len);
        }
    }
}

static int lpbD_message(lpb_Env *e, const pb_Type *t) {
    lua_State *L = e->L;
    pb_Slice *s = e->s;
    uint32_t tag;
    luaL_checkstack(L, t->field_count * 2, "not enough stack space for fields");
    while (pb_readvarint32(s, &tag)) {
        const pb_Field *f = pb_field(t, pb_gettag(tag));
        if (f == NULL)
            pb_skipvalue(s, tag);
        else if (f->type && f->type->is_map) {
            lpb_fetchtable(e, f);
            lpbD_checktype(e, f, tag);
            lpbD_map(e, f);
            lua_pop(L, 1);
        } else if (f->repeated) {
            lpb_fetchtable(e, f);
            lpbD_repeated(e, f, tag);
            lua_pop(L, 1);
        } else {
            lua_pushstring(L, (const char*)f->name);
            if (f->oneof_idx) {
                lua_pushstring(L, (const char*)pb_oneofname(t, f->oneof_idx));
                lua_pushvalue(L, -2);
                lua_rawset(L, -4);
            }
            lpbD_field(e, f, tag);
            lua_rawset(L, -3);
        }
    }
    if (e->LS->use_dec_hooks) lpb_usedechooks(L, e->LS, t);
    return 1;
}

static int lpbD_decode(lua_State *L, pb_Slice s, int start) {
    lpb_State *LS = lpb_lstate(L);
    const pb_Type *t = lpb_type(LS, lpb_checkslice(L, 1));
    lpb_Env e;
    argcheck(L, t!=NULL, 1, "type '%s' does not exists", lua_tostring(L, 1));
    lua_settop(L, start);
    if (!lua_istable(L, start)) {
        lua_pop(L, 1);
        lpb_pushtypetable(L, LS, t);
    }
    e.L = L, e.LS = LS, e.s = &s;
    return lpbD_message(&e, t);
}

static int Lpb_decode(lua_State *L) {
    return lpbD_decode(L, lua_isnoneornil(L, 2) ?
            pb_lslice(NULL, 0) :
            lpb_checkslice(L, 2), 3);
}


void lpb_pushunpackdef(lua_State* L, lpb_State* LS, const pb_Type* t, pb_Field** l, int top) {
    unsigned int i;
    int mode = LS->encode_mode;
    mode = t->is_proto3 && mode == LPB_DEFDEF ? LPB_COPYDEF : mode;
    if (mode != LPB_COPYDEF && mode != LPB_METADEF) return;

    for (i = 0; i < t->field_count; i++) {
        int idx = top + i + 1;
        if (lua_isnoneornil(L, idx) && lpb_pushdeffield(L, LS, l[i], t->is_proto3)) {
            lua_replace(L, idx);
        }
    }
}

static int lpb_unpackfield(lpb_Env *e, const pb_Field* f, uint32_t tag, int last) {
    if (!f) {
        pb_skipvalue(e->s, tag);
        return 0;
    }
    if (f->type && f->type->is_map) {
        lpbD_checktype(e, f, tag);
        if (!last) lua_newtable(e->L);
        lpbD_map(e, f);
    }
    else if (f->repeated) {
        if (!last) lua_newtable(e->L);
        lpbD_repeated(e, f, tag);
    }
    else {
        lpbD_field(e, f, tag);
    }
    return f->sort_index;
}

static int lpbD_unpack(lpb_Env* e, const pb_Type* t) {
    lua_State* L = e->L;
    int top = lua_gettop(L);
    uint32_t tag;
    int last_index = 0;
    unsigned int decode_count = 0;
    pb_Field **list = pb_sortfield((pb_Type*)t);
    lua_settop(L, top + t->field_count);
    luaL_checkstack(L, t->field_count * 2, "not enough stack space for fields");
    while (pb_readvarint32(e->s, &tag)) {
        const pb_Field* f = pb_field(t, pb_gettag(tag));
        if (last_index && (!f || f->sort_index != last_index)) {
            decode_count++;
            lua_replace(L, top + last_index);
            last_index = 0;
        }
        last_index = lpb_unpackfield(e, f, tag, last_index);
    }
    if (last_index) {
        decode_count++;
        lua_replace(L, top + last_index);
    }
    if (decode_count != t->field_count) lpb_pushunpackdef(L, e->LS, t, list, top);
    return t->field_count;
}

static int Lpb_unpack(lua_State* L) {
    lpb_State* LS = lpb_lstate(L);
    const pb_Type* t = lpb_type(LS, lpb_checkslice(L, 1));
    pb_Slice s = lpb_checkslice(L, 2);
    lpb_Env e;
    e.L = L, e.LS = LS, e.s = &s;
    argcheck(L, t != NULL, 1, "type '%s' does not exists", lua_tostring(L, 1));
    return lpbD_unpack(&e, t);
}

/* pb module interface */

static int Lpb_option(lua_State *L) {
#define OPTS(X) \
    X(0,  enum_as_name,         LS->enum_as_value = 0)               \
    X(1,  enum_as_value,        LS->enum_as_value = 1)               \
    X(2,  int64_as_number,      LS->int64_mode = LPB_NUMBER)         \
    X(3,  int64_as_string,      LS->int64_mode = LPB_STRING)         \
    X(4,  int64_as_hexstring,   LS->int64_mode = LPB_HEXSTRING)      \
    X(5,  encode_order,         LS->encode_order = 1)                \
    X(6,  no_encode_order,      LS->encode_order = 0)                \
    X(7,  encode_default_values, LS->encode_default_values = 1)      \
    X(8,  no_encode_default_values, LS->encode_default_values = 0)   \
    X(9,  auto_default_values,  LS->encode_mode = LPB_DEFDEF)        \
    X(10, no_default_values,    LS->encode_mode = LPB_NODEF)         \
    X(11, use_default_values,   LS->encode_mode = LPB_COPYDEF)       \
    X(12, use_default_metatable, LS->encode_mode = LPB_METADEF)      \
    X(13, decode_default_array, LS->decode_default_array = 1)        \
    X(14, no_decode_default_array, LS->decode_default_array = 0)     \
    X(15, decode_default_message, LS->decode_default_message = 1)    \
    X(16, no_decode_default_message, LS->decode_default_message = 0) \
    X(17, enable_hooks,         LS->use_dec_hooks = 1)               \
    X(18, disable_hooks,        LS->use_dec_hooks = 0)               \
    X(19, enable_enchooks,      LS->use_enc_hooks = 1)               \
    X(20, disable_enchooks,     LS->use_enc_hooks = 0)               \

    static const char *opts[] = {
#define X(ID,NAME,CODE) #NAME,
        OPTS(X)
#undef  X
        NULL
    };
    lpb_State *LS = lpb_lstate(L);
    switch (luaL_checkoption(L, 1, NULL, opts)) {
#define X(ID,NAME,CODE) case ID: CODE; break;
        OPTS(X)
#undef  X
    }
    return 0;
#undef  OPTS
}

LUALIB_API int luaopen_pb(lua_State *L) {
    luaL_Reg libs[] = {
#define ENTRY(name) { #name, Lpb_##name }
        ENTRY(clear),
        ENTRY(load),
        ENTRY(loadfile),
        ENTRY(encode),
        ENTRY(decode),
        ENTRY(types),
        ENTRY(fields),
        ENTRY(type),
        ENTRY(field),
        ENTRY(typefmt),
        ENTRY(enum),
        ENTRY(defaults),
        ENTRY(hook),
        ENTRY(encode_hook),
        ENTRY(tohex),
        ENTRY(fromhex),
        ENTRY(result),
        ENTRY(option),
        ENTRY(state),
        ENTRY(pack),
        ENTRY(unpack),
#undef  ENTRY
        { NULL, NULL }
    };
    luaL_Reg meta[] = {
        { "__gc", Lpb_delete },
        { "setdefault", Lpb_state },
        { NULL, NULL }
    };
    if (luaL_newmetatable(L, PB_STATE)) {
        luaL_setfuncs(L, meta, 0);
        lua_pushvalue(L, -1);
        lua_setfield(L, -2, "__index");
    }
    luaL_newlib(L, libs);
    return 1;
}

static int Lpb_decode_unsafe(lua_State *L) {
    const char *data = (const char *)lua_touserdata(L, 2);
    size_t size = (size_t)luaL_checkinteger(L, 3);
    if (data == NULL) typeerror(L, 2, "userdata");
    return lpbD_decode(L, pb_lslice(data, size), 4);
}

static int Lpb_slice_unsafe(lua_State *L) {
    const char *data = (const char *)lua_touserdata(L, 1);
    size_t size = (size_t)luaL_checkinteger(L, 2);
    if (data == NULL) typeerror(L, 1, "userdata");
    return lpb_newslice(L, data, size);
}

static int Lpb_touserdata(lua_State *L) {
    pb_Slice s = lpb_toslice(L, 1);
    lua_pushlightuserdata(L, (void*)s.p);
    lua_pushinteger(L, pb_len(s));
    return 2;
}

static int Lpb_use(lua_State *L) {
    const char *opts[] = { "global", "local", NULL };
    lpb_State *LS = lpb_lstate(L);
    const pb_State *GS = global_state;
    switch (luaL_checkoption(L, 1, NULL, opts)) {
    case 0: if (GS) LS->state = GS; break;
    case 1: LS->state = &LS->local; break;
    }
    lua_pushboolean(L, GS != NULL);
    return 1;
}

LUALIB_API int luaopen_pb_unsafe(lua_State *L) {
    luaL_Reg libs[] = {
        { "decode",     Lpb_decode_unsafe },
        { "slice",      Lpb_slice_unsafe  },
        { "touserdata", Lpb_touserdata    },
        { "use",        Lpb_use           },
        { NULL, NULL }
    };
    luaL_newlib(L, libs);
    return 1;
}


PB_NS_END

/* cc: flags+='-O3 -ggdb -pedantic -std=c90 -Wall -Wextra --coverage'
 * maccc: flags+='-ggdb -shared -undefined dynamic_lookup' output='pb.so'
 * win32cc: flags+='-s -mdll -DLUA_BUILD_AS_DLL ' output='pb.dll' libs+='-llua54' */

