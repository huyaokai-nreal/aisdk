    # -*- coding: utf-8 -*-     
import json
import os
import sys
import argparse
import platform
import subprocess
import time
import shutil
import hashlib

dry_run=False

# tool_version=1.0 首版基于linux平台的，单次录制需要人工手动start、stop
# tool_version=1.1 去掉人工手动start、stop办法，在android APP上多次录制，在linux平台上批量生成报告
# tool_version=1.2 补充回1.1的功能
tool_version='1.2'

# aisdk_version='v1' 第1版sdk，2个算法so
# aisdk_version='v2' 第2版sdk，1个算法so
aisdk_version='v1'

PWD=os.getcwd()
SCRIPT_DIR=PWD+'/script/'
PREPARE_DIR=PWD+'/prepare/'
RESULT_DIR=PWD+'/result/'
print(SCRIPT_DIR)
print(PREPARE_DIR)
print(RESULT_DIR)

# Print command and execute with the error check
def run_cmd(cmd, print_cmd=True):
    if print_cmd:
        print(cmd)
    if not dry_run:
        if os.system(cmd) != 0 : sys.exit(2)
        

# Prepend and appends '/' to the path name
def check_path(path):
    if not path.startswith('/'):
        path = '/' + path
    if not path.endswith('/'):
        path = path + '/'
    return path

        
def read_phone_file(device_id, file_path, expected_state):
    # 使用adb shell命令读取手机上的文件内容
    cmd = ['adb', '-s', device_id, 'shell', "cat", file_path]
    
    start_time = time.time()
    while True:
        current_output = subprocess.check_output(cmd).decode('utf-8').strip()
        if current_output == expected_state or (time.time() - start_time) > 20:  # 检查是否超过10秒
            break
    if current_output == expected_state:  # 超过10秒后依然为record_end\n
        return True
    else:
        return False
    
# This function returns output of command as a list
def get_output(command):
    try:
        ret = subprocess.run(command, shell=True, stdout=subprocess.PIPE)
    except Exception as e:
        sys.exit("ERROR: Cannot execute {}:\n{}".format(command,e))

    if platform.system() == "Windows":
        return ret.stdout.decode().strip().split('\r\n')
    else:
        return ret.stdout.decode().strip().split('\n')

def calculate_md5(file_path):
    # 创建一个 MD5 hash 对象
    md5_hash = hashlib.md5()

    # 以二进制读取文件内容并更新 MD5 hash 对象
    with open(file_path, "rb") as file:
        # 逐块读取文件内容，以节省内存
        for chunk in iter(lambda: file.read(4096), b""):
            md5_hash.update(chunk)

    # 返回 MD5 哈希值的十六进制表示
    return md5_hash.hexdigest()
    
def read_parser():
    parser = argparse.ArgumentParser()
    parser.add_argument('--device', '-d', type=str, default='10.2.16.130',
                        help='选择设备的ip地址')
    parser.add_argument('--request', '-r', type=str, required= True, choices=["prepare","start","stop","batchreport","deletecache"],
                        help='选择动作')
    parser.add_argument('--config_path', '-p', type=str, default=PREPARE_DIR+'record_configs.json',
                    help='选择配置文件')
    parser.add_argument('--fps', '-f', type=str, default='30',
                help='选择配置文件')
    
    return parser.parse_args()


