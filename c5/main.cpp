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

void inspectDevice(WGPUDevice device) {
    std::vector<WGPUFeatureName> features;
    //WGPUFeatureName featureName = WGPUFeatureName_TextureCompressionASTC;
    size_t featureCount = wgpuDeviceEnumerateFeatures(device, nullptr);
    wgpuDeviceEnumerateFeatures(device, features.data());


    std::cout << "has F16 feature " << wgpuDeviceHasFeature(device, WGPUFeatureName_ShaderF16) << std::endl;

    std::cout << "Device features:" << std::endl;
    std::cout << " Feature count: " << featureCount << std::endl;
    std::cout << std::hex;
    for (auto f : features) {
        std::cout << " - 0x" << f << std::endl;
    }
    std::cout << std::dec;

    WGPUSupportedLimits limits = {};
    limits.nextInChain = nullptr;

#ifdef WEBGPU_BACKEND_DAWN
    bool success = wgpuDeviceGetLimits(device, &limits) == WGPUStatus_Success;
#else
    bool success = wgpuDeviceGetLimits(device, &limits);
#endif

    if (success) {
        std::cout << "Device limits:" << std::endl;
        std::cout << " - maxTextureDimension1D: " << limits.limits.maxTextureDimension1D << std::endl;
        std::cout << " - maxTextureDimension2D: " << limits.limits.maxTextureDimension2D << std::endl;
        std::cout << " - maxTextureDimension3D: " << limits.limits.maxTextureDimension3D << std::endl;
        std::cout << " - maxTextureArrayLayers: " << limits.limits.maxTextureArrayLayers << std::endl;
        // [...] Extra device limits
    }
}

int main() {
    WGPUInstanceDescriptor desc = {};
    desc.nextInChain = nullptr;
    WGPUInstance instance = wgpuCreateInstance(&desc);

    if (!instance) {
        std::cout << "falied to create wgpu instance" << std::endl;
        return 1;
    }

    
    WGPURequestAdapterOptions options = {};
    options.nextInChain = nullptr;
    options.powerPreference = WGPUPowerPreference_HighPerformance;
    WGPUAdapter adapter = requestAdapterSync(instance, &options);
   
    std::vector<WGPUFeatureName> features;
    features.emplace_back(WGPUFeatureName_ShaderF16); // sample feature name
    WGPUDeviceDescriptor deviceDesc = {};
    deviceDesc.nextInChain = nullptr;
    deviceDesc.label = "GPU"; // anything works here, that's your call
    deviceDesc.requiredFeatureCount = 1; // we do not require any specific feature
    deviceDesc.requiredFeatures = features.data();
    deviceDesc.requiredLimits = nullptr; // we do not require any specific limit
    deviceDesc.defaultQueue.nextInChain = nullptr;
    deviceDesc.defaultQueue.label = "The default queue";
    deviceDesc.deviceLostCallback = [](WGPUDeviceLostReason reason, char const* message, void* /* pUserData */) {
        std::cout << "Device lost: reason " << reason;
        if (message) std::cout << " (" << message << ")";
        std::cout << std::endl;
    };

    WGPUDevice device = requestDeviceSync(adapter, &deviceDesc);


    inspectDevice(device);
    wgpuInstanceRelease(instance);
    wgpuAdapterRelease(adapter);
    wgpuDeviceRelease(device);
}