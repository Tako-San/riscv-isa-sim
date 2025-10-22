#!/bin/bash
# Test script to verify LLVM XRay configuration option

set -e

echo "=== Testing LLVM XRay Configuration Option ==="
echo ""

# Test 1: Check that --enable-llvm-xray option exists
echo "Test 1: Checking --enable-llvm-xray option exists in configure..."
if ./configure --help | grep -q "enable-llvm-xray"; then
    echo "✓ PASS: --enable-llvm-xray option found in configure"
else
    echo "✗ FAIL: --enable-llvm-xray option not found"
    exit 1
fi
echo ""

# Test 2: Verify configure accepts the option with clang
echo "Test 2: Testing configure with --enable-llvm-xray using clang..."
TEMP_DIR=$(mktemp -d)
cd "$TEMP_DIR"

# Run configure with xray enabled using clang (which supports xray)
if command -v clang++ &> /dev/null; then
    echo "Found clang, testing with LLVM XRay support..."
    CC=clang CXX=clang++ "$OLDPWD/configure" --enable-llvm-xray 2>&1 | tee config.out

    # Check if xray was enabled
    if grep -q "Enabling LLVM XRay instrumentation" config.out; then
        echo "✓ PASS: LLVM XRay instrumentation enabled"
    else
        echo "✗ FAIL: LLVM XRay not enabled in configure output"
        cat config.out
        exit 1
    fi

    # Check if the flags were accepted
    if grep -q "fxray-instrument" config.out || grep -q "checking.*fxray-instrument" config.out; then
        echo "✓ PASS: XRay compiler flags detected"
    else
        echo "⚠ WARNING: XRay flags may not have been checked"
    fi
else
    echo "⚠ WARNING: clang not found, skipping xray flag test"
    echo "  (XRay is primarily supported by LLVM/clang)"
fi

cd "$OLDPWD"
rm -rf "$TEMP_DIR"

echo ""
echo "=== All tests completed ==="
