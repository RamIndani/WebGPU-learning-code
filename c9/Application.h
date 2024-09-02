#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>

#define WEBGPU_CPP_IMPLEMENTATION
#include <webgpu/webgpu.hpp>

#pragma once

namespace learn::webgpu
{

    class Application
    {
    public:
        bool init();
        bool isRunning();
        void mainLoop();
        void terminate();
        bool renderColor();

    private:
        GLFWwindow *mWindow;
        WGPUDevice mDevice;
        WGPUSurface mSurface;
        WGPUQueue mQueue;

        wgpu::TextureView getNextSurfaceTextureView();
    };
} // namespace learn::webgpu
