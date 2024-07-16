#include <iostream>
#include <vector>
#include <webgpu/webgpu.h>

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

int main() {
    WGPUInstanceDescriptor desc = {};
    desc.nextInChain = nullptr;
    WGPUInstance instance = wgpuCreateInstance(&desc);

    if (!instance) {
        std::cout << "falied to create instance" << std::endl;
        return 1;
    }

    
    WGPURequestAdapterOptions options = {};
    options.nextInChain = nullptr;
    options.powerPreference = WGPUPowerPreference_HighPerformance;
    WGPUAdapter adapter = requestAdapterSync(instance, &options);
    wgpuInstanceRelease(instance);


    // Limits
    WGPUSupportedLimits supportedLimits = {};
    supportedLimits.nextInChain = nullptr;

    const bool success = wgpuAdapterGetLimits(adapter, &supportedLimits);

    if (success) {
        std::cout << "Adapter limits:" << std::endl;
        std::cout << " - maxTextureDimension1D: " << supportedLimits.limits.maxTextureDimension1D << std::endl;
        std::cout << " - maxTextureDimension2D: " << supportedLimits.limits.maxTextureDimension2D << std::endl;
        std::cout << " - maxTextureDimension3D: " << supportedLimits.limits.maxTextureDimension3D << std::endl;
        std::cout << " - maxTextureArrayLayers: " << supportedLimits.limits.maxTextureArrayLayers << std::endl;
    }

    // Features
    std::vector<WGPUFeatureName> features;

    const size_t featureCount = wgpuAdapterEnumerateFeatures(adapter, nullptr);
    features.resize(featureCount);
    wgpuAdapterEnumerateFeatures(adapter, features.data());

    std::cout << "Adapter features:" << std::endl;
    std::cout << std::hex; // Write integers as hexadecimal to ease comparison with webgpu.h literals
    for (auto f : features) {
        std::cout << " - 0x" << f << std::endl;
    }
    std::cout << std::dec; // Restore decimal numbers

    // Properites

    WGPUAdapterProperties properties = {};
    properties.nextInChain = nullptr;
    wgpuAdapterGetProperties(adapter, &properties);
    std::cout << "Adapter properties:" << std::endl;
    std::cout << " - vendorID: " << properties.vendorID << std::endl;
    if (properties.vendorName) {
        std::cout << " - vendorName: " << properties.vendorName << std::endl;
    }
    if (properties.architecture) {
        std::cout << " - architecture: " << properties.architecture << std::endl;
    }
    std::cout << " - deviceID: " << properties.deviceID << std::endl;
    if (properties.name) {
        std::cout << " - name: " << properties.name << std::endl;
    }
    if (properties.driverDescription) {
        std::cout << " - driverDescription: " << properties.driverDescription << std::endl;
    }
    std::cout << std::hex;
    std::cout << " - adapterType: 0x" << properties.adapterType << std::endl;
    std::cout << " - backendType: 0x" << properties.backendType << std::endl;
    std::cout << std::dec; // Restore decimal numbers
    
    wgpuAdapterRelease(adapter);
}