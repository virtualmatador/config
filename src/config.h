#ifndef CONFIG_H
#define CONFIG_H

#include <atomic>
#include <chrono>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <vector>

#include <json.h>

class config : public jsonio::json
{
public:
    config(const std::filesystem::path& config_dir, const std::string&
        pipe_name, const std::chrono::milliseconds& nap_time);
    ~config();
    const std::filesystem::path& get_dir() const;
    void run();
    void add_reloader(const std::shared_ptr<std::function<void()>>& reloader);
    void add_ticker(const std::shared_ptr<std::function<void()>>& ticker,
        const std::chrono::milliseconds& interval);
    void add_handler(const std::string& command, const std::shared_ptr<
        std::function<void(const jsonio::json&)>>& handler);
    bool need_reload() const;
    bool need_stop() const;
    void request_reload();
    void request_stop();

private:
    void read();

private:
    std::atomic<bool> reload_;
    std::atomic<bool> stop_;
    const std::filesystem::path config_dir_;
    const std::chrono::milliseconds nap_time_;
    std::string pipe_name_;
    std::vector<std::weak_ptr<std::function<void()>>> reloaders_;
    std::vector<std::pair<std::weak_ptr<std::function<void()>>,
        std::array<std::chrono::milliseconds, 2>>> tickers_;
    std::map<std::string, std::vector<std::weak_ptr<
        std::function<void(const jsonio::json&)>>>> handlers_;

private:
    static void handle_signal(int signal);
};

extern config* the_config_;

#endif//CONFIG_H
