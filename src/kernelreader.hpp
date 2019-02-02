#pragma once

#include <vector>
#include <string>
#include "../include/cl_header.hpp"

namespace clt {

void kernelFromSource(const std::string filename, cl::Context &context, cl::Program &program, int &err);
void kernelFromSourceExpanded(const std::string filename, cl::Context &context, cl::Program &program, int &err);
void kernelFromBinary(const std::string filename, cl::Context &context, cl::Device &device, cl::Program &program, int &err);
cl::Program kernelFromFile(const std::string filename, const std::string buildOpts, const std::string cacheDir, cl::Platform &platform, cl::Context &context, cl::Device &device, int &err);

std::string readKernel(std::string path, std::vector<std::string> &incl);
std::string readKernel(std::string path);

} // end namespace clt
