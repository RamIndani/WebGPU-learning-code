#include <chrono>
#include <iostream>
#include <vector>

#include "Application.h"

namespace learn::webgpu
{

    bool Application::init()
    {

        
        SDL_SetMainReady();
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << "Could not initialize SDL! Error: " << SDL_GetError() << std::endl;
            return 1;
        }
       
        int windowFlags = 0;
        mWindow = SDL_CreateWindow("Learn WebGPU", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, kWindowWidth, kWindowHieght, windowFlags);
        if (!mWindow)
        {
            SDL_DestroyWindow(mWindow);
            SDL_Quit();
            return false;
        }

        wgpu::InstanceDescriptor desc = {};
        wgpu::Instance instance = wgpu::createInstance(desc);

        if (!instance)
        {
            std::cout << "falied to create wgpu instance" << std::endl;
            return false;
        }

        mSurface = SDL_GetWGPUSurface(instance, mWindow);
        wgpu::RequestAdapterOptions options = {};
        options.compatibleSurface = mSurface;
        options.powerPreference = wgpu::PowerPreference::HighPerformance;
        wgpu::Adapter adapter = instance.requestAdapter(options);


        std::vector<wgpu::FeatureName> features;
        wgpu::DeviceDescriptor deviceDesc = {};
        deviceDesc.label = "GPU"; 
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

        // Get the adapter limits
        wgpu::RequiredLimits requiredLimits = getRequiredLimits(adapter);
        deviceDesc.requiredLimits = &requiredLimits;
        // Get the actual device that we will use.
        // Think of device as a GPU, basically anything that we want to do with GPU
        // we need to work with the device. 
        mDevice = adapter.requestDevice(deviceDesc);

        // Get teh format of the surface that we will draw on to.
        mTextureFormat = mSurface.getPreferredFormat(adapter);

        // Release the WGPUInstance and WGPUAdapter, we no longer need them after getting our device.
        instance.release();
        adapter.release();

        // Setup the global error handler on the  device because sometimes we have errors 
        // on the device/GPU and we need a capture all error handler for these global
        // unknown errors.
        auto onDeviceError = [](wgpu::ErrorType type, char const *message)
        {
            std::cout << "Uncaptured device error: type " << type;
            if (message)
                std::cout << " (" << message << ")";
            std::cout << std::endl;
        };
        mDevice.setUncapturedErrorCallback(onDeviceError);

        // configure the surface
        // Even though the surface is acquired from the window with the same adapter from which
        // we got the device, we should reconfigure the surface, this way we can control height and width 
        // of the surface when we change the window to max or min width, and few other prameters so that
        // we can control how frames from the surface gets on to the display, either directly as front
        // buffer rendering or as a latest complete buffer instead of readering in the pipeline one after another
        // this is important to avoid tearning or undersired behavior.
        wgpu::SurfaceConfiguration surfaceConfig = {};
        surfaceConfig.width = kWindowWidth;
        surfaceConfig.height = kWindowHieght;
        // Specify the format for the Surface, we got this from the surface that we got from the glfw
        surfaceConfig.format = mTextureFormat;
        surfaceConfig.usage = wgpu::TextureUsage::RenderAttachment;
        // Link ths surface and the device
        surfaceConfig.device = mDevice;
        surfaceConfig.presentMode = wgpu::PresentMode::Fifo;
        surfaceConfig.alphaMode = wgpu::CompositeAlphaMode::Auto;
        // Reconfigure the surface using the SurfaceConfig.
        mSurface.configure(surfaceConfig);


        // Initialize the command queue for the current device that we are working with.
        mQueue = mDevice.getQueue();

        // Once the device is acquired, configuration for the rendering on the device is complete,
        // and also we got our Queue from the device to send buffers/commands. it's time to setup
        // the rendering pipeline.
        setupPipeline();

        setupBuffers();

