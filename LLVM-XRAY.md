# LLVM XRay Support for Spike

This document describes how to build Spike with LLVM XRay instrumentation support.

## What is LLVM XRay?

LLVM XRay is a function call tracing system that allows you to trace function entry and exit points in your programs with minimal overhead. It's particularly useful for performance analysis and debugging.

## Building Spike with XRay Support

To enable LLVM XRay instrumentation when building Spike, use the `--enable-llvm-xray` configure option:

```bash
# Configure with XRay support (requires clang compiler)
./configure --enable-llvm-xray CC=clang CXX=clang++

# Build
make

# Install
make install
```

### Prerequisites

- LLVM/Clang compiler (GCC does not support XRay)
- LLVM XRay tools for analyzing traces

### Configuration Options

When `--enable-llvm-xray` is enabled, the following compiler flags are added:

- `-fxray-instrument`: Enables XRay instrumentation
- `-fxray-instruction-threshold=1`: Instruments even small functions (optional, automatically added if supported)

## Using XRay with Spike

Once Spike is built with XRay support, you can collect and analyze function traces:

```bash
# Run Spike with XRay tracing enabled
XRAY_OPTIONS="patch_premain=true xray_mode=xray-basic" ./spike [args...]

# This will generate an xray log file (e.g., xray-log.spike.*)

# Extract the trace
llvm-xray extract xray-log.spike.*

# Convert to human-readable format
llvm-xray convert --output-format=yaml xray-log.spike.* -o trace.yaml

# Or generate a flame graph
llvm-xray stack xray-log.spike.* -o stack.txt
```

## Performance Impact

XRay is designed to have minimal overhead when tracing is not enabled. The instrumentation points are implemented as NOP sleds that can be patched at runtime.

- Without tracing enabled: Minimal overhead (a few NOP instructions per function)
- With tracing enabled: Small overhead for each traced function call

## Example Use Cases

1. **Performance profiling**: Identify hot paths and bottlenecks in simulation code
2. **Call trace analysis**: Understand the execution flow through Spike's codebase
3. **Debugging**: Track function call sequences for debugging complex issues

## Testing

To verify that the XRay configuration option is working correctly, run:

```bash
./test-xray-config.sh
```

## References

- [LLVM XRay Documentation](https://llvm.org/docs/XRay.html)
- [XRay Tutorial](https://llvm.org/docs/XRayExample.html)
