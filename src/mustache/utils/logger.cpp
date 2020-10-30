#include "logger.hpp"
#include <cstdarg>

#ifdef ANDROID
#include <android/log.h>
#endif

using namespace mustache;

namespace {
    std::shared_ptr<LogWriter> active_writer = std::make_shared<LogWriter>();
}


#ifdef ANDROID
void LogWriter::onMessage(const Context& ctx, LogLevel lvl, std::string str, ...) {

    const auto get_lvl = [lvl] {
        switch (lvl) {
            case LogLevel::kError:
                return ANDROID_LOG_ERROR;
            case LogLevel::kWarn:
                return ANDROID_LOG_WARN;
            case LogLevel::kInfo:
                return ANDROID_LOG_INFO;
            case LogLevel::kDebug:
                return ANDROID_LOG_DEBUG;
        }
        return ANDROID_LOG_ERROR;
    };

    std::string source_location = "";
    if (ctx.show_context_) {
        source_location = " | at: " + std::string (ctx.file) + ":" + std::to_string(ctx.line);
    }
    va_list args;
    va_start (args, str);

#pragma GCC diagnostic ignored "-Wformat-nonliteral"
    const auto buffer_size = vsnprintf(nullptr, 0, str.c_str(), args);
    static std::string buffer;
    buffer.resize(buffer_size);
    va_end (args);

    va_start (args, str);
    vsnprintf(&buffer[0], buffer_size, str.c_str(), args);
    va_end (args);

#pragma GCC diagnostic warning "-Wformat-nonliteral"

    __android_log_print(get_lvl(), "Mustache", "%s\n", (source_location + buffer).c_str());
}

#else
void LogWriter::onMessage(const Context& ctx, LogLevel lvl, std::string str, ...) {
    if(ctx.show_context_) {
        printf("%s: ", toStr(lvl));
    }
    va_list args;
    va_start (args, str);

#pragma GCC diagnostic ignored "-Wformat-nonliteral"
    vprintf(str.c_str(), args);
#pragma GCC diagnostic warning "-Wformat-nonliteral"
    va_end (args);
    if(ctx.show_context_) {
        printf(" | at: %s:%d\n", ctx.file, ctx.line);
    } else {
        printf("\n");
    }
    fflush(stdout);
}
#endif

LogWriter& LogWriter::active() noexcept {
    return *active_writer;
}

void LogWriter::setActive(const std::shared_ptr<LogWriter>& writer) noexcept {
    if (writer) {
        active_writer = writer;
    }
}
