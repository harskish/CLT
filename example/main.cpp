#include "clt.hpp"
#include <numeric>

// Export cl2 include dir from CLT?

cl::Buffer input;
cl::Buffer output;
const cl_uint N = 5;
cl_uint P = 1;

class TestKernel : public clt::Kernel {
public:
    TestKernel(void) : Kernel("device.cl", "power") {};
    std::string getAdditionalBuildOptions() override {
        std::string opts;        
        opts += " -DN=" + std::to_string(N);
        opts += " -DP=" + std::to_string(P);
        return opts;
    }
    void setArgs() override {
        setArg("input", input);
        setArg("output", output);
    }
    /*
    CLT_KERNEL_IMPL(
    kernel void power(global int* input, global int* output) {
        uint gid = get_global_id(0);
        if (gid >= N)
            return;
    
        output[gid] = gid;
    })
    */
};

int main(int argc, char* argv[]) {
    clt::printDevices();
    clt::State state = clt::initialize("Intel", "i7");

    // Configure CLT
    clt::setKernelCacheDir("./kernel_cache");
    clt::setGlobalBuildOptions("-DTEST=1");
    clt::setCpuDebug(false);
    
    cl_uint indata[N];
    std::iota(indata, indata + N, 1); // fill with 1 ... N

    int err = 0;
    input = cl::Buffer(state.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, N * sizeof(cl_uint), indata, &err);
    clt::check(err, "Input buffer creation failed");

    output = cl::Buffer(state.context, CL_MEM_WRITE_ONLY, N * sizeof(cl_uint), nullptr, &err);
    clt::check(err, "Output buffer creation failed");

    TestKernel kernel;
    kernel.build(state.context, state.device, state.platform);

    for (int i = 1; i <= 5; i++) {
        P = (cl_uint)i;
        kernel.rebuild(false); // P has changed

        err = state.cmdQueue.enqueueNDRangeKernel(kernel, cl::NDRange(0), cl::NDRange(N));
        clt::check(err, "Failed to enqueue kernel");

        err = state.cmdQueue.finish();
        clt::check(err, "Could not finish command queue");

        err = state.cmdQueue.enqueueReadBuffer(output, CL_TRUE, 0, N * sizeof(cl_uint), indata);
        clt::check(err, "Could not read from output buffer");

        for (int i = 0; i < N; i++)
            std::cout << (i + 1) << "^" << P << " = " << indata[i] << std::endl;
    }

    return 0;
}
