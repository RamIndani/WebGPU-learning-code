#define WEBGPU_CPP_IMPLEMENTATION
#include <webgpu/webgpu.hpp>

#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>

#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#endif // __EMSCRIPTEN__

#pragma once

namespace learn::webgpu
{

    class Application
    {
    public:
        Application() : mDevice(nullptr), mSurface(nullptr), mQueue(nullptr),
        mTrianglePipeline(nullptr), mTextureFormat(wgpu::TextureFormat::Undefined) {}
        
        bool init();
        bool isRunning();
        void mainLoop();
        void setupPipeline();
        void setupBuffers();
        bool render();
        void terminate();

    private:
        GLFWwindow *mWindow;
        wgpu::Device mDevice;
        wgpu::Surface mSurface;
        wgpu::Queue mQueue;
        wgpu::RenderPipeline mTrianglePipeline;
        wgpu::TextureFormat mTextureFormat;
     

        const int kWindowWidth = 600;
        const int kWindowHieght = 600;

        wgpu::TextureView getNextSurfaceTextureView();
        void pollDevice();

        const char* shaderSource = R"(
            @vertex
            fn vs_main(@builtin(vertex_index) in_vertex_index: u32) -> @builtin(position) vec4f {
                var p = vec2f(0.0, 0.0);
                if (in_vertex_index == 0u) {
                    p = vec2f(-0.5, -0.5);
                } else if (in_vertex_index == 1u) {
                    p = vec2f(0.5, -0.5);
                } else {
                    p = vec2f(0.0, 0.5);
                }
                return vec4f(p, 0.0, 1.0);
            }

            @fragment
            fn fs_main() -> @location(0) vec4f {
                return vec4f(0.95, 0.05, 0.2, 1.0);
            }
        )";
    
    
    };
} // namespace learn::webgpu
