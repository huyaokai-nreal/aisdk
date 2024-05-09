import os
import sys
import platform
import subprocess

dry_run=False

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


# Return root_dir,apps_dir,lib_dir and dsp_dir
def get_dirs_on_device(target_info,options):

    if 'hlos_root_dest' in vars(options) and options.hlos_root_dest != None:
        root_dir = '{}'.format(options.hlos_root_dest)
    else:
        root_dir = target_info['root_dir']

    root_dir = check_path(root_dir)

    # directory to push HLOS binaries
    apps_dir = root_dir + 'bin/'
    # directory to push DSP binaries
    dsp_dir = '{}/lib/rfsa/dsp/sdk'.format(root_dir)
    # directory to push HLOS libs
    lib_dir = target_info['app_libs_dir']

    if 'hlos_lib_dest' in vars(options) and options.hlos_lib_dest:
        lib_dir = options.hlos_lib_dest
        lib_dir = lib_dir.replace('\\','/')

    lib_dir = check_path(lib_dir)

    return root_dir,apps_dir,lib_dir,dsp_dir


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


# Mount the device
def mount_device(device_number, hlos):

    run_cmd('adb -s {} wait-for-device root'.format(device_number))
    print("#checking disable-verity ...")
    cmd = "adb -s {} wait-for-device disable-verity".format(device_number)
    output = get_output(cmd)
    disable_Verity_flag = False
    for i in output:
        if "Now reboot your device for settings to take effect" in i:
            print("\n--- Successfully disabled verity. Device rebooting ...")
            run_cmd("adb -s {} wait-for-device reboot".format(device_number))
            run_cmd('adb -s {} wait-for-device root'.format(device_number))
            disable_Verity_flag = True
    if(not disable_Verity_flag):
        print ("#verity is already disabled")

    run_cmd('adb -s {} wait-for-device remount'.format(device_number))
    if hlos == 'LE':
        run_cmd('adb -s {} wait-for-device shell mount -o remount,rw,exec /'.format(device_number))


# This function checks the ADB version installed
def adb_version_check():
    adb_version_output = get_output("adb version")
    version_lines = adb_version_output[0]
    ver_num = version_lines.split(' ')[4].split('.')[2]
    adb_version = int(ver_num)
    print("adb version is ",adb_version)
    if(adb_version < 39):
        print("adb version below 1.0.39 not supported, please upgrade to higher version")
        sys.exit()


def check_verinfo(verinfo_path_list, dev_no):
    found_verinfo = False
    for path in verinfo_path_list:
        try:
            cmd = subprocess.check_output(["adb","-s",dev_no,"wait-for-device","shell","ls",path], stderr=subprocess.PIPE)
            if "No such file or directory" in cmd.decode():
                continue
            else:
                found_verinfo = True
                break
        except:
            continue
    return path, found_verinfo

# This function returns target name corresponding to the ADB serial number
def get_target_name_using_dev_num(dev_no, options):
    global dry_run
    if options is not None:
        dry_run=options.dry_run
    # List of Paths containing ver_info.txt
    verinfo_path_list = ["/firmware/verinfo/Ver_Info.txt", "/vendor/firmware/verinfo/ver_info.txt", "/vendor/firmware_mnt/verinfo/ver_info.txt", "/vendor/firmware_mnt/verinfo/Ver_info.txt", "/firmware/verinfo/ver_info.txt"]
    verinfo_path_list_k2C = ["/lib/firmware/qcom/qcm6490/Ver_Info.txt"]

    cmd = "adb -s {} wait-for-device root".format(dev_no)
    run_cmd(cmd)
    path, found_verinfo = check_verinfo(verinfo_path_list, dev_no)
    if found_verinfo != True:
        path, found_verinfo = check_verinfo(verinfo_path_list_k2C, dev_no)

    verinfo = get_output("adb -s {} wait-for-device shell cat {}".format(dev_no,path))
    target = 'none'
    for j in verinfo:
        if "Meta_Build_ID" in j:
            target = j.split(":")[1].strip()
            target = target.split('"')[1].split(".")[0].lower()
    print("target=",target)
    return target


