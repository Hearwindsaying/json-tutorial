#define  CL_CHECK_MEMORY_LEAKS
#ifdef CL_CHECK_MEMORY_LEAKS
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#define CL_CHECK_MEMORY_LEAKS_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new CL_CHECK_MEMORY_LEAKS_NEW
#endif
#include <cstdio>
#include <cstdlib>
#include <string>
#include <array>
#include "leptjson.hpp"
#include "leptjsonWrapper.hpp"

using namespace leptjson;

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;

#define EXPECT_EQ_BASE(equality, expect, actual, format) \
    do {\
        test_count++;\
        if (equality){\
            fprintf(stderr, "%s:%d: expect: " format " actual: " format "\n", __FILE__, __LINE__, expect, actual);\
            test_pass++;\
        }\
        else {\
            fprintf(stderr, "%s:%d: expect: " format " actual: " format "\n", __FILE__, __LINE__, expect, actual);\
            main_ret = 1;\
        }\
    } while(0)

#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")
#define EXPECT_EQ_DOUBLE(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%.17g")
#define EXPECT_EQ_STRING(expect, actual, alength) \
    EXPECT_EQ_BASE(sizeof(expect) - 1 == alength && memcmp(expect, actual, alength) == 0, expect, actual, "%s")
#define EXPECT_TRUE(actual) EXPECT_EQ_BASE((actual) != 0, "true", "false", "%s")
#define EXPECT_FALSE(actual) EXPECT_EQ_BASE((actual) == 0, "false", "true", "%s")

#define EXPECT_EQ_SIZE_T(expect, actual) EXPECT_EQ_BASE((expect) == (actual), (size_t)expect, (size_t)actual, "%zu")

static void test_parse_null() {
    Lept_value v;
    lept_init(&v);
    lept_set_boolean(&v, 0);
    EXPECT_EQ_INT(ELEPT_PARSE_ECODE::LEPT_PARSE_OK, lept_parse(&v, "null"));
    EXPECT_EQ_INT(ELeptType::LEPT_NULL, lept_get_type(&v));
    lept_free(&v);
}

static void test_parse_true() {
    Lept_value v;
    lept_init(&v);
    lept_set_boolean(&v, 0);
    EXPECT_EQ_INT(ELEPT_PARSE_ECODE::LEPT_PARSE_OK, lept_parse(&v, "true"));
    EXPECT_EQ_INT(ELeptType::LEPT_TRUE, lept_get_type(&v));
    lept_free(&v);
}

static void test_parse_false() {
    Lept_value v;
    lept_init(&v);
    lept_set_boolean(&v, 1);
    EXPECT_EQ_INT(ELEPT_PARSE_ECODE::LEPT_PARSE_OK, lept_parse(&v, "false"));
    EXPECT_EQ_INT(ELeptType::LEPT_FALSE, lept_get_type(&v));
    lept_free(&v);
}

#define TEST_NUMBER(expect, json)\
    do {\
        Lept_value v;\
        lept_init(&v);\
        EXPECT_EQ_INT(ELEPT_PARSE_ECODE::LEPT_PARSE_OK, lept_parse(&v, json));\
        EXPECT_EQ_INT(ELeptType::LEPT_NUMBER, lept_get_type(&v));\
        EXPECT_EQ_DOUBLE(expect, lept_get_number(&v));\
        lept_free(&v);\
    } while(0)

