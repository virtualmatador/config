#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <regex>
#include <sstream>

#include <config.h>

bool t01()
{
    the_config_ = nullptr;
    std::size_t reloads = 0;
    config the_config(".", std::chrono::seconds(0));
    the_config.request_reload();
    auto reloader = std::make_shared<std::function<void()>>([&]()
    {
        reloads += 1;
        the_config.request_stop();
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
    the_config_ = nullptr;
    std::size_t reloads = 0;
    config the_config(".", std::chrono::seconds(0));
    auto ticker = std::make_shared<std::function<void()>>([&]()
    {
        reloads += 2;
        the_config.request_reload();
        if (reloads > 5)
        {
            the_config.request_stop();
        }
    });
    auto reloader = std::make_shared<std::function<void()>>([&]()
    {
        reloads += 1;
    });
    the_config_->add_reloader(reloader);
    the_config_->add_ticker(ticker, std::chrono::milliseconds(0));
    the_config.run();
    if (reloads != 8)
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
