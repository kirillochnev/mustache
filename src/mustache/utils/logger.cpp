#include "logger.hpp"
#include <cstdarg>

using namespace mustache;


namespace {
    constexpr const char* toStr(LogLevel lvl) noexcept {
        switch (lvl) {
            case LogLevel::kError:
                return "Error";
            case LogLevel::kWarn:
                return "Warning";
            case LogLevel::kInfo:
                return "Info";
            case LogLevel::kDebug:
                return "Debug";
        }
        return nullptr;
    }
}


void LogWriter::onMessage(const Context& ctx, LogLevel lvl, std::string str, ...) {
    if(ctx.show_context_) {
        printf("%s: ", toStr(lvl));
    }
    va_list args;
    va_start (args, str);
    vprintf(str.c_str(), args);
    va_end (args);
    if(ctx.show_context_) {
        printf(" | at: %s:%d\n", ctx.file, ctx.line);
    } else {
        printf("\n");
    }
    fflush(stdout);
}

LogWriter& LogWriter::active() noexcept {
    static LogWriter instance;
    return instance;
}