# This function checks the setup on host machine
# @options: command line options
# @supported_targets: Targets that are supported by LA or LE
# Return value: Name and ADB serial number of the target
def check_host_setup(options,supported_targets):

    # Checking if neither the target name nor ADB serial number is passed
    if options.target == None and options.device_id == None:
        sys.exit('ERROR: Please pass either the target name (-T) or ADB serial number (-s) of the device')

    # Check if ADB version is proper or not
    adb_version_check()

    connected_devices = get_devices(options)

    # If both target name and serial number are passed
    if options.target != None and options.device_id != None:

        if options.target in supported_targets and options.device_id in connected_devices[options.target.lower()]:
            return options.target, options.device_id
        else:
            sys.exit("ERROR: Failed to find the target {} with serial number {}".format(options.target,options.device_id))

    # If target name is passed
    if options.target != None:

        # Check if the target is supported or not
        if options.target not in supported_targets:
            sys.exit("ERROR: Target \"{}\" is not supported. Supported targets are: \n\t{}".format(options.target,'\n\t'.join(supported_targets)))

        # Check if the target is connected or not
        if options.target not in connected_devices.keys():
            if options.target == "qcs403" and "qcs405" in list(connected_devices.keys()) and "qcs403" not in list(connected_devices.keys()):
                    print("QCS403 is not supported , using QCS405 instead of QCS403 as meta build info says QCS405")
                    options.target = "qcs405"
            else:
                sys.exit('ERROR: Target \"{}\" is not connected. Connected targets are: {}'.format(options.target, list(connected_devices.keys())))

        # Check if multiple devices with same names are connected
        if len(connected_devices[options.target.lower()]) > 1:
            print("ERROR: multiple {} devices are connected. Please provide ADB serial number(-s) along with target name(-T)".format(options.target))
            print("Target name: ".format(options.target))
            sys.exit("Devices connected: ".format(connected_devices[options.target.lower()]))

        target_name = options.target.lower()
        serial_number = connected_devices[options.target.lower()][0]

    # If ADB serial number is passed
    elif options.device_id != None:

        # Check if device with the serial number is connected or not
        print(options.device_id)
        print(list(x for value in connected_devices.values() for x in value))
        if options.device_id not in list(x for value in connected_devices.values() for x in value):
            sys.exit('ERROR: Device with serial number {} is not connected'.format(options.device_id))

        # Get device name for serial number
        for key, value in connected_devices.items():
            if options.device_id in value:
                target_name = key

        # Check if the target is supported or not
        if target_name not in supported_targets:
            sys.exit("ERROR: Target \"{}\" is not supported. Supported targets are: \n\t{}".format(target_name,'\n\t'.join(supported_targets)))
        serial_number = options.device_id

    # Return target name and serial number
    return target_name,serial_number


# This function prepares list of the devices connected to the host machine
def get_devices(options=None):

    adb_serial_numbers = list()
    global connected_devices
    connected_devices = dict()

    # Fetch the list of ADB serial numbers of the devices connected
    output = get_output('adb devices')
    for i in output:
        if '\t' in i:
            adb_serial_numbers.append(i.split('\t')[0])

    # Fetch target name of the connected device and create a "connected_devices" dictionary
    for serial_number in adb_serial_numbers:
        target_name = get_target_name_using_dev_num(serial_number, options)
        if target_name != 'none':
            if target_name not in list(connected_devices.keys()):
                connected_devices[target_name.lower()] = [serial_number]
            else:
                connected_devices[target_name.lower()].append(serial_number)

    return connected_devices


