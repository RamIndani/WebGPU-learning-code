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
        mWindow = glfwCreateWindow(kWindowWidth, kWindowHieght, "Learn WebGPU", nullptr, nullptr);

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

        // Setup vertex shader
        // We are not using the buffers
        // Rendering pipeline configuration so that we can render our triangle using the GPU pipeline
        // This takes the vertext and fragment information on the pipeline.
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

        // Create an actual pipeline that chains together our vertex and fragment shader
        mTrianglePipeline = mDevice.createRenderPipeline(trianglePipelineDesc);
        shaderModule.release();
    }

    // Buffers are a memory location in the GPU/VRAM.
    // They represent data/information or whatever you want to store there for GPU to access.
    void Application::setupBuffers()
    {
        wgpu::BufferDescriptor bufferDesc;
        bufferDesc.label = "Some GPU side data";
        bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::CopySrc;
        //Hard coding not good but okay for now.
        bufferDesc.size = 16;
        bufferDesc.mappedAtCreation = false;
        //TADA we have a buffer 
        // Congratulations you just used your VRAM directly for the first time.
        wgpu::Buffer mInputBuffer = mDevice.createBuffer(bufferDesc);

        //Redine the label for the second buffer
        // We will write data into mInputBuffer and the copy it to the mOutputBuffer and output it from there.
       // wgpu::BufferDescriptor outputBufferDesc;
        bufferDesc.label = "Output buffer"; 
        // Change the usage bit as well to use MapRead and remove CopySrc
        // MapRead so that we can read back from the sencond buffer as we planned.
        bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;
        //bufferDesc.mappedAtCreation = false;
       // outputBufferDesc.size = 16;
        wgpu::Buffer mOutputBuffer = mDevice.createBuffer(bufferDesc);
        //NOTE: We release these buffers in the terminate, the fact that we call setupBuffers in the init()
        // we don't allocate buffers again while looping, so one release at the end in terminate is enough for now.

        //Now that buffers are setup and can write to the CPU as if we are storing in the vector
        // then use the Queue method to submit that data to write into a buffer on the GPU side VRAM
        // see it's just a memory location on the GPU side.
        std::vector<uint8_t> intes(16);
        for (uint8_t i = 0; i < 16; ++i) {
            intes[i] = i;
        }
        // write using mQueue, see we created mQueue before calling setupBuffers.
        // writeBuffer is an async function, it will return immediately but the operation of copying data
        // might still be working in the background.
        // We can release the ints RAM data at this point because graphics backend will maintain it's own copy
        // to transfter the data to GPU/VRAM.
        // NOTE: NO new command will work until the data is copied so we are blocked on this function being done
        // before we can process further functions.
        mQueue.writeBuffer(mInputBuffer, 0, intes.data(), intes.size());

        // Now lets copy data from the mInputBuffer GPU/VRAM To another param/memory on the VRAM
        // For this we can't use mQueue because mQueue is for sending info to GPU, once we are in the GPU
        // realm we have to use something that works inside the GPU/VRAM context, that is encoder.
        // encoder is basically commandBufferEncoder which is also the one created on the Queue itself.
        wgpu::CommandEncoder encoder = mDevice.createCommandEncoder(wgpu::Default);
        encoder.copyBufferToBuffer(mInputBuffer, 0, mOutputBuffer, 0, 16);
        wgpu::CommandBuffer command = encoder.finish(wgpu::Default);
        encoder.release();
        mQueue.submit(command);
        command.release();

        struct Context {
            bool ready;
            wgpu::Buffer buffer;
        };
        Context context = { false, mOutputBuffer};

        auto onCallback = [] (WGPUBufferMapAsyncStatus status, void* pUserData) {
            Context* context = reinterpret_cast<Context*>(pUserData);
            context->ready = true;
            std::cout << "Buffer reading status " << status << std::endl;
            if (status != wgpu::BufferMapAsyncStatus::Success) {
                std::cout << "Failed reading buffer data" << std::endl;
                return;
            }

            uint8_t* bufferData = (uint8_t*) context->buffer.getConstMappedRange(0, 16);

            std::cout << "bufferData = [";
            for (int i = 0; i < 16; ++i) {
                if ( i > 0) std::cout << ", ";
                std::cout << (int)bufferData[i];
            }
              std::cout << "]" << std::endl;

            context->buffer.unmap();
        };
      
        wgpuBufferMapAsync(mOutputBuffer, wgpu::MapMode::Read, /*offset*/ 0, /*size_t*/ 16, onCallback, (void*)&context);

        while(!context.ready) {
            pollDevice();
        }
       
        mInputBuffer.release();
        mOutputBuffer.release();
    }

    void Application::pollDevice() {
        #if defined(WEBGPU_BACKEND_DAWN)
            wgpuDeviceTick(mDevice);
        #elif defined(WEBGPU_BACKEND_WGPU)
            wgpuDevicePoll(mDevice, false, nullptr);
        #elif defined(WEBGPU_BACKEND_EMSCRIPTEN)
            emscripten_sleep(100);
        #endif
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
        // We begin our render pass here, by calling beginRenderPass.
        wgpu::RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDesc);

        // One we have have our render pass by executing encoder.beingRenderPass
        // It's our time to setup our rendering pipeline that we created on it. 
        // This way before drawing we link our pipeline to the GPU.
        renderPass.setPipeline(mTrianglePipeline);
        // Now instruct the GPU to draw, we want 3 vertices to be drawn for our triangle that's why 3.
        // And we want them to be rendered exactly once, that's why second param is 1.
        // Another way to say this is 1 instance of 3 vertices.
        renderPass.draw(3, 1, 0, 0);
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

        pollDevice();

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