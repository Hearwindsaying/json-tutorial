#ifndef LEPTJSON_H__
#define LEPTJSON_H__

#include <variant>
#include <stddef.h> /* size_t */

typedef enum { LEPT_NULL, LEPT_FALSE, LEPT_TRUE, LEPT_NUMBER, LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT } lept_type;

/**
 * @brief This struct is used to store result value from parsed json string.
 */
struct lept_value{
    /* Type declaration: */
    struct JString
    {
        char* s;    /* string ended with '\0' */
        size_t len; /* length of string */
    };


    /* Member declaration: */
    std::variant<JString, double> value;

    /* json type */
    lept_type type;
};

enum {
    LEPT_PARSE_OK = 0,
    LEPT_PARSE_EXPECT_VALUE,        /* Json string is empty. */
    LEPT_PARSE_INVALID_VALUE,       /* Not supported value. */
    LEPT_PARSE_ROOT_NOT_SINGULAR,   /* After a json value, a blank is followed by a character. */
    LEPT_PARSE_NUMBER_TOO_BIG       /* Number is too big. */
};

/**
 * @brief Parse |json| string and write to |v|.
 * @param[in]  json the json string to be parsed
 * @param[out] v    value
 * @return Return error code for parsing.
 */
int lept_parse(lept_value* v, const char* json);

/**
 * @brief Retrieve a type from |v|.
 * @param[in] v value that has been parsed before
 * @return Return type stored in |v|.
 */
lept_type lept_get_type(const lept_value* v);

/**
 * @brief Retrieve the number from |v|.
 * @param[in] v value that has been parsed before
 * @return Return number stored in |v|.
 */
double lept_get_number(const lept_value* v);

#endif /* LEPTJSON_H__ */
