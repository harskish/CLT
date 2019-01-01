#pragma once

#include <string>
#include <stdlib.h>
#include <vector>
#include <CL/cl2.hpp>

namespace clt {

// Determine target
#if _WIN64
#define ENVIRONMENT64
#elif _WIN32
#define ENVIRONMENT32
#endif

#if __GNUC__
#if __x86_64__ || __ppc64__
#define ENVIRONMENT64
#else
#define ENVIRONMENT32
#endif
#endif

inline void waitExit()
{
#ifdef WIN32
    system("pause");
#endif
    exit(-1);
}

void check(int err, const char* msg);

bool platformIsNvidia(cl::Platform& platform);

std::string getCLErrorString(int code);
std::string getAbsolutePath(std::string filename);
std::string getFileName(const std::string path);

void printDevices();
void setKernelCacheDir(const std::string path);
void setGlobalBuildOptions(const std::string opts);
void setCpuDebug(bool v);

bool endsWith(const std::string s, const std::string end);
std::string unixifyPath(std::string path);

size_t computeHash(const void* buffer, size_t length);
size_t fileHash(const std::string filename);

typedef struct {
    cl::Platform platform;
    cl::Device device;
    cl::Context context;
    cl::CommandQueue cmdQueue;
} State;

// Does OpenCL initialization
State initialize(const std::string& platformName, const std::string& deviceName);

}
