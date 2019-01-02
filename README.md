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
    - Kernel arguments set by name (not by idx)
		- Adding new arguments does not invalidate old argument indices
    - Supports conservative recompilation when preprocessor definitions change
		- Can turn off branches with #ifdefs to keep register pressure low
- Kernel binaries cached for a massive speedup
    - Special care is taken to support #includes on all platforms (default NVIDIA kernel cache does not)


## Usage

1. Add CLT as a git submodule.
2. Call clt::initialize() (or initialize OpenCL manually)
3. Create a kernel class that extends clt::Kernel:
	```c++
    class MyKernel : public clt::Kernel {
    public:
        MyKernel (void) : Kernel("kernel.cl", "mainFunc") {};
        std::string getAdditionalBuildOptions() override {
            return " -DCONFIG_OPTION=" + std::to_string(global_variable);
        };
        void setArgs() override {
            setArg("input", inputArr);
            setArg("output", outputArr);
        }
    };
    ```
4. Call mykernel.build(), call rebuild() whenever configuration has changed

Check [example/](example/) for more information.

## License

See the [LICENSE](./LICENSE.md) file for license rights and limitations (MIT).
