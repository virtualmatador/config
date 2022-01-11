#include <fstream>

#include "config.h"

config::config(const std::filesystem::path& config_path, compose& the_compose)
    : config_path_{ config_path }
    , reloader_{ std::make_shared<std::function<void()>>(
        std::bind(&config::read, this)) }
{
    the_compose.add_reloader(reloader_);
    read();
}

config::~config()
{
}

void config::read()
{
    std::ifstream is { config_path_ };
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
