#!/bin/bash

# Update Makefile to use C++17
sed -i 's/-std=c++11/-std=c++17/g' Makefile

# Compile the project
make clean
make

# Run tests and save output
./optimalhash > test_results.txt

# Use Python script to visualize performance results
python2 visualize.py load_results.csv

# Display results
echo "Test completed, results saved to test_results.txt"
echo "Performance comparison chart saved to load_comparison.png"
