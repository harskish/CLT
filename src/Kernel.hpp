#pragma once

#include <CL/cl2.hpp>
#include <string>
#include <iostream>
#include <map>

namespace clt {

class Kernel
{
public:
    Kernel(std::string srcPath, std::string entryPoint) : m_sourcePath(srcPath), m_entryPoint(entryPoint) {};
    ~Kernel(void) = default;

    explicit operator bool() const { return m_kernel() != nullptr; }
    operator cl::Kernel&() { return m_kernel; }

    // Build, but only if needed!
    void build(cl::Context& context, cl::Device& device, cl::Platform& platform, bool setArgs = true);
    void rebuild(bool setArgs);

    template <typename T>
    cl_int setArg(const std::string name, const T& value) {
        auto it = argMap.find(name);
        if (it == argMap.end())
        {
            std::cout << "Kernel " << m_sourcePath << " has no argument '" << name << "'" << std::endl;
            throw std::runtime_error("Unknown kernel argument " + name);
        }
        else
        {
            return m_kernel.setArg(it->second, value);
        }
    }

    bool hasArg(const std::string name) { return argMap.find(name) != argMap.end(); }

    // For accessing compilation settings and device buffers
    static void setUserPointer(void* p) { Kernel::userPtr = p; }
    static void setBuildOptions(std::string s) { Kernel::globalBuildOpts = s; }
    
    // Kernel cache directory
    static void setCacheDir(std::string s) { Kernel::cacheDir = s; }

    // Flag that enables CPU debugging on Intel processors
    static void setCpuDebug(bool v) { Kernel::CPU_DEBUG = v; }

private:
    // For checking if recompilation is necessary
    bool configHasChanged();
    
    // Cached for recompilation
    cl::Context* context;
    cl::Device* device;
    cl::Platform* platform;

    static std::string globalBuildOpts;
    static std::string cacheDir;
    static bool CPU_DEBUG;
    std::string m_sourcePath = ""; // path to kernel source file
    std::string m_entryPoint = ""; // name of main function in kernel
    cl::Kernel m_kernel;
    std::string lastBuildOpts; // for detecting need to recompile
    std::map<std::string, cl_uint> argMap;

protected:
    virtual std::string getAdditionalBuildOptions() { return ""; };
    virtual void setArgs() = 0;

    // Check CL return code
    void verify(int code, const std::string msg);

    static void* userPtr;
};

} // end namespace clt
