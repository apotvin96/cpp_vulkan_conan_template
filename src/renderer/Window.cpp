#include "../pch.hpp"
#include "Window.hpp"

#include "GraphicsContext.hpp"
#include "../Logger.hpp"

Window::Window(GLFWwindow* window) : window(window) {
    glfwSetWindowSizeCallback(window, windowResizeCallback);
}

Window::~Window() { destroy(); }

void Window::destroy() {
    Logger::renderer_logger->info("Destroying Window");
    glfwDestroyWindow(window);
    glfwTerminate();
}

bool Window::shouldClose() { return glfwWindowShouldClose(window); }

bool Window::keyDown(int key) { return glfwGetKey(window, key); }

GLFWwindow* Window::get() { return window; }

int Window::getWidth() {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    return width;
}

int Window::getHeight() {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    return height;
}

void Window::poll() { glfwPollEvents(); }

std::shared_ptr<Window> Window::create(const char* title, int width, int height) {
    Logger::renderer_logger->info("Creating Window");

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);

    return std::make_shared<Window>(window);
}

void Window::windowResizeCallback(GLFWwindow* window, int width, int height) {
    if (width == 0 && height == 0) { // Don't allow a minimize
        glfwRestoreWindow(window);
    } else {
        void* userPointer = glfwGetWindowUserPointer(window);

        if (userPointer != nullptr) {
            // GraphicsContext* graphicsContext = static_cast<GraphicsContext*>(userPointer);

            // graphicsContext->windowResize(width, height);
        }
    }
}