static void test_parse_number() {
    TEST_NUMBER(0.0, "0");
    TEST_NUMBER(0.0, "-0");
    TEST_NUMBER(0.0, "-0.0");
    TEST_NUMBER(1.0, "1");
    TEST_NUMBER(-1.0, "-1");
    TEST_NUMBER(1.5, "1.5");
    TEST_NUMBER(-1.5, "-1.5");
    TEST_NUMBER(3.1416, "3.1416");
    TEST_NUMBER(1E10, "1E10");
    TEST_NUMBER(1e10, "1e10");
    TEST_NUMBER(1E+10, "1E+10");
    TEST_NUMBER(1E-10, "1E-10");
    TEST_NUMBER(-1E10, "-1E10");
    TEST_NUMBER(-1e10, "-1e10");
    TEST_NUMBER(-1E+10, "-1E+10");
    TEST_NUMBER(-1E-10, "-1E-10");
    TEST_NUMBER(1.234E+10, "1.234E+10");
    TEST_NUMBER(1.234E-10, "1.234E-10");
    TEST_NUMBER(0.0, "1e-10000"); /* must underflow */

    TEST_NUMBER(1.0000000000000002, "1.0000000000000002"); /* the smallest number > 1 */
    TEST_NUMBER(4.9406564584124654e-324, "4.9406564584124654e-324"); /* minimum denormal */
    TEST_NUMBER(-4.9406564584124654e-324, "-4.9406564584124654e-324");
    TEST_NUMBER(2.2250738585072009e-308, "2.2250738585072009e-308");  /* Max subnormal double */
    TEST_NUMBER(-2.2250738585072009e-308, "-2.2250738585072009e-308");
    TEST_NUMBER(2.2250738585072014e-308, "2.2250738585072014e-308");  /* Min normal positive double */
    TEST_NUMBER(-2.2250738585072014e-308, "-2.2250738585072014e-308");
    TEST_NUMBER(1.7976931348623157e+308, "1.7976931348623157e+308");  /* Max double */
    TEST_NUMBER(-1.7976931348623157e+308, "-1.7976931348623157e+308");
}

#define TEST_STRING(expect, json)\
    do {\
        Lept_value v;\
        lept_init(&v);\
        EXPECT_EQ_INT(ELEPT_PARSE_ECODE::LEPT_PARSE_OK, lept_parse(&v, json));\
        EXPECT_EQ_INT(ELeptType::LEPT_STRING, lept_get_type(&v));\
        EXPECT_EQ_STRING(expect, lept_get_string(&v), lept_get_string_length(&v));\
        lept_free(&v);\
    } while(0)

static void test_parse_string() {
    TEST_STRING("", "\"\"");
    TEST_STRING("Hello", "\"Hello\"");
#if 1
    TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
    TEST_STRING("\" \\ / \b \f \n \r \t", "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");
#endif
}

static void test_parse_array() {
    Lept_value v;

    lept_init(&v);
    EXPECT_EQ_INT(ELEPT_PARSE_ECODE::LEPT_PARSE_OK, lept_parse(&v, "[ ]"));
    EXPECT_EQ_INT(ELeptType::LEPT_ARRAY, lept_get_type(&v));
    EXPECT_EQ_SIZE_T(0, lept_get_array_size(&v));
    lept_free(&v);

    lept_init(&v);
    EXPECT_EQ_INT(ELEPT_PARSE_ECODE::LEPT_PARSE_OK, lept_parse(&v, "[ null , false , true , 123 , \"abc\" ]"));
    EXPECT_EQ_INT(ELeptType::LEPT_ARRAY, lept_get_type(&v));
    EXPECT_EQ_SIZE_T(5, lept_get_array_size(&v));
    EXPECT_EQ_INT(ELeptType::LEPT_NULL, lept_get_type(lept_get_array_element(&v, 0)));
    EXPECT_EQ_INT(ELeptType::LEPT_FALSE, lept_get_type(lept_get_array_element(&v, 1)));
    EXPECT_EQ_INT(ELeptType::LEPT_TRUE, lept_get_type(lept_get_array_element(&v, 2)));
    EXPECT_EQ_INT(ELeptType::LEPT_NUMBER, lept_get_type(lept_get_array_element(&v, 3)));
    EXPECT_EQ_INT(ELeptType::LEPT_STRING, lept_get_type(lept_get_array_element(&v, 4)));
    EXPECT_EQ_DOUBLE(123.0, lept_get_number(lept_get_array_element(&v, 3)));
    EXPECT_EQ_STRING("abc", lept_get_string(lept_get_array_element(&v, 4)), lept_get_string_length(lept_get_array_element(&v, 4)));
    lept_free(&v);

    lept_init(&v);
    EXPECT_EQ_INT(ELEPT_PARSE_ECODE::LEPT_PARSE_OK, lept_parse(&v, "[ [ ] , [ 0 ] , [ 0 , 1 ] , [ 0 , 1 , 2 ] ]"));
    EXPECT_EQ_INT(ELeptType::LEPT_ARRAY, lept_get_type(&v));
    EXPECT_EQ_SIZE_T(4, lept_get_array_size(&v));
    for (int i = 0; i < 4; i++) {
        Lept_value* a = lept_get_array_element(&v, i);
        EXPECT_EQ_INT(ELeptType::LEPT_ARRAY, lept_get_type(a));
        EXPECT_EQ_SIZE_T(i, lept_get_array_size(a));
        for (int j = 0; j < i; j++) {
            Lept_value* e = lept_get_array_element(a, j);
            EXPECT_EQ_INT(ELeptType::LEPT_NUMBER, lept_get_type(e));
            EXPECT_EQ_DOUBLE(static_cast<double>(j), lept_get_number(e));
        }
    }
    lept_free(&v);


}

