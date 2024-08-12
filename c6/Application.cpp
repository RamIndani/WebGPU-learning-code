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
        if (status == WGPURequestAdapterStatus_Success) {    
            userData.adapter = wgpuAdapter;
        } else {
            std::cout << "Adapter request failed" << message << std::endl;
        }
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
        if (status == WGPURequestAdapterStatus_Success) {
            userData.devide = device;
        } else {
            std::cout << "Device request failed" << message << std::endl; 
        }
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