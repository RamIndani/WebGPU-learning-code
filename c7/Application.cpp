#include <iostream>
#include <vector>

#include "Application.h"

namespace learn::webgpu {

    
wgpu::Adapter requestAdapterSync(wgpu::Instance wgpuInstance, wgpu::RequestAdapterOptions* options) {
    
    struct UserData {
       wgpu::Adapter adapter = nullptr;
        bool requestEnded = false;
    }; 
    UserData userData;
    WGPURequestAdapterCallback adapterCallback = [](WGPURequestAdapterStatus status, 
    WGPUAdapter wgpuAdapter, char const * message, void * pUserData) {
        UserData& userData = *reinterpret_cast<UserData*>(pUserData);
        if (status == WGPURequestAdapterStatus_Success) {    
            userData.adapter = reinterpret_cast<wgpu::Adapter>(wgpuAdapter);
        } else {
            std::cout << "Adapter request failed" << message << std::endl;
        }
        userData.requestEnded = true;
    };

    wgpuInstance.RequestAdapter(options, adapterCallback, &userData);
    
    return userData.adapter;
}

wgpu::Device requestDeviceSync(wgpu::Adapter wgpuAdapter, wgpu::DeviceDescriptor* descriptor) {
    struct UserData {
        wgpu::Device device = nullptr;
        bool requestEnded = false;
    };
    UserData userData;

    auto onDeviceRequestEnded = [](WGPURequestDeviceStatus status, WGPUDevice device, char const * message, void * pUserData) {
        UserData& userData = *reinterpret_cast<UserData*>(pUserData);
        if (status == WGPURequestAdapterStatus_Success) {
            userData.device = reinterpret_cast<wgpu::Device>(device);
        } else {
            std::cout << "Device request failed" << message << std::endl; 
        }
        userData.requestEnded = true;
    };

    wgpuAdapter.RequestDevice(descriptor, onDeviceRequestEnded, (void*)&userData);
    return userData.device;
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

        wgpu::InstanceDescriptor desc = {};
        wgpu::Instance instance = wgpu::CreateInstance(&desc);

        if (!instance) {
            std::cout << "falied to create wgpu instance" << std::endl;
            return false;
        }

        mSurface = glfwGetWGPUSurface(instance, mWindow);
        wgpu::RequestAdapterOptions options = {
            .compatibleSurface = mSurface,
            .powerPreference = wgpu::PowerPreference::HighPerformance
        }
        wgpu::Adapter adapter = requestAdapterSync(instance, &options);
   
        std::vector<wgpu::FeatureName> features;
        wgpu::DeviceDescriptor deviceDesc = {};

        deviceDesc.label = "GPU"; // anything works here, that's your call
        deviceDesc.requiredFeatures = features.data();
        deviceDesc.requiredLimits = nullptr; // we do not require any specific limit
        deviceDesc.defaultQueue.nextInChain = nullptr;
        deviceDesc.defaultQueue.label = "The default queue";
        deviceDesc.deviceLostCallback = [](wgpu::DeviceLostReason reason, char const* message, void* /* pUserData */) {
            std::cout << "Device lost: reason " << reason;
            if (message) std::cout << " (" << message << ")";
            std::cout << std::endl;
        };

        mDevice = requestDeviceSync(adapter, &deviceDesc);
        wgpuInstanceRelease(instance);
        wgpuAdapterRelease(adapter);

        return true;
    }

    bool Application::isRunning() {
        return !glfwWindowShouldClose(mWindow);
    }

    void Application::mainLoop() {
        glfwPollEvents();
    }

    void Application::terminate() {
        wgpuDeviceRelease(mDevice);
        wgpuSurfaceRelease(mSurface);
        glfwDestroyWindow(mWindow);
        glfwTerminate();
    }
} // namespace learn::webgpu