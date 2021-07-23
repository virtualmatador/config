#include <csignal>
#include <fstream>
#include <thread>

#include "config.h"

config* the_config_{ nullptr };

void config::handle_signal(int signal)
{
    switch (signal)
    {
    case SIGHUP:
        the_config_->reload_ = true;
        break;
    case SIGTERM:
        the_config_->stop_ = true;
        break;
    }
}

config::config(const std::filesystem::path& config_dir,
    const std::chrono::milliseconds& nap_time)
    : config_dir_{ config_dir }
    , reload_{ false }
    , stop_{ false }
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
}

const std::filesystem::path& config::get_dir()
{
    return config_dir_;
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

bool config::need_reload() const
{
    return reload_;
}

bool config::need_stop() const
{
    return stop_;
}

void config::request_reload()
{
    reload_ = true;
}

void config::request_stop()
{
    stop_ = true;
}

void config::run()
{
    while (!stop_)
    {
        if (reload_)
        {
            reload_ = false;
            read();
            for (auto it = reloaders_.begin(); it != reloaders_.end();)
            {
                if (auto active_callback = it->lock())
                {
                    (*active_callback)();
                    ++it;
                }
                else
                {
                    it = reloaders_.erase(it);
                }
            }
        }
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
        std::this_thread::sleep_for(nap_time_);
    }
}
