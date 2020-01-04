#define  CL_CHECK_MEMORY_LEAKS
#ifdef CL_CHECK_MEMORY_LEAKS
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#define CL_CHECK_MEMORY_LEAKS_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new CL_CHECK_MEMORY_LEAKS_NEW
#endif
#include "leptjson.hpp"
#include <cassert>  /* assert() */
#include <cstdlib>  /* NULL, strtod() */
#include <cerrno>   /* errno */
#include <string_view>
#include <string>

#ifndef LEPT_PARSE_STACK_INIT_SIZE
#define LEPT_PARSE_STACK_INIT_SIZE 256
#endif

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')

struct lept_context 
{
    /* Current parsing position of json string. */
    const char* json;

    /* Stack */
    char *stack;
    size_t size, top;
};

/**
 * @brief Push |size| bytes (one element) into the stack in |c|.
 * @note Note that the content which is needed to store should
 * be assigned using return value from the function
 * @param[in] c
 * @param[in] size size of the element in bytes
 * @return Return pointer to start position of data that needed 
 * to be assigned.
 */
static void* lept_context_push(lept_context* c, size_t size) {
    void* ret;
    assert(size > 0);

    size_t orgStackSize = c->size;
    if (c->top + size >= c->size) {
        if (c->size == 0)
        {
            c->size = LEPT_PARSE_STACK_INIT_SIZE;
            c->stack = new char[c->size];
        }

        while (c->top + size >= c->size)
        {
            c->size += c->size >> 1;  /* c->size * 1.5 */
        }
        char *oldStack = c->stack;
        c->stack = new char[c->size];
        memcpy(c->stack, oldStack, orgStackSize);
        delete[] oldStack;
        
    }
    ret = c->stack + c->top;
    c->top += size;
    return ret;
}

static void* lept_context_pop(lept_context* c, size_t size) {
    assert(c->top >= size);
    return c->stack + (c->top -= size);
}

static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}


static int lept_parse_literal(lept_context* c, Lept_value* v, std::string_view literal, ELeptType type) {
    assert(*c->json == literal[0]);
    for (const auto& lit : literal)
    {
        if (*(c->json)++ != lit)
            return LEPT_PARSE_INVALID_VALUE;
    }
    v->type = type;
    return LEPT_PARSE_OK;
}

static int lept_parse_number(lept_context* c, Lept_value* v) {
    /* validate number */
    const char *p = c->json;
    if (*p == '-')
        ++p;
    if (*p == '0')
        ++p;
    else if (ISDIGIT1TO9(*p))
    {
        ++p; /* could be ignored */
        while (ISDIGIT(*p))
            ++p;
    }
    else
        return LEPT_PARSE_INVALID_VALUE;

    if (*p == '.')
    {
        ++p; 
        if (!ISDIGIT(*p)) 
            return LEPT_PARSE_INVALID_VALUE;
        /* ++p; could be ignored, same as above (and below) */
        while (ISDIGIT(*p))
            ++p;
    }

    if (*p == 'e' || *p == 'E')
    {
        ++p;
        if (*p == '+' || *p == '-')
            ++p;
        if (!ISDIGIT(*p))
            return LEPT_PARSE_INVALID_VALUE;
        while (ISDIGIT(*p))
            ++p;
    }

    errno = 0;
    v->value = strtod(c->json, nullptr);
    if (errno == ERANGE && (std::get<double>(v->value) == HUGE_VAL || std::get<double>(v->value) == -HUGE_VAL))
        return LEPT_PARSE_NUMBER_TOO_BIG;
    c->json = p;
    v->type = ELeptType::LEPT_NUMBER;
    return LEPT_PARSE_OK;
}

#define PUTC(c, ch) do { *static_cast<char*>(lept_context_push(c, sizeof(char))) = (ch); } while(0)

/**
 * @brief Parse JSON String and write the result to |str| and |len|
 * @param[in] c
 * @param[out] str str is pointed to s->stack element as a temporary string
 * @param[out] len len of str
 * @return Error code.
 */
