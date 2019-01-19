#include "utils.hpp"
#include "xxhash/xxhash.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include "Kernel.hpp"

#if defined(__APPLE__)
#include <OpenCL/cl_gl_ext.h>
#include <OpenGL/OpenGL.h>
#elif defined(__linux__)
#include <GL/glxew.h>
#elif defined(_WIN32)
#define NOMINMAX
#include <Windows.h>
#endif

namespace clt {

void check(int err, const std::string msg)
{
    if (err != CL_SUCCESS)
    {
        std::cout << msg << " (" << getCLErrorString(err) << ")" << std::endl;
        throw std::runtime_error(msg);
    }
}

bool platformIsNvidia(cl::Platform& platform)
{
    std::string name = platform.getInfo<CL_PLATFORM_NAME>();
    return name.find("NVIDIA") != std::string::npos;
}

std::string getAbsolutePath(std::string filename)
{
    const int MAX_LENTH = 4096;
    char resolved_path[MAX_LENTH];
#ifdef _WIN32
    _fullpath(resolved_path, filename.c_str(), MAX_LENTH);
#else
    realpath(filename.c_str(), resolved_path);
#endif
    return std::string(resolved_path);
}

bool endsWith(const std::string s, const std::string end)
{
    size_t len = end.size();
    if (len > s.size()) return false;

    std::string substr = s.substr(s.size() - len, len);
    return end == substr;
}

std::string unixifyPath(std::string path)
{
    size_t index = 0;
    while (true)
    {
        index = path.find("\\", index);
        if (index == std::string::npos) break;

        path.replace(index, 1, "/");
        index += 1;
    }

    return path;
}

// Inline kernels are saved to disk
std::string createTempKernelFile(const std::string source, const std::string entryPoint)
{
    const std::string targetPath = entryPoint + "_inline.cl";
    std::ofstream f(targetPath);
    if (!f)
    {
        std::cout << "Could not create tmp file for inlined kernel " << entryPoint << std::endl;
        waitExit();
    }

    f.write(source.c_str(), source.length());
    return targetPath;
}

std::string getFileName(const std::string path)
{
    const std::string upath = unixifyPath(path);
    size_t idx = path.find_last_of("/") + 1;
    return upath.substr(idx, upath.length() - idx);
}

size_t computeHash(const void* buffer, size_t length)
{
    const size_t seed = 0;
#ifdef ENVIRONMENT64
    size_t const hash = XXH64(buffer, length, seed);
#else
    size_t const hash = XXH32(buffer, length, seed);
#endif
    return hash;
}

size_t fileHash(const std::string filename)
{
    std::ifstream f(filename, std::ios::binary | std::ios::ate);

    if (!f)
    {
        std::cout << "Could not open file " << filename << " for hashing, exiting..." << std::endl;
        waitExit();
    }

    std::ifstream::pos_type pos = f.tellg();
    std::vector<char> data((unsigned int)pos);
    f.seekg(0, std::ios::beg);
    f.read(&data[0], pos);
    f.close();

    return computeHash((const void*)data.data(), (size_t)pos);
}

cl::Platform& getPlatformByName(std::vector<cl::Platform> &platforms, std::string name) {
    for (cl::Platform &p : platforms) {
        std::string platformName = p.getInfo<CL_PLATFORM_NAME>();
        if (platformName.find(name) != std::string::npos) {
            return p;
        }
    }

    std::cout << "No platform name containing \"" << name << "\" found!" << std::endl;
    return platforms[0];
}

cl::Device& getDeviceByName(std::vector<cl::Device> &devices, std::string name) {
    for (cl::Device &d : devices) {
        std::string deviceName = d.getInfo<CL_DEVICE_NAME>();
        if (deviceName.find(name) != std::string::npos) {
            return d;
        }
    }

    std::cout << "No device name containing \"" << name << "\" in selected context!" << std::endl;
    return devices[0];
}

// Print the devices, C++ style
void printDevices()
{
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);

    int platform_id = 0;
    for (cl::Platform &platform : platforms)
    {
        std::cout << "Platform " << platform_id++ << ": " << platform.getInfo<CL_PLATFORM_NAME>() << std::endl;

        std::vector<cl::Device> devices;
        platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);

