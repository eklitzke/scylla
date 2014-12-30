/*
 * Copyright (C) 2014 Cloudius Systems, Ltd.
 */

#ifndef LOG_HH_
#define LOG_HH_

#include "core/sstring.hh"
#include <unordered_map>

namespace logging {

enum class log_level {
    error,
    warn,
    info,
    debug,
    trace,
};

class logger;
class registry;

class logger {
    sstring _name;
    log_level _level = log_level::warn;
private:
    struct stringer {
        // no need for virtual dtor, since not dynamically destroyed
        virtual void append(std::ostream& os) = 0;
    };
    template <typename Arg>
    struct stringer_for final : stringer {
        explicit stringer_for(const Arg& arg) : arg(arg) {}
        const Arg& arg;
        virtual void append(std::ostream& os) override {
            os << arg;
        }
    };
    template <typename... Args>
    void do_log(log_level level, const char* fmt, Args&&... args);
    template <typename Arg, typename... Args>
    void do_log_step(log_level level, const char* fmt, stringer** s, size_t n, size_t idx, Arg&& arg, Args&&... args);
    void do_log_step(log_level level, const char* fmt, stringer** s, size_t n, size_t idx);
    void really_do_log(log_level level, const char* fmt, stringer** stringers, size_t n);
public:
    explicit logger(sstring name);
    logger(logger&& x);
    ~logger();
    template <typename... Args>
    void log(log_level level, const char* fmt, Args&&... args) {
        if (level <= _level) {
            do_log(level, fmt, std::forward<Args>(args)...);
        }
    }
    template <typename... Args>
    void error(const char* fmt, Args&&... args) {
        log(log_level::error, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void warn(const char* fmt, Args&&... args) {
        log(log_level::warn, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void info(const char* fmt, Args&&... args) {
        log(log_level::info, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void debug(const char* fmt, Args&&... args) {
        log(log_level::debug, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    void trace(const char* fmt, Args&&... args) {
        log(log_level::trace, fmt, std::forward<Args>(args)...);
    }
    const sstring& name() const {
        return _name;
    }
};

class registry {
    std::unordered_map<sstring, logger*> _loggers;
public:
    void register_logger(logger* l);
    void unregister_logger(logger* l);
    void moved(logger* from, logger* to);
};

sstring pretty_type_name(const std::type_info&);

extern thread_local registry g_registry;

template <typename T>
class logger_for : public logger {
public:
    logger_for() : logger(pretty_type_name(typeid(T))) {}
};

inline
void
logger::do_log_step(log_level level, const char* fmt, stringer** s, size_t n, size_t idx) {
    really_do_log(level, fmt, s, n);
}

template <typename Arg, typename... Args>
inline
void
logger::do_log_step(log_level level, const char* fmt, stringer** s, size_t n, size_t idx, Arg&& arg, Args&&... args) {
    stringer_for<Arg> sarg{arg};
    s[idx] = &sarg;
    do_log_step(level, fmt, s, n, idx + 1, std::forward<Args>(args)...);
}


template <typename... Args>
void
logger::do_log(log_level level, const char* fmt, Args&&... args) {
    stringer* s[sizeof...(Args)];
    do_log_step(level, fmt, s, sizeof...(Args), 0, std::forward<Args>(args)...);
}

}

#endif /* LOG_HH_ */