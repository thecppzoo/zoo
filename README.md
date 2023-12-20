[![License](https://img.shields.io/badge/license-MIT-blue.svg)](https://opensource.org/licenses/MIT)	
[![C++ CI](https://github.com/thecppzoo/zoo/actions/workflows/master.yaml/badge.svg)](https://github.com/thecppzoo/zoo/actions/workflows/master.yaml)

## Build suggestion

Create a build directory (we suggest ~/builds)
Create a sub-dir of that directory per branch to build in (this maintains build isolation and avoids polluting your checked out repository)
Execute `cmake -g "Unix Makefiles" ~/<path to test>` to generate makefiles.
Use `make -j16` (for some parallelism in building) to build testing binaries.

If you understand GitHub actions, see our "actions configuration" file, [`.github/workflows/master.yaml`](https://github.com/thecppzoo/zoo/blob/master/.github/workflows/master.yaml)

`cmake -G Xcode` is supported, however, only the target `zooTestDebugTest` is debuggable within XCode, this is the target with all tests, the individual sections use "object libraries" that XCode is not supporting.

# zoo

## CPPCon 2022:
March 1: We're working on a release of the code this week (March 3): A complete implementation that may not be production-ready.  Thanks for the interest.

Please contact us for information about the work we are doing for C++ Now 2023, we are working on it privately

## Library of components.

See the Catch2 based tests in
`/test/`
