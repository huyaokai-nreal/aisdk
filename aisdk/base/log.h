#pragma  once
#include <framework/util/android_globals.h>
#include <framework/util/log_config.h>
#include <framework/util/logger.h>
#include <framework/util/singleton.h>
#include <string_view>

constexpr std::string_view SECTION_NAME =  "AISDK";

namespace aisdk::base {

class Logger : public framework::util::Singleton<Logger> {
   public:
    bool IsLogAllLevel() const { return log_all_level_; }
    void SetLogAllLevel(bool value) {
        log_all_level_ = value;
        if (value) {
            GetLogger()->set_level(framework::util::log::LogLevel::trace);
        }
    }
    framework::util::log::LoggerPtr GetLogger() { return framework::util::log::Logger::defaultLogger(section_); }
    void SetSection(const std::string &section) { section_ = section; }

   private:
    bool log_all_level_{false};
    std::string section_{SECTION_NAME};
};

}  // namespace aisdk::base
#define AISDK_LOG_TRACE(format, ...)                        \
    if (aisdk::base::Logger::GetInstance()->IsLogAllLevel()) \
        aisdk::base::Logger::GetInstance()->GetLogger()->trace(format, ##__VA_ARGS__);
#define AISDK_LOG_DEBUG(format, ...)                        \
    if (aisdk::base::Logger::GetInstance()->IsLogAllLevel()) \
        aisdk::base::Logger::GetInstance()->GetLogger()->debug(format, ##__VA_ARGS__);
#define AISDK_LOG_TRACE2(format1, format2, ...)                                               \
    if (aisdk::base::Logger::GetInstance()->IsLogAllLevel()) {                                 \
        std::string format = format1;                                                             \
        format += format2;                                                                        \
        AISDK_LOGs::Logger::GetInstance()->GetLogger()->trace(format.c_str(), ##__VA_ARGS__); \
    }
#define AISDK_LOG_INFO(format, ...) aisdk::base::Logger::GetInstance()->GetLogger()->info(format, ##__VA_ARGS__);
#define AISDK_LOG_WARN(format, ...) aisdk::base::Logger::GetInstance()->GetLogger()->warn(format, ##__VA_ARGS__);
#define AISDK_LOG_ERROR(format, ...) \
    aisdk::base::Logger::GetInstance()->GetLogger()->error(format, ##__VA_ARGS__);
#define AISDK_LOG_FATAL(format, ...) \
    aisdk::base::Logger::GetInstance()->GetLogger()->fatal(format, ##__VA_ARGS__);
