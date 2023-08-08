
#include "json/json.h"

// Payload for websockets

namespace wsPayload
{
    class JsonPayload
    {
    public:
        typedef Json::Value valueType;

    private:
        valueType base;
        JSONCPP_STRING err;
    public:
        JsonPayload(/*logger*/) = default;
        ~JsonPayload() = default;

         // copy constructor - transferring rights to the compiler to form this constructor
        JsonPayload(const JsonPayload&) = delete;
        // move constructor
        JsonPayload(JsonPayload&&) = delete;
        // copy assignment operator
        JsonPayload& operator=(const JsonPayload&) = delete;
        // move assignment operator
        JsonPayload& operator=(JsonPayload&&) = delete;

        bool decode(const std::string &data){
            Json::CharReaderBuilder builder;
            builder.settings_["indentation"] = "";
            const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
            return reader->parse( data.c_str(),  data.c_str() + data.length(), &base, &err);
        }

        valueType getValue(){
            return base;
        }

        // Convert data type to a string
        std::string serialize(valueType &data){
            Json::StreamWriterBuilder builder;
            builder.settings_["indentation"] = "";
            return Json::writeString(builder, data);
        }

        std::string encode(const std::string &data){
            Json::Value root(data);
            Json::StreamWriterBuilder builder;
            //root["Payload"] = data;
            const std::string encoded = Json::writeString(builder, root);
            std::cout << "Sending:" << encoded << std::endl;
            return encoded;
        }
    };
};

