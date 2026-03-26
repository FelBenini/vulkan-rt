#include "platform/Window.hpp"
#include "core/VulkanContext.hpp"
#include "core/Renderer.hpp"
#include "camera/Camera.hpp"
#include <glm/glm.hpp>
#include <iostream>

int main() {
    try {
        Window window(1280, 720, "Vulkan CUBE");

        VulkanContext vkContext(window.getRequiredExtensions(), true);

        Renderer renderer(window,
                          vkContext.getInstance(),
                          window.createSurface(vkContext.getInstance()));

        auto [width, height] = window.getDrawableSize();
        Camera camera(60.0f, static_cast<float>(width) / static_cast<float>(height));

        std::cout << "Vulkan Renderer Initialized Successfully!\n";
        std::cout << "Controls: Click=LMB grab mouse, WASD=Move, Q/E=Up/Down\n";
        std::cout << (renderer.getDevice().hasRayTracingSupport()
                      ? "Ray Tracing: ENABLED\n"
                      : "Ray Tracing: NOT available — rasterization fallback.\n");

        while (!window.shouldClose()) {
            window.pollEvents();
            
            // Update camera aspect ratio on resize
            if (window.wasResized()) {
                auto [width, height] = window.getDrawableSize();
                camera.setAspectRatio(static_cast<float>(width) / static_cast<float>(height));
                window.clearResizeFlag();
            }
            
            // Camera controls - look when mouse grabbed, always move with keyboard
            if (window.isMouseGrabbed()) {
                int dx, dy;
                window.getMouseDelta(dx, dy);
                camera.rotate(static_cast<float>(-dx) * 0.002f, static_cast<float>(-dy) * 0.002f);
            }
            float moveSpeed = 0.005f;
            if (window.isKeyPressed(SDLK_w)) camera.moveForward(moveSpeed);
            if (window.isKeyPressed(SDLK_s)) camera.moveForward(-moveSpeed);
            if (window.isKeyPressed(SDLK_d)) camera.moveRight(moveSpeed);
            if (window.isKeyPressed(SDLK_a)) camera.moveRight(-moveSpeed);
            if (window.isKeyPressed(SDLK_SPACE)) camera.moveUp(moveSpeed);
            if (window.isKeyPressed(SDLK_LSHIFT)) camera.moveUp(-moveSpeed);

            renderer.drawFrame(camera);
        }

    } catch (const std::exception& e) {
        std::cerr << "Application Error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}