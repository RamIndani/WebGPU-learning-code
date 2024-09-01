#include <iostream>
#include <vector>

#include "Application.h"

namespace learn::webgpu {

    
WGPUAdapter requestAdapterSync(WGPUInstance wgpuInstance, WGPURequestAdapterOptions* options) {
    
    struct UserData {
        WGPUAdapter adapter = nullptr;
        bool requestEnded = false;
    }; 
    UserData userData;
    WGPURequestAdapterCallback adapterCallback = [](WGPURequestAdapterStatus status, 
    WGPUAdapter wgpuAdapter, char const * message, void * pUserData) {
        UserData& userData = *reinterpret_cast<UserData*>(pUserData);
        //if (status == WGPURequestAdapterStatus_Success) {    
            userData.adapter = wgpuAdapter;
        //} else {
 //           std::cout << "Adapter request failed" << message << std::endl;
            if (!status) {
std::cout << "Adapter request failed" << message << std::endl;
            
            }
        //}
        userData.requestEnded = true;
    };

    wgpuInstanceRequestAdapter(wgpuInstance, options, adapterCallback, &userData);
    
    return userData.adapter;
}

WGPUDevice requestDeviceSync(WGPUAdapter wgpuAdapter, WGPUDeviceDescriptor* descriptor) {
    struct UserData {
        WGPUDevice devide = nullptr;
        bool requestEnded = false;
    };
    UserData userData;

    auto onDeviceRequestEnded = [](WGPURequestDeviceStatus status, WGPUDevice device, char const * message, void * pUserData) {
        UserData& userData = *reinterpret_cast<UserData*>(pUserData);
       // if (status == WGPURequestAdapterStatus_Success) {
            userData.devide = device;
        //} else {
            if (!status) {
                std::cout << "Device request failed" << message << std::endl; 
            }
        //}
        userData.requestEnded = true;
    };

    wgpuAdapterRequestDevice(wgpuAdapter, descriptor, onDeviceRequestEnded, (void*)&userData);
    return userData.devide;
}