static void test_parse_object() {
    Lept_value v;
    size_t i;

    lept_init(&v);
    EXPECT_EQ_INT(ELEPT_PARSE_ECODE::LEPT_PARSE_OK, lept_parse(&v, " { } "));
    EXPECT_EQ_INT(ELeptType::LEPT_OBJECT, lept_get_type(&v));
    EXPECT_EQ_SIZE_T(0, lept_get_object_size(&v));
    lept_free(&v);

    lept_init(&v);
    EXPECT_EQ_INT(ELEPT_PARSE_ECODE::LEPT_PARSE_OK, lept_parse(&v,
        " { "
        "\"a\" : [ 1],"
        "\"a\" : [ 1],"
        "\"a\" : [ 1 ],"
        "\"a\" : [ 1],"
        "\"a\" : [ 1],"
        "\"a\" : [ 1],"
        "\"a\" : [ 1 ],"
        "\"a\" : [ 1],"
        "\"a\" : [ 1],"
        "\"a\" : [ 1],"
        "\"a\" : [ 1 ],"
        "\"a\" : [ 1],"
        "\"a\" : [ 1],"
        "\"a\" : [ 1],"
        "\"a\" : [ 1 ],"
        "\"a\" : [ 1],"
        "\"a\" : [ 1],"
        "\"a\" : [ 1],"
        "\"a\" : [ 1 ],"
        "\"a\" : [ 1],"
        "\"a\" : [ 1 ],"
        "\"a\" : [ 1 ]"
        " } "
    ));
    lept_free(&v);

    lept_init(&v);
    EXPECT_EQ_INT(ELEPT_PARSE_ECODE::LEPT_PARSE_OK, lept_parse(&v,
        " { "
        "\"n\" : null , "
        "\"f\" : false , "
        "\"t\" : true , "
        "\"i\" : 123 , "
        "\"s\" : \"abc\", "
        "\"a\" : [ 1, 2, 3 ],"
        "\"o\" : { \"1\" : 1, \"2\" : 2, \"3\" : 3 }"
        " } "
    ));
    EXPECT_EQ_INT(ELeptType::LEPT_OBJECT, lept_get_type(&v));
    EXPECT_EQ_SIZE_T(7, lept_get_object_size(&v));
    EXPECT_EQ_STRING("n", lept_get_object_key(&v, 0), lept_get_object_key_length(&v, 0));
    EXPECT_EQ_INT(ELeptType::LEPT_NULL, lept_get_type(lept_get_object_value(&v, 0)));
    EXPECT_EQ_STRING("f", lept_get_object_key(&v, 1), lept_get_object_key_length(&v, 1));
    EXPECT_EQ_INT(ELeptType::LEPT_FALSE, lept_get_type(lept_get_object_value(&v, 1)));
    EXPECT_EQ_STRING("t", lept_get_object_key(&v, 2), lept_get_object_key_length(&v, 2));
    EXPECT_EQ_INT(ELeptType::LEPT_TRUE, lept_get_type(lept_get_object_value(&v, 2)));
    EXPECT_EQ_STRING("i", lept_get_object_key(&v, 3), lept_get_object_key_length(&v, 3));
    EXPECT_EQ_INT(ELeptType::LEPT_NUMBER, lept_get_type(lept_get_object_value(&v, 3)));
    EXPECT_EQ_DOUBLE(123.0, lept_get_number(lept_get_object_value(&v, 3)));
    EXPECT_EQ_STRING("s", lept_get_object_key(&v, 4), lept_get_object_key_length(&v, 4));
    EXPECT_EQ_INT(ELeptType::LEPT_STRING, lept_get_type(lept_get_object_value(&v, 4)));
    EXPECT_EQ_STRING("abc", lept_get_string(lept_get_object_value(&v, 4)), lept_get_string_length(lept_get_object_value(&v, 4)));
    EXPECT_EQ_STRING("a", lept_get_object_key(&v, 5), lept_get_object_key_length(&v, 5));
    EXPECT_EQ_INT(ELeptType::LEPT_ARRAY, lept_get_type(lept_get_object_value(&v, 5)));
    EXPECT_EQ_SIZE_T(3, lept_get_array_size(lept_get_object_value(&v, 5)));
    for (i = 0; i < 3; i++) {
        Lept_value* e = lept_get_array_element(lept_get_object_value(&v, 5), i);
        EXPECT_EQ_INT(ELeptType::LEPT_NUMBER, lept_get_type(e));
        EXPECT_EQ_DOUBLE(i + 1.0, lept_get_number(e));
    }
    EXPECT_EQ_STRING("o", lept_get_object_key(&v, 6), lept_get_object_key_length(&v, 6));
    {
        Lept_value* o = lept_get_object_value(&v, 6);
        EXPECT_EQ_INT(ELeptType::LEPT_OBJECT, lept_get_type(o));
        for (i = 0; i < 3; i++) {
            Lept_value* ov = lept_get_object_value(o, i);
            EXPECT_TRUE('1' + i == lept_get_object_key(o, i)[0]);
            EXPECT_EQ_SIZE_T(1, lept_get_object_key_length(o, i));
            EXPECT_EQ_INT(ELeptType::LEPT_NUMBER, lept_get_type(ov));
            EXPECT_EQ_DOUBLE(i + 1.0, lept_get_number(ov));
        }
    }
    lept_free(&v);
}

#define TEST_ERROR(error, json)\
    do {\
        Lept_value v;\
        lept_init(&v);\
        v.type = ELeptType::LEPT_FALSE;\
        EXPECT_EQ_INT(error, lept_parse(&v, json));\
        EXPECT_EQ_INT(ELeptType::LEPT_NULL, lept_get_type(&v));\
        lept_free(&v);\
    } while(0)

static void test_parse_expect_value() {
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_EXPECT_VALUE, "");
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_EXPECT_VALUE, " ");
}

static void test_parse_invalid_value() {
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_INVALID_VALUE, "nul");
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_INVALID_VALUE, "?");

    /* invalid number */
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_INVALID_VALUE, "+0");
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_INVALID_VALUE, "+1");
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_INVALID_VALUE, ".123"); /* at least one digit before '.' */
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_INVALID_VALUE, "1.");   /* at least one digit after '.' */
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_INVALID_VALUE, "INF");
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_INVALID_VALUE, "inf");
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_INVALID_VALUE, "NAN");
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_INVALID_VALUE, "nan");

    /* invalid value in array */
#if 1
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_INVALID_VALUE, "[1,]");
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_INVALID_VALUE, "[\"a\", nul]");
#endif
}

static void test_parse_root_not_singular() {
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_ROOT_NOT_SINGULAR, "null x");

    /* invalid number */
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_ROOT_NOT_SINGULAR, "0123"); /* after zero should be '.' or nothing */
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_ROOT_NOT_SINGULAR, "0x0");
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_ROOT_NOT_SINGULAR, "0x123");
}

static void test_parse_number_too_big() {
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_NUMBER_TOO_BIG, "1e309");
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_NUMBER_TOO_BIG, "-1e309");
}

static void test_parse_missing_quotation_mark() {
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_MISS_QUOTATION_MARK, "\"");
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_MISS_QUOTATION_MARK, "\"abc");
}

static void test_parse_miss_comma_or_square_bracket() {
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1");
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1}");
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1 2");
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[[]");
}

static void test_parse_miss_key() {
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_MISS_KEY, "{:1,");
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_MISS_KEY, "{1:1,");
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_MISS_KEY, "{true:1,");
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_MISS_KEY, "{false:1,");
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_MISS_KEY, "{null:1,");
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_MISS_KEY, "{[]:1,");
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_MISS_KEY, "{{}:1,");
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_MISS_KEY, "{\"a\":1,");
}

static void test_parse_miss_colon() {
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_MISS_COLON, "{\"a\"}");
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_MISS_COLON, "{\"a\",\"b\"}");
}

static void test_parse_miss_comma_or_curly_bracket() {
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1");
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1]");
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1 \"b\"");
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":{}");
}

static void test_parse_invalid_string_escape() {
#if 1
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\v\"");
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\'\"");
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\0\"");
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\x12\"");
#endif
}

static void test_parse_invalid_string_char() {
#if 1
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_INVALID_STRING_CHAR, "\"\x01\"");
    TEST_ERROR(ELEPT_PARSE_ECODE::LEPT_PARSE_INVALID_STRING_CHAR, "\"\x1F\"");
#endif
}

static void test_access_null() {
    Lept_value v;
    lept_init(&v);
    lept_set_string(&v, "a", 1);
    lept_set_null(&v);
    EXPECT_EQ_INT(ELeptType::LEPT_NULL, lept_get_type(&v));
    lept_free(&v);
}

static void test_access_boolean() {
    Lept_value v;
    lept_init(&v);
    lept_set_boolean(&v, 1);
    EXPECT_TRUE(lept_get_boolean(&v));
    lept_free(&v);

    lept_set_boolean(&v, 0);
    EXPECT_FALSE(lept_get_boolean(&v));
    lept_free(&v);
}

static void test_access_number() {
    Lept_value v;
    lept_init(&v);
    lept_set_number(&v, 12.345);
    EXPECT_EQ_DOUBLE(12.345, lept_get_number(&v));
    lept_free(&v);
}

static void test_access_string() {
    Lept_value v;
    lept_init(&v);
    lept_set_string(&v, "", 0);
    EXPECT_EQ_STRING("", lept_get_string(&v), lept_get_string_length(&v));
    lept_set_string(&v, "Hello", 5);
    EXPECT_EQ_STRING("Hello", lept_get_string(&v), lept_get_string_length(&v));
    lept_free(&v);
}

static void test_parse() {
    test_parse_null();
    test_parse_true();
    test_parse_false();
    test_parse_number();
    test_parse_string();
    test_parse_array();
    test_parse_object();
    test_parse_expect_value();
    test_parse_invalid_value();
    test_parse_root_not_singular();
    test_parse_number_too_big();
    test_parse_missing_quotation_mark();
    test_parse_invalid_string_escape();
    test_parse_invalid_string_char();
    test_parse_miss_comma_or_square_bracket();
    test_parse_miss_key();
    test_parse_miss_colon();
    test_parse_miss_comma_or_curly_bracket();

    test_access_null();
    test_access_boolean();
    test_access_number();
    test_access_string();
}

