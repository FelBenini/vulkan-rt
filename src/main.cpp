#include "platform/Window.hpp"
#include "core/VulkanContext.hpp"
#include "core/Renderer.hpp"
#include <iostream>

int main() {
    try {
        Window window(1280, 720, "Vulkan RAII PBR Ray Tracer");

        VulkanContext vkContext(window.getRequiredExtensions(), true);

        Renderer renderer(window,
                          vkContext.getInstance(),
                          window.createSurface(vkContext.getInstance()));

        std::cout << "Vulkan Renderer Initialized Successfully!\n";
        std::cout << (renderer.getDevice().hasRayTracingSupport()
                      ? "Ray Tracing: ENABLED\n"
                      : "Ray Tracing: NOT available — rasterization fallback.\n");

        while (!window.shouldClose()) {
            window.pollEvents();
            renderer.drawFrame();
        }

    } catch (const std::exception& e) {
        std::cerr << "Application Error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}