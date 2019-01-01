#include "clt.hpp"
#include <numeric>

// Export cl2 include dir from CLT?

cl::Buffer input;
cl::Buffer output;
cl_uint N = 0;

class TestKernel : public clt::Kernel {
public:
    TestKernel(void) : Kernel("device.cl", "square") {};
    std::string getAdditionalBuildOptions() override {
        return " -DN=" + std::to_string(N);
    };
    void setArgs() override {
        setArg("input", input);
        setArg("output", output);
    }
};

int main(int argc, char* argv[]) {
    clt::printDevices();
    clt::State state = clt::initialize("Intel", "i7");

    // Configure CLT
    clt::setKernelCacheDir("./kernel_cache");
    clt::setGlobalBuildOptions("-DTEST=1");
    clt::setCpuDebug(false);

    std::cout << "Enter array length: ";
    std::cin >> N;
    N = std::max(1U, std::min(1000U, N));
    
    cl_uint* indata = new cl_uint[N];
    std::iota(indata, indata + N, 0); // fill with 0 ... N-1

    int err = 0;
    input = cl::Buffer(state.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, N * sizeof(cl_uint), indata, &err);
    clt::check(err, "Input buffer creation failed");

    output = cl::Buffer(state.context, CL_MEM_WRITE_ONLY, N * sizeof(cl_uint), nullptr, &err);
    clt::check(err, "Output buffer creation failed");

    TestKernel kernel;
    kernel.build(state.context, state.device, state.platform);

    err = state.cmdQueue.enqueueNDRangeKernel(kernel, cl::NDRange(0), cl::NDRange(N));
    clt::check(err, "Failed to enqueue kernel");

    err = state.cmdQueue.finish();
    clt::check(err, "Could not finish command queue");

    err = state.cmdQueue.enqueueReadBuffer(output, CL_TRUE, 0, N * sizeof(cl_uint), indata);
    clt::check(err, "Could not read from output buffer");

    std::cout << "Output: " << std::endl;
    for (int i = 0; i < N; i++)
        std::cout << i << "^2 = " << indata[i] << std::endl;

    delete[] indata;
    return 0;
}