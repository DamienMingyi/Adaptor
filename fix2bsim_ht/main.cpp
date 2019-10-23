#include<iostream>
#include "hfs_fix2bsim.hpp"
using namespace std;
std::shared_ptr<spdlog::logger> A2FLog::logger;
int main()
{
    //std::cout << "main()" << endl;
    string configFile = "config.xml";
    a2ftool::XmlNode cfg(configFile);
    A2FLog::logger = spdlog::daily_logger_mt("file_logger", "gateway", 8, 0);
    spdlog::set_async_mode(4096, 0);
    std::unordered_map<std::string, spdlog::level::level_enum> log_levels = {{"warn", spdlog::level::warn}, {"info", spdlog::level::info}, {"debug", spdlog::level::debug}};
    A2FLog::logger->set_level(log_levels[cfg.getAttrDefault("log_level", "debug")]);
    A2FLog::logger->flush_on(spdlog::level::info);
    spdlog::set_pattern("[%H:%M:%S.%f] [%l]%v");
    
    hfs_fix2bsim* mgr  = new hfs_fix2bsim(cfg);
    mgr->start();
    mgr->run();

    return 0;
}
