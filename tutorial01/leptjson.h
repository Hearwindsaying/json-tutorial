#ifndef LEPTJSON_H__
#define LEPTJSON_H__

typedef enum { LEPT_NULL, LEPT_FALSE, LEPT_TRUE, LEPT_NUMBER, LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT } lept_type;

typedef struct {
    lept_type type;
}lept_value;

enum {
    LEPT_PARSE_OK = 0,
    LEPT_PARSE_EXPECT_VALUE,        /* Json string is empty. */
    LEPT_PARSE_INVALID_VALUE,       /* Not supported value. */
    LEPT_PARSE_ROOT_NOT_SINGULAR    /* After a json value, a blank is followed by a character. */
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
 * @return Return type stored in v.
 */
lept_type lept_get_type(const lept_value* v);

#endif /* LEPTJSON_H__ */
