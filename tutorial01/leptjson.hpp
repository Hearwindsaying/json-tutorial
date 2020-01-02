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

    /* json type, todo:remove */
    lept_type type;
};

enum {
    LEPT_PARSE_OK = 0,
    LEPT_PARSE_EXPECT_VALUE,          /* Json string is empty. */
    LEPT_PARSE_INVALID_VALUE,         /* Not supported value. */
    LEPT_PARSE_ROOT_NOT_SINGULAR,     /* After a json value, a blank is followed by a character. */
    LEPT_PARSE_NUMBER_TOO_BIG,        /* Number is too big. */
    LEPT_PARSE_MISS_QUOTATION_MARK,
    LEPT_PARSE_INVALID_STRING_ESCAPE,
    LEPT_PARSE_INVALID_STRING_CHAR
};

#define lept_init(v) do { (v)->type = LEPT_NULL; } while(0)

/**
 * @brief Parse |json| string and write to |v|.
 * @param[in]  json the json string to be parsed
 * @param[out] v    value
 * @return Return error code for parsing.
 */
int lept_parse(lept_value* v, const char* json);

/**
 * @brief Deallocate space allocated for string storage in |v|
 * if it owns a string. Set |v| type to LEPT_NULL as well.
 * @param[in] v
 */
void lept_free(lept_value* v);

/**
 * @brief Retrieve a type from |v|.
 * @param[in] v value that has been parsed before
 * @return Return type stored in |v|.
 */
lept_type lept_get_type(const lept_value* v);

#define lept_set_null(v) lept_free(v)

int lept_get_boolean(const lept_value* v);
void lept_set_boolean(lept_value* v, int b);

/**
 * @brief Retrieve the number from |v|.
 * @param[in] v value that has been parsed before
 * @return Return number stored in |v|.
 */
double lept_get_number(const lept_value* v);
void lept_set_number(lept_value* v, double n);


const char* lept_get_string(const lept_value* v);
size_t lept_get_string_length(const lept_value* v);
/**
 * @brief Modify string stored in lept_value.
 * @param[in] v 
 * @param[in] s
 * @param[in] len length of |s|
 */
void lept_set_string(lept_value* v, const char* s, size_t len);

#endif /* LEPTJSON_H__ */