        return true;
    }

    // A function that gets the limits on the adapter
    wgpu::RequiredLimits Application::getRequiredLimits(wgpu::Adapter adapter) {
        wgpu::SupportedLimits supportedLimits;
        adapter.getLimits(&supportedLimits);

        wgpu::RequiredLimits requiredLimits = wgpu::Default;

        //Uniform limits
        requiredLimits.limits.maxBindGroups = 1;
        requiredLimits.limits.maxUniformBuffersPerShaderStage = 1;
        requiredLimits.limits.maxUniformBufferBindingSize = 16 * 4;

        requiredLimits.limits.maxVertexAttributes = 2;

        requiredLimits.limits.maxVertexBuffers = 2;

        requiredLimits.limits.maxBufferSize = 6 * 3 * sizeof(float);

        requiredLimits.limits.maxVertexBufferArrayStride = 3 * sizeof(float);

        requiredLimits.limits.maxInterStageShaderComponents = 3;

        requiredLimits.limits.minUniformBufferOffsetAlignment = supportedLimits.limits.minUniformBufferOffsetAlignment;
        requiredLimits.limits.minStorageBufferOffsetAlignment = supportedLimits.limits.minStorageBufferOffsetAlignment;

        requiredLimits.limits.maxTextureDimension2D = 1080;

        return requiredLimits;
    }
    // Pipeline is nothing but setting up our shaders, GPU is not a general pupose
    // programming hardware, it's a specialist hardware with fixed set of pipeline executions on it.
    // We simply configure the pipeline with the appropriate data and code we want to execute at
    // the specific stage of the pipeline. 
    // There are many items in t he pipeline but the most important ones are the vertex processing to draw
    // primitive shapes, and a fragment processor which is a component that adds color to the computed value
    // in the simplest of languages. 
    // To configure the different stages there is a separate languge that is used for WebGPU the language
    // is WGSL, we write WGSL code to program where we want the pixels and how we want to color them. 
    // Then we link these shaders to the software that basically acts as a glue between GPU and the shader that we have.
    // Shaders is how we describe our intent to the GPU and a glue code helps us pipe through our intents to the GPU
    // that's why we call it a pipeline setup. Basically linking the shader that we have with the GPU exeuction 
    // functions.
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
        // ShaderModule needs code, and it can't be directly connected to the ShaderModule.
        // We need a ShaderModuleWGSLDescriptor for that.
        wgpu::ShaderModuleWGSLDescriptor shaderCodeDesc;
        shaderCodeDesc.chain.next = nullptr;
        shaderCodeDesc.chain.sType = wgpu::SType::ShaderModuleWGSLDescriptor;
        shaderCodeDesc.code = shaderSource;

        // Chain the module with the code.
        shaderModuleDesc.nextInChain = &shaderCodeDesc.chain;
        // Create the ShaderModule
        wgpu::ShaderModule shaderModule = mDevice.createShaderModule(shaderModuleDesc);

        std::vector<wgpu::VertexBufferLayout> vertexBufferLayouts(2);
        
        //VertextBufferLayout setup for vertex position
        wgpu::VertexAttribute positionAttrib;
        positionAttrib.shaderLocation = 0;
        positionAttrib.format = wgpu::VertexFormat::Float32x2;
        positionAttrib.offset = 0;
        //Setup the VertexBufferLayout for the position
        vertexBufferLayouts[0].attributeCount = 1;
        vertexBufferLayouts[0].attributes = &positionAttrib;
        vertexBufferLayouts[0].arrayStride = 2 * sizeof(float);
        vertexBufferLayouts[0].stepMode = wgpu::VertexStepMode::Vertex;

        // VertexAttrib for the color
        wgpu::VertexAttribute colorAttrib;
        colorAttrib.shaderLocation = 1; //@location(1) from the shader  code
        colorAttrib.format = wgpu::VertexFormat::Float32x3;
        colorAttrib.offset = 0;
        //Setup the VertexBufferLayout for the color
        vertexBufferLayouts[1].attributeCount = 1;
        vertexBufferLayouts[1].attributes = &colorAttrib;
        vertexBufferLayouts[1].arrayStride = 3 * sizeof(float);
        vertexBufferLayouts[1].stepMode = wgpu::VertexStepMode::Vertex;


        // Setup vertex shader
        // We are not using the buffers
        // Rendering pipeline configuration so that we can render our triangle using the GPU pipeline
        // This takes the vertext and fragment information on the pipeline.
        wgpu::RenderPipelineDescriptor trianglePipelineDesc;
        trianglePipelineDesc.layout = nullptr;
        trianglePipelineDesc.vertex.bufferCount = static_cast<uint32_t>(vertexBufferLayouts.size());
        trianglePipelineDesc.vertex.buffers = vertexBufferLayouts.data();
        trianglePipelineDesc.vertex.module = shaderModule;
        trianglePipelineDesc.vertex.entryPoint = "vs_main";
        trianglePipelineDesc.vertex.constantCount = 0;
        trianglePipelineDesc.vertex.constants = nullptr;
        trianglePipelineDesc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
        trianglePipelineDesc.primitive.stripIndexFormat = wgpu::IndexFormat::Undefined;
        trianglePipelineDesc.primitive.cullMode = wgpu::CullMode::None;
        trianglePipelineDesc.primitive.frontFace = wgpu::FrontFace::CCW;

        // setup fragment shader
        // Fragment shader is basically how we determine the color of the pixel at
        // the end of the rendering pipeline. We could have just directly said use
        // certain fixed color, though it's a good practice to setup our Blend and the color
        // format with that blend.

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

        //Setup Uniform
        wgpu::BindGroupLayoutEntry bindingLayout = wgpu::Default;
        bindingLayout.binding = 0;
        bindingLayout.visibility = wgpu::ShaderStage::Vertex;
        bindingLayout.buffer.type = wgpu::BufferBindingType::Uniform;
        bindingLayout.buffer.minBindingSize = sizeof(float);

        wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc;
        bindGroupLayoutDesc.entryCount = 1;
        bindGroupLayoutDesc.entries = &bindingLayout;
        mBindGroupLayout = mDevice.createBindGroupLayout(bindGroupLayoutDesc);

        wgpu::PipelineLayoutDescriptor layoutDesc{};
        layoutDesc.bindGroupLayoutCount = 1;
        layoutDesc.bindGroupLayouts = (WGPUBindGroupLayout*)&mBindGroupLayout;
        wgpu::PipelineLayout layout = mDevice.createPipelineLayout(layoutDesc);
        trianglePipelineDesc.layout = layout;


        
        // Create an actual pipeline that chains together our vertex and fragment shader
        mTrianglePipeline = mDevice.createRenderPipeline(trianglePipelineDesc);
        shaderModule.release();
    }

    // Buffers are a memory location in the GPU/VRAM.
    // They represent data/information or whatever you want to store there for GPU to access.
    void Application::setupBuffers()
    {
        
        wgpu::BufferDescriptor bufferDesc;
        bufferDesc.mappedAtCreation = false;
        
        bufferDesc.label = "Vertex Point Buffer";
        bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex;
        bufferDesc.size = mPointData.size() * sizeof(float);
        mPointBuffer = mDevice.createBuffer(bufferDesc);
        mQueue.writeBuffer(mPointBuffer, 0, mPointData.data(), bufferDesc.size);

        bufferDesc.label = "Color buffer";
        bufferDesc.size = mColorData.size() * sizeof(float);
        mColorBuffer = mDevice.createBuffer(bufferDesc);
        mQueue.writeBuffer(mColorBuffer, 0, mColorData.data(), bufferDesc.size);

        bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Index;
        bufferDesc.label = "Index Buffer";
        bufferDesc.size = mIndexData.size() * sizeof(uint16_t);
        mIndexBuffer = mDevice.createBuffer(bufferDesc);
        mQueue.writeBuffer(mIndexBuffer, 0, mIndexData.data(), bufferDesc.size);

        // Uniform buffer setup
        bufferDesc.label = "Uniform";
        bufferDesc.size = sizeof(float);
        bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform;
        mUniformBuffer = mDevice.createBuffer(bufferDesc);
        mQueue.writeBuffer(mUniformBuffer, 0, &mCurrentTime, sizeof(float));



        wgpu::BindGroupEntry binding;
        binding.binding = 0;
        binding.buffer = mUniformBuffer;
        binding.offset = 0;
        binding.size = sizeof(float);

        wgpu::BindGroupDescriptor bindGroupDesc;
        bindGroupDesc.layout = mBindGroupLayout;
        bindGroupDesc.entryCount = 1; // bindGroupLayoutDesc.entryCount;
        bindGroupDesc.entries = &binding;
        mBindGroup = mDevice.createBindGroup(bindGroupDesc);


        wgpu::CommandEncoder encoder = mDevice.createCommandEncoder(wgpu::Default);
        wgpu::CommandBuffer command = encoder.finish(wgpu::Default);
        encoder.release();
        mQueue.submit(command);
        command.release();
       
    }

    bool Application::isRunning()
    {
        return !mShouldCloseWindow;
    }

    void Application::mainLoop()
    {
     

            // Poll events and handle them.
            // (contrary to GLFW, close event is not automatically managed, and there
            // is no callback mechanism by default.)
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                switch (event.type) {
                    case SDL_QUIT:
                        mShouldCloseWindow = true;
                        break;
                    default:
                        std::cout << event.type << std::endl;
                        break;
                }
            }
    }

    wgpu::TextureView Application::getNextSurfaceTextureView()
    {
        wgpu::SurfaceTexture surfaceTexture;
        // Get the next SurfaceTexture that we should write into.
        mSurface.getCurrentTexture(&surfaceTexture);
        // The actual Texture on which we draw.
        wgpu::Texture texture = surfaceTexture.texture;
        // Make sure the texture can be used for the drawing.
        if (surfaceTexture.status != wgpu::SurfaceGetCurrentTextureStatus::Success)
        {
            return nullptr;
        }

        // Configure the TextureView out of a texture that we have. 
        // We don't draw on the texture directly rather draw on the TextureView.
        wgpu::TextureViewDescriptor viewDescriptor{};
        viewDescriptor.label = "Surface texture view";
        viewDescriptor.format = mTextureFormat;
        viewDescriptor.dimension = wgpu::TextureViewDimension::_2D;
        viewDescriptor.baseMipLevel = 0;
        viewDescriptor.mipLevelCount = 1;
        viewDescriptor.baseArrayLayer = 0;
        viewDescriptor.arrayLayerCount = 1;
        viewDescriptor.aspect = wgpu::TextureAspect::All;

        wgpu::TextureView targetView = texture.createView(viewDescriptor);
        return targetView;
    }

    bool Application::render()
    {
        auto textureView = getNextSurfaceTextureView();
        if (!textureView)
        {
            return false;
        }

        wgpu::CommandEncoderDescriptor encoderDesc = {};
        encoderDesc.label = "encoder";
        wgpu::CommandEncoder encoder = mDevice.createCommandEncoder(encoderDesc);

        wgpu::RenderPassColorAttachment renderPassColorAttachment = {};
        // Setup the textureView where we will draw our content.
        renderPassColorAttachment.view = textureView;
        renderPassColorAttachment.resolveTarget = nullptr;
        renderPassColorAttachment.loadOp = wgpu::LoadOp::Clear;
        renderPassColorAttachment.storeOp = wgpu::StoreOp::Store;
        renderPassColorAttachment.clearValue = wgpu::Color{0.05, 0.05, 0.05, 1.0};

        wgpu::RenderPassDescriptor renderPassDesc = {};
        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments = &renderPassColorAttachment;
        renderPassDesc.depthStencilAttachment = nullptr;
        renderPassDesc.timestampWrites = nullptr;

     
       
        // We begin our render pass here, by calling beginRenderPass.
        wgpu::RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDesc);

        // One we have have our render pass by executing encoder.beingRenderPass
        // It's our time to setup our rendering pipeline that we created on it. 
        // This way before drawing we link our pipeline to the GPU.
        renderPass.setPipeline(mTrianglePipeline);
        renderPass.setVertexBuffer(0, mPointBuffer, 0, mPointBuffer.getSize());
        renderPass.setVertexBuffer(1, mColorBuffer, 0, mColorBuffer.getSize());
        renderPass.setIndexBuffer(mIndexBuffer, wgpu::IndexFormat::Uint16, 0, mIndexBuffer.getSize());

        renderPass.setBindGroup(0, mBindGroup, 0, nullptr);      

        mCurrentTime =  static_cast<float>(SDL_GetTicks() * 0.001); // milliseconds to second
        std::cout << mCurrentTime << std::endl;
        mQueue.writeBuffer(mUniformBuffer, 0, &mCurrentTime, sizeof(float));

        // Now instruct the GPU to draw, we want 3 vertices to be drawn for our triangle that's why 3.
        // And we want them to be rendered exactly once, that's why second param is 1.
        // Another way to say this is 1 instance of 3 vertices.
        renderPass.drawIndexed(mIndexCount, 1, 0, 0, 0);
        // End the rendering pass because we are done drawing.
        renderPass.end();
        // Release the render pass so that GPU can release the resource when it's done.
        // Otherwise we end up leaking memory here.
        renderPass.release();

        // Render pass is linked to the encoder, but sending these values
        // just by calling end and release. 
        // To send the whole render pass to the GPU we use the command encoder queue that we have.
        // So get the command and then send it as the command over the queue.
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
        #ifndef __EMSCRIPTEN__
            mSurface.present();
        #endif

        return true;
    }

    void Application::terminate()
    {
        mPointBuffer.destroy();
        mPointBuffer.release();
        mColorBuffer.destroy();
        mColorBuffer.release();
        mIndexBuffer.destroy();
        mIndexBuffer.release();
        mUniformBuffer.destroy();
        mUniformBuffer.release();
        mTrianglePipeline.release();
        mQueue.release();
        mDevice.release();
        mSurface.unconfigure();
        mSurface.release();
        SDL_DestroyWindow(mWindow);
        SDL_Quit();
    }
} // namespace learn::webgpu