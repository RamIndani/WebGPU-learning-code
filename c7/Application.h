#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>
#include <webgpu/webgpu.h>

#pragma once

namespace learn::webgpu{

class Application {
    public:
        bool init();
        bool isRunning();
        void mainLoop();
        void terminate();

    private:
        GLFWwindow* mWindow;
        WGPUDevice mDevice;
        WGPUSurface mSurface;
};
} // namespace learn::webgpu
