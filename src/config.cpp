#include <csignal>
#include <filesystem>
#include <fstream>
#include <memory>
#include <thread>

#include <ext/stdio_filebuf.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

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
    const std::string& pipe_name, const std::chrono::milliseconds& nap_time)
    : reload_{ false }
    , stop_{ false }
    , config_dir_{ config_dir }
    , nap_time_{ nap_time }
    , pipe_name_{ pipe_name }
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

const std::filesystem::path& config::get_dir() const
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

void config::add_handler(const std::string& command,
    const std::shared_ptr<std::function<void(const jsonio::json&)>>& handler)
{
    handlers_[command].emplace_back(handler);
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
    jsonio::json pipe_json;
    std::unique_ptr<int, void(*)(int*)> descriptor{ nullptr, [](int* fd)
    {
        if (fd)
        {
            close(*fd);
            delete fd;
        }
    } };
    std::unique_ptr<__gnu_cxx::stdio_filebuf<char>> file_buf;
    std::unique_ptr<std::istream> pipe;
    std::filesystem::path pipe_path;
    if (!pipe_name_.empty())
    {
        pipe_path = the_config_->get_dir() / pipe_name_;
        std::filesystem::remove(pipe_path);
        if (mkfifo(pipe_path.c_str(), S_IROTH | S_IWOTH | S_IRUSR | S_IWUSR) != 0)
        {
            throw std::runtime_error("create pipe");
        }
        descriptor.reset(
            new int{ open(pipe_path.c_str(), O_RDONLY | O_NONBLOCK) });
        if (*descriptor == -1)
        {
            throw std::runtime_error("open pipe");
        }
        file_buf = std::make_unique<decltype(file_buf)::element_type>(
            *descriptor, std::ios_base::in);
        pipe = std::make_unique<decltype(pipe)::element_type>(file_buf.get());
    }
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
        if (pipe.get())
        {
            pipe->clear();
            (*pipe) >> pipe_json;
            if (pipe_json.completed())
            {
                if (pipe_json.get_type() == jsonio::JsonType::J_OBJECT)
                {
                    auto command = pipe_json.get_value("command");
                    auto payload = pipe_json.get_value("payload");
                    if (command &&
                        command->get_type() == jsonio::JsonType::J_STRING &&
                        payload &&
                        payload->get_type() == jsonio::JsonType::J_OBJECT)
                    {
                        auto callbacks = handlers_[command->get_string()];
                        for (auto it = callbacks.begin(); it != callbacks.end();)
                        {
                            if (auto active_callback = it->lock())
                            {
                                (*active_callback)(*payload);
                                ++it;
                            }
                            else
                            {
                                it = callbacks.erase(it);
                            }
                        }
                    }
                }
                pipe_json.clear();
            }
        }
        std::this_thread::sleep_for(nap_time_);
    }
    if (!pipe_name_.empty())
    {
        std::filesystem::remove(pipe_path);
    }
}
