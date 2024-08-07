import sys
from collections import defaultdict
import numpy as np
import os

def extract_function_data(line):
    if "[fun:" in line and "[cost:" in line:
        function_name = line.split("[fun:")[1].split("]")[0]
        execution_time = float(line.split("[cost:")[1].split("ms]")[0])
        return function_name, execution_time
    return None, None

def calculate_execution_time_state(file_path):
    function_data = defaultdict(lambda: {'time': [], 'count': 0})

    with open(file_path, 'r') as file:
        for line in file:
            function_name, execution_time = extract_function_data(line)
            if function_name is not None and execution_time is not None:
                function_data[function_name]['time'].append(execution_time)
                function_data[function_name]['count'] += 1

    return function_data

file_path = sys.argv[1]

data_times = calculate_execution_time_state(file_path)

import matplotlib.pyplot as plt
fig = plt.figure()
graph_time = data_times['XGraph::inference']['time']
data_size = len(graph_time)
bottom = np.zeros(data_size)
x = np.linspace(0, data_size, data_size)
plt.plot(x[::10], graph_time[::10], label='graph_time')
for function_name, time_data in data_times.items():
    time_data = np.array(time_data['time'])[:data_size]
    if function_name.endswith("Calculator::Process"):
        x = np.linspace(0, data_size, data_size)[:time_data.shape[0]]
        plt.bar(x, time_data, bottom=bottom[:time_data.shape[0]], label=function_name)
        bottom[:time_data.shape[0]] += time_data
    print(f" {function_name} mean : {np.mean(time_data):.3f} ms")
    print(f" {function_name} std : {np.std(time_data):.3f} ms")
    print(f" {function_name} min : {np.min(time_data):.3f} ms")
    print(f" {function_name} max : {np.max(time_data):.3f} ms")
    print(f" {function_name} 95per : {np.percentile(time_data, 95):.3f} ms")
cal_cost = bottom
function_name = 'calculator sum'
time_data = bottom
print(f" {function_name} mean : {np.mean(time_data):.3f} ms")
print(f" {function_name} std : {np.std(time_data):.3f} ms")
print(f" {function_name} min : {np.min(time_data):.3f} ms")
print(f" {function_name} max : {np.max(time_data):.3f} ms")
print(f" {function_name} 95per : {np.percentile(time_data, 95):.3f} ms")
graph_cost = graph_time - bottom
function_name = 'graph delta'
time_data = graph_cost
print(f" {function_name} mean : {np.mean(time_data):.3f} ms")
print(f" {function_name} std : {np.std(time_data):.3f} ms")
print(f" {function_name} min : {np.min(time_data):.3f} ms")
print(f" {function_name} max : {np.max(time_data):.3f} ms")
print(f" {function_name} 95per : {np.percentile(time_data, 95):.3f} ms")
plt.legend()
plt.title('time cost')
plt.xlabel('time')
plt.ylabel('ms')
img_name = os.path.splitext(os.path.basename(file_path))[0]
plt.savefig(f"{img_name}.png")