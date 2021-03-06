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

    // Inlined kernels are saved to temporary files
    if (isInlined())
        m_sourcePath = createTempKernelFile(getSource(), m_entryPoint);

    const std::string filename = getFileName(m_sourcePath);

    this->context = &context;
    this->device = &device;
    this->platform = &platform;
    this->deviceIsCPU = (device.getInfo<CL_DEVICE_TYPE>() == CL_DEVICE_TYPE_CPU);

    if (m_kernel())
        std::cout << "Rebuilding kernel " << filename << std::endl;

    // Define build options based on global + specialized options
    std::string buildOpts = globalBuildOpts + getAdditionalBuildOptions();
    buildOpts += " -cl-kernel-arg-info";
    if (Kernel::CPU_DEBUG && deviceIsCPU)
        buildOpts += " -g -s \"" + getAbsolutePath(m_sourcePath) + "\"";
    this->lastBuildOpts = buildOpts;
    cl::Program program;

    // CPU debugging segfaults if trying to use cached kernel!
    // Also need to let the driver do the include handling
    int err = 0;
    if (Kernel::CPU_DEBUG)
    {
        kernelFromSource(m_sourcePath, context, program, err);
        std::vector<cl::Device> devices = { device };
        CLT_CALL(err = program.build(devices, buildOpts.c_str()), err);
        
        // Check build log
        m_buildLog = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device);
        if (m_buildLog.length() > 2)
            std::cout << "\n[" << m_sourcePath << " build log]:" << m_buildLog << std::endl;

        check(err, "Kernel compilation failed");
    }
    else
    {
        // Build program using cache or sources
        CLT_CALL(program = kernelFromFile(m_sourcePath, buildOpts, Kernel::cacheDir, platform, context, device, err), err);
        check(err, "Failed to create kernel program");
        CLT_CALL(m_buildLog = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device, &err), err);
        check(err, "Failed to get program build log");
    }

    // Creating compute kernel from program
    CLT_CALL(m_kernel = cl::Kernel(program, m_entryPoint.c_str(), &err), err);
    check(err, "Failed to create compute kernel!");

    // Get kernel argument names
    // NB: kernels built from binaries SHOULD NOT have arg info, but they do at least on Intel/NV!
    argMap.clear();
    cl_uint numArgs;
    CLT_CALL(numArgs = m_kernel.getInfo<CL_KERNEL_NUM_ARGS>(&err), err);
    check(err, "Getting KERNEL_NUM_ARGS failed for " + filename);
    
    // Copied into temp buffer because cl.hpp seems to produce invalid strings somehow
    // Not an issue when using cl2.hpp, but need to support old header for compatibility
    char buffer[128];
    for (cl_uint i = 0; i < numArgs; i++)
    {
        std::string argname;
        CLT_CALL(argname = m_kernel.getArgInfo<CL_KERNEL_ARG_NAME>(i, &err), err);
        check(err, "Getting CL_KERNEL_ARG_NAME failed for " + filename);
        snprintf(buffer, sizeof(buffer), "%s", argname.c_str());
        argMap[buffer] = i; // save to mapping
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
    if (Kernel::CPU_DEBUG && deviceIsCPU)
        buildOpts += " -g -s \"" + getAbsolutePath(m_sourcePath) + "\"";
    return (buildOpts.compare(lastBuildOpts) != 0);
}


} // end namespace clt
