import sys
from collections import defaultdict

def extract_function_data(line):
    if "[fun:" in line and "[cost:" in line:
        function_name = line.split("[fun:")[1].split("]")[0]
        execution_time = float(line.split("[cost:")[1].split("ms]")[0])
        return function_name, execution_time
    return None, None

def calculate_average_execution_time(file_path):
    function_data = defaultdict(lambda: {'total_time': 0.0, 'count': 0})

    with open(file_path, 'r') as file:
        for line in file:
            function_name, execution_time = extract_function_data(line)
            if function_name is not None and execution_time is not None:
                function_data[function_name]['total_time'] += execution_time
                function_data[function_name]['count'] += 1

    average_execution_times = {}
    for function_name, data in function_data.items():
        if data['count'] > 0:
            average_execution_times[function_name] = data['total_time'] / data['count']

    return average_execution_times

file_path = sys.argv[1]

average_times = calculate_average_execution_time(file_path)

for function_name, average_time in average_times.items():
    print(f" {function_name} : {average_time:.3f} ms")