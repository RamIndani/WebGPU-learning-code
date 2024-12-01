#define WEBGPU_CPP_IMPLEMENTATION
#include <webgpu/webgpu.hpp>

#define SDL_MAIN_HANDLED
#include <sdl2webgpu.h>
#include <SDL2/SDL.h>

#include <vector>
#include <cassert>

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
         mColorBuffer(nullptr),mPointBuffer(nullptr), mIndexBuffer(nullptr),
         mUniformBuffer(nullptr), mBindGroup(nullptr) {}
        
        bool init();
        bool isRunning();
        void mainLoop();
        void setupPipeline();
        void setupBuffers();
        bool render();
        void terminate();

    private:
        SDL_Window *mWindow;
        wgpu::Device mDevice;
        wgpu::Surface mSurface;
        wgpu::Queue mQueue;
        wgpu::RenderPipeline mTrianglePipeline;
        wgpu::TextureFormat mTextureFormat;
     

        const int kWindowWidth = 600;
        const int kWindowHieght = 600;
        bool mShouldCloseWindow = false;

        wgpu::TextureView getNextSurfaceTextureView();
        wgpu::RequiredLimits getRequiredLimits(wgpu::Adapter adapter);
        
        std::vector<float> mPointData = {
            -0.4f, -0.6f, // Point 0
            +0.4f, -0.6f, // Point 1
            +0.4f, +0.6f, // Point 2
            -0.4f, +0.6f // Point 3
        };

        std::vector<uint16_t> mIndexData = {
            0, 1, 2, // Triangle 1
            0, 2, 3 // Triangle 2
        };

        std::vector<float> mColorData = {
            1.0f, 0.0f, 0.0f,

            0.0f, 1.0f, 0.0f,
            
            0.0f, 0.0f, 1.0f,

            1.0f, 0.0f, 1.0f

        };

        wgpu::Buffer mColorBuffer;
        wgpu::Buffer mPointBuffer;
        wgpu::Buffer mIndexBuffer;
        wgpu::Buffer mUniformBuffer;
        wgpu::BindGroup mBindGroup;
        wgpu::BindGroupLayout mBindGroupLayout = nullptr;
        uint32_t mIndexCount = static_cast<uint32_t>(mIndexData.size());
        float mCurrentTime = 0.0f;

         
        const char* shaderSource = R"(
            @group(0) @binding(0) var<uniform> uTime: f32;

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
                out.color = vec3f(sin(in.color[0] + uTime), cos(in.color[1]), sin(in.color[2] * uTime));
                return out;
            }

            @fragment
            fn fs_main(in: VertexOutput) -> @location(0) vec4f {
                return vec4f(in.color, 1.0f);
            }
        )";
    };
} // namespace learn::webgpu
