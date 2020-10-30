#include "logger.hpp"
#include <cstdarg>

using namespace mustache;


namespace {
    std::shared_ptr<LogWriter> active_writer = std::make_shared<LogWriter>();
}


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

LogWriter& LogWriter::active() noexcept {
    return *active_writer;
}

void LogWriter::setActive(const std::shared_ptr<LogWriter>& writer) noexcept {
    if (writer) {
        active_writer = writer;
    }
}
