#pragma once

#include <string>
#include <mustache/utils/default_settings.hpp>
#include <memory>

namespace mustache {
    enum class LogLevel : uint32_t {
        kError = 0,
        kWarn,
        kInfo,
        kDebug
    };

    class LogWriter {
    public:
        static constexpr const char* toStr(LogLevel lvl) noexcept {
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

        static void setActive(const std::shared_ptr<LogWriter>& writer) noexcept;
        static LogWriter& active() noexcept;

        LogWriter() = default;
        virtual ~LogWriter() = default;

        struct Context {
            const char* file;
            const char* function;
            uint32_t line;
            bool show_context_{true};
        };

        virtual void onMessage(const Context& ctx, LogLevel, std::string, ...);

    };

    class Logger {
    public:
        Logger(const char* file = MUSTACHE_FILE, const char* function = MUSTACHE_FUNCTION, uint32_t line = MUSTACHE_LINE):
                context_{file, function, line} {

        }
        bool isDebugEnabled() const noexcept {
            return true;
        }
        template <typename... _ARGS>
        void info(_ARGS&&... args) const {
            onMsg(LogLevel::kInfo, std::forward<_ARGS>(args)...);
        }
        template <typename... _ARGS>
        void warn(_ARGS&&... args) const {
            onMsg(LogLevel::kWarn, std::forward<_ARGS>(args)...);
        }

        template <typename... _ARGS>
        void debug(_ARGS&&... args) const {
            if(!isDebugEnabled()) {
                return;
            }
            onMsg(LogLevel::kDebug, std::forward<_ARGS>(args)...);
        }

        template <typename... _ARGS>
        void error(_ARGS&&... args) const {
            onMsg(LogLevel::kError, std::forward<_ARGS>(args)...);
        }
        Logger& hideContext() noexcept {
            context_.show_context_ = false;
            return *this;
        }
    private:
        template <typename _Msg, typename... _ARGS>
        void onMsg(LogLevel lvl, _Msg&& msg, _ARGS&&... args) const {
            // TODO: add compile time arg check if possible
            LogWriter::active().onMessage(context_,
                    lvl, toMsgFormat(msg), forwardArg(args)...);
        }

        static std::string toMsgFormat(const std::string& str) noexcept {
            return str;
        }
        static std::string toMsgFormat(std::string&& str) noexcept {
            return std::move(str);
        }
        static std::string toMsgFormat(const char*&& str) noexcept {
            return str;
        }
        static const char* forwardArg(const std::string& arg) noexcept {
            return arg.c_str();
        }
        template <typename T>
        static T forwardArg(T arg) noexcept {
            return arg;
        }
        LogWriter::Context context_;
    };

}
