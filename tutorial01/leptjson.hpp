#ifndef LEPTJSON_H__
#define LEPTJSON_H__

#include <variant>
#include <stddef.h> /* size_t */

namespace leptjson
{
    enum class ELeptType { LEPT_NULL, LEPT_FALSE, LEPT_TRUE, LEPT_NUMBER, LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT };

#define LEPT_KEY_NOT_EXIST ((size_t)-1)

    struct Lept_value;
    struct Lept_member;
    /**
     * @brief This struct is used to store result value from parsed json string.
     */
    struct Lept_value {
        /* Type declaration: */
        struct JString
        {
            char* s;    /* string ended with '\0' */
            size_t len; /* length of string */
        };

        struct JArray
        {
            Lept_value *e;
            size_t size;
        };

        struct JObject
        {
            Lept_member* m;
            size_t size;
        };


        /* Member declaration: */
        std::variant<JObject, JString, JArray, double> value;

        /* json type, todo:remove */
        ELeptType type;
    };

    struct Lept_member
    {
        char* k; size_t klen;   /* member key string, key string length */
        Lept_value v;           /* member value */
    };

    enum class ELEPT_PARSE_ECODE
    {
        LEPT_PARSE_OK = 0,
        LEPT_PARSE_EXPECT_VALUE,          /* Json string is empty. */
        LEPT_PARSE_INVALID_VALUE,         /* Not supported value. */
        LEPT_PARSE_ROOT_NOT_SINGULAR,     /* After a json value, a blank is followed by a character. */
        LEPT_PARSE_NUMBER_TOO_BIG,        /* Number is too big. */
        LEPT_PARSE_MISS_QUOTATION_MARK,
        LEPT_PARSE_INVALID_STRING_ESCAPE,
        LEPT_PARSE_INVALID_STRING_CHAR,
        LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, /* Array lacking of comma or bracket. */
        LEPT_PARSE_MISS_KEY,
        LEPT_PARSE_MISS_COLON,
        LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET
    };

#define lept_init(v) do { (v)->type = ELeptType::LEPT_NULL; } while(0)

    /**
     * @brief Parse |json| string and write to |v|.
     * @param[in]  json the json string to be parsed
     * @param[out] v    value
     * @return Return error code for parsing.
     */
    ELEPT_PARSE_ECODE lept_parse(Lept_value* v, const char* json);

    /**
     * @brief Deallocate space allocated for string storage in |v|
     * if it owns a string. Set |v| type to LEPT_NULL as well.
     * @param[in] v
     */
    void lept_free(Lept_value* v);

    /**
     * @brief Retrieve a type from |v|.
     * @param[in] v value that has been parsed before
     * @return Return type stored in |v|.
     */
    ELeptType lept_get_type(const Lept_value* v);

#define lept_set_null(v) lept_free(v)

    int lept_get_boolean(const Lept_value* v);
    void lept_set_boolean(Lept_value* v, int b);

    /**
     * @brief Retrieve the number from |v|.
     * @param[in] v value that has been parsed before
     * @return Return number stored in |v|.
     */
    double lept_get_number(const Lept_value* v);
    void lept_set_number(Lept_value* v, double n);


    const char* lept_get_string(const Lept_value* v);
    size_t lept_get_string_length(const Lept_value* v);
    /**
     * @brief Modify string stored in lept_value.
     * @param[in] v
     * @param[in] s
     * @param[in] len length of |s|
     */
    void lept_set_string(Lept_value* v, const char* s, size_t len);

    /**
     * @brief Get array size from |v|.
     * @note Note that |v| should own an JArray.
     * @param[in] v
     * @return size of array
     */
    size_t lept_get_array_size(const Lept_value* v);
    Lept_value* lept_get_array_element(const Lept_value* v, size_t index);


    size_t lept_get_object_size(const Lept_value* v);
    const char* lept_get_object_key(const Lept_value* v, size_t index);
    size_t lept_get_object_key_length(const Lept_value* v, size_t index);
    Lept_value* lept_get_object_value(const Lept_value* v, size_t index);

    size_t lept_find_object_index(const Lept_value* v, const char* key, size_t klen);
    Lept_value* lept_find_object_value(const Lept_value* v, const char* key, size_t klen);
}



#endif /* LEPTJSON_H__ */
