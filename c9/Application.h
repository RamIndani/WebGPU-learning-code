#define WEBGPU_CPP_IMPLEMENTATION
#include <webgpu/webgpu.hpp>

#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>

#pragma once
using namespace wgpu;

namespace learn::webgpu
{

    class Application
    {
    public:
        Application() : mDevice(nullptr), mSurface(nullptr), mQueue(nullptr) {}
        
        bool init();
        bool isRunning();
        void mainLoop();
        void terminate();
        bool renderColor();

    private:
        GLFWwindow *mWindow;
        wgpu::Device mDevice;
        wgpu::Surface mSurface;
        wgpu::Queue mQueue;

        TextureView getNextSurfaceTextureView();
    };
} // namespace learn::webgpu
