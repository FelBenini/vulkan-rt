#include "platform/Window.hpp"
#include <stdexcept>
#include <algorithm>

Window::Window(int width, int height, const std::string& title)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        throw std::runtime_error("SDL_Init Failed: " + std::string(SDL_GetError()));
    }

    m_window = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height,
        SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN
    );

    if (!m_window){
        throw std::runtime_error("SDL_CreateWindow Failed: " + std::string(SDL_GetError()));
    }
}

Window::~Window()
{
    if (m_window) SDL_DestroyWindow(m_window);
    SDL_Quit();
}

bool Window::shouldClose() const
{
    return m_shouldClose;
}

void Window::pollEvents()
{
    m_mouseDx = 0;
    m_mouseDy = 0;
    
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                m_shouldClose = true;
                break;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_CLOSE)
                    m_shouldClose = true;
                if (event.window.event == SDL_WINDOWEVENT_RESIZED ||
                    event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                    m_framebufferResized = true;
                break;
            case SDL_KEYDOWN:
                if (std::find(m_pressedKeys.begin(), m_pressedKeys.end(), event.key.keysym.sym) == m_pressedKeys.end()) {
                    m_pressedKeys.push_back(event.key.keysym.sym);
                }
                if (event.key.keysym.sym == SDLK_ESCAPE && m_mouseGrabbed) {
                    ungrabMouse();
                }
                break;
            case SDL_KEYUP:
                m_pressedKeys.erase(std::remove(m_pressedKeys.begin(), m_pressedKeys.end(), event.key.keysym.sym), m_pressedKeys.end());
                break;
            case SDL_MOUSEMOTION:
                if (m_mouseGrabbed) {
                    m_mouseDx += event.motion.xrel;
                    m_mouseDy += event.motion.yrel;
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_MIDDLE) {
                    if (!m_mouseGrabbed) {
                        grabMouse();
                    }
                }
                break;
            case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_MIDDLE) {
                    if (m_mouseGrabbed) {
                        ungrabMouse();
                    }
                }
                break;
            default:
                break;
        }
    }
}

void Window::grabMouse()
{
    SDL_SetRelativeMouseMode(SDL_TRUE);
    SDL_SetWindowGrab(m_window, SDL_TRUE);
    m_mouseGrabbed = true;
}

void Window::ungrabMouse()
{
    SDL_SetRelativeMouseMode(SDL_FALSE);
    SDL_SetWindowGrab(m_window, SDL_FALSE);
    m_mouseGrabbed = false;
    m_mouseDx = 0;
    m_mouseDy = 0;
}

void Window::getMouseDelta(int& dx, int& dy)
{
    dx = m_mouseDx;
    dy = m_mouseDy;
    m_mouseDx = 0;
    m_mouseDy = 0;
}

int Window::getScrollDelta()
{
    return m_scrollDelta;
}

void Window::clearScrollDelta()
{
    m_scrollDelta = 0;
}

bool Window::isKeyPressed(int keycode) const
{
    return std::find(m_pressedKeys.begin(), m_pressedKeys.end(), keycode) != m_pressedKeys.end();
}

std::pair<int, int> Window::getDrawableSize() const
{
    int w, h;
    SDL_Vulkan_GetDrawableSize(m_window, &w, &h);
    return {w, h};
}

std::vector<const char*> Window::getRequiredExtensions() const
{
    uint32_t count = 0;
    if (!SDL_Vulkan_GetInstanceExtensions(m_window, &count, nullptr))
        throw std::runtime_error("Failed to get SDL Vulkan extension count");

    std::vector<const char*> extensions(count);
    if (!SDL_Vulkan_GetInstanceExtensions(m_window, &count, extensions.data()))
        throw std::runtime_error("Failed to get SDL Vulkan extensions");

    return extensions;
}

vk::raii::SurfaceKHR Window::createSurface(const vk::raii::Instance& instance) const
{
    VkSurfaceKHR rawSurface;
    if (!SDL_Vulkan_CreateSurface(m_window, static_cast<VkInstance>(*instance), &rawSurface))
        throw std::runtime_error("Failed to create Window Surface: " + std::string(SDL_GetError()));

    return vk::raii::SurfaceKHR(instance, rawSurface);
}

SDL_Window* Window::getHandle() const
{
    return m_window;
}