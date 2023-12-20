[![License](https://img.shields.io/badge/license-MIT-blue.svg)](https://opensource.org/licenses/MIT)	
[![C++ CI](https://github.com/thecppzoo/zoo/actions/workflows/master.yaml/badge.svg)](https://github.com/thecppzoo/zoo/actions/workflows/master.yaml)

## Build suggestion

This library is header-only, written in portable C++ 17.  MSVC is supported, but occasionally some bugs in MSVC take a long time to be identified, please report MSVC compilation issues.  We're working on including MSVC to continuous integration.

There are comprehensive examples in the Catch2 based tests, here's a simple way to build them:

1. Do not forget to initialize at least the Catch2 submodule (`git submodule update --init --recursive`)
2. Create a build directory (we suggest ~/builds)
3. Create a sub-dir of that directory per branch to build in (this maintains build isolation and avoids polluting your checked out repository)
4. Execute `cmake -g "Unix Makefiles" ~/<path to repository root of your checkout>/test` to generate makefiles.
5. Use `make -j16` (for some parallelism in building) to build testing binaries.

If you understand GitHub actions, see our "actions configuration" file, [`.github/workflows/master.yaml`](https://github.com/thecppzoo/zoo/blob/master/.github/workflows/master.yaml)

`cmake -G Xcode` is supported, however, only the target `zooTestDebugTest` is debuggable within XCode, this is the target with all tests, the individual sections use "object libraries" that XCode is not supporting.

# zoo

## CPPCon 2022:
March 1: We're working on a release of the code this week (March 3): A complete implementation that may not be production-ready.  Thanks for the interest.

Please contact us for information about the work we are doing for C++ Now 2023, we are working on it privately

## Library of components.

See the Catch2 based tests in
`/test/`