    bool Application::init() {

        if(!glfwInit()) {
            std::cout << "failed to initialize glfw" << std::endl;
            return false;
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        mWindow = glfwCreateWindow(600, 600, "Learn WebGPU", nullptr, nullptr);

        if (!mWindow) {
            glfwTerminate();
            return false;
        }

        WGPUInstanceDescriptor desc = {};
        desc.nextInChain = nullptr;
        WGPUInstance instance = wgpuCreateInstance(&desc);

        if (!instance) {
            std::cout << "falied to create wgpu instance" << std::endl;
            return false;
        }



        mSurface = glfwGetWGPUSurface(instance, mWindow);
        WGPURequestAdapterOptions options = {};
        options.nextInChain = nullptr;
        options.compatibleSurface = mSurface;
        options.powerPreference = WGPUPowerPreference_HighPerformance;
        WGPUAdapter adapter = requestAdapterSync(instance, &options);
   
        std::vector<WGPUFeatureName> features;
        WGPUDeviceDescriptor deviceDesc = {};
        deviceDesc.nextInChain = nullptr;
        deviceDesc.label = "GPU"; // anything works here, that's your call
        deviceDesc.requiredFeatures = features.data();
        deviceDesc.requiredLimits = nullptr; // we do not require any specific limit
        deviceDesc.defaultQueue.nextInChain = nullptr;
        deviceDesc.defaultQueue.label = "The default queue";
        deviceDesc.deviceLostCallback = [](WGPUDeviceLostReason reason, char const* message, void* /* pUserData */) {
            std::cout << "Device lost: reason " << reason;
            if (message) std::cout << " (" << message << ")";
            std::cout << std::endl;
        };
    
        
        mDevice = requestDeviceSync(adapter, &deviceDesc);
        WGPUTextureFormat textureFormat = wgpuSurfaceGetPreferredFormat(mSurface, adapter);
        wgpuInstanceRelease(instance);
        wgpuAdapterRelease(adapter);


        auto onDeviceError = [](WGPUErrorType type, char const* message, void* /* pUserData */) {
            std::cout << "Uncaptured device error: type " << type;
            if (message) std::cout << " (" << message << ")";
            std::cout << std::endl;
        };
        wgpuDeviceSetUncapturedErrorCallback(mDevice, onDeviceError, nullptr /* pUserData */);
        

         // configure the surface
        WGPUSurfaceConfiguration surfaceConfig = {};
        surfaceConfig.nextInChain = nullptr;
        surfaceConfig.width = 600;
        surfaceConfig.height = 600;
        surfaceConfig.format = textureFormat;
        surfaceConfig.usage = WGPUTextureUsage_RenderAttachment;
        surfaceConfig.device = mDevice;
        surfaceConfig.presentMode = WGPUPresentMode_Fifo;
        surfaceConfig.alphaMode = WGPUCompositeAlphaMode_Auto;
        wgpuSurfaceConfigure(mSurface, &surfaceConfig);


        // Initialize the command queue
        mQueue = wgpuDeviceGetQueue(mDevice);

        return true;
    }

    bool Application::isRunning() {
        return !glfwWindowShouldClose(mWindow);
    }

    void Application::mainLoop() {
        glfwPollEvents();
    }

    WGPUTextureView Application::getNextSurfaceTextureView() {
        WGPUSurfaceTexture surfaceTexture;
        wgpuSurfaceGetCurrentTexture(mSurface, &surfaceTexture);
        if (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_Success) {
            return nullptr;
        }

        WGPUTextureViewDescriptor viewDescriptor;
        viewDescriptor.nextInChain = nullptr;
        viewDescriptor.label = "Surface texture view";
        viewDescriptor.format = wgpuTextureGetFormat(surfaceTexture.texture);
        viewDescriptor.dimension = WGPUTextureViewDimension_2D;
        viewDescriptor.baseMipLevel = 0;
        viewDescriptor.mipLevelCount = 1;
        viewDescriptor.baseArrayLayer = 0;
        viewDescriptor.arrayLayerCount = 1;
        viewDescriptor.aspect = WGPUTextureAspect_All;
        WGPUTextureView targetView = wgpuTextureCreateView(surfaceTexture.texture, &viewDescriptor);


        return targetView;
    }


bool Application::renderColor() {
        WGPUTextureView textureView = getNextSurfaceTextureView();
        if (!textureView) {
            return false;
        }

        WGPURenderPassColorAttachment renderPassColorAttachment = {};
        renderPassColorAttachment.view = textureView;
        renderPassColorAttachment.resolveTarget = nullptr;
        renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
        renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
        renderPassColorAttachment.clearValue = WGPUColor{ 0.9, 0.1, 0.2, 1.0 };
        #ifndef WEBGPU_BACKEND_WGPU
            renderPassColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
        #endif // NOT WEBGPU_BACKEND_WGPU

        WGPURenderPassDescriptor renderPassDesc = {};
        renderPassDesc.nextInChain = nullptr;
        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments = &renderPassColorAttachment;
        renderPassDesc.depthStencilAttachment = nullptr;
        renderPassDesc.timestampWrites = nullptr;


        WGPUCommandEncoderDescriptor encoderDesc = {};
        encoderDesc.nextInChain = nullptr;
        encoderDesc.label = "My command encoder";
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(mDevice, &encoderDesc);

        WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);

        wgpuRenderPassEncoderEnd(renderPass);
        wgpuRenderPassEncoderRelease(renderPass);

        WGPUCommandBufferDescriptor cmdBufferDescriptor = {};
        cmdBufferDescriptor.nextInChain = nullptr;
        cmdBufferDescriptor.label = "Command buffer";
        WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);
        wgpuCommandEncoderRelease(encoder);

        std::cout << "Submitting command..." << std::endl;
        wgpuQueueSubmit(mQueue, 1, &command);
        wgpuCommandBufferRelease(command);

      
        wgpuTextureViewRelease(textureView);
        wgpuSurfacePresent(mSurface);
        return true;
    }

    void Application::terminate() {
        wgpuQueueRelease(mQueue);
        wgpuDeviceRelease(mDevice);
        wgpuSurfaceUnconfigure(mSurface);
        wgpuSurfaceRelease(mSurface);
        glfwDestroyWindow(mWindow);
        glfwTerminate();
    }
} // namespace learn::webgpu