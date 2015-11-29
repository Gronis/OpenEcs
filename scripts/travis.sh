#!/usr/bin/env bash
cmake -DCMAKE_BUILD_TYPE=Debug
make UnitTests
make PerformanceTests
./UnitTests