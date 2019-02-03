#include "kernelreader.hpp"
#include "utils.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <errno.h>
#if defined(_WIN32)
#include <direct.h>   // _mkdir
#endif


namespace clt {

void kernelFromSource(const std::string filename, cl::Context &context, cl::Program &program, int &err)
{
    std::ifstream f(filename);
    if (!f)
    {
        std::cout << "Could not open kernel file '" + filename + "'" << std::endl;
        waitExit();
    }

    std::stringstream buffer;
    buffer << f.rdbuf();

    const std::string &tmp = buffer.str();
    CLT_CALL(program = cl::Program(context, tmp, false, &err), err);
}

// Perform include expanding
void kernelFromSourceExpanded(const std::string filename, cl::Context & context, cl::Program & program, int & err)
{
    std::string expandedSrc = readKernel(filename);
    CLT_CALL(program = cl::Program(context, expandedSrc, false, &err), err);
}

void kernelFromBinary(const std::string filename, cl::Context & context, cl::Device & device, cl::Program & program, int & err)
{
    std::ifstream f(filename, std::ios::binary | std::ios::ate);
    std::cout << "Reding kernel binary " << filename << std::endl;

    if (!f)
    {
        std::cout << "Could not open kernel binary '" + filename + "'" << std::endl;
        waitExit();
    }

    std::ifstream::pos_type pos = f.tellg();
    std::vector<unsigned char> binary((unsigned int)pos);
    f.seekg(0, std::ios::beg);
    f.read((char*)(&binary[0]), pos);

#ifdef CLT_CL_LEGACY_HEADER
    cl::Program::Binaries binaries(1, std::make_pair(static_cast<const void*>(binary.data()), pos));
#else
    cl::Program::Binaries binaries = { binary }; // push_back
#endif

    std::vector<cl_int> status;
    std::vector<cl::Device> devices = { device };
    CLT_CALL(program = cl::Program(context, devices, binaries, &status, &err), err);

    // Check compilation status
    for (cl_int i : status) {
        err |= i;
    }
}

void verify(const char* msg, int err)
{
    if (err != CL_SUCCESS)
    {
        std::cout << "ERROR: " << msg << std::endl;
        waitExit();
    }
}
    
bool createPath(const std::string& inpath)
{
    auto makeDirFun = [](const std::string& p)
    {
#if defined(_WIN32)
        return _mkdir(p.c_str());
#else
        return mkdir(p.c_str(), 0755);
#endif
    };
    
    std::string path = unixifyPath(inpath);
    if(makeDirFun(path) == -1)
    {
        switch(errno)
        {
            case ENOENT:
                if (createPath(path.substr(0, path.find_last_of('/'))))
                    return (makeDirFun(path) == 0);
                else
                    return false;
            case EEXIST:
                return true;
            default:
                return false;
        }
    }
    
    return true;
}

// Create directory if it doesn't exist
void createDirectory(const std::string dir)
{
    if (!createPath(dir))
        std::cout << "Could not create kernel cache directory " << dir << std::endl;
}


// Checks kernel cache for match, otherwise loads from source
cl::Program kernelFromFile(const std::string path, const std::string buildOpts, const std::string cacheDir, cl::Platform & platform, cl::Context & context, cl::Device & device, int & err)
{
    std::string filename = getFileName(path);

    // Compute hash of kernel source + build configuration
    std::string kernelSource = readKernel(path);

    // Separate binaries by (build options X platform name X device name)
    kernelSource += buildOpts;
    kernelSource += platform.getInfo<CL_PLATFORM_NAME>();
    kernelSource += device.getInfo<CL_DEVICE_NAME>();

    // Check that binary directory exists
    createDirectory(cacheDir);

    size_t hash = computeHash(kernelSource.data(), kernelSource.size());
    std::string binaryPath = cacheDir + "/" + filename + "." + std::to_string(hash) + ".bin";

    cl::Program program;

    // Try to open cached kernel binary
    std::ifstream binaryFile(binaryPath, std::ios::binary | std::ios::ate);
    if (binaryFile)
    {
        std::cout << "Loading hashed kernel " << binaryPath << std::endl;

        std::ifstream::pos_type pos = binaryFile.tellg();
        std::vector<unsigned char> binary((unsigned int)pos);
        binaryFile.seekg(0, std::ios::beg);
        binaryFile.read((char*)(&binary[0]), pos);

#ifdef CLT_CL_LEGACY_HEADER
        cl::Program::Binaries binaries(1, std::make_pair(static_cast<const void*>(binary.data()), pos));
#else
        cl::Program::Binaries binaries = { binary }; // push_back?
#endif

        std::vector<cl_int> status;
        std::vector<cl::Device> devices = { device };
        CLT_CALL(program = cl::Program(context, devices, binaries, &status, &err), err);

        // Check program status
        for (cl_int i : status) err |= i;
        verify("Failed to create program from binary", err);

        // Build
        err = program.build(devices, buildOpts.c_str());
        verify("Failed to build program loaded from binary", err);
    }
    else
    {
        std::cout << "Building kernel " << filename << std::endl;

        kernelFromSourceExpanded(path, context, program, err);
        std::vector<cl::Device> devices = { device };
        CLT_CALL(err = program.build(devices, buildOpts.c_str()), err);

        // Check build log
        std::string buildLog;
        CLT_CALL(program.getBuildInfo(device, CL_PROGRAM_BUILD_LOG, &buildLog), err);
        if (buildLog.length() > 2)
            std::cout << "\n[" << filename << " build log]:" << buildLog << std::endl;

        verify("Kernel compilation failed", err);
        
        std::vector<size_t> sizes = program.getInfo<CL_PROGRAM_BINARY_SIZES>();
        verify("Incorrect number of kernel binaries generated!", sizes.size() != 1);

        // Open target file in overwrite-mode
        std::ofstream stream;
        stream.open(binaryPath, std::ofstream::out | std::ofstream::trunc | std::ofstream::binary);
        if (!stream.good())
        {
            std::cout << "Failed to open kernel binary output file" << std::endl;
            waitExit();
        }

#ifdef CLT_CL_LEGACY_HEADER
        std::vector<char*> ptxs = program.getInfo<CL_PROGRAM_BINARIES>();
        stream.write(ptxs[0], sizes[0]);
        delete[] ptxs[0];
#else
        std::vector<std::vector<unsigned char>> ptxs = program.getInfo<CL_PROGRAM_BINARIES>();
        stream.write((const char*)ptxs[0].data(), sizes[0]);
#endif

        // Check write
        if (!stream.good())
        {
            std::cout << "Failed to write kernel binary" << std::endl;
            waitExit();
        }
        
        stream.close();
        std::cout << "Created cached kernel " << binaryPath << std::endl;
    }

    return program;
}

std::string readKernel(std::string path)
{
    std::vector<std::string> incl;
    return readKernel(path, incl);
}

// Read kernel file, handle includes by recursion
// Used to enable kernel caching on NVIDIA hardware
std::string readKernel(std::string path, std::vector<std::string> &incl)
{
    path = unixifyPath(path);
    std::ifstream file(path);
    if (!file)
    {
        std::cout << "Cannot open file " << path << std::endl;
        waitExit();
    }

    // Don't include same file several times
    if (std::find(incl.begin(), incl.end(), path) != incl.end())
        return "";
    else
        incl.push_back(path);

    size_t idx = path.find_last_of('/');
    std::string dir = path.substr(0, idx);

    std::string output;
    std::string line;

    while (file.good())
    {
        getline(file, line);

        if (line.find("#include") == std::string::npos)
        {
            output.append(line + "\n");
        }
        else
        {
            std::string includeFileName = line.substr(10, line.length() - 11);
            if (endsWith(includeFileName, ".cl") || endsWith(includeFileName, ".h")) {
                std::string toAppend = readKernel(dir + "/" + includeFileName, incl);
                output.append(toAppend + "\n");
            }
        }
    }

    return output;
}

} // end namespace clt
