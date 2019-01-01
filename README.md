CLT - An OpenCL Toolkit
====================

CLT is a toolkit that makes managing large-scale OpenCL codebases easier. In particular, CLT does the following:

## Features
- Context creation:
	- Platform and device selection by name
	- GL-CL context sharing setup
	- CPU debugging setup (Intel processors)
- Custom kernel class with several convenience features
	- Kernels implemented as classes
		- All initialization exists in one place
    - Kernel arguments set by name (not by idx)
		- Adding new arguments does not invalidate old argument indices
    - Supports conservative recompilation when preprocessor definitions change
		- Can turn off branches with #ifdefs to keep register pressure low
    - Kernel binaries cached for massive speedup


## Usage

1. Add CLT as a git submodule.
2. Create a kernel class that extends clt::Kernel.
	- Define kernel arguments
	- Define preprocessor definitions
3. Call clt::initialize() (or initialize OpenCL manually)

Check examples/ for more information.

## License

See the [LICENSE](./LICENSE.md) file for license rights and limitations (MIT).