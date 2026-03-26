#pragma once
#define SDL_MAIN_HANDLED
#include <vulkan/vulkan_raii.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <string>
#include <vector>
#include <utility> // std::pair

class Window {
public:
    Window(int width, int height, const std::string& title);
    ~Window();
    Window(const Window&)            = delete;
    Window& operator=(const Window&) = delete;

    bool shouldClose() const;
    void pollEvents();

    // Returns the actual drawable pixel size — handles HiDPI / Retina correctly
    std::pair<int, int> getDrawableSize() const;

    std::vector<const char*> getRequiredExtensions() const;
    vk::raii::SurfaceKHR     createSurface(const vk::raii::Instance& instance) const;

    SDL_Window* getHandle() const;

    // Set by Renderer when a resize event is detected mid-frame
    bool wasResized() const    { return m_framebufferResized; }
    void clearResizeFlag()     { m_framebufferResized = false; }

private:
    SDL_Window* m_window              = nullptr;
    bool        m_shouldClose         = false;
    bool        m_framebufferResized  = false;
};