# This function pushes the HLOS and hexagon binaries on LA/LE device
def copy_binaries_with_adb(device,executables,stubfiles,skelfiles):

    apps_dst = device.app_exe_dir
    dsp_dst = device.dsp_libs_dir
    lib_dst = device.app_libs_dir

    mount_device(device.device_id, 'LA' if 'vendor' in device.root_dir else 'LE')

    print('#--- Push HLOS components ---')
    # Push executable to the target
    run_cmd('adb -s {} wait-for-device shell mkdir -p {}'.format(device.device_id,apps_dst))
    for executable in executables:
        run_cmd('adb -s {} wait-for-device push {} {}'.format(device.device_id,executable,apps_dst))

    # Push HLOS binaries to the target
    run_cmd('adb -s {} wait-for-device shell mkdir -p {}'.format(device.device_id,lib_dst))
    for lib in stubfiles:
        run_cmd('adb -s {} wait-for-device push {} {}'.format(device.device_id,lib,lib_dst))

    print('#--- Push Hexagon components ---')
    # Push Hexagon binaries to the target
    run_cmd('adb -s {} wait-for-device shell mkdir -p {}'.format(device.device_id,dsp_dst))
    for lib in skelfiles:
        run_cmd('adb -s {} wait-for-device push {} {}'.format(device.device_id,lib,dsp_dst))


# Run executable on LA/LE device
def run_with_adb(device,executable,args_list):

    apps_dst = device.app_exe_dir
    dsp_dst = device.dsp_libs_dir
    lib_dst = device.app_libs_dir
    dsp_lib_search_path = "DSP_LIBRARY_PATH"
    dsp_lib_path = dsp_dst

    if device.prior_sm8250:
        dsp_lib_search_path = "ADSP_LIBRARY_PATH"
        dsp_lib_path = '{}\;{}/lib/rfsa/adsp'.format(dsp_dst,device.root_dir)

    run_cmd('adb -s {} wait-for-device shell chmod 777 {}/{}'.format(device.device_id, apps_dst, executable))

    # Direct DSP messages to logcat
    run_cmd('adb -s {} wait-for-device shell \"echo 0x1f > {}/{}.farf\"'.format(device.device_id, dsp_dst, executable))

    if len(args_list) == 0:
        command = 'adb -s {} wait-for-device shell \"export LD_LIBRARY_PATH={} {}={}; {}/{}\"'.format(device.device_id, lib_dst, dsp_lib_search_path, dsp_lib_path, apps_dst, executable)
        run_cmd(command)
        print("\n\n")
    else:
        for args in args_list:
            command = 'adb -s {} wait-for-device shell \"export LD_LIBRARY_PATH={} {}={}; {}/{} {}\"'.format(device.device_id, lib_dst, dsp_lib_search_path, dsp_lib_path, apps_dst, executable, args)
            run_cmd(command)
            print("\n\n")

# This function will be merged into the above run_with_adb functions in later releases
def run_env_with_adb(device,executable,environment,args_list):

    apps_dst = device.app_exe_dir
    dsp_dst = device.dsp_libs_dir
    lib_dst = device.app_libs_dir
    dsp_lib_search_path = "DSP_LIBRARY_PATH"
    dsp_lib_path = dsp_dst

    if device.prior_sm8250:
        dsp_lib_search_path = "ADSP_LIBRARY_PATH"
        dsp_lib_path = '{}\;{}/lib/rfsa/adsp'.format(dsp_dst,device.root_dir)

    run_cmd('adb -s {} wait-for-device shell chmod 777 {}/{}'.format(device.device_id, apps_dst, executable))

    # Direct DSP messages to logcat
    run_cmd('adb -s {} wait-for-device shell \"echo 0x1f > {}/{}.farf\"'.format(device.device_id, dsp_dst, executable))

    if len(args_list) == 0:
        print("In args_list 0")
        command = 'adb -s {} wait-for-device shell \"export LD_LIBRARY_PATH={} {}={}; {} {}/{}\"'.format(device.device_id, lib_dst, dsp_lib_search_path, dsp_lib_path, environment, apps_dst, executable)
        run_cmd(command)
        print("\n\n")
    else:
        for args in args_list:
            print("In args_list "+args)
            command = 'adb -s {} wait-for-device shell \"export LD_LIBRARY_PATH={} {}={}; {} {}/{} {}\"'.format(device.device_id, lib_dst, dsp_lib_search_path, dsp_lib_path, environment, apps_dst, executable, args)
            run_cmd(command)
            print("\n\n")
            
if __name__ == "__main__":
    adb_version_check()
