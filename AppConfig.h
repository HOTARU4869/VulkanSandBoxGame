// AppConfig.h

#pragma once
#include <string>

struct AppConfig {
    uint32_t    windowWidth = 800;
    uint32_t    windowHeight = 600;
    bool        fullscreen = false;
    std::string appName = "Vulkan Sandbox";
};