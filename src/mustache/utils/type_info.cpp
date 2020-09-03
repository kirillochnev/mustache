#include "type_info.hpp"
using namespace mustache;

std::string detail::make_type_name_from_func_name(const char* func_name) noexcept {
    static std::string magic_phrase = "T = ";
    std::string result = func_name;
    result = result.substr(result.find(magic_phrase) + magic_phrase.size());
    auto end_pos = result.find(";");
    if(end_pos == result.npos) {
        end_pos = result.find("]");
    }
    return  result.substr(0, end_pos);
}
