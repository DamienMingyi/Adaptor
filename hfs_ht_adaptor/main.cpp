#include "hfs_ht.hpp"
#include "hfs_log.hpp"
std::shared_ptr<spdlog::logger> A2FLog::logger;
int main()
{
    string configFile = "config.xml";
    a2ftool::XmlNode cfg(configFile);
    A2FLog::logger = spdlog::daily_logger_mt("file_logger", "bsimlog", 8, 0);
    // A2FLog::logger = spdlog::stdout_color_mt("console");
    spdlog::set_async_mode(4096, 0);
    std::unordered_map<std::string, spdlog::level::level_enum> log_levels = {{"warn", spdlog::level::warn}, {"info", spdlog::level::info}, {"debug", spdlog::level::debug}};
    A2FLog::logger->set_level(log_levels["debug"]);
    A2FLog::logger->flush_on(spdlog::level::info);
    spdlog::set_pattern("[%H:%M:%S.%f] %v");

    hfs_ht *trader = new hfs_ht(cfg,"101");
    trader->Init();
    // for (int i=0;i<3200;++i)
        //cout << trader->outside_order[i] << endl;
    cout << trader->insert_limit_order("000001.SZ", "SZ", 10, 100, HFS_ORDER_DIRECTION_LONG, HFS_ORDER_FLAG_OPEN) << endl;
    int id = trader->insert_limit_order("000001.SZ", "SZ", 9, 100, HFS_ORDER_DIRECTION_LONG, HFS_ORDER_FLAG_OPEN);
    sleep(3);
    trader->cancel_order(id);
    while(true) {
        // cout << "insert order " << endl;
        // cout << trader->insert_limit_order("000001.SZ", "SZ", 12, 100, HFS_ORDER_DIRECTION_LONG, HFS_ORDER_FLAG_OPEN) << endl;
        sleep(3);
    }
}
