#include "Kernel.hpp"
#include "kernelreader.hpp"
#include <iostream>
#include <cassert>
#include "utils.hpp"

namespace clt {

std::string Kernel::globalBuildOpts = "";
std::string Kernel::cacheDir = "cache/kernel_binaries";
bool Kernel::CPU_DEBUG = false;
void* Kernel::userPtr = nullptr;

void Kernel::build(cl::Context& context, cl::Device& device, cl::Platform& platform, bool setArgs)
{
    // No need to recompile, just update arguments
    if (m_kernel() && !configHasChanged())
    {
        if (setArgs)
            this->setArgs();
        return;
    }

    const std::string filename = getFileName(m_sourcePath);

    this->context = &context;
    this->device = &device;
    this->platform = &platform;

    if (m_kernel())
        std::cout << "Rebuilding kernel " << filename << std::endl;

    // Define build options based on global + specialized options
    std::string buildOpts = globalBuildOpts + getAdditionalBuildOptions();
    buildOpts += " -cl-kernel-arg-info";
    if (Kernel::CPU_DEBUG)
        buildOpts += " -g -s \"" + getAbsolutePath(m_sourcePath) + "\"";
    this->lastBuildOpts = buildOpts;
    cl::Program program;

    // CPU debugging segfaults if trying to use cached kernel!
    // Also need to let the driver do the include handling
    int err = 0;
    if (Kernel::CPU_DEBUG)
    {
        kernelFromSource(m_sourcePath, context, program, err);
        cl::vector<cl::Device> devices = { device };
        err = program.build(devices, buildOpts.c_str());

        // Check build log
        std::string buildLog = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device);
        if (buildLog.length() > 2)
            std::cout << "\n[" << m_sourcePath << " build log]:" << buildLog << std::endl;

        check(err, "Kernel compilation failed");
    }
    else
    {
        // Build program using cache or sources
        program = kernelFromFile(m_sourcePath, buildOpts, Kernel::cacheDir, platform, context, device, err);
        check(err, "Failed to create kernel program");
    }

    // Creating compute kernel from program
    m_kernel = cl::Kernel(program, m_entryPoint.c_str(), &err);
    check(err, "Failed to create compute kernel!");

    // Get kernel argument names
    // NB: kernels built from binaries SHOULD NOT have arg info, but they do at least on Intel/NV!
    argMap.clear();
    cl_uint numArgs = m_kernel.getInfo<CL_KERNEL_NUM_ARGS>(&err);
    check(err, "Getting KERNEL_NUM_ARGS failed for " + filename);
    for (cl_uint i = 0; i < numArgs; i++)
    {
        auto argname = m_kernel.getArgInfo<CL_KERNEL_ARG_NAME>(i, &err);
        check(err, "Getting CL_KERNEL_ARG_NAME failed for " + filename);
        argMap[argname] = i; // save to mapping
    }

    // Set default arguments
    this->setArgs();
}

void Kernel::rebuild(bool setArgs)
{
    build(*context, *device, *platform, setArgs);
}

bool Kernel::configHasChanged()
{
    std::string buildOpts = globalBuildOpts + getAdditionalBuildOptions();
    buildOpts += " -cl-kernel-arg-info";
    if (Kernel::CPU_DEBUG)
        buildOpts += " -g -s \"" + getAbsolutePath(m_sourcePath) + "\"";
    return (buildOpts.compare(lastBuildOpts) != 0);
}


} // end namespace clt
