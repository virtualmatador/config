#ifndef TRADIN_CONFIG_H
#define TRADIN_CONFIG_H

#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <vector>

#include <json.h>

class config : public jsonio::json
{
public:
    config(const std::filesystem::path& config_dir,
        const std::chrono::duration<int64_t>& nap_time);
    ~config();
    void run();
    void add_reloader(const std::shared_ptr<std::function<void()>>& reloader);
    bool alive();

private:
    void read();

public:
    const std::filesystem::path config_dir_;
    const std::chrono::duration<int64_t> nap_time_;
    std::vector<std::weak_ptr<std::function<void()>>> reloaders_;
};

extern config* the_config_;

#endif//TRADIN_CONFIG_H
