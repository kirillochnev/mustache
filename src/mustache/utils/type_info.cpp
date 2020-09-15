#include "type_info.hpp"
using namespace mustache;

std::string detail::make_type_name_from_func_name(const char* func_name) noexcept {
#ifdef _MSC_BUILD
    const auto replace_all = [](std::string str, const std::string& from, const std::string& to) {
        size_t start_pos = 0;
        while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
        }
        return str;
    };
    static std::string magic_phrase = "mustache::type_name<";
    static std::string class_magic_phrase = "class ";
    static std::string struct_magic_phrase = "struct ";
    std::string result = func_name;
    result = result.substr(result.find(magic_phrase) + magic_phrase.size());
    auto end_pos = result.rfind(">(void) noexcept");
    result = result.substr(0, end_pos); // result: "struct\class Foo"
    result = replace_all(result, class_magic_phrase, "");
    result = replace_all(result, struct_magic_phrase, "");
    return  result;
#else
    static std::string magic_phrase = "T = ";
    std::string result = func_name;
    result = result.substr(result.find(magic_phrase) + magic_phrase.size());
    auto end_pos = result.find(";");
    if(end_pos == result.npos) {
        end_pos = result.find("]");
    }
    return  result.substr(0, end_pos);
#endif
}