static int lept_parse_string_raw(lept_context* c, char** str, size_t* len) {
    size_t head = c->top;
    const char* p;
    EXPECT(c, '\"');
    p = c->json;
    for (;;) {
        char ch = *p++;
        switch (ch) {
        case '\"':
            *len = c->top - head;
            *str = static_cast<char *>(lept_context_pop(c, *len));
            c->json = p;
            return LEPT_PARSE_OK;
        case '\\':
            switch (*p++) {
            case '\"': PUTC(c, '\"'); break;
            case '\\': PUTC(c, '\\'); break;
            case '/':  PUTC(c, '/'); break;
            case 'b':  PUTC(c, '\b'); break;
            case 'f':  PUTC(c, '\f'); break;
            case 'n':  PUTC(c, '\n'); break;
            case 'r':  PUTC(c, '\r'); break;
            case 't':  PUTC(c, '\t'); break;
            default:
                c->top = head;
                return LEPT_PARSE_INVALID_STRING_ESCAPE;
            }
            break;
        case '\0':
            c->top = head;
            return LEPT_PARSE_MISS_QUOTATION_MARK;
        default:
            if (static_cast<unsigned char>(ch) < 0x20) {
                c->top = head;
                return LEPT_PARSE_INVALID_STRING_CHAR;
            }
            PUTC(c, ch);
        }
    }
}

static int lept_parse_string(lept_context* c, Lept_value* v) {
    int ret;
    char* s;
    size_t len;
    if ((ret = lept_parse_string_raw(c, &s, &len)) == LEPT_PARSE_OK)
        lept_set_string(v, s, len);
    return ret;
}

static int lept_parse_value(lept_context* c, Lept_value* v);

static int lept_parse_array(lept_context* c, Lept_value* v) {
    size_t size = 0;
    int ret;
    EXPECT(c, '[');

    lept_parse_whitespace(c);

    if (*c->json == ']') {
        c->json++;
        
        v->type = ELeptType::LEPT_ARRAY;

        Lept_value::JArray jarray;
        jarray.size = 0;
        jarray.e = nullptr;
        
        v->value = jarray;
        return LEPT_PARSE_OK;
    }
    for (;;) {
        Lept_value e;
        lept_init(&e);
        if ((ret = lept_parse_value(c, &e)) != LEPT_PARSE_OK)
        {
            break;
        }

        //memcpy(lept_context_push(c, sizeof(lept_value)), &e, sizeof(lept_value));
        *static_cast<Lept_value *>(lept_context_push(c, sizeof(Lept_value))) = e;

        size++;
        lept_parse_whitespace(c);
        if (*c->json == ',')
        {
            c->json++;
            lept_parse_whitespace(c);
        }
        else if (*c->json == ']') {
            c->json++;
            v->type = ELeptType::LEPT_ARRAY;

            Lept_value::JArray jarray;
            jarray.size = size;
            jarray.e = new Lept_value[size];
           // memcpy(jarray.e, lept_context_pop(c, size * sizeof(lept_value)), size * sizeof(lept_value));
            for (int i = size-1; i >= 0; --i)
            {
                /* Note that elements poping from stack is in reversed order. */
                jarray.e[i] = *static_cast<Lept_value *>(lept_context_pop(c, sizeof(Lept_value)));
            }

            v->value = jarray;

            return LEPT_PARSE_OK;
        }
        else
        {
            ret = LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
            break;
        }
            
    }
    /* Pop and free values on the stack */
    for (int i = 0; i < size; i++)
        lept_free(static_cast<Lept_value*>(lept_context_pop(c, sizeof(Lept_value))));
    return ret;
}

static int lept_parse_object(lept_context* c, Lept_value* v) 
{
    size_t size;
    Lept_member m;
    int ret;
    EXPECT(c, '{');
    lept_parse_whitespace(c);
    if (*c->json == '}') {
        c->json++;
        v->type = ELeptType::LEPT_OBJECT;

        Lept_value::JObject jobject;
        jobject.size = 0;
        jobject.m = nullptr;
        
        v->value = jobject;
        return LEPT_PARSE_OK;
    }
    m.k = nullptr;
    size = 0;
    for (;;) {
        char* str;
        lept_init(&m.v);
        /* parse key */
        if (*c->json != '"') 
        {
            ret = LEPT_PARSE_MISS_KEY;
            break;
        }
        if ((ret = lept_parse_string_raw(c, &str, &m.klen)) != LEPT_PARSE_OK)
            break;
        m.k = new char[m.klen + 1];
        memcpy(m.k, str, m.klen);
        m.k[m.klen] = '\0';
        /* parse ws colon ws */
        lept_parse_whitespace(c);
        if (*c->json != ':') 
        {
            ret = LEPT_PARSE_MISS_COLON;
            break;
        }
        c->json++;
        lept_parse_whitespace(c);
        /* parse value */
        if ((ret = lept_parse_value(c, &m.v)) != LEPT_PARSE_OK)
            break;
        //memcpy(lept_context_push(c, sizeof(lept_member)), &m, sizeof(lept_member));
        *static_cast<Lept_member *>(lept_context_push(c, sizeof(Lept_member))) = m;
        size++;
        m.k = nullptr; /* ownership is transferred to member on stack */
        /* parse ws [comma | right-curly-brace] ws */
        lept_parse_whitespace(c);
        if (*c->json == ',') 
        {
            c->json++;
            lept_parse_whitespace(c);
        }
        else if (*c->json == '}')
        {
            c->json++;
            v->type = ELeptType::LEPT_OBJECT;

            Lept_value::JObject jobject;
            jobject.size = size;
            jobject.m = new Lept_member[size];
            for (int i = size - 1; i >= 0; --i)
            {
                jobject.m[i] = *static_cast<Lept_member *>(lept_context_pop(c, sizeof(Lept_member)));
            }
            //memcpy(jobject.m = new lept_member[size], lept_context_pop(c, size * sizeof(lept_member)), size * sizeof(lept_member));

            v->value = jobject;

            return LEPT_PARSE_OK;
        }
        else
        {
            ret = LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET;
            break;
        }
    }
    /* Pop and free members on the stack */
    if (m.k)
        delete[] m.k;
    for (int i = 0; i < size; i++)
    {
        Lept_member* m = static_cast<Lept_member*>(lept_context_pop(c, sizeof(Lept_member)));
        delete[] m->k;
        lept_free(&m->v);
    }
    v->type = ELeptType::LEPT_NULL;
    return ret;
}

static int lept_parse_value(lept_context* c, Lept_value* v) {
    switch (*c->json) {
        case 't':  return lept_parse_literal(c, v, "true", ELeptType::LEPT_TRUE);
        case 'f':  return lept_parse_literal(c, v, "false", ELeptType::LEPT_FALSE);
        case 'n':  return lept_parse_literal(c, v, "null", ELeptType::LEPT_NULL);
        default:   return lept_parse_number(c, v);
        case '"':  return lept_parse_string(c, v);
        case '[':  return lept_parse_array(c, v);
        case '{':  return lept_parse_object(c, v);
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
    }
}

int lept_parse(Lept_value* v, const char* json) {
    lept_context c;
    assert(v != nullptr);
    c.json = json;
    c.stack = nullptr;
    c.size = c.top = 0;
    lept_init(v);
    lept_parse_whitespace(&c);

    int parseErrorCode = lept_parse_value(&c, v);

    assert(c.top == 0);
    delete[] c.stack;

    if (parseErrorCode != LEPT_PARSE_OK)
        return parseErrorCode;

    lept_parse_whitespace(&c);
    if (*c.json != '\0')
    {
        v->type = ELeptType::LEPT_NULL;
        return LEPT_PARSE_ROOT_NOT_SINGULAR;
    }
    else
        return parseErrorCode;
}

void lept_free(Lept_value* v) 
{
    assert(v != nullptr);

    switch (v->type) {
        case ELeptType::LEPT_STRING:
        {
            assert(std::holds_alternative<Lept_value::JString>(v->value));
            delete[] std::get<Lept_value::JString>(v->value).s;
            break;
        }
            
        case ELeptType::LEPT_ARRAY:
        {
            assert(std::holds_alternative<Lept_value::JArray>(v->value));
            for (int i = 0; i < std::get<Lept_value::JArray>(v->value).size; i++)
                lept_free(&(std::get<Lept_value::JArray>(v->value).e[i]));
            delete[] std::get<Lept_value::JArray>(v->value).e;
            break;
        }

        case ELeptType::LEPT_OBJECT:
        {
            assert(std::holds_alternative<Lept_value::JObject>(v->value));
            for (int i = 0; i < std::get<Lept_value::JObject>(v->value).size; i++)
            {
                delete[] std::get<Lept_value::JObject>(v->value).m[i].k;
                lept_free(&(std::get<Lept_value::JObject>(v->value).m[i].v));
            }
            delete[] std::get<Lept_value::JObject>(v->value).m;
            break;
        }
        default: break;
    }

    v->type = ELeptType::LEPT_NULL;
}

ELeptType lept_get_type(const Lept_value* v) {
    assert(v != nullptr);
    return v->type;
}

int lept_get_boolean(const Lept_value* v) {
    assert(v != nullptr && (v->type == ELeptType::LEPT_FALSE || v->type == ELeptType::LEPT_TRUE));
    return v->type == ELeptType::LEPT_TRUE;
}

void lept_set_boolean(Lept_value* v, int b) {
    lept_free(v);
    v->type = b ? ELeptType::LEPT_TRUE : ELeptType::LEPT_FALSE;
}

double lept_get_number(const Lept_value* v) {
    assert(v != nullptr && v->type == ELeptType::LEPT_NUMBER);
    return std::get<double>(v->value);
}

void lept_set_number(Lept_value* v, double n) {
    lept_free(v);
    v->type = ELeptType::LEPT_NUMBER;
    v->value = n;
}

const char* lept_get_string(const Lept_value* v) {
    assert(v != nullptr && v->type == ELeptType::LEPT_STRING);
    return std::get<Lept_value::JString>(v->value).s;
}

size_t lept_get_string_length(const Lept_value* v) {
    assert(v != nullptr && v->type == ELeptType::LEPT_STRING);
    return std::get<Lept_value::JString>(v->value).len;
}

void lept_set_string(Lept_value * v, const char * s, size_t len)
{
    assert(v != nullptr && (s != nullptr || len == 0));
    lept_free(v);

    Lept_value::JString jstring;
    jstring.s = new char[len + 1];
    memcpy(jstring.s, s, len);
    jstring.s[len] = '\0';
    jstring.len = len;

    v->value = jstring;
    v->type = ELeptType::LEPT_STRING;
}


size_t lept_get_array_size(const Lept_value* v) 
{
    assert(v != nullptr && v->type == ELeptType::LEPT_ARRAY && std::holds_alternative<Lept_value::JArray>(v->value));
    return std::get<Lept_value::JArray>(v->value).size;
}

Lept_value* lept_get_array_element(const Lept_value* v, size_t index) 
{
    assert(v != nullptr && v->type == ELeptType::LEPT_ARRAY && std::holds_alternative<Lept_value::JArray>(v->value));
    assert(index < std::get<Lept_value::JArray>(v->value).size);
    return &(std::get<Lept_value::JArray>(v->value).e[index]);
}

size_t lept_get_object_size(const Lept_value* v) 
{
    assert(v != nullptr && v->type == ELeptType::LEPT_OBJECT && std::holds_alternative<Lept_value::JObject>(v->value));
    return std::get<Lept_value::JObject>(v->value).size;
}

const char* lept_get_object_key(const Lept_value* v, size_t index) 
{
    assert(v != nullptr && v->type == ELeptType::LEPT_OBJECT && std::holds_alternative<Lept_value::JObject>(v->value));
    assert(index < std::get<Lept_value::JObject>(v->value).size);
    return std::get<Lept_value::JObject>(v->value).m[index].k;
}

size_t lept_get_object_key_length(const Lept_value* v, size_t index) 
{
    assert(v != nullptr && v->type == ELeptType::LEPT_OBJECT && std::holds_alternative<Lept_value::JObject>(v->value));
    assert(index < std::get<Lept_value::JObject>(v->value).size);
    return std::get<Lept_value::JObject>(v->value).m[index].klen;
}

Lept_value* lept_get_object_value(const Lept_value* v, size_t index) 
{
    assert(v != nullptr && v->type == ELeptType::LEPT_OBJECT && std::holds_alternative<Lept_value::JObject>(v->value));
    assert(index < std::get<Lept_value::JObject>(v->value).size);
    return &(std::get<Lept_value::JObject>(v->value).m[index]).v;
}

size_t lept_find_object_index(const Lept_value * v, const char * key, size_t klen)
{
    size_t i;
    assert(v != nullptr && v->type == ELeptType::LEPT_OBJECT && key != nullptr && std::holds_alternative<Lept_value::JObject>(v->value));
    for (i = 0; i < std::get<Lept_value::JObject>(v->value).size; i++)
        if (std::get<Lept_value::JObject>(v->value).m[i].klen == klen && 
            memcmp(std::get<Lept_value::JObject>(v->value).m[i].k, key, klen) == 0)
            return i;
    return LEPT_KEY_NOT_EXIST;
}

Lept_value* lept_find_object_value(Lept_value* v, const char* key, size_t klen) 
{
    size_t index = lept_find_object_index(v, key, klen);
    return index != LEPT_KEY_NOT_EXIST ? &(std::get<Lept_value::JObject>(v->value).m[index].v) : nullptr;
}
