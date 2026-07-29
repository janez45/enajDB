#!/bin/bash

cmake --build build --target enajDB && ./build/enajDB "$@"