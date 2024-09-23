#define WEBGPU_CPP_IMPLEMENTATION
#include <webgpu/webgpu.hpp>

#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>

#include <vector>

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
        mTrianglePipeline(nullptr), mTextureFormat(wgpu::TextureFormat::Undefined),
        mVertexBuffer(nullptr), mColorBuffer(nullptr) {}
        
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
        wgpu::RequiredLimits getRequiredLimits(wgpu::Adapter adapter);
        void pollDevice();

        std::vector<float> mVertexData = {
            //x0, y0
            -0.4f, -0.6f,
            //x1, y1
            +0.4f, -0.6f,
            // x2, y2
            +0.4f, +0.6f,

            //x3, x3
            -0.4f, -0.6f,
            //x4, x4
            +0.4f, +0.6f,
            //x5, x5
            -0.4f, +0.6f,

        };

        std::vector<float> mColorData = {
            1.0, 0.0, 0.0,

            0.0, 1.0, 0.0,
            
            0.0, 0.0, 1.0,
            
            1.0, 0.0, 0.0,
            
            0.0, 0.0, 1.0,
            
            0.0, 1.0, 1.0
        };



        uint32_t mVertexCount = static_cast<uint32_t>(mVertexData.size() / 2);
        //assert(mVertexCount == static_cast<uint32_t>(mColorData.size() / 3));
        wgpu::Buffer mVertexBuffer;
        wgpu::Buffer mColorBuffer;
        
        const char* shaderSource = R"(
            struct VertexInput {
                @location(0) position: vec2f,
                @location(1) color: vec3f,
            };

            struct VertexOutput {
                @builtin(position) position: vec4f,
                @location(0) color: vec3f
            };

            @vertex
            fn vs_main(in: VertexInput) -> VertexOutput {
                var out: VertexOutput;
                out.position = vec4f(in.position, 0.0, 1.0);
                out.color = in.color;
                return out;
            }

            @fragment
            fn fs_main(in: VertexOutput) -> @location(0) vec4f {
                return vec4f(in.color, 1.0);
            }
        )";
    
        
    
    };
} // namespace learn::webgpu