if __name__ == "__main__":  
    # 解析命令行参数
    args = read_parser()
    device = args.device
    request = args.request
    config_path = args.config_path
    fps = args.fps
    sdk_json_path = PREPARE_DIR+"sdk.json"
    libnr_hand_algo_so_path = PREPARE_DIR+"libnr_hand_algo.so"
    libnr_hand_tracking_so_path = PREPARE_DIR+"libnr_hand_tracking.so"

    
    if request == "prepare":
        # 打开并加载JSON文件
        with open(config_path, 'r') as file:
            data = json.load(file)
        
        files_path = data["local_records_config"]["record_path"]
                
        # 判断文件目录是否存在
        run_cmd('adb shell "if [ ! -d {} ]; then mkdir {}; fi"'.format(files_path, files_path))
        # 显示MD5值
        print("libnr_hand_algo.so md5sum= ",calculate_md5(libnr_hand_algo_so_path))
        print("libnr_hand_tracking.so md5sum= ",calculate_md5(libnr_hand_tracking_so_path))
        
        # 手动推库和推配置
        print("aisdk_version= ",aisdk_version)
        if aisdk_version == "v1":
            run_cmd('adb -s {} wait-for-device push {} {} {} {} {}'.format(device, config_path, sdk_json_path, libnr_hand_algo_so_path, libnr_hand_tracking_so_path, files_path))
        elif aisdk_version == "v2":
            run_cmd('adb -s {} wait-for-device push {} {} {} {}'.format(device, config_path, sdk_json_path, libnr_hand_tracking_so_path, files_path))

        print("[record prepare ok !!!!]")
    elif request == "start":
        # 打开并加载JSON文件
        with open(config_path, 'r') as file:
            data = json.load(file)
        
        files_path = data["local_records_config"]["record_path"]
        # 刷新JSON文件
        data['enable_local_records'] = True
        with open(config_path, 'w') as file:
            json.dump(data, file, indent=4)  
        run_cmd('adb -s {} wait-for-device push {} {}'.format(device, config_path, data["local_records_config"]["record_path"]))
        if(read_phone_file(device, check_path(files_path) + "record.status", "record_start")):
            print("Recode has successfully started")
        else:
            print("Recode startup failed")
        
    elif request == "stop":
        # 刷新JSON文件
        # 打开并加载JSON文件
        with open(config_path, 'r') as file:
            data = json.load(file)
        
        files_path = data["local_records_config"]["record_path"]
        data['enable_local_records'] = False
        with open(config_path, 'w') as file:
            json.dump(data, file, indent=4)  

        run_cmd('adb -s {} wait-for-device push {} {}'.format(device , config_path, data["local_records_config"]["record_path"]))
        if(read_phone_file(device, check_path(files_path) + "record.status", "record_stop")):
            print("Recode has successfully stoped")
        else:
            print("Recode stop failed")
    elif request == "stopreport":
        pass
        # 刷新JSON文件
        # 打开并加载JSON文件
        # with open(config_path, 'r') as file:
        #     data = json.load(file)
        
        # files_path = data["local_records_config"]["record_path"]
        # data['enable_local_records'] = False
        # with open(config_path, 'w') as file:
        #     json.dump(data, file, indent=4)  

        # run_cmd('adb -s {} wait-for-device push {} {}'.format(device , config_path, data["local_records_config"]["record_path"]))
        # if(read_phone_file(device, check_path(files_path) + "record.status", "record_stop")):
        #     # 构建adb pull命令
        #     record_name = data["local_records_config"]["record_name"]
        #     record_path = check_path(files_path) + record_name
        #     adb_pull_command = ['adb', '-s', device, 'wait-for-device', 'pull', record_path, RESULT_DIR]
            
        #     # 使用subprocess.run()来执行命令并等待其完成
        #     subprocess.run(adb_pull_command)
        #     # 当subprocess.run()返回时，表示文件已经成功下载
        #     print("File download completed.")
            
        #     # 假设脚本在当前目录下
        #     result = subprocess.run(["bash", SCRIPT_DIR+"build_hand_res.sh", RESULT_DIR+record_name, fps])
        #     # print(result.stdout)  # 输出标准输出
        #     # print(result.stderr)  # 输出标准错误
        #     if result.returncode == 0:
        #         print("Script executed successfully")
        # else:
        #     print("Recode shutdown failed")
        
    elif request == "batchreport":
        # 刷新JSON文件
        # 打开并加载JSON文件
        with open(config_path, 'r') as file:
            data = json.load(file)
        
        record_path = data["local_records_config"]["record_path"]
        record_name = data["local_records_config"]["record_name"]
        
        adb_command = 'adb -s {} shell "find {} -type d -name \'{}_[0-9]*\' -printf \'%f\\n\'" '.format(device,record_path,record_name)
        print(adb_command)
        batch_recordname_list = get_output(adb_command)
        print("search record_name list: ",batch_recordname_list)
        for subrecord in batch_recordname_list:
            subrecord_path = record_path + '/' + subrecord
            # 构建adb pull命令
            adb_pull_command = ['adb', '-s', device, 'wait-for-device', 'pull', subrecord_path, RESULT_DIR]
            
            # 使用subprocess.run()来执行命令并等待其完成
            subprocess.run(adb_pull_command)
            # 当subprocess.run()返回时，表示文件已经成功下载
            print("{} download completed.".format(subrecord_path))
            
            # 假设脚本在当前目录下
            result = subprocess.run(["bash", SCRIPT_DIR+"build_hand_res.sh", RESULT_DIR+subrecord, fps])
            # print(result.stdout)  # 输出标准输出
            # print(result.stderr)  # 输出标准错误
            if result.returncode == 0:
                print("Script executed successfully")
        print("[record batchreport ok !!!!]")
    elif request == "deletecache":
        # 刷新JSON文件
        # 打开并加载JSON文件
        with open(config_path, 'r') as file:
            data = json.load(file)
        
        record_path = data["local_records_config"]["record_path"]
        record_name = data["local_records_config"]["record_name"]
        
        adb_command = 'adb -s {} shell "find {} -type d -name \'{}_[0-9]*\' -printf \'%f\\n\'" '.format(device,record_path,record_name)
        print(adb_command)
        batch_recordname_list = get_output(adb_command)
        print("search record_name list: ",batch_recordname_list)
        for subrecord in batch_recordname_list:
            subrecord_path = record_path + '/' + subrecord
            adb_pull_command = ['adb', '-s', device, 'wait-for-device', 'shell', 'rm -rf',subrecord_path]
            subprocess.run(adb_pull_command)        
            print("rm {} completed.".format(subrecord_path))
            
        print("[record deletecache ok !!!!]")