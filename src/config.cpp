#include <atomic>
#include <csignal>
#include <fstream>
#include <thread>

#include "config.h"

config* the_config_{ nullptr };

std::atomic<bool> reload_{ true };
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
    const std::chrono::milliseconds& nap_time)
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
    clear();
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

void config::add_ticker(const std::shared_ptr<std::function<void()>>& ticker,
    const std::chrono::milliseconds& interval)
{
    tickers_.push_back({ ticker, { std::chrono::milliseconds(0), interval } });
}

bool config::alive()
{
    return !stop_;
}

void config::run()
{
    while (!stop_)
    {
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
            for (auto it = tickers_.begin(); it != tickers_.end();)
            {
                if (auto active_ticker = it->first.lock())
                {
                    it->second[0] += nap_time_;
                    if (it->second[0] >= it->second[1])
                    {
                        it->second[0] = std::chrono::milliseconds(0);
                        (*active_ticker)();
                    }
                    ++it;
                }
                else
                {
                    it = tickers_.erase(it);
                }
            }
        }
        if (!stop_)
        {
            read();
            reload_ = false;
        }
    }
}
