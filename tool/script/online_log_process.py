import re
import sys
from datetime import datetime

def parse_timestamp(timestamp_str):
    """解析时间戳"""
    return datetime.strptime(timestamp_str, "%Y-%m-%d %H:%M:%S.%f")

def format_timestamp(dt):
    """格式化时间戳"""
    return dt.strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]  # 保留3位毫秒

def main(input_file, output_file):
    EVENT_SEQUENCE = [
        "HandTracking: NRPluginCreate",
        "HandTracking: NRPluginCreated",
        "HandTracking: Initializing",
        "HandTracking: Initialized!",
        "HandTracking: Start",
        "HandTracking: Started"
    ]

    valid_sequences = []
    current_sequence = []
    expected_step = 0

    # 正则：严格匹配第一个[]内的完整时间戳
    timestamp_pattern = re.compile(r".*?\[(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3})\]")                     

    with open(input_file, 'r') as f:
        for line_num, line in enumerate(f, 1):
            # 筛选包含AISDK的行
            if 'AISDK' not in line:
                continue

            # 验证目标事件
            current_event = next((e for e in sorted(EVENT_SEQUENCE, key=len, reverse=True) if e in line), None)
            if not current_event:
                continue

            # 提取第一个[]中的内容作为时间戳
            timestamp_match = timestamp_pattern.match(line)
            if not timestamp_match:
                print(f"行 {line_num} 时间戳匹配失败: {line.strip()}")
                continue
            
            try:
                event_time = parse_timestamp(timestamp_match.group(1))
            except Exception as e:
                print(f"行 {line_num} 时间解析失败: {line.strip()} | {str(e)}")
                continue

            # 严格顺序验证
            if current_event != EVENT_SEQUENCE[expected_step]:
                print(f"行 {line_num} 顺序错误: 期待 {EVENT_SEQUENCE[expected_step]} 实际 {current_event}")
                current_sequence = []
                expected_step = 0
                continue

            # 记录有效事件
            current_sequence.append({
                "event": current_event,
                "time": event_time,
                "time_str": format_timestamp(event_time),
                "raw_line": line.strip()
            })
            expected_step += 1

            # 完整序列处理
            if expected_step == len(EVENT_SEQUENCE):
                valid_sequences.append(current_sequence)
                current_sequence = []
                expected_step = 0

    # 生成结果报告
    with open(output_file, 'w') as f:
        if not valid_sequences:
            f.write("未找到完整的有效事件序列\n")
            return
        
        # 初始化统计数据
        total_create, total_init, total_start, total_all = 0, 0, 0, 0
        sequence_count = len(valid_sequences)

        # 收集所有序列的耗时数据
        all_create = []
        all_init = []
        all_start = []
        all_total = []

        for seq in valid_sequences:
            # 计算各阶段耗时
            create_created = (seq[1]['time'] - seq[0]['time']).total_seconds() * 1000
            init_initialized = (seq[3]['time'] - seq[2]['time']).total_seconds() * 1000
            start_started = (seq[5]['time'] - seq[4]['time']).total_seconds() * 1000
            one_loop_total_time = (seq[5]['time'] - seq[0]['time']).total_seconds() * 1000

            # 累加到统计列表
            all_create.append(create_created)
            all_init.append(init_initialized)
            all_start.append(start_started)
            all_total.append(one_loop_total_time)

        # 计算平均值
        avg_create = sum(all_create) / sequence_count
        avg_init = sum(all_init) / sequence_count
        avg_start = sum(all_start) / sequence_count
        avg_total = sum(all_total) / sequence_count

        # 输出统计头部
        f.write(f"分析结果（共 {sequence_count} 个有效序列）\n")
        f.write("全局平均值统计：\n")
        f.write(f"NRPluginCreate → NRPluginCreated 平均耗时: {avg_create:.3f} ms\n")
        f.write(f"Initializing → Initialized 平均耗时: {avg_init:.3f} ms\n")
        f.write(f"Start → Started 平均耗时: {avg_start:.3f} ms\n")
        f.write(f"全流程总耗时平均值: {avg_total:.3f} ms\n")
        f.write("="*50 + "\n\n")

        # 输出详细序列信息
        for seq_idx, seq in enumerate(valid_sequences, 1):
            # 使用已计算的耗时数据
            create_created = all_create[seq_idx-1]
            init_initialized = all_init[seq_idx-1]
            start_started = all_start[seq_idx-1]
            one_loop_total_time = all_total[seq_idx-1]

            f.write(f"序列 #{seq_idx}\n")
            f.write(f"完整事件流：\n")
            for item in seq:
                f.write(f"  [{item['time_str']}] {item['event'].split(': ')[1]}\n")
            
            f.write("\n各阶段耗时: \n")
            f.write(f"1. NRPluginCreate → NRPluginCreated: {create_created:.3f} ms\n")
            f.write(f"2. Initializing → Initialized: {init_initialized:.3f} ms\n")
            f.write(f"3. Start → Started: {start_started:.3f} ms\n")
            f.write(f"总耗时: {one_loop_total_time:.3f} ms\n")
            f.write("-" * 50 + "\n")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("usage: python script.py input.log output.txt")
        sys.exit(1)
    
    main(sys.argv[1], sys.argv[2])