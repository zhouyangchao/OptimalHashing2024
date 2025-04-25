#!/usr/bin/env python2
# -*- coding: utf-8 -*-

import matplotlib
matplotlib.use('Agg')  # Use non-interactive backend
import matplotlib.pyplot as plt
import numpy as np
import sys
import csv

def parse_performance_data(filename):
    """Parse performance data from output file"""
    with open(filename, 'r') as f:
        content = f.read()
    
    # Use regex to extract performance data
    pattern = r"性能比较结果.*?算法\s+构建时间\s+查询时间.*?MinimalPerfectHash\s+(\d+)\s+(\d+).*?SimpleHash\s+(\d+)\s+(\d+).*?ElasticHash\s+(\d+)\s+(\d+).*?FunnelHash\s+(\d+)\s+(\d+)"
    match = re.search(pattern, content, re.DOTALL)
    
    if match:
        mph_build = int(match.group(1))
        mph_lookup = int(match.group(2))
        sh_build = int(match.group(3))
        sh_lookup = int(match.group(4))
        eh_build = int(match.group(5))
        eh_lookup = int(match.group(6))
        fh_build = int(match.group(7))
        fh_lookup = int(match.group(8))
        
        return {
            'MinimalPerfectHash': {'build': mph_build, 'lookup': mph_lookup},
            'SimpleHash': {'build': sh_build, 'lookup': sh_lookup},
            'ElasticHash': {'build': eh_build, 'lookup': eh_lookup},
            'FunnelHash': {'build': fh_build, 'lookup': fh_lookup}
        }
    return None

def create_plots(data):
    """Create performance comparison charts"""
    algorithms = list(data.keys())
    build_times = [data[alg]['build'] for alg in algorithms]
    lookup_times = [data[alg]['lookup'] for alg in algorithms]
    
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(15, 6))
    
    # Build time chart
    bars1 = ax1.bar(range(len(algorithms)), build_times, color=['blue', 'green', 'red', 'purple'])
    ax1.set_xticks(range(len(algorithms)))
    ax1.set_xticklabels(algorithms)
    ax1.set_title('Build Time Comparison (ms)')
    ax1.set_ylabel('Time (milliseconds)')
    
    # Add values on bars - Python 2 compatible
    for i, bar in enumerate(bars1):
        height = bar.get_height()
        ax1.text(bar.get_x() + bar.get_width()/2., height + 5,
                str(height),
                ha='center', va='bottom')
    
    # Lookup time chart
    bars2 = ax2.bar(range(len(algorithms)), lookup_times, color=['blue', 'green', 'red', 'purple'])
    ax2.set_xticks(range(len(algorithms)))
    ax2.set_xticklabels(algorithms)
    ax2.set_title('Lookup Time (ms, 100000 iterations)')
    ax2.set_ylabel('Time (milliseconds)')
    
    # Add values on bars - Python 2 compatible
    for i, bar in enumerate(bars2):
        height = bar.get_height()
        ax2.text(bar.get_x() + bar.get_width()/2., height + 5,
                str(height),
                ha='center', va='bottom')
    
    plt.tight_layout()
    plt.savefig('hash_performance_comparison.png')
    print("Performance chart saved as hash_performance_comparison.png")

def load_results_from_csv(filename):
    """从CSV中加载负载测试结果"""
    sizes = []
    mph_times = []
    sh_times = []
    eh_times = []
    fh_times = []
    
    with open(filename, 'r') as f:
        lines = f.readlines()
        # Skip the first two header lines
        data_lines = lines[2:]
        for line in data_lines:
            if line.strip():  # Skip empty lines
                values = line.strip().split(',')
                if len(values) >= 5:
                    try:
                        sizes.append(int(values[0]))
                        mph_times.append(int(values[1]))
                        sh_times.append(int(values[2]))
                        eh_times.append(int(values[3]))
                        fh_times.append(int(values[4]))
                    except ValueError:
                        print("Skipping invalid line: " + line.strip())
    
    return {
        'sizes': sizes,
        'mph_times': mph_times,
        'sh_times': sh_times,
        'eh_times': eh_times,
        'fh_times': fh_times
    }

def plot_load_performance(data):
    """绘制不同负载下的性能对比"""
    sizes = data['sizes']
    mph_times = data['mph_times']
    sh_times = data['sh_times']
    eh_times = data['eh_times']
    fh_times = data['fh_times']
    
    plt.figure(figsize=(10, 6))
    
    plt.plot(sizes, mph_times, 'bo-', label='MinimalPerfectHash')
    plt.plot(sizes, sh_times, 'gs-', label='SimpleHash')
    plt.plot(sizes, eh_times, 'r^-', label='ElasticHash')
    plt.plot(sizes, fh_times, 'mD-', label='FunnelHash')
    
    plt.title('Different Hash Algorithms Performance Under Various Loads')
    plt.xlabel('Load Size (Number of Keys)')
    plt.ylabel('Lookup Time for 10000 iterations (ms)')
    plt.legend()
    plt.grid(True)
    plt.savefig('load_comparison.png')
    print("Load comparison chart saved as load_comparison.png")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python visualize.py <load_results_csv>")
        sys.exit(1)
    
    data = load_results_from_csv(sys.argv[1])
    if data['sizes']:  # Check if we have valid data
        plot_load_performance(data)
    else:
        print("No valid data could be parsed from the CSV file")
