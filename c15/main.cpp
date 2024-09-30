
#include <webgpu/webgpu.hpp>
#include <iostream>
#include <vector>
#include <GLFW/glfw3.h>

#include "Application.h"

int main()
{
    learn::webgpu::Application app;

    if (!app.init())
    {
        return -1;
    }

    while (app.isRunning() && app.render())
    {
        app.mainLoop();
    }

    app.terminate();
}