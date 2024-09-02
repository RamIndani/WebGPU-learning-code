#include <iostream>
#include <vector>

#include "Application.h"

namespace learn::webgpu
{

    bool Application::init()
    {

        if (!glfwInit())
        {
            std::cout << "failed to initialize glfw" << std::endl;
            return false;
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        mWindow = glfwCreateWindow(600, 600, "Learn WebGPU", nullptr, nullptr);

        if (!mWindow)
        {
            glfwTerminate();
            return false;
        }

        wgpu::InstanceDescriptor desc = {};
        wgpu::Instance instance = wgpu::createInstance(desc);

        if (!instance)
        {
            std::cout << "falied to create wgpu instance" << std::endl;
            return false;
        }

        mSurface = glfwGetWGPUSurface(instance, mWindow);
        wgpu::RequestAdapterOptions options = {};
        options.compatibleSurface = mSurface;
        options.powerPreference = wgpu::PowerPreference::HighPerformance;
        wgpu::Adapter adapter = instance.requestAdapter(options);

        std::vector<wgpu::FeatureName> features;
        wgpu::DeviceDescriptor deviceDesc = {};
        deviceDesc.label = "GPU"; // anything works here, that's your call
        // deviceDesc.requiredFeatures = features.data;
        deviceDesc.requiredLimits = nullptr; // we do not require any specific limit
        deviceDesc.defaultQueue.nextInChain = nullptr;
        deviceDesc.defaultQueue.label = "The default queue";
        deviceDesc.deviceLostCallback = [](WGPUDeviceLostReason reason, char const *message, void * /* pUserData */)
        {
            std::cout << "Device lost: reason " << reason;
            if (message)
                std::cout << " (" << message << ")";
            std::cout << std::endl;
        };

        mDevice = adapter.requestDevice(deviceDesc);
        wgpu::TextureFormat textureFormat = mSurface.getPreferredFormat(adapter);
        instance.release();
        adapter.release();

        auto onDeviceError = [](wgpu::ErrorType type, char const *message)
        {
            std::cout << "Uncaptured device error: type " << type;
            if (message)
                std::cout << " (" << message << ")";
            std::cout << std::endl;
        };
        mDevice.setUncapturedErrorCallback(onDeviceError);

        // configure the surface
        wgpu::SurfaceConfiguration surfaceConfig = {};
        surfaceConfig.width = 600;
        surfaceConfig.height = 600;
        surfaceConfig.format = textureFormat;
        surfaceConfig.usage = wgpu::TextureUsage::RenderAttachment;
        surfaceConfig.device = mDevice;
        surfaceConfig.presentMode = wgpu::PresentMode::Fifo;
        surfaceConfig.alphaMode = wgpu::CompositeAlphaMode::Auto;
        mSurface.configure(surfaceConfig);

        // Initialize the command queue
        mQueue = mDevice.getQueue();

        return true;
    }

    bool Application::isRunning()
    {
        return !glfwWindowShouldClose(mWindow);
    }

    void Application::mainLoop()
    {
        glfwPollEvents();
    }

    wgpu::TextureView Application::getNextSurfaceTextureView()
    {
        wgpu::SurfaceTexture surfaceTexture;
        mSurface.getCurrentTexture(&surfaceTexture);
        wgpu::Texture texture = surfaceTexture.texture;
        if (surfaceTexture.status != wgpu::SurfaceGetCurrentTextureStatus::Success)
        {
            return nullptr;
        }

        wgpu::TextureViewDescriptor viewDescriptor{};
        viewDescriptor.label = "Surface texture view";
        viewDescriptor.format = wgpuTextureGetFormat(texture);
        viewDescriptor.dimension = wgpu::TextureViewDimension::_2D;
        viewDescriptor.baseMipLevel = 0;
        viewDescriptor.mipLevelCount = 1;
        viewDescriptor.baseArrayLayer = 0;
        viewDescriptor.arrayLayerCount = 1;
        viewDescriptor.aspect = wgpu::TextureAspect::All;
        
        wgpu::TextureView targetView = texture.createView(viewDescriptor);
        return targetView;
    }

    bool Application::renderColor()
    {
        wgpu::TextureView textureView = getNextSurfaceTextureView();
        if (!textureView)
        {
            return false;
        }

        wgpu::RenderPassColorAttachment renderPassColorAttachment = {};
        renderPassColorAttachment.view = textureView;
        renderPassColorAttachment.resolveTarget = nullptr;
        renderPassColorAttachment.loadOp = wgpu::LoadOp::Clear;
        renderPassColorAttachment.storeOp = wgpu::StoreOp::Store;
        renderPassColorAttachment.clearValue = wgpu::Color{0.9, 0.9, 0.2, 1.0};
#ifndef WEBGPU_BACKEND_WGPU
        renderPassColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif // NOT WEBGPU_BACKEND_WGPU

        wgpu::RenderPassDescriptor renderPassDesc = {};
        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments = &renderPassColorAttachment;
        renderPassDesc.depthStencilAttachment = nullptr;
        renderPassDesc.timestampWrites = nullptr;

        wgpu::CommandEncoderDescriptor encoderDesc = {};
        encoderDesc.label = "My command encoder";
        wgpu::CommandEncoder encoder = mDevice.createCommandEncoder(encoderDesc);
        wgpu::RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDesc);
        renderPass.end();
        renderPass.release();

        wgpu::CommandBufferDescriptor cmdBufferDescriptor = {};
        cmdBufferDescriptor.label = "Command buffer";
        wgpu::CommandBuffer command = encoder.finish(cmdBufferDescriptor);
        encoder.release();
        encoder.release();

        std::cout << "Submitting command..." << std::endl;
        mQueue.submit(command);
        command.release();
        textureView.release();
        mSurface.present();
        return true;
    }

    void Application::terminate()
    {
        mQueue.release();
        mDevice.release();
        mSurface.unconfigure();
        mSurface.release();
        glfwDestroyWindow(mWindow);
        glfwTerminate();
    }
} // namespace learn::webgpu