
#include <webgpu/webgpu.hpp>
#include <iostream>
#include <vector>

#include "Application.h"

int main()
{
    learn::webgpu::Application app;

    if (!app.init())
    {
        return -1;
    }

    while (app.isRunning())
    {
        app.render();
        app.mainLoop();
    }

    app.terminate();
}