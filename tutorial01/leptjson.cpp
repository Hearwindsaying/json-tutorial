#include "leptjson.h"
#include <cassert>  /* assert() */
#include <cstdlib>  /* NULL, strtod() */
#include <cerrno>   /* errno */
#include <string_view>

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')

typedef struct {
    /* Current parsing position of json string. */
    const char* json;
}lept_context;

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

static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case 'n':  return lept_parse_literal(c, v, "null", LEPT_NULL);
        case 't':  return lept_parse_literal(c, v, "true", LEPT_TRUE);
        case 'f':  return lept_parse_literal(c, v, "false", LEPT_FALSE);
        default:   return lept_parse_number(c, v);
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
    }
}

int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    assert(v != NULL);
    c.json = json;
    v->type = LEPT_NULL;
    lept_parse_whitespace(&c);

    int parseErrorCode = lept_parse_value(&c, v);
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

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

double lept_get_number(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return std::get<double>(v->value);
}