        int device_id = 0;
        for (cl::Device &device : devices)
        {
            auto extensions = device.getInfo<CL_DEVICE_EXTENSIONS>();
            bool hasGLSharing = extensions.find("cl_APPLE_gl_sharing") != std::string::npos
                || extensions.find("cl_khr_gl_sharing") != std::string::npos;

            const cl_device_type type = device.getInfo<CL_DEVICE_TYPE>();
            const std::string typeDesc = (type == CL_DEVICE_TYPE_GPU) ? "GPU"
                : (type == CL_DEVICE_TYPE_CPU) ? "CPU"
                : (type == CL_DEVICE_TYPE_ACCELERATOR) ? "ACCELERATOR"
                : (type == CL_DEVICE_TYPE_CUSTOM) ? "CUSTOM"
                : "UNKNOWN";

            std::cout << "Device " << device_id++ << ": " << std::endl;
            std::cout << std::left << std::setw(27) << "\tName: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;
            std::cout << std::left << std::setw(27) << "\tType: " << typeDesc << std::endl;
            std::cout << std::left << std::setw(27) << "\tVendor: " << device.getInfo<CL_DEVICE_VENDOR>() << std::endl;
            std::cout << std::left << std::setw(27) << "\tCompute Units: " << device.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() << std::endl;
            std::cout << std::left << std::setw(27) << "\tGlobal Memory: " << device.getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>() / 1000000 <<  " MB" << std::endl;
            std::cout << std::left << std::setw(27) << "\tMax Clock Frequency: " << device.getInfo<CL_DEVICE_MAX_CLOCK_FREQUENCY>() << " MHz" << std::endl;
            std::cout << std::left << std::setw(27) << "\tMax Allocation Size: " << device.getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>() / 1000000 << " MB" << std::endl;
            std::cout << std::left << std::setw(27) << "\tLocal Memory: " << device.getInfo<CL_DEVICE_LOCAL_MEM_SIZE>() / 1000 << " kB" << std::endl;
            std::cout << std::left << std::setw(27) << "\tCL-GL interop: " << (int)hasGLSharing << std::endl;
            std::cout << std::left << std::setw(27) << "\tAvailable: " << device.getInfo< CL_DEVICE_AVAILABLE>() << std::endl;
        }
        std::cout << std::endl;
    }
}

void setKernelCacheDir(const std::string path)
{
    Kernel::setCacheDir(path);
}

void setGlobalBuildOptions(const std::string opts)
{
    Kernel::setBuildOptions(opts);
}

void setCpuDebug(bool v)
{
    Kernel::setCpuDebug(v);
}

State initialize(const std::string& platformName, const std::string& deviceName)
{
    State state;
    int err = 0;
    
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);

    if (platforms.size() == 0) {
        std::cout << "No OpenCL platforms found. Please install an OpenCL SDK" << std::endl;
        exit(-1);
    }

    state.platform = getPlatformByName(platforms, platformName);
    std::cout << "PLATFORM: " << state.platform.getInfo<CL_PLATFORM_NAME>() << std::endl;

    std::vector<cl::Device> devices;
    state.platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);

    if (devices.size() == 0) {
        std::cout << "No device found that matches the given criteria" << std::endl;
        exit(-1);
    }

    // Select correct device
    state.device = getDeviceByName(devices, deviceName);
    std::cout << "DEVICE: " << state.device.getInfo<CL_DEVICE_NAME>() << std::endl;

    // Restrict context to selected device
    devices = { state.device };

    // Check if GL-CL sharing is available
    auto extensions = state.device.getInfo<CL_DEVICE_EXTENSIONS>();
    bool hasGLSharing = extensions.find("cl_APPLE_gl_sharing") != std::string::npos
            || extensions.find("cl_khr_gl_sharing") != std::string::npos;

#ifdef CLT_HAS_GL
    if (hasGLSharing)
    {
        auto getGLContext = []()
        {
        #ifdef __APPLE__
            return CGLGetCurrentContext();
        #elif defined(__linux__)
            return glXGetCurrentContext();
        #elif defined(_WIN32)
            return wglGetCurrentContext();
        #else
            return nullptr;
        #endif
        };

        auto glCtx = getGLContext();
        if (!glCtx)
        {
            std::cout << "OpenGL has not been initialized, cannot create CL-GL shared context" << std::endl;
            state.context = cl::Context(devices, NULL, NULL, NULL, &err);
        }
        else
        {
        #ifdef __APPLE__
            CGLContextObj kCGLContext = glCtx;
            CGLShareGroupObj kCGLShareGroup = CGLGetShareGroup(kCGLContext);
            cl_context_properties props[] =
            {
                CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE,
                    (cl_context_properties)kCGLShareGroup, 0
            };
        #else
            cl_context_properties props[] = {
                CL_CONTEXT_PLATFORM, (cl_context_properties)state.platform(),
            #if defined(__linux__)
                CL_GL_CONTEXT_KHR, (cl_context_properties)glCtx,
                CL_GLX_DISPLAY_KHR, (cl_context_properties)glXGetCurrentDisplay(),
            #elif defined(_WIN32)
                CL_GL_CONTEXT_KHR, (cl_context_properties)glCtx,
                CL_WGL_HDC_KHR, (cl_context_properties)wglGetCurrentDC(),
            #endif
                0
            };

            std::cout << "Creating GL-CL context" << std::endl;
            state.context = cl::Context(devices, props, NULL, NULL, &err);
            state.hasGLInterop = true;
        #endif
        }
    }
    else {
        std::cout << "Creating CL only context" << std::endl;
        state.context = cl::Context(devices, NULL, NULL, NULL, &err);
    }
