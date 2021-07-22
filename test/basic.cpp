#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <regex>
#include <sstream>

#include <config.h>

extern std::atomic<bool> reload_;
extern std::atomic<bool> stop_;

bool t01()
{
    reload_ = false;
    stop_ = false;
    std::size_t reloads = 0;
    config the_config(".", std::chrono::seconds(0));
    auto reloader = std::make_shared<std::function<void()>>([&]()
    {
        reloads += 1;
        stop_ = true;
    });
    the_config_->add_reloader(reloader);
    the_config.run();
    if (reloads != 1)
    {
        return false;
    }
    return true;
}

bool t02()
{
    reload_ = false;
    stop_ = false;
    std::size_t reloads = 0;
    config the_config(".", std::chrono::seconds(0));
    auto reloader = std::make_shared<std::function<void()>>([&]()
    {
        reloads += 1;
        reload_ = true;
        if (reloads == 2)
        {
            stop_ = true;
        }
    });
    the_config_->add_reloader(reloader);
    the_config.run();
    if (reloads != 2)
    {
        return false;
    }
    return true;
}

int main()
{
    if (
        t01() &&
        t02() &&
        true)
    {
        return 0;
    }
    return -1;
}
