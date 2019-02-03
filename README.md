CLT - An OpenCL Toolkit
====================

CLT is a toolkit that makes managing large-scale OpenCL codebases easier.

## Features
- Context creation:
    - Platform and device selection by name
    - GL-CL interop setup
    - CPU debugging setup (Intel processors)
- Custom kernel class with several convenience features
    - Kernels implemented as classes
        - All setup exists in one place
        - No initialization step is accidentally forgotten
        - Kernel source can be inlined in class
    - Kernel arguments set by name (not by idx)
        - Adding new arguments does not invalidate old argument indices
    - Supports conservative recompilation when preprocessor definitions change
        - Can turn off branches with #ifdefs to keep register pressure low
- Kernel binaries cached for a massive speedup
    - Special care is taken to support #includes on all platforms (default NVIDIA kernel cache does not)


## Usage

1. Add CLT as a git submodule, include clt.hpp
2. Call clt::initialize() (or initialize OpenCL manually)
3. Create a kernel class that extends clt::Kernel:
    ```c++
    class MyKernel : public clt::Kernel {
    public:
        MyKernel (void) : Kernel("kernel.cl", "mainFunc") {};
        std::string getAdditionalBuildOptions() override {
            return " -DCONFIG_POWER=" + std::to_string(global_variable);
        }
        void setArgs() override {
            setArg("input", inputArr);
            setArg("output", outputArr);
        }
        CLT_KERNEL_IMPL(
        kernel void mainFunc(global int* input, global int* output) {
            // Optional inline source, can also be read from file
            uint gid = get_global_id(0);
            output[gid] = pow(input[gid], CONFIG_POWER);
        })
    };
    ```
4. Call mykernel.build()
5. Call mykernel.rebuild() whenever configuration changes (only recompiled if needed)

See [example/](example/) for a usage example.  
Check out [Fluctus][fluctus] to see CLT in use in a large-scale OpenCL codebase.

## Configuration

CLT can be configured to use cl.hpp instead of cl2.hpp (for compatibility with older projects).
This is done by adding `set(CLT_USE_LEGACY_HEADER ON CACHE BOOL " " FORCE)` and `add_definitions(-DCLT_CL_LEGACY_HEADER)` to `CMakeLists.txt`

## License

See the [LICENSE](./LICENSE.md) file for license rights and limitations (MIT).

[fluctus]: https://github.com/harskish/fluctus
