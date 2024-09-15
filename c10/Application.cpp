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
        deviceDesc.requiredFeatures = nullptr;
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
        mTextureFormat = mSurface.getPreferredFormat(adapter);
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
        surfaceConfig.format = mTextureFormat;
        surfaceConfig.usage = wgpu::TextureUsage::RenderAttachment;
        surfaceConfig.device = mDevice;
        surfaceConfig.presentMode = wgpu::PresentMode::Fifo;
        surfaceConfig.alphaMode = wgpu::CompositeAlphaMode::Auto;
        mSurface.configure(surfaceConfig);

        // Initialize the command queue
        mQueue = mDevice.getQueue();

        setupPipeline();

        return true;
    }

    void Application::setupPipeline()
    {
        // Setup shaderModule that binds together our shader code WGSL with
        // vertext and fragment shader calls
        wgpu::ShaderModuleDescriptor shaderModuleDesc = {};
        shaderModuleDesc.label = "triangle shader module";
#ifdef WEBGPU_BACKEND_WGPU
        shaderModuleDesc.hintCount = 0;
        shaderModuleDesc.hints = nullptr;
#endif

        wgpu::ShaderModuleWGSLDescriptor shaderCodeDesc;
        shaderCodeDesc.chain.next = nullptr;
        shaderCodeDesc.chain.sType = wgpu::SType::ShaderModuleWGSLDescriptor;
        shaderModuleDesc.nextInChain = &shaderCodeDesc.chain;
        shaderCodeDesc.code = shaderSource;
        wgpu::ShaderModule shaderModule = mDevice.createShaderModule(shaderModuleDesc);

        // Setup vertex shader
        // We are not using the buffers
        wgpu::RenderPipelineDescriptor trianglePipelineDesc;
        trianglePipelineDesc.layout = nullptr;
        trianglePipelineDesc.vertex.bufferCount = 0;
        trianglePipelineDesc.vertex.buffers = nullptr;
        trianglePipelineDesc.vertex.module = shaderModule;
        trianglePipelineDesc.vertex.entryPoint = "vs_main";
        trianglePipelineDesc.vertex.constantCount = 0;
        trianglePipelineDesc.vertex.constants = nullptr;
        trianglePipelineDesc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
        trianglePipelineDesc.primitive.stripIndexFormat = wgpu::IndexFormat::Undefined;
        trianglePipelineDesc.primitive.cullMode = wgpu::CullMode::None;
        trianglePipelineDesc.primitive.frontFace = wgpu::FrontFace::CCW;

        // setup fragment shader
        // first blend config
        wgpu::BlendState blendState;
        blendState.color.srcFactor = wgpu::BlendFactor::SrcAlpha;
        blendState.color.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
        blendState.color.operation = wgpu::BlendOperation::Add;
        blendState.alpha.srcFactor = wgpu::BlendFactor::Zero;
        blendState.alpha.dstFactor = wgpu::BlendFactor::One;
        blendState.alpha.operation = wgpu::BlendOperation::Add;
        // second color target state using above blend of color/alpha configs
        wgpu::ColorTargetState colorTargetState;
        colorTargetState.format = mTextureFormat;
        colorTargetState.blend = &blendState;
        colorTargetState.writeMask = wgpu::ColorWriteMask::All;
        // then fragment shader using above properties
        wgpu::FragmentState fragmentState;
        fragmentState.module = shaderModule;
        fragmentState.entryPoint = "fs_main";
        fragmentState.constantCount = 0;
        fragmentState.constants = nullptr;
        fragmentState.targetCount = 1;
        fragmentState.targets = &colorTargetState;

        // Add the above created fragment shader config to the pipeline descriptor
        trianglePipelineDesc.fragment = &fragmentState;
        trianglePipelineDesc.multisample.count = 1;
        trianglePipelineDesc.multisample.mask = ~0u;
        trianglePipelineDesc.multisample.alphaToCoverageEnabled = false;
        // Not using the depth
        trianglePipelineDesc.depthStencil = nullptr;

        // Create an actual pipeline that chains together our vertex and fragment shader
        mTrianglePipeline = mDevice.createRenderPipeline(trianglePipelineDesc);
        shaderModule.release();
    }

    bool Application::isRunning()
    {
        return !glfwWindowShouldClose(mWindow);
    }

    void Application::mainLoop()
    {
        glfwPollEvents();
    }

    std::pair<wgpu::TextureView, wgpu::Texture> Application::getNextSurfaceTextureView()
    {
        wgpu::SurfaceTexture surfaceTexture;
        mSurface.getCurrentTexture(&surfaceTexture);
        wgpu::Texture texture = surfaceTexture.texture;
        if (surfaceTexture.status != wgpu::SurfaceGetCurrentTextureStatus::Success)
        {
            return {nullptr, nullptr};
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
        return {targetView, texture};
    }

    bool Application::render()
    {
        auto [textureView, texture] = getNextSurfaceTextureView();
        if (!textureView || !texture)
        {
            return false;
        }

        wgpu::RenderPassColorAttachment renderPassColorAttachment = {};
        // Setup the textureView where we will draw our content.
        renderPassColorAttachment.view = textureView;
        renderPassColorAttachment.resolveTarget = nullptr;
        renderPassColorAttachment.loadOp = wgpu::LoadOp::Clear;
        renderPassColorAttachment.storeOp = wgpu::StoreOp::Store;
        renderPassColorAttachment.clearValue = wgpu::Color{1.0, 0.6, 0.65, 1.0};

        wgpu::RenderPassDescriptor renderPassDesc = {};
        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments = &renderPassColorAttachment;
        renderPassDesc.depthStencilAttachment = nullptr;
        renderPassDesc.timestampWrites = nullptr;

        wgpu::CommandEncoderDescriptor encoderDesc = {};
        encoderDesc.label = "background color encoder";
        wgpu::CommandEncoder encoder = mDevice.createCommandEncoder(encoderDesc);
        wgpu::RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDesc);

        renderPass.setPipeline(mTrianglePipeline);
        renderPass.draw(3, 1, 0, 0);
        renderPass.end();
        renderPass.release();

        wgpu::CommandBufferDescriptor cmdBufferDescriptor = {};
        cmdBufferDescriptor.label = "Command buffer";
        // Finish configuring the encoder and get the final command that we will
        // submit to the command queue.
        wgpu::CommandBuffer command = encoder.finish(cmdBufferDescriptor);
        encoder.release();

        // Submitting command.
        mQueue.submit(command);
        // Release command that we created
        command.release();
        textureView.release();

        // Present the surface
        mSurface.present();
        return true;
    }

    void Application::terminate()
    {
        mTrianglePipeline.release();
        mQueue.release();
        mDevice.release();
        mSurface.unconfigure();
        mSurface.release();
        glfwDestroyWindow(mWindow);
        glfwTerminate();
    }
} // namespace learn::webgpu