#else
    if (hasGLSharing)
        std::cout << "CLT not built with OpenGL support, cannot create shared context" << std::endl;
    state.context = cl::Context(devices, NULL, NULL, NULL, &err);
#endif

    check(err, "Failed to create context");

    // Create command queue for context
    state.cmdQueue = cl::CommandQueue(state.context, state.device, CL_QUEUE_PROFILING_ENABLE, &err);
    check(err, "Failed to create command queue");

    return state;
}

std::string getCLErrorString(int code)
{
    const int SIZE = 64;
    std::string errors[SIZE] =
    {
        "CL_SUCCESS", "CL_DEVICE_NOT_FOUND", "CL_DEVICE_NOT_AVAILABLE",
        "CL_COMPILER_NOT_AVAILABLE", "CL_MEM_OBJECT_ALLOCATION_FAILURE",
        "CL_OUT_OF_RESOURCES", "CL_OUT_OF_HOST_MEMORY",
        "CL_PROFILING_INFO_NOT_AVAILABLE", "CL_MEM_COPY_OVERLAP",
        "CL_IMAGE_FORMAT_MISMATCH", "CL_IMAGE_FORMAT_NOT_SUPPORTED",
        "CL_BUILD_PROGRAM_FAILURE", "CL_MAP_FAILURE",
        "CL_MISALIGNED_SUB_BUFFER_OFFSET", "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST",
        "CL_COMPILE_PROGRAM_FAILURE", "CL_LINKER_NOT_AVAILABLE", "CL_LINK_PROGRAM_FAILURE",
        "CL_DEVICE_PARTITION_FAILED", "CL_KERNEL_ARG_INFO_NOT_AVAILABLE",
        "-20", "-21", "-22", "-23", "-24", "-25", "-26", "-27", "-28", "-29",
        "CL_INVALID_VALUE", "CL_INVALID_DEVICE_TYPE", "CL_INVALID_PLATFORM",
        "CL_INVALID_DEVICE", "CL_INVALID_CONTEXT", "CL_INVALID_QUEUE_PROPERTIES",
        "CL_INVALID_COMMAND_QUEUE", "CL_INVALID_HOST_PTR", "CL_INVALID_MEM_OBJECT",
        "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR", "CL_INVALID_IMAGE_SIZE",
        "CL_INVALID_SAMPLER", "CL_INVALID_BINARY", "CL_INVALID_BUILD_OPTIONS",
        "CL_INVALID_PROGRAM", "CL_INVALID_PROGRAM_EXECUTABLE",
        "CL_INVALID_KERNEL_NAME", "CL_INVALID_KERNEL_DEFINITION", "CL_INVALID_KERNEL",
        "CL_INVALID_ARG_INDEX", "CL_INVALID_ARG_VALUE", "CL_INVALID_ARG_SIZE",
        "CL_INVALID_KERNEL_ARGS", "CL_INVALID_WORK_DIMENSION",
        "CL_INVALID_WORK_GROUP_SIZE", "CL_INVALID_WORK_ITEM_SIZE",
        "CL_INVALID_GLOBAL_OFFSET", "CL_INVALID_EVENT_WAIT_LIST", "CL_INVALID_EVENT",
        "CL_INVALID_OPERATION", "CL_INVALID_GL_OBJECT", "CL_INVALID_BUFFER_SIZE",
        "CL_INVALID_MIP_LEVEL", "CL_INVALID_GLOBAL_WORK_SIZE"
    };

    const int ind = -code;
    return (ind >= 0 && ind < SIZE) ? errors[ind] : "unknown";
}


}