void testjsonScene()
{
    Lept_value v;
    lept_init(&v);
    EXPECT_EQ_INT(ELEPT_PARSE_ECODE::LEPT_PARSE_OK, lept_parse(&v,
        R"--(
        {
        "renderer": {
        "output_file": "cornell-box.png",
        "resume_render_file": "TungstenRenderState.dat",
        "overwrite_output_files": true,
        "adaptive_sampling": true,
        "enable_resume_render": false,
        "stratified_sampler": true,
        "scene_bvh": true,
        "spp": 64,
        "spp_step": 16,
        "checkpoint_interval": "0",
        "timeout": "0",
        "hdr_output_file": "cornell-box.exr"}
        }
        )--"
    ));
    auto lept_valueRenderer = lept_find_object_value(&v, "renderer", 8);
    auto lept_valueRenderer_spp = lept_find_object_value(lept_valueRenderer, "spp", 3);
    lept_free(&v);

    {
        /* 1. Load json string we would like to parse. 
         * You can load from file or imbedding string literal as below: */
        std::string testJson = 
            R"--(
        {
	          "Camera":{
			            "Eye":[-0.160734,-18.9798,5.83172],
			            "LookAt Destination":[0.10083,1.514,4.82932]
			           }
        }
        )--";

        /* 2. Create LeptjsonParser and bind the result, we can use structural
         * binding in C++17. */
        const auto& [status,testParser] = LeptjsonParser::createLeptjsonParser(testJson);

        /* 3. Check parsing status. */
        EXPECT_EQ_INT(leptjson::ELEPT_PARSE_ECODE::LEPT_PARSE_OK, status);
        EXPECT_EQ_INT(leptjson::ELeptType::LEPT_OBJECT, testParser->LeptValueType());
        
        /* 4. Get root jsonvalue using LeptjsonParser::LeptValue(). */
        const auto& sceneJsonValue = testParser->LeptValue();

        /* 5. Get the first object "camera". */
        auto cameraObjectValue = sceneJsonValue["Camera"];

        /* 6. Get "eye" property from camera value, which is an array incorporate 3 components. */
        auto cameraEyeValue = cameraObjectValue["Eye"];
        EXPECT_EQ_INT(leptjson::ELeptType::LEPT_ARRAY, cameraEyeValue.LeptValueType());

        /* 7. Get individual components using LeptjsonValue::getNumber(). */
        std::array<double, 3> eyePosition
        { cameraEyeValue[0].getNumber(),cameraEyeValue[1].getNumber(),cameraEyeValue[2].getNumber() };

        EXPECT_EQ_DOUBLE(-0.160734, eyePosition[0]);
        EXPECT_EQ_DOUBLE(-18.9798, eyePosition[1]);
        EXPECT_EQ_DOUBLE(5.83172, eyePosition[2]);

        /* Same as above! */
        auto cameraLookAtValue = cameraObjectValue["LookAt Destination"];
        EXPECT_EQ_INT(leptjson::ELeptType::LEPT_ARRAY, cameraLookAtValue.LeptValueType());
        std::array<double, 3> cameraLookAtPosition
        { cameraLookAtValue[0].getNumber(),cameraLookAtValue[1].getNumber(),cameraLookAtValue[2].getNumber() };

        EXPECT_EQ_DOUBLE(0.10083, cameraLookAtPosition[0]);
        EXPECT_EQ_DOUBLE(1.514, cameraLookAtPosition[1]);
        EXPECT_EQ_DOUBLE(4.82932, cameraLookAtPosition[2]);

        /* Bonus: No need to deallocate resources due to the usage of RAII! */
    }
}

int main() {
#ifdef _WINDOWS
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    test_parse();
    testjsonScene();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);

    _CrtDumpMemoryLeaks();


    return main_ret;
}
