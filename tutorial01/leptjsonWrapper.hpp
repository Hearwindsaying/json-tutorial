#pragma once
#ifndef LEPTJSONWRAPPER_H__
#define LEPTJSONWRAPPER_H__

#include "leptjson.hpp"
#include <string_view>
#include <memory>

namespace leptjson
{
    struct LeptjsonValue : public Lept_value
    {
    public:
        explicit LeptjsonValue(const Lept_value& v) : Lept_value(v)
        {

        }

        LeptjsonValue() = default;
        LeptjsonValue(const LeptjsonValue&) = delete;
        LeptjsonValue& operator=(const LeptjsonValue&) = delete;

        ELeptType LeptValueType() const
        {
            return this->type;
        }

        bool getBoolean() const
        {
            return lept_get_boolean(this);
        }

        double getNumber() const
        {
            return lept_get_number(this);
        }

        // Non-standard support for JSON string format using conversion from c_str to std::string.
        std::string getString() const
        {
            return std::string(lept_get_string(this));
        }

        /************************************************************************/
        /* Array                                                                */
        /************************************************************************/
        const LeptjsonValue getArrayElement(size_t index) const
        {
            return LeptjsonValue(*lept_get_array_element(this, index));
        }

        /*LeptjsonValue &getArrayElement(size_t index)
        {
            return LeptjsonValue(*lept_get_array_element(this, index));
        }*/

        size_t getArraySize(size_t arraySize) const
        {
            return lept_get_array_size(this);
        }

        /*LeptjsonValue &operator[](size_t index)
        {
            return this->getArrayElement(index);
        }*/

        const LeptjsonValue operator[](size_t index) const
        {
            return this->getArrayElement(index);
        }

        /************************************************************************/
        /* Object                                                               */
        /************************************************************************/
        size_t getObjectSize(size_t objectSize) const
        {
            return lept_get_object_size(this);
        }

        const LeptjsonValue getObjectValue(const std::string &key, size_t keyLen = DefaultKeyLen) const
        {
            if (keyLen == DefaultKeyLen)
                keyLen = key.size();
            return LeptjsonValue(*lept_find_object_value(this, key.c_str(), keyLen));
        }

        const LeptjsonValue operator[](const std::string &key) const
        {
            return this->getObjectValue(key);
        }

        static constexpr size_t DefaultKeyLen = static_cast<size_t>(-1);
    };

    class LeptjsonParser
    {
    public:
        static std::pair<ELEPT_PARSE_ECODE,std::unique_ptr<LeptjsonParser>> createLeptjsonParser(const std::string& jsonString)
        {
            std::unique_ptr<LeptjsonParser> parser = std::make_unique<LeptjsonParser>();
            return std::make_pair(parser->Parse(jsonString), std::move(parser));
        }

        explicit LeptjsonParser()
        {
            
        }

        LeptjsonParser(const LeptjsonParser&) = delete;
        LeptjsonParser& operator=(const LeptjsonParser&) = delete;

        ELEPT_PARSE_ECODE Parse(const std::string& jsonString)
        {
            return lept_parse(&this->m_value, jsonString.c_str());
        }

        ELeptType LeptValueType() const
        {
            return this->m_value.type;
        }

        const LeptjsonValue& LeptValue() const
        {
            return this->m_value;
        }

    public:
        ~LeptjsonParser()
        {
            lept_free(&this->m_value);
        }

    private:
        LeptjsonValue   m_value;
    };
}

#endif