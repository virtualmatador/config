#include <atomic>
#include <chrono>
#include <csignal>
#include <fstream>
#include <iostream>
#include <memory>
#include <regex>
#include <sstream>

#include <config.h>

bool t01()
{
    the_config_ = nullptr;
    std::size_t reloads = 0;
    config the_config(".", "", std::chrono::seconds(0));
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
    config the_config(".", "", std::chrono::seconds(0));
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

bool t03()
{
    the_config_ = nullptr;
    std::size_t handles = 0;
    std::size_t ticks = 0;
    config the_config(".", "test.pipe", std::chrono::seconds(0));
    auto ticker = std::make_shared<std::function<void()>>([&]()
    {
        if (ticks++ == 0)
        {
            std::ofstream pipe{"./test.pipe"};
            pipe << "{\"command\": \"test\", \"payload\": "
                "{\"data\": \"some data\"}}" << std::endl;
        }
    });
    auto handler = std::make_shared<std::function<void(const jsonio::json&)>>(
        [&](auto payload)
    {
        if (payload["data"].get_string() == "some data")
        {
            handles += 1;
        }
        the_config.request_stop();
    });
    the_config_->add_handler("test", handler);
    the_config_->add_ticker(ticker, std::chrono::milliseconds(0));
    the_config.run();
    if (handles != 1)
    {
        return false;
    }
    return true;
}

bool t04()
{
    the_config_ = nullptr;
    std::size_t handles = 0;
    std::size_t ticks = 0;
    config the_config(".", "test.pipe", std::chrono::milliseconds(1));
    auto ticker = std::make_shared<std::function<void()>>([&]()
    {
        if (ticks++ == 0)
        {
            the_config.request_reload();
        }
    });
    auto reloader = std::make_shared<std::function<void()>>([&]()
    {
        std::ofstream pipe{"./test.pipe"};
        if (pipe.is_open())
        {
            pipe << "{\"command\": \"test\", \"payload\": "
                "{\"data\": \"reload\"}}" << std::endl;
        }
    });
    auto handler = std::make_shared<std::function<void(const jsonio::json&)>>(
        [&](auto payload)
    {
        if (payload["data"].get_string() == "reload")
        {
            handles += 1;
            the_config.request_stop();
        }
    });
    the_config_->add_handler("test", handler);
    the_config_->add_ticker(ticker, std::chrono::milliseconds(10));
    the_config_->add_reloader(reloader);
    the_config.run();
    if (handles != 1)
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
        t03() &&
        t04() &&
        true)
    {
        return 0;
    }
    return -1;
}
