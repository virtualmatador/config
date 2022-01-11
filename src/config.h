#ifndef CONFIG_H
#define CONFIG_H

#include <functional>
#include <filesystem>
#include <memory>

#include <compose.h>
#include <json.h>

class config : public jsonio::json
{
public:
    config(const std::filesystem::path& config_path, compose& the_compose);
    ~config();

private:
    void read();

private:
    const std::filesystem::path config_path_;
    std::shared_ptr<std::function<void()>> reloader_;
};

#endif //CONFIG_H
