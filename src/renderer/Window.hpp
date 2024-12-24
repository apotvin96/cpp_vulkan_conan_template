#pragma once

#include "../pch.hpp"

class Window {
public:
    Window(GLFWwindow* window);

    ~Window();

    void destroy();

    bool shouldClose();

    bool keyDown(int key);

    GLFWwindow* get();

    int getWidth();

    int getHeight();

    static void poll();

    static std::shared_ptr<Window> create(const char* title, int width, int height);

    static void windowResizeCallback(GLFWwindow* window, int width, int height);

protected:
private:
    GLFWwindow* window;
};