
#include <webgpu/webgpu.hpp>
#include <iostream>
#include <vector>
#include <GLFW/glfw3.h>

#include "Application.h"

int main()
{
    learn::webgpu::Application appPtr;

    if (!appPtr.init())
    {
        return -1;
    }

    while (appPtr.isRunning() && appPtr.renderColor())
    {
        appPtr.mainLoop();
    }

    appPtr.terminate();
}