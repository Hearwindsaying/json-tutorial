#include "leptjson.h"
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

typedef struct {
    /* Current parsing position of json string. */
    const char* json;

    /* Stack */
    char* stack;
    size_t size, top;
}lept_context;

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
    if (c->top + size >= c->size) {
        if (c->size == 0)
        {
            c->size = LEPT_PARSE_STACK_INIT_SIZE;

            c->stack = new char[c->size];
        }
            
        while (c->top + size >= c->size)
        {
            char *oldStack = c->stack;
            c->stack = new char[c->size];
            memcpy(c->stack, oldStack, c->size);
            delete oldStack;

            c->size += c->size >> 1;  /* c->size * 1.5 */
        }
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


static int lept_parse_literal(lept_context* c, lept_value* v, std::string_view literal, lept_type type) {
    assert(*c->json == literal[0]);
    for (const auto& lit : literal)
    {
        if (*(c->json)++ != lit)
            return LEPT_PARSE_INVALID_VALUE;
    }
    v->type = type;
    return LEPT_PARSE_OK;
}

static int lept_parse_number(lept_context* c, lept_value* v) {
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
    v->type = LEPT_NUMBER;
    return LEPT_PARSE_OK;
}


#define PUTC(c, ch) do { *static_cast<char*>(lept_context_push(c, sizeof(char))) = (ch); } while(0)
static int lept_parse_string(lept_context* c, lept_value* v) {
    size_t head = c->top, len;
    const char* p;
    EXPECT(c, '\"');
    p = c->json;
    for (;;) {
        char ch = *p++;
        switch (ch) {
        case '\"':
            len = c->top - head;
            lept_set_string(v, static_cast<const char*>(lept_context_pop(c, len)), len);
            c->json = p;
            return LEPT_PARSE_OK;
        case '\0':
            c->top = head;
            return LEPT_PARSE_MISS_QUOTATION_MARK;
        default:
            PUTC(c, ch);
        }
    }
}

static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case 't':  return lept_parse_literal(c, v, "true", LEPT_TRUE);
        case 'f':  return lept_parse_literal(c, v, "false", LEPT_FALSE);
        case 'n':  return lept_parse_literal(c, v, "null", LEPT_NULL);
        default:   return lept_parse_number(c, v);
        case '"':  return lept_parse_string(c, v);
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
    }
}

int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    assert(v != nullptr);
    c.json = json;
    c.stack = nullptr;
    c.size = c.top = 0;
    lept_init(v);
    lept_parse_whitespace(&c);

    int parseErrorCode = lept_parse_value(&c, v);

    assert(c.top == 0);
    delete c.stack;

    if (parseErrorCode != LEPT_PARSE_OK)
        return parseErrorCode;

    lept_parse_whitespace(&c);
    if (*c.json != '\0')
    {
        v->type = LEPT_NULL;
        return LEPT_PARSE_ROOT_NOT_SINGULAR;
    }
    else
        return parseErrorCode;
}

void lept_free(lept_value* v) 
{
    assert(v != nullptr);
    if (v->type == LEPT_STRING)
    {
        assert(std::holds_alternative<lept_value::JString>(v->value));
        delete std::get<lept_value::JString>(v->value).s;
    }
       
    v->type = LEPT_NULL;
}

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

int lept_get_boolean(const lept_value* v) {
    assert(v != nullptr && (v->type == LEPT_FALSE || v->type == LEPT_TRUE));
    return v->type == LEPT_TRUE;
}

void lept_set_boolean(lept_value* v, int b) {
    lept_free(v);
    v->type = b ? LEPT_TRUE : LEPT_FALSE;
}

double lept_get_number(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return std::get<double>(v->value);
}

void lept_set_number(lept_value* v, double n) {
    lept_free(v);
    v->type = LEPT_NUMBER;
    v->value = n;
}

const char* lept_get_string(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_STRING);
    return std::get<lept_value::JString>(v->value).s;
}

size_t lept_get_string_length(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_STRING);
    return std::get<lept_value::JString>(v->value).len;
}

void lept_set_string(lept_value * v, const char * s, size_t len)
{
    assert(v != nullptr && (s != nullptr || len == 0));
    lept_free(v);

    lept_value::JString jstring;
    jstring.s = new char[len + 1];
    memcpy(jstring.s, s, len);
    jstring.s[len] = '\0';
    jstring.len = len;

    v->value = jstring;
    v->type = LEPT_STRING;
}
