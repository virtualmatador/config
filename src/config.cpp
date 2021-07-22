#include <atomic>
#include <csignal>
#include <fstream>
#include <thread>

#include "config.h"

config* the_config_{ nullptr };

std::atomic<bool> reload_{ false };
std::atomic<bool> stop_{ false };

void handle_signal(int signal)
{
    switch (signal)
    {
    case SIGHUP:
        reload_ = true;
        break;
    case SIGTERM:
        stop_ = true;
        break;
    default:
        break;
    }
}

config::config(const std::filesystem::path& config_dir,
    const std::chrono::duration<int64_t>& nap_time)
    : config_dir_{ config_dir }
    , nap_time_{ nap_time }
{
    if (the_config_)
    {
        throw std::runtime_error("multiple config");
    }
    the_config_ = this;
    signal(SIGHUP, handle_signal);
    signal(SIGTERM, handle_signal);
    read();
}

config::~config()
{
    the_config_ = nullptr;
}

void config::read()
{
    std::ifstream is{ config_dir_ / "config.json" };
    if (!is)
    {
        throw std::runtime_error("no config file");
    }
    is >> *this;
    if (!completed())
    {
        throw std::runtime_error("no config json");
    }
}

void config::add_reloader(
    const std::shared_ptr<std::function<void()>>& reloader)
{
    reloaders_.emplace_back(reloader);
}

bool config::alive()
{
    return !stop_;
}

void config::run()
{
    while (!stop_)
    {
        reload_ = false;
        read();
        for (auto it = reloaders_.begin(); it != reloaders_.end();)
        {
            if (auto active_reloader = it->lock())
            {
                (*active_reloader)();
                ++it;
            }
            else
            {
                it = reloaders_.erase(it);
            }
        }
        while (!stop_ && !reload_)
        {
            std::this_thread::sleep_for(nap_time_);
        }
    }
}
