#pragma once

#include "nr_plugin_lifecycle.h"
#include "nr_plugin_glasses_types.h"
#include "nr_plugin_glasses_types.h"
typedef struct NRGlassesInvokeProvider {
	/*
		
		/// @brief 获取GlassesID（老版本十位SN）
		/// \n air-like: "MSG_R_GLASSID": "0x15",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetGlassesID)(
        NRPluginHandle handle,
        const char ** id_data,
        uint32_t * id_size
    );
	/*
		
		/// @brief 获取GlassesSN（新版本十六位SN）参考：https://xreal.feishu.cn/docx/NldIdu0U4oZgM3xIsYGcHkfMnqp
		/// \n air-like: "MSG_R_RESERVED_SN0": "0x57",
		/// \n light:
		/// @param index 获取 sn 的索引: 其中: 0: SN0 对应文档中的位置一, 1: SN1 对应文档中的位置二
		/// @param code: 客户代码
		/// @param sn: 眼镜的SN
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetGlassesSN)(
        NRPluginHandle handle,
        int32_t index,
        const char ** code_data,
        uint32_t * code_size,
        const char ** sn_data,
        uint32_t * sn_size
    );
	/*
		
		/// @brief 获取系统版本号
		/// \n air-like:  "MSG_R_MCU_APP_FW_VERSION": "0x26",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetGlassesSystemVersion)(
        NRPluginHandle handle,
        const char ** version_data,
        uint32_t * version_size
    );
	/*
		
		/// @brief 获取硬件版本号
		/// \n air-like: "MSG_R_HW_VERSION": "0x27",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetGlassesHWVersion)(
        NRPluginHandle handle,
        const char ** version_data,
        uint32_t * version_size
    );
	/*
		
		/// @brief 获取DSP固件版本号
		/// \n air-like:  "MSG_R_DSP_VERSION": "0x18",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetGlassesDspVersion)(
        NRPluginHandle handle,
        const char ** version_data,
        uint32_t * version_size
    );
	/*
		
		/// @brief 重启眼镜 （目前用于 RGB 摄像头 插入拔出）
		/// \n air-like: "MSG_W_REBOOT_DEVICE": "0xA4",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *RebootGlasses)(
        NRPluginHandle handle
    );
	/*
		
		/// @brief 关闭眼镜系统（目前用于过温保护）
		/// \n air-like: 
		/// \n light:
		/// \n gina: new
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *ShutdownGlasses)(
        NRPluginHandle handle
    );
	/*
		
		/// @brief 获取 “远” 阈值
		/// \n air-like: "MSG_R_PSENSOR_CLOSED": "0x24",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetProximityFarThreshold)(
        NRPluginHandle handle,
        int32_t * data
    );
	/*
		
		/// @brief 设置 “远” 阈值
		/// \n air-like: "MSG_W_PSENSOR_CLOSED": "0x25",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetProximityFarThreshold)(
        NRPluginHandle handle,
        int32_t data
    );
	/*
		
		/// @brief 获取 “近” 阈值
		/// \n air-like: "MSG_R_PSENSOR_FAR_AWAY": "0x0B",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetProximityNearThreshold)(
        NRPluginHandle handle,
        int32_t * data
    );
	/*
		
		/// @brief 设置 “近” 阈值
		/// \n air-like: "MSG_W_PSENSOR_FAR_AWAY": "0x0C",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetProximityNearThreshold)(
        NRPluginHandle handle,
        int32_t data
    );
	/*
		
		/// @brief 获取 接近传感器的实际值
		/// \n air-like: "MSG_R_PSENSOR_VALUE": "0x09",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetProximityValue)(
        NRPluginHandle handle,
        int32_t * value
    );
	/*
		
		/// @brief 获取 是否佩戴眼镜
		/// \n air-like: "MSG_R_GLASSES_IS_CLOSE_HEAD": "0x7A",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetProximityWearingState)(
        NRPluginHandle handle,
        NRProximityWearingState * state
    );
	/*
		
		/// @brief 设置 led 灯的开关状态
		/// \n air-like:
		/// \n light:
		/// @param led_id led 的ID
		/// @param enable_value led 的开关
		/// @return 结果
		
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetLedEnable)(
        NRPluginHandle handle,
        NRLedID led_id,
        NREnableValue enable_value
    );
	/*
		
		/// @brief 获取 led 灯的开关状态
		/// \n air-like:
		/// \n light:
		/// @param led_id led 的ID
		/// @param enable_value led 的开关
		/// @return 结果
		
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetLedEnable)(
        NRPluginHandle handle,
        NRLedID led_id,
        NREnableValue * enable_value
    );
	/*
		
		/// @brief 获取眼镜亮度最大数值（尼特）
		/// \n air-like:
		/// \n light:
		/// \n gina: new
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetDisplayLuminanceMaxValue)(
        NRPluginHandle handle,
        int32_t * value
    );
	/*
		
		/// @brief 获取眼镜亮度最小数值（尼特）
		/// \n air-like:
		/// \n light:
		/// \n gina: new
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetDisplayLuminanceMinValue)(
        NRPluginHandle handle,
        int32_t * value
    );
	/*
		
		/// @brief 获取眼镜亮度当前数值（尼特）
		/// \n air-like: "MSG_R_BRIGHTNESS_FINE_GRAINED": "0x05",
		/// \n light:
		/// \n gina: new
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetDisplayLuminanceValue)(
        NRPluginHandle handle,
        int32_t * value
    );
	/*
		
		/// @brief 设置眼镜亮度当前数值（尼特）
		/// \n air-like: "MSG_W_BRIGHTNESS_FINE_GRAINED": "0x06",
		/// \n light:
		/// \n gina: new
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetDisplayLuminanceValue)(
        NRPluginHandle handle,
        int32_t value
    );
	/*
		
		/// @brief 获取眼镜占空比 最大数值
		/// \n air-like:
		/// \n light:
		/// \n gina: new
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetDisplayDutyMaxValue)(
        NRPluginHandle handle,
        int32_t * value
    );
	/*
		
		/// @brief 获取眼镜占空比 最小数值
		/// \n air-like:
		/// \n light:
		/// \n gina: new
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetDisplayDutyMinValue)(
        NRPluginHandle handle,
        int32_t * value
    );
	/*
		
		/// @brief 获取眼镜占空比 当前数值
		/// \n air-like:
		/// \n light:
		/// \n gina: new
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetDisplayDutyValue)(
        NRPluginHandle handle,
        int32_t * value
    );
	/*
		
		/// @brief 设置眼镜占空比当前数值
		/// \n air-like:
		/// \n light:
		/// \n gina: new
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetDisplayDutyValue)(
        NRPluginHandle handle,
        int32_t value
    );
	/*
		
		/// @brief 获取 x/y轴上的 色温
		/// \n air-like: "MSG_R_OLED_COORDINATE": "0x4D",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetDisplayColorTemperature)(
        NRPluginHandle handle,
        NRDisplayUsage display_usage,
        int32_t * x_data,
        int32_t * y_data
    );
	/*
		
		/// @brief 设置 x/y轴上的 色温
		/// \n air-like: "MSG_W_OLED_COORDINATE": "0x4E",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetDisplayColorTemperature)(
        NRPluginHandle handle,
        NRDisplayUsage display_usage,
        int32_t x_data,
        int32_t y_data
    );
	/*
		
		/// @brief 获取 眼镜当前分辨率/刷新率
		/// \n air-like:
		/// \n light: 
		/// \n gina: new 
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetDisplayCurrentResolution)(
        NRPluginHandle handle,
        NRResolution * resolution
    );
	/*
		
		/// @brief 设置 眼镜当前分辨率/刷新率
		/// \n air-like:
		/// \n light: 
		/// \n gina: new 
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetDisplayCurrentResolution)(
        NRPluginHandle handle,
        NRResolution resolution
    );
	/*
		
		/// @brief 获取 眼镜上电后的默认分辨率
		/// \n air-like: "MSG_R_DEFAULT_DISPLAY_2D_3D":"0xC1",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetDisplayDefaultResolution)(
        NRPluginHandle handle,
        NRResolution * resolution
    );
	/*
		
		/// @brief 设置 眼镜上电后的默认分辨率
		/// \n air-like: "MSG_W_DEFAULT_DISPLAY_2D_3D":"0xC0",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetDisplayDefaultResolution)(
        NRPluginHandle handle,
        NRResolution resolution
    );
	/*
		
		/// @brief 获取 眼镜上电后, dp 默认的 EDID 信息
		/// \n air-like: "MSG_R_DEFAULT_DISPLAY_2D_3D":"0xC1",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetDpDefaultEdid)(
        NRPluginHandle handle,
        NREdid * edid
    );
	/*
		
		/// @brief 设置 眼镜上电后, dp 默认的 EDID 信息
		/// \n air-like: "MSG_W_DEFAULT_DISPLAY_2D_3D":"0xC0",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetDpDefaultEdid)(
        NRPluginHandle handle,
        NREdid edid
    );
	/*
		
		/// @brief 设置启动/关闭DHCP
		/// \n air-like: "MSG_W_DP_HDCP_ENABLE": "0x81",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetDpHDCPEnable)(
        NRPluginHandle handle,
        NREnableValue enable_value
    );
	/*
		
		/// @brief 设置DP的工作模式
		/// \n air-like: "MSG_W_DP_LEVEL": "0x83",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetDpWorkingMode)(
        NRPluginHandle handle,
        NRDpWorkingMode mode
    );
	/*
		
		/// @brief 获取 dp系统工作状态
		/// \n air-like: "MSG_R_GLASSES_DISPLAY_STATUS": "0xA3",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetDpWorkingState)(
        NRPluginHandle handle,
        NRDpWorkingState * state
    );
	/*
		
		/// @brief 直接进入深休眠
		/// \n air-like: "MSG_W_ENTER_SLEEP":"0x54",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *EnterPowerSave)(
        NRPluginHandle handle
    );
	/*
		
		/// @brief 打开音频输入(开始录音）
		
	*/
    NRPluginResult(NR_INTERFACE_API *StartAudioIn)(
        NRPluginHandle handle
    );
	/*
		
		/// @brief 停止音频输入
		
	*/
    NRPluginResult(NR_INTERFACE_API *StopAudioIn)(
        NRPluginHandle handle
    );
	/*
		
		/// @brief 获取 音频当前模式
		/// \n air-like: 
		/// \n light: 
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetAudioCurrentMode)(
        NRPluginHandle handle,
        NRAudioMode * mode
    );
	/*
		
		/// @brief 设置 音频当前模式
		/// \n air-like: 
		/// \n light: 
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetAudioCurrentMode)(
        NRPluginHandle handle,
        NRAudioMode mode
    );
	/*
		
		/// @brief 获取 音频默认模式
		/// \n air-like: 
		/// \n light: 
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetAudioDefaultMode)(
        NRPluginHandle handle,
        NRAudioMode * mode
    );
	/*
		
		/// @brief 设置 音频默认模式
		/// \n air-like: 
		/// \n light: 
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetAudioDefaultMode)(
        NRPluginHandle handle,
        NRAudioMode mode
    );
	/*
		
		/// @brief 增加 UAC 音频的音量
		/// \n air-like: 
		/// \n light: 
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *IncreaseAudioUacVolume)(
        NRPluginHandle handle
    );
	/*
		
		/// @brief 减少 UAC 音频的音量
		/// \n air-like: 
		/// \n light: 
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *DecreaseAudioUacVolume)(
        NRPluginHandle handle
    );
	/*
		
		/// @brief 获取 PA 音频最大音量（单位：dB）
		/// 暂不使用 请使用 thousandth 相关接口
		/// \n air-like:
		/// \n light:
		/// \n gina: new
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetAudioVolumeMaxValue)(
        NRPluginHandle handle,
        int32_t * value
    );
	/*
		
		/// @brief 获取 PA 音频最小音量 （单位：dB）
		/// 暂不使用 请使用 thousandth 相关接口
		/// \n air-like:
		/// \n light:
		/// \n gina: new
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetAudioVolumeMinValue)(
        NRPluginHandle handle,
        int32_t * value
    );
	/*
		
		/// @brief 获取 PA 音频当前音量（单位：dB）
		/// 暂不使用 请使用 thousandth 相关接口
		/// \n air-like:
		/// \n light:
		/// \n gina: new
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetAudioVolumeValue)(
        NRPluginHandle handle,
        int32_t * value
    );
	/*
		
		/// @brief 设置 PA 音频当前音量（单位：dB）
		/// 暂不使用 请使用 thousandth 相关接口
		/// \n air-like:
		/// \n light:
		/// \n gina: new
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetAudioVolumeValue)(
        NRPluginHandle handle,
        int32_t value
    );
	/*
		
		/// @brief 获取 PA 音频当前算法
		/// \n air-like:
		/// \n light:
		/// \n gina: new
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetAudioAlgorithm)(
        NRPluginHandle handle,
        NRAudioAlgorithmType * algorithm_type
    );
	/*
		
		/// @brief 设置 PA 音频当前算法
		/// \n light:
		/// \n gina: new
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetAudioAlgorithm)(
        NRPluginHandle handle,
        NRAudioAlgorithmType algorithm_type
    );
	/*
		
		/// @brief 获取 左右PA 打开/关闭 状态
		/// \n 可能被 host 修改，host 的静音控制会影响此状态，其实就是UAC中的 Host 静音状态
		/// \n 目前不要使用此接口
		/// \n air-like:
		/// \n light: 
		/// \n gina: new 
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetAudioPAEnable)(
        NRPluginHandle handle,
        NRAudioUsage audio_usage,
        NREnableValue * enable_value
    );
	/*
		
		/// @brief 打开/关闭左右PA
		/// \n 可能被 host 修改，host 的静音控制会影响此状态，其实就是UAC中的 Host 静音状态
		/// \n 目前不要使用此接口
		/// \n air-like:
		/// \n light: 
		/// \n gina: new 
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetAudioPAEnable)(
        NRPluginHandle handle,
        NRAudioUsage audio_usage,
        NREnableValue enable_value
    );
	/*
		
		/// @brief 获取 RGB Camera 插入/拔出状态, 不需要NRRgbCameraCreate即可使用
		/// \n air-like: 
		/// \n light:
		/// \n gina: new
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetRgbCameraPluginState)(
        NRPluginHandle handle,
        NRRgbCameraPluginState * state
    );
	/*
		
		/// @brief 获取眼镜电致变色的占空比 当前数值
		/// \n air-like:
		/// \n light:
		/// \n gina: new
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetEcValue)(
        NRPluginHandle handle,
        NRDisplayUsage display_usage,
        int32_t * value
    );
	/*
		
		/// @brief 设置眼镜电致变色的占空比当前数值
		/// \n air-like:
		/// \n light:
		/// \n gina: new
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetEcValue)(
        NRPluginHandle handle,
        NRDisplayUsage display_usage,
        int32_t value
    );
	/*
		
		/// @brief 获取温度值
		/// \n air-like: "MSG_R_TEMPLE_TEMPERATURE": "0x13",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetTemperatureValue)(
        NRPluginHandle handle,
        NRTemperatureID id,
        float * value
    );
	/*
		
		/// @brief 播音
		/// \n NRAudioPlay 播音时，包括了NRAudioSetPAForceSound的功能，即播音时会自动打开PA
		/// \n 播放音频的文件，需要：
		/// \n 1. WAV 格式文件
		/// \n 2. PCM 格式
		/// \n 3. 48k 采样率
		/// \n 4. 32bit 采样位宽
		/// \n air-like:
		/// \n light: 
		/// \n gina: new 
		/// @param[in] file_path_data 音频文件路径的指针
		/// @param[in] file_path_size 音频文件路径的长度
		/// @param[in] type 播音类型
		/// @param[in] volume_thousandth 如果是指定音量类型，则使用这个音量 千分比
		/// @param[in] user_data 用户数据
		/// @param[in] callback 声音播完回调函数, 如果为 NULL，则不回调
		/// @param[out] play_duration 输出将要播多少时间, 单位：毫秒
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *PlayAudio)(
        NRPluginHandle handle,
        const char * file_path_data,
        uint32_t file_path_size,
        NRAudioPlayType type,
        int32_t volume_thousandth,
        void * user_data,
        void * callback,
        int32_t * play_duration
    );
	/*
		
		/// @brief 获取 左右PA 静音功能的 打开/关闭 状态
		/// \n 不能被 host 修改，host 的静音控制不会影响此状态
		/// \n 目前不要使用此接口
		/// \n air-like:
		/// \n light: 
		/// \n gina: new 
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetAudioPAForceSilent)(
        NRPluginHandle handle,
        NRAudioUsage audio_usage,
        NREnableValue * enable_value
    );
	/*
		
		/// @brief 左右PA 静音功能 打开/关闭
		/// \n 不能被 host 修改，host 的静音控制不会影响此状态
		/// \n 与 NRAudioGetPAForceSound 一起使用时，后操作的有效
		/// \n 目前不要使用此接口
		/// \n air-like:
		/// \n light: 
		/// \n gina: new 
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetAudioPAForceSilent)(
        NRPluginHandle handle,
        NRAudioUsage audio_usage,
        NREnableValue enable_value
    );
	/*
		
		/// @brief 获取 左右PA 播音功能的 打开/关闭 状态
		/// \n 不能被 host 修改，host 的静音控制不会影响此状态
		/// \n NRAudioPlay 接口包括了此功能，所以一般不用此接口
		/// \n 目前不要使用此接口
		/// \n air-like:
		/// \n light: 
		/// \n gina: new 
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetAudioPAForceSound)(
        NRPluginHandle handle,
        NRAudioUsage audio_usage,
        NREnableValue * enable_value
    );
	/*
		
		/// @brief 左右PA 播音功能 打开/关闭
		/// \n 不能被 host 修改，host 的静音控制不会影响此状态
		/// \n 与 NRAudioSetPAForceSilent 一起使用时，后操作的有效
		/// \n NRAudioPlay 接口包括了此功能，所以一般不用此接口
		/// \n 目前不要使用此接口
		/// \n air-like:
		/// \n light: 
		/// \n gina: new 
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetAudioPAForceSound)(
        NRPluginHandle handle,
        NRAudioUsage audio_usage,
        NREnableValue enable_value
    );
	/*
		
		/// @brief 设置线程的 Fifo 优先级
		/// \n 类似 linux 中的 sched_setscheduler
		/// \n 
		/// \n air-like: 
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetMiscScheduler)(
        NRPluginHandle handle,
        int32_t pid,
        NRMiscSchedPolicy policy,
        int32_t priority
    );
	/*
		
		/// @brief 获取 PA 音频当前音量百分比
		/// \n value 音量百分比的值，取值范围 0-100
		/// \n air-like:
		/// \n light:
		/// \n gina: new
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetAudioVolumePercentage)(
        NRPluginHandle handle,
        int32_t * value
    );
	/*
		
		/// @brief 设置 PA 音频当前音量百分比
		/// \n value 音量百分比的值，取值范围 0-100
		/// \n air-like:
		/// \n light:
		/// \n gina: new
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetAudioVolumePercentage)(
        NRPluginHandle handle,
        int32_t value
    );
	/*
		
		/// @brief 获取 x/y轴上的 色温基准值
		/// \n display_usage OLED的左右目
		/// \n current_luminance_value 当前亮度值
		/// \n x_data 产品配置的x轴坐标值
		/// \n y_data 产品配置的y轴坐标值
		/// \n base_x_data 当前亮度下，x轴坐标的基准值
		/// \n base_y_data 当前亮度下，y轴坐标的基准值
		/// \n air-like: "MSG_R_OLED_COORDINATE": "0x4D",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetDisplayColorTemperatureBaseline)(
        NRPluginHandle handle,
        NRDisplayUsage display_usage,
        int32_t current_luminance_value,
        int32_t x_data,
        int32_t y_data,
        int32_t * base_x_data,
        int32_t * base_y_data
    );
	/*
		
		/// @brief 启动/停止 gamma校准
		/// \n air-like:
		/// \n light: 
		/// \n gina: new 
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetDisplayGammaEnable)(
        NRPluginHandle handle,
        NREnableValue enable_value
    );
	/*
		
		/// @brief 获取累计开机上电次数
		/// \n air-like:  
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetGlassesBootCount)(
        NRPluginHandle handle,
        int32_t * count
    );
	/*
		
		/// @brief 设置 开关屏幕
		/// \n 这个操作是同步的，会阻塞调用线程，直到操作完成，预估时间为100ms
		/// \n air-like: "MSG_W_CTRL_OLED": "0x72",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetDisplayScreenEnableBsp)(
        NRPluginHandle handle,
        NREnableValue enable_value
    );
	/*
		
		/// @brief 获取屏幕状态(开/关)
		/// \n air-like: "MSG_R_SCREEN_STATUS": "0xA1",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetDisplayScreenEnableBsp)(
        NRPluginHandle handle,
        NREnableValue * enable_value
    );
	/*
		
		/// @brief 获取 dp 当前 EDID 信息
		/// \n air-like: 
		/// \n light: 
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetDpCurrentEdidBsp)(
        NRPluginHandle handle,
        NREdid * edid
    );
	/*
		
		/// @brief 设置 dp 当前 EDID 信息
		/// \n air-like: 
		/// \n light: 
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetDpCurrentEdidBsp)(
        NRPluginHandle handle,
        NREdid edid
    );
	/*
		
		/// @brief 获取 dp 和 host 协商后的真实的分辨率/刷新率
		/// \n air-like: 
		/// \n light: 
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetDpCurrentResolutionBsp)(
        NRPluginHandle handle,
        NRResolutionInfo * resolution
    );
	/*
		
		/// @brief 获取系统版本号的数字代码
		/// \n air-like:
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetGlassesSystemVersionCode)(
        NRPluginHandle handle,
        const char ** version_code_data,
        uint32_t * version_code_size
    );
	/*
		
		/// @brief 获取产品对外名称
		/// \n air-like:  
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetGlassesProductName)(
        NRPluginHandle handle,
        const char ** product_name_data,
        uint32_t * product_name_size
    );
	/*
		
		/// @brief 获取磁盘是否可用
		/// \n 这个函数会帮组libstorage.so 判断 storage service 是否启动好了，如果没有启动好，会等待启动好。
		/// \n 所以这个必须是 storage 模块中，第一个调用的函数。理论上也应该是第一个调用的函数。
		/// \n air-like:
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetStorageAvailable)(
        NRPluginHandle handle,
        NRAvailableValue * available_value
    );
	/*
		
		/// @brief 获取磁盘总容量 单位：字节
		/// \n air-like:
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetStorageTotalSize)(
        NRPluginHandle handle,
        uint64_t * total_size
    );
	/*
		
		/// @brief 获取磁盘剩余容量 单位：字节
		/// \n air-like:
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetStorageFreeSize)(
        NRPluginHandle handle,
        uint64_t * free_size
    );
	/*
		
		/// @brief 清空磁盘（删除所有文件）
		/// \n 这个接口是调用的系统调用rm，这个用时和当前有多少个文件强相关。
		/// \n 五千个文件，用时有十秒以上。文件很少的话就几毫秒。
		/// \n 基于这个情况的话，建议清理空间就用NRStorageSetFormat接口。
		/// \n air-like:
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *ClearStorageAll)(
        NRPluginHandle handle
    );
	/*
		
		/// @brief 格式化磁盘
		/// \n air-like:
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetStorageFormat)(
        NRPluginHandle handle
    );
	/*
		
		/// @brief 设置磁盘的对外传输模式
		/// \n air-like:
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetStorageMode)(
        NRPluginHandle handle,
        NRStorageMode mode
    );
	/*
		
		/// @brief 获取磁盘的对外传输模式
		/// \n air-like:
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetStorageMode)(
        NRPluginHandle handle,
        NRStorageMode * mode
    );
	/*
		
		/// @brief 获取USB的VID
		/// \n air-like:  
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetGlassesUsbVid)(
        NRPluginHandle handle,
        const char ** vid_data,
        uint32_t * vid_size
    );
	/*
		
		/// @brief 获取USB的PID
		/// \n air-like:  
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetGlassesUsbPid)(
        NRPluginHandle handle,
        const char ** pid_data,
        uint32_t * pid_size
    );
	/*
		
		/// @brief 获取眼镜类型，用于区分不同的眼镜
		/// \n air-like: 
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetMiscDeviceType)(
        NRPluginHandle handle,
        NRMiscDeviceType * device_type
    );
	/*
		
		/// @brief 获取RGB Camera SN, 不需要NRRgbCameraCreate即可使用
		/// \n air-like: 
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetRgbCameraSN)(
        NRPluginHandle handle,
        const char ** sn_data,
        uint32_t * sn_size
    );
	/*
		
		/// @brief 获取RGB Camera 配置, 不需要NRRgbCameraCreate即可使用
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetRgbCameraConfig)(
        NRPluginHandle handle,
        const char ** data_data,
        uint32_t * data_size
    );
	/*
		
		/// @brief 设置RGB Camera 配置, 不需要NRRgbCameraCreate即可使用
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetRgbCameraConfig)(
        NRPluginHandle handle,
        const char * data_data,
        uint32_t data_size
    );
	/*
		
		/// @brief 获取 dp 数据流是否中断 ( 目前用于 RGB 拍照录像 )
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetDpDataInterruptEnable)(
        NRPluginHandle handle,
        NREnableValue * enable_value
    );
	/*
		
		/// @brief 设置 dp 数据是否中断 ( 目前用于 RGB 拍照录像 )
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetDpDataInterruptEnable)(
        NRPluginHandle handle,
        NREnableValue enable_value
    );
	/*
		
		/// @brief 获取 dp 数据传输模式
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetDpDataTransmitMode)(
        NRPluginHandle handle,
        NRDpDataTransmitMode * data_transmit_mode
    );
	/*
		
		/// @brief 设置 dp 数据传输模式
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetDpDataTransmitMode)(
        NRPluginHandle handle,
        NRDpDataTransmitMode data_transmit_mode
    );
	/*
		
		/// @brief 获取 dp 当前 EDID 信息 和 dp 音频是否开启
		/// \n air-like: 
		/// \n light: 
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetDpCurrentEdidAndAudioBsp)(
        NRPluginHandle handle,
        NREdid * edid,
        NREnableValue * dp_audio_enable
    );
	/*
		
		/// @brief 设置 dp 当前 EDID 信息 和 dp 音频是否开启
		/// \n air-like: 
		/// \n light: 
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetDpCurrentEdidAndAudioBsp)(
        NRPluginHandle handle,
        NREdid edid,
        NREnableValue dp_audio_enable
    );
	/*
		
		/// @brief 获取 PA 音频当前音量千分比
		/// \n value 音量百分比的值，取值范围 0-1000
		/// \n air-like:
		/// \n light:
		/// \n gina: new
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetAudioVolumeThousandth)(
        NRPluginHandle handle,
        int32_t * value
    );
	/*
		
		/// @brief 设置 PA 音频当前音量千分比
		/// \n value 音量百分比的值，取值范围 0-1000
		/// \n air-like:
		/// \n light:
		/// \n gina: new
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetAudioVolumeThousandth)(
        NRPluginHandle handle,
        int32_t value
    );
	/*
		
		/// @brief 获取眼镜当前的升级状态
		/// \n 会返回NR_MISC_SYSTEM_UPGRADE_STATE_NOT_RUNNING，NR_MISC_SYSTEM_UPGRADE_STATE_RUNNING
		/// \n air-like: 
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetMiscSystemUpgradeState)(
        NRPluginHandle handle,
        NRMiscSystemUpgradeState * state
    );
	/*
		
		/// @brief 获取 左右Host 播音功能的 打开/关闭 状态
		/// \n 打开时， Host 的声音不会输出到左右声道
		/// \n 关闭时， Host 的声音会输出到左右声道
		/// \n 目前一般使用这个接口
		/// \n air-like:
		/// \n light: 
		/// \n gina: new 
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetAudioHostForceSilent)(
        NRPluginHandle handle,
        NRAudioUsage audio_usage,
        NREnableValue * enable_value
    );
	/*
		
		/// @brief 设置 左右Host 播音功能的 打开/关闭 状态
		/// \n 打开时， Host 的声音不会输出到左右声道
		/// \n 关闭时， Host 的声音会输出到左右声道
		/// \n 目前一般使用这个接口
		/// \n air-like:
		/// \n light: 
		/// \n gina: new 
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetAudioHostForceSilent)(
        NRPluginHandle handle,
        NRAudioUsage audio_usage,
        NREnableValue enable_value
    );
	/*
		
		/// @brief 获取GlassesSN Value（新版本十六位SN）参考：https://xreal.feishu.cn/docx/NldIdu0U4oZgM3xIsYGcHkfMnqp
		/// \n air-like: "MSG_R_RESERVED_SN0": "0x57",
		/// \n light:
		/// @param index 获取 sn 的索引: 其中: 0: SN0 对应文档中的位置一, 1: SN1 对应文档中的位置二
		/// @param code: 客户代码
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetGlassesSNCode)(
        NRPluginHandle handle,
        int32_t index,
        const char ** code_data,
        uint32_t * code_size
    );
	/*
		
		/// @brief 获取GlassesSN Value（新版本十六位SN）参考：https://xreal.feishu.cn/docx/NldIdu0U4oZgM3xIsYGcHkfMnqp
		/// \n air-like: "MSG_R_RESERVED_SN0": "0x57",
		/// \n light:
		/// @param index 获取 sn 的索引: 其中: 0: SN0 对应文档中的位置一, 1: SN1 对应文档中的位置二
		/// @param value: 眼镜的SN
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetGlassesSNValue)(
        NRPluginHandle handle,
        int32_t index,
        const char ** value_data,
        uint32_t * value_size
    );
	/*

	*/
    NRPluginResult(NR_INTERFACE_API *GetMiscHostType)(
        NRPluginHandle handle,
        NRMiscHostType * type
    );
	/*
		
		/// @brief 设置 CPU 频率(模式)
		/// \n air-like: 
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetGlassesCpuFrequencyMode)(
        NRPluginHandle handle,
        NRPowerCpuFrequencyMode mode
    );
	/*
		
		/// @brief 获取 CPU 频率(模式)
		/// \n air-like:
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetGlassesCpuFrequencyMode)(
        NRPluginHandle handle,
        NRPowerCpuFrequencyMode * mode
    );
} NRGlassesInvokeProvider;

typedef struct NRGlassesNotifyProvider {
	/*
		
		/// @brief 设置发送功能回调函数
		
	*/
    NRPluginResult(NR_INTERFACE_API *SubmitEventData)(
        NRPluginHandle handle,
        const NREventReportData * data,
        uint32_t data_size
    );
	/*
		
		/// @brief 设置发送功能回调函数
		
	*/
    NRPluginResult(NR_INTERFACE_API *SubmitAction)(
        NRPluginHandle handle,
        const NRActionData * data,
        uint32_t data_size
    );
	/*
		
		/// @brief 设置PowerSave上报的回调函数
		/// \n air-like:
		/// \n light:
		/// \n gina: new
		/// @param[in] callback 回调函数指针
		
	*/
    NRPluginResult(NR_INTERFACE_API *SubmitPowerSaveState)(
        NRPluginHandle handle,
        NRPowerSaveState state
    );
	/*
		
		/// @brief 设置温度状态上报的回调函数
		/// \n air-like:
		/// \n light:
		/// \n gina: new
		/// @param[in] callback 回调函数指针
		
	*/
    NRPluginResult(NR_INTERFACE_API *SubmitTemperatureState)(
        NRPluginHandle handle,
        NRTemperatureState state
    );
} NRGlassesNotifyProvider;

typedef struct NRGlassesExecuteProvider {
	/*
		
		/// @brief  获取是否使能深休眠(断电）
		/// \n air-like: "MSG_R_SLEEP_MODE": "0x56",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *IsPowerSaveEnable)(
        NRPluginHandle handle,
        int32_t * data
    );
	/*
		
		/// @brief 设置是否使能深休眠(断电）
		/// \n air-like: "MSG_W_SLEEP_MODE": "0x55",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetPowerSaveEnable)(
        NRPluginHandle handle,
        int32_t data
    );
	/*
		
		/// @brief 获取关闭屏幕后多少时间进入深休眠
		/// \n air-like: "MSG_R_SLEEP_TIME": "0x1D",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetPowerSaveSleepTime)(
        NRPluginHandle handle,
        int32_t * data
    );
	/*
		
		/// @brief 设置关闭屏幕后多少时间进入深休眠
		/// \n air-like: "MSG_W_SLEEP_TIME": "0x1E",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetPowerSaveSleepTime)(
        NRPluginHandle handle,
        int32_t data
    );
	/*
		
		/// @brief 获取 是否使用接近传感器（ 浅睡眠开关 ）
		/// \n air-like: "MSG_R_PSENSOR_SWITCH": "0x6A",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *IsProximityEnable)(
        NRPluginHandle handle,
        int32_t * data
    );
	/*
		
		/// @brief 设置 是否使用接近传感器（ 浅睡眠开关 ）
		/// \n air-like: MSG_W_PSENSOR_SWITCH": "0x6B",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetProximityEnable)(
        NRPluginHandle handle,
        int32_t data
    );
	/*
		
		/// @brief 获取眼镜（亮度-占空比)档位的个数
		/// \n air-like: "MSG_R_GLASSES_BRIGHTNESS_LEVEL_NUM": "0x7B",
		/// \n light:
		/// @return 档位个数
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetDisplayBrightnessLevelCount)(
        NRPluginHandle handle,
        int32_t * data
    );
	/*
		
		/// @brief 获取眼镜当前（亮度-占空比)档位
		/// \n air-like: "MSG_R_BRIGHTNESS_LEVEL": "0x03",
		/// \n light:
		/// @return 档位
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetDisplayBrightnessLevel)(
        NRPluginHandle handle,
        int32_t * data
    );
	/*
		
		/// @brief 设置眼镜当前（亮度-占空比)档位
		/// \n air-like: "MSG_W_BRIGHTNESS_LEVEL": "0x04",
		/// \n light:
		/// @param[in] 档位
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetDisplayBrightnessLevel)(
        NRPluginHandle handle,
        int32_t data
    );
	/*
		
		/// @brief 获取眼镜pilot版本号
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetGlassesSWVersion)(
        NRPluginHandle handle,
        const char ** data_data,
        uint32_t * data_size
    );
	/*
		
		/// @brief 设置 NRSDK 版本号
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetGlassesSDKVersion)(
        NRPluginHandle handle,
        const char * data_data,
        uint32_t data_size
    );
	/*
		
		/// @brief 获取眼镜配置
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetGlassesConfig)(
        NRPluginHandle handle,
        const char ** data_data,
        uint32_t * data_size
    );
	/*
		
		/// @brief 获取 支持的 设备
		/// \n air-like: "MSG_R_DEV_LIST": "0x34",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetGlassesSupportedDevices)(
        NRPluginHandle handle,
        uint64_t * data
    );
	/*
		
		/// @brief 获取 vsync 前后的 blank 时间 (vblank)
		/// \n air-like: "MSG_R_DEVICE_PARAMETERS": "0x17",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetGlassesVsyncOffsetTime)(
        NRPluginHandle handle,
        uint64_t * front,
        uint64_t * end
    );
	/*
		
		/// @brief 读取磁力计校准数据
		/// \n air-like: "MSG_R_MAG_CALIBR_DATA": "0x1B",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetGlassesMagCalibrationData)(
        NRPluginHandle handle,
        const char ** data_data,
        uint32_t * data_size
    );
	/*
		
		/// @brief 设置磁力计校准数据
		/// \n air-like: "MSG_W_MAG_CALIBR_DATA": "0x1C",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetGlassesMagCalibrationData)(
        NRPluginHandle handle,
        const char * data_data,
        uint32_t data_size
    );
	/*
		
		/// @brief 开始传输 眼镜的错误和事件报告
		/// \n air-like: "MSG_N_START_ERRORS_AND_EVENTS_REPORT": "0x73",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *StartGlassesEventsReport)(
        NRPluginHandle handle,
        int32_t data
    );
	/*
		
		/// @brief 获取 电致变色档位的个数
		/// \n 目前总共四档
		/// \n 透过率：0.1%（最暗）、3%、7%、15%（最透/通透模式）
		/// \n 对应的设置duty：100%、35%、15%、0%
		/// \n 上述的 duty 是电致变色的 duty，不是屏幕的。
		/// \n air-like: "MSG_R_EC_GEAR_NUM": "0xB0",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetEcLevelCount)(
        NRPluginHandle handle,
        int32_t * data
    );
	/*
		
		/// @brief 获取 电致变色当前档位
		/// \n air-like: "MSG_R_EC_GEAR_NOW": "0xB1",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetEcLevel)(
        NRPluginHandle handle,
        int32_t * data
    );
	/*
		
		/// @brief 设置 电致变色当前档位
		/// \n air-like:  "MSG_W_SET_EC_GEAR": "0xB2",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetEcLevel)(
        NRPluginHandle handle,
        int32_t data
    );
	/*
		
		/// @brief 设置 开关屏幕
		/// \n 这个操作是同步的，会阻塞调用线程，直到操作完成，预估时间为100ms
		/// \n air-like: "MSG_W_CTRL_OLED": "0x72",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetDisplayScreenEnable)(
        NRPluginHandle handle,
        NREnableValue enable_value,
        NRDisplayScreenEnableMethod method
    );
	/*
		
		/// @brief 获取屏幕状态(开/关)
		/// \n air-like: "MSG_R_SCREEN_STATUS": "0xA1",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetDisplayScreenEnable)(
        NRPluginHandle handle,
        NREnableValue * enable_value
    );
	/*
		
		/// @brief 获取眼镜屏幕颜色校准方式
		/// \n air-like:   "MSG_R_SWITCH_CAL_OLED": "0xB6",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetDisplayColorCalibrationType)(
        NRPluginHandle handle,
        NRDisplayColorCalibrationType * type
    );
	/*
		
		/// @brief 设置 眼镜屏幕颜色校准方式，0：不校准，1：索尼或光学校准方案（光学优先）
		/// \n air-like: "MSG_W_SWITCH_CAL_OLED": "0xB7",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetDisplayColorCalibrationType)(
        NRPluginHandle handle,
        NRDisplayColorCalibrationType type
    );
	/*
		
		/// @brief 获取 dp 当前 EDID 信息
		/// \n air-like: 
		/// \n light: 
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetDpCurrentEdid)(
        NRPluginHandle handle,
        NREdid * edid
    );
	/*
		
		/// @brief 设置 dp 当前 EDID 信息
		/// \n air-like: 
		/// \n light: 
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetDpCurrentEdid)(
        NRPluginHandle handle,
        NREdid edid
    );
	/*
		
		/// @brief 获取 dp 和 host 协商后的真实的分辨率/刷新率
		/// \n air-like: 
		/// \n light: 
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetDpCurrentResolution)(
        NRPluginHandle handle,
        NRResolutionInfo * resolution
    );
	/*
		
		/// @brief 获取关闭屏幕后多少时间进入深休眠的档位数量
		/// \n air-like:
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetPowerSaveSleepTimeLevelCount)(
        NRPluginHandle handle,
        int32_t * level_count
    );
	/*
		
		/// @brief 获取关闭屏幕后多少时间进入深休眠的档位
		/// \n air-like:
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetPowerSaveSleepTimeLevel)(
        NRPluginHandle handle,
        int32_t * level
    );
	/*
		
		/// @brief 设置关闭屏幕后多少时间进入深休眠的档位
		/// \n air-like: 
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetPowerSaveSleepTimeLevel)(
        NRPluginHandle handle,
        int32_t level
    );
	/*
		
		/// @brief 获取 眼镜的开机流程状态
		/// \n air-like: 
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetGlassesStartupState)(
        NRPluginHandle handle,
        NRGlassesStartupState * data
    );
	/*
		
		/// @brief 停止传输 眼镜的错误和事件报告
		/// \n air-like:
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *StopGlassesEventsReport)(
        NRPluginHandle handle,
        int32_t data
    );
	/*
		
		/// @brief 获取眼镜色温档位的个数
		/// \n air-like:
		/// \n light:
		/// @return 档位个数
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetDisplayColorTemperatureLevelCount)(
        NRPluginHandle handle,
        int32_t * data
    );
	/*
		
		/// @brief 获取眼镜当前色温档位
		/// \n air-like:
		/// \n light:
		/// @return 色温档位
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetDisplayColorTemperatureLevel)(
        NRPluginHandle handle,
        int32_t * data
    );
	/*
		
		/// @brief 设置眼镜当前色温档位
		/// \n air-like:
		/// \n light:
		/// @param[in] 色温档位
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetDisplayColorTemperatureLevel)(
        NRPluginHandle handle,
        int32_t data
    );
	/*
		
		/// @brief 获取 单双目信息, 用于控制硬件（OSD）的显示，
		/// 用于在软件 render（chameleon）生效前保证硬件显示正确
		/// \n air-like: 
		/// \n light: 
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetDpInputMode)(
        NRPluginHandle handle,
        NRDpInputMode * input_mode
    );
	/*
		
		/// @brief 设置 单双目信息, 用于控制硬件（OSD）的显示，
		/// 用于在软件 render（chameleon）生效前保证硬件显示正确
		/// \n air-like: 
		/// \n light: 
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetDpInputMode)(
        NRPluginHandle handle,
        NRDpInputMode input_mode
    );
	/*
		
		/// @brief 获取 过温/高温(温度状态) 功能是否开启
		/// \n air-like: 
		/// \n light: 
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetTemperatureStateProcessEnable)(
        NRPluginHandle handle,
        NRTemperatureState state,
        NREnableValue * enable_value
    );
	/*
		
		/// @brief 设置 过温/高温(温度状态) 功能是否开启
		/// \n air-like: 
		/// \n light: 
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetTemperatureStateProcessEnable)(
        NRPluginHandle handle,
        NRTemperatureState state,
        NREnableValue enable_value
    );
	/*
		
		/// @brief 获取 超宽屏模式是否开启
		/// \n air-like: 
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetGlassesUltraWideEnable)(
        NRPluginHandle handle,
        NREnableValue * enable_value
    );
	/*
		
		/// @brief 设置 超宽屏模式是否开启
		/// \n air-like: 
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetGlassesUltraWideEnable)(
        NRPluginHandle handle,
        NREnableValue enable_value
    );
	/*
		
		/// @brief 画面校准
		/// \n air-like: 
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *RecenterGlasses)(
        NRPluginHandle handle
    );
} NRGlassesExecuteProvider;

NR_DECLARE_INTERFACE(NRGlassesSubmitInterface) {

    NRPluginResult(NR_INTERFACE_API *SubmitHostOrDeviceStatus)(
        NRPluginHandle handle,
        NREnableValue status
    );
	/*
		
		/// @brief 设置按键状态回调函数
		/// \n 函数指针定义如下：
		/// \n int32_t (*NRKeySubmitState_DispatchCallback)(const NRKeyStateData * data, uint32_t data_size)
		/// \n data: 按键信息
		/// \n data_size: 按键信息结构体大小
		/// @param[in] callback 回调函数指针
		
	*/
    NRPluginResult(NR_INTERFACE_API *SubmitKeyState)(
        NRPluginHandle handle,
        const NRKeyStateData * data,
        uint32_t data_size
    );
	/*
		
		/// @brief 设置接近传感器回调函数
		/// \n air-like:
		/// \n light:
		/// \n gina: new
		/// @param[in] callback 回调函数指针
		
	*/
    NRPluginResult(NR_INTERFACE_API *SubmitProximityWearingState)(
        NRPluginHandle handle,
        NRProximityWearingState value
    );
	/*
		
		/// @brief 设置Display上报状态的回调函数
		/// \n air-like:
		/// \n light:
		/// \n gina: new
		/// \n 函数指针定义如下：
		/// \n int32_t (*NRDisplaySubmitState_DispatchCallback)(NREnableValue enable_value)
		/// \n enable_value: Display 状态 (NR_ENABLE_VALUE_DISABLE:关闭，NR_ENABLE_VALUE_ENABLE:打开)
		/// @param[in] callback 回调函数指针
		
	*/
    NRPluginResult(NR_INTERFACE_API *SubmitDisplayState)(
        NRPluginHandle handle,
        NREnableValue enable_value
    );
	/*
		
		/// @brief 设置DP上报工作状态的回调函数
		/// \n air-like:
		/// \n light:
		/// \n gina: new
		/// @param[in] callback 回调函数指针
		
	*/
    NRPluginResult(NR_INTERFACE_API *SubmitDpWorkingState)(
        NRPluginHandle handle,
        NRDpWorkingState state
    );
	/*
		
		/// @brief 设置RGB Camera 插入/拔出状态回调函数, 不需要NRRgbCameraCreate即可使用
		/// \n air-like:
		/// \n light:
		/// \n gina: new
		/// @param[in] callback 回调函数指针
		
	*/
    NRPluginResult(NR_INTERFACE_API *SubmitRgbCameraPluginState)(
        NRPluginHandle handle,
        NRRgbCameraPluginState state
    );
	/*
		
		/// @brief 设置温度数据上报的回调函数
		/// \n 函数指针定义如下：
		/// \n int32_t (*NRTemperatureSubmitData_DispatchCallback)(const NRTemperatureData* data, uint32_t data_size)
		/// \n data: 温度信息
		/// \n data_size: 温度信息结构体大小
		/// @param[in] callback 回调函数指针
		
	*/
    NRPluginResult(NR_INTERFACE_API *SubmitTemperatureData)(
        NRPluginHandle handle,
        const NRTemperatureData * data,
        uint32_t data_size
    );
	/*
		
		/// @brief 上报音频算法发生变化(包括pilot 设置的算法和Host系统通过 UVC 设置的算法)
		/// \n 函数指针定义如下：
		/// \n int32_t (*NRAudioSubmitAlgorithmChange_DispatchCallback)(NRAudioAlgorithmType algorithm_type)
		/// \n algorithm_type: 变化后的算法类型
		/// \n air-like:
		/// \n light:
		/// \n gina: new
		/// @param[in] callback 回调函数指针
		
	*/
    NRPluginResult(NR_INTERFACE_API *SubmitAudioAlgorithmChange)(
        NRPluginHandle handle,
        NRAudioAlgorithmType algorithm_type
    );
	/*
		
		/// @brief 设置眼镜当前的升级状态上报的回调函数
		/// \n 会返回NR_MISC_SYSTEM_UPGRADE_STATE_START 和 NR_MISC_SYSTEM_UPGRADE_STATE_END
		/// \n air-like:
		/// \n light:
		/// \n gina: new
		/// \n 函数指针定义如下：
		/// \n int32_t (*NRMiscSubmitSystemUpgradeState_DispatchCallback)(NRMiscSystemUpgradeState state)
		/// @param[in] callback 回调函数指针
		
	*/
    NRPluginResult(NR_INTERFACE_API *SubmitMiscSystemUpgradeState)(
        NRPluginHandle handle,
        NRMiscSystemUpgradeState state
    );
};

NR_REGISTER_INTERFACE_GUID(0x96B93C9A7CB24D82ULL, 0xA89C0A602CED889DULL,
                            NRGlassesSubmitInterface)

NR_DECLARE_INTERFACE(NRGlassesExecuteInterface) {
	/*
		
		/// @brief  获取是否使能深休眠(断电）
		/// \n air-like: "MSG_R_SLEEP_MODE": "0x56",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *IsPowerSaveEnable)(
        NRPluginHandle handle,
        int32_t * data
    );
	/*
		
		/// @brief 设置是否使能深休眠(断电）
		/// \n air-like: "MSG_W_SLEEP_MODE": "0x55",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetPowerSaveEnable)(
        NRPluginHandle handle,
        int32_t data
    );
	/*
		
		/// @brief 获取关闭屏幕后多少时间进入深休眠
		/// \n air-like: "MSG_R_SLEEP_TIME": "0x1D",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetPowerSaveSleepTime)(
        NRPluginHandle handle,
        int32_t * data
    );
	/*
		
		/// @brief 设置关闭屏幕后多少时间进入深休眠
		/// \n air-like: "MSG_W_SLEEP_TIME": "0x1E",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetPowerSaveSleepTime)(
        NRPluginHandle handle,
        int32_t data
    );
	/*
		
		/// @brief 获取 是否使用接近传感器（ 浅睡眠开关 ）
		/// \n air-like: "MSG_R_PSENSOR_SWITCH": "0x6A",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *IsProximityEnable)(
        NRPluginHandle handle,
        int32_t * data
    );
	/*
		
		/// @brief 设置 是否使用接近传感器（ 浅睡眠开关 ）
		/// \n air-like: MSG_W_PSENSOR_SWITCH": "0x6B",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetProximityEnable)(
        NRPluginHandle handle,
        int32_t data
    );
	/*
		
		/// @brief 获取眼镜（亮度-占空比)档位的个数
		/// \n air-like: "MSG_R_GLASSES_BRIGHTNESS_LEVEL_NUM": "0x7B",
		/// \n light:
		/// @return 档位个数
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetDisplayBrightnessLevelCount)(
        NRPluginHandle handle,
        int32_t * data
    );
	/*
		
		/// @brief 获取眼镜当前（亮度-占空比)档位
		/// \n air-like: "MSG_R_BRIGHTNESS_LEVEL": "0x03",
		/// \n light:
		/// @return 档位
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetDisplayBrightnessLevel)(
        NRPluginHandle handle,
        int32_t * data
    );
	/*
		
		/// @brief 设置眼镜当前（亮度-占空比)档位
		/// \n air-like: "MSG_W_BRIGHTNESS_LEVEL": "0x04",
		/// \n light:
		/// @param[in] 档位
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetDisplayBrightnessLevel)(
        NRPluginHandle handle,
        int32_t data
    );
	/*
		
		/// @brief 获取眼镜pilot版本号
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetGlassesSWVersion)(
        NRPluginHandle handle,
        const char ** data_data,
        uint32_t * data_size
    );
	/*
		
		/// @brief 设置 NRSDK 版本号
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetGlassesSDKVersion)(
        NRPluginHandle handle,
        const char * data_data,
        uint32_t data_size
    );
	/*
		
		/// @brief 获取眼镜配置
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetGlassesConfig)(
        NRPluginHandle handle,
        const char ** data_data,
        uint32_t * data_size
    );
	/*
		
		/// @brief 获取 支持的 设备
		/// \n air-like: "MSG_R_DEV_LIST": "0x34",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetGlassesSupportedDevices)(
        NRPluginHandle handle,
        uint64_t * data
    );
	/*
		
		/// @brief 获取 vsync 前后的 blank 时间 (vblank)
		/// \n air-like: "MSG_R_DEVICE_PARAMETERS": "0x17",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetGlassesVsyncOffsetTime)(
        NRPluginHandle handle,
        uint64_t * front,
        uint64_t * end
    );
	/*
		
		/// @brief 读取磁力计校准数据
		/// \n air-like: "MSG_R_MAG_CALIBR_DATA": "0x1B",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetGlassesMagCalibrationData)(
        NRPluginHandle handle,
        const char ** data_data,
        uint32_t * data_size
    );
	/*
		
		/// @brief 设置磁力计校准数据
		/// \n air-like: "MSG_W_MAG_CALIBR_DATA": "0x1C",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetGlassesMagCalibrationData)(
        NRPluginHandle handle,
        const char * data_data,
        uint32_t data_size
    );
	/*
		
		/// @brief 开始传输 眼镜的错误和事件报告
		/// \n air-like: "MSG_N_START_ERRORS_AND_EVENTS_REPORT": "0x73",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *StartGlassesEventsReport)(
        NRPluginHandle handle,
        int32_t data
    );
	/*
		
		/// @brief 获取 电致变色档位的个数
		/// \n 目前总共四档
		/// \n 透过率：0.1%（最暗）、3%、7%、15%（最透/通透模式）
		/// \n 对应的设置duty：100%、35%、15%、0%
		/// \n 上述的 duty 是电致变色的 duty，不是屏幕的。
		/// \n air-like: "MSG_R_EC_GEAR_NUM": "0xB0",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetEcLevelCount)(
        NRPluginHandle handle,
        int32_t * data
    );
	/*
		
		/// @brief 获取 电致变色当前档位
		/// \n air-like: "MSG_R_EC_GEAR_NOW": "0xB1",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetEcLevel)(
        NRPluginHandle handle,
        int32_t * data
    );
	/*
		
		/// @brief 设置 电致变色当前档位
		/// \n air-like:  "MSG_W_SET_EC_GEAR": "0xB2",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetEcLevel)(
        NRPluginHandle handle,
        int32_t data
    );
	/*
		
		/// @brief 设置 开关屏幕
		/// \n 这个操作是同步的，会阻塞调用线程，直到操作完成，预估时间为100ms
		/// \n air-like: "MSG_W_CTRL_OLED": "0x72",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetDisplayScreenEnable)(
        NRPluginHandle handle,
        NREnableValue enable_value,
        NRDisplayScreenEnableMethod method
    );
	/*
		
		/// @brief 获取屏幕状态(开/关)
		/// \n air-like: "MSG_R_SCREEN_STATUS": "0xA1",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetDisplayScreenEnable)(
        NRPluginHandle handle,
        NREnableValue * enable_value
    );
	/*
		
		/// @brief 获取眼镜屏幕颜色校准方式
		/// \n air-like:   "MSG_R_SWITCH_CAL_OLED": "0xB6",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetDisplayColorCalibrationType)(
        NRPluginHandle handle,
        NRDisplayColorCalibrationType * type
    );
	/*
		
		/// @brief 设置 眼镜屏幕颜色校准方式，0：不校准，1：索尼或光学校准方案（光学优先）
		/// \n air-like: "MSG_W_SWITCH_CAL_OLED": "0xB7",
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetDisplayColorCalibrationType)(
        NRPluginHandle handle,
        NRDisplayColorCalibrationType type
    );
	/*
		
		/// @brief 获取 dp 当前 EDID 信息
		/// \n air-like: 
		/// \n light: 
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetDpCurrentEdid)(
        NRPluginHandle handle,
        NREdid * edid
    );
	/*
		
		/// @brief 设置 dp 当前 EDID 信息
		/// \n air-like: 
		/// \n light: 
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetDpCurrentEdid)(
        NRPluginHandle handle,
        NREdid edid
    );
	/*
		
		/// @brief 获取 dp 和 host 协商后的真实的分辨率/刷新率
		/// \n air-like: 
		/// \n light: 
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetDpCurrentResolution)(
        NRPluginHandle handle,
        NRResolutionInfo * resolution
    );
	/*
		
		/// @brief 获取关闭屏幕后多少时间进入深休眠的档位数量
		/// \n air-like:
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetPowerSaveSleepTimeLevelCount)(
        NRPluginHandle handle,
        int32_t * level_count
    );
	/*
		
		/// @brief 获取关闭屏幕后多少时间进入深休眠的档位
		/// \n air-like:
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetPowerSaveSleepTimeLevel)(
        NRPluginHandle handle,
        int32_t * level
    );
	/*
		
		/// @brief 设置关闭屏幕后多少时间进入深休眠的档位
		/// \n air-like: 
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetPowerSaveSleepTimeLevel)(
        NRPluginHandle handle,
        int32_t level
    );
	/*
		
		/// @brief 获取 眼镜的开机流程状态
		/// \n air-like: 
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetGlassesStartupState)(
        NRPluginHandle handle,
        NRGlassesStartupState * data
    );
	/*
		
		/// @brief 停止传输 眼镜的错误和事件报告
		/// \n air-like:
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *StopGlassesEventsReport)(
        NRPluginHandle handle,
        int32_t data
    );
	/*
		
		/// @brief 获取眼镜色温档位的个数
		/// \n air-like:
		/// \n light:
		/// @return 档位个数
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetDisplayColorTemperatureLevelCount)(
        NRPluginHandle handle,
        int32_t * data
    );
	/*
		
		/// @brief 获取眼镜当前色温档位
		/// \n air-like:
		/// \n light:
		/// @return 色温档位
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetDisplayColorTemperatureLevel)(
        NRPluginHandle handle,
        int32_t * data
    );
	/*
		
		/// @brief 设置眼镜当前色温档位
		/// \n air-like:
		/// \n light:
		/// @param[in] 色温档位
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetDisplayColorTemperatureLevel)(
        NRPluginHandle handle,
        int32_t data
    );
	/*
		
		/// @brief 获取 单双目信息, 用于控制硬件（OSD）的显示，
		/// 用于在软件 render（chameleon）生效前保证硬件显示正确
		/// \n air-like: 
		/// \n light: 
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetDpInputMode)(
        NRPluginHandle handle,
        NRDpInputMode * input_mode
    );
	/*
		
		/// @brief 设置 单双目信息, 用于控制硬件（OSD）的显示，
		/// 用于在软件 render（chameleon）生效前保证硬件显示正确
		/// \n air-like: 
		/// \n light: 
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetDpInputMode)(
        NRPluginHandle handle,
        NRDpInputMode input_mode
    );
	/*
		
		/// @brief 获取 过温/高温(温度状态) 功能是否开启
		/// \n air-like: 
		/// \n light: 
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetTemperatureStateProcessEnable)(
        NRPluginHandle handle,
        NRTemperatureState state,
        NREnableValue * enable_value
    );
	/*
		
		/// @brief 设置 过温/高温(温度状态) 功能是否开启
		/// \n air-like: 
		/// \n light: 
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetTemperatureStateProcessEnable)(
        NRPluginHandle handle,
        NRTemperatureState state,
        NREnableValue enable_value
    );
	/*
		
		/// @brief 获取 超宽屏模式是否开启
		/// \n air-like: 
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *GetGlassesUltraWideEnable)(
        NRPluginHandle handle,
        NREnableValue * enable_value
    );
	/*
		
		/// @brief 设置 超宽屏模式是否开启
		/// \n air-like: 
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *SetGlassesUltraWideEnable)(
        NRPluginHandle handle,
        NREnableValue enable_value
    );
	/*
		
		/// @brief 画面校准
		/// \n air-like: 
		/// \n light:
		/// @return 结果
		
	*/
    NRPluginResult(NR_INTERFACE_API *RecenterGlasses)(
        NRPluginHandle handle
    );
};

NR_REGISTER_INTERFACE_GUID(0x24C032F452204443ULL, 0x878060F2E2F0F012ULL,
                            NRGlassesExecuteInterface)

NR_DECLARE_INTERFACE(NRGlassesNotifyInterface) {
	/*
		
		/// @brief 设置发送功能回调函数
		
	*/
    NRPluginResult(NR_INTERFACE_API *SubmitEventData)(
        NRPluginHandle handle,
        const NREventReportData * data,
        uint32_t data_size
    );
	/*
		
		/// @brief 设置发送功能回调函数
		
	*/
    NRPluginResult(NR_INTERFACE_API *SubmitAction)(
        NRPluginHandle handle,
        const NRActionData * data,
        uint32_t data_size
    );
	/*
		
		/// @brief 设置PowerSave上报的回调函数
		/// \n air-like:
		/// \n light:
		/// \n gina: new
		/// @param[in] callback 回调函数指针
		
	*/
    NRPluginResult(NR_INTERFACE_API *SubmitPowerSaveState)(
        NRPluginHandle handle,
        NRPowerSaveState state
    );
	/*
		
		/// @brief 设置温度状态上报的回调函数
		/// \n air-like:
		/// \n light:
		/// \n gina: new
		/// @param[in] callback 回调函数指针
		
	*/
    NRPluginResult(NR_INTERFACE_API *SubmitTemperatureState)(
        NRPluginHandle handle,
        NRTemperatureState state
    );
};

NR_REGISTER_INTERFACE_GUID(0xA888E824E3C44758ULL, 0xB438361CFD222336ULL,
                            NRGlassesNotifyInterface)

NR_DECLARE_INTERFACE(NRGlassesInterface) {

    NRPluginResult(NR_INTERFACE_API *RegisterLifecycleProvider)(
        const char * plugin_id,
        const char * plugin_version,
        const NRPluginLifecycleProvider * provider,
        uint32_t provider_size
    );

    NRPluginResult(NR_INTERFACE_API *RegisterInvokeProvider)(
        NRPluginHandle handle,
        const NRGlassesInvokeProvider * provider,
        uint32_t provider_size
    );

    NRPluginResult(NR_INTERFACE_API *RegisterNotifyProvider)(
        NRPluginHandle handle,
        const NRGlassesNotifyProvider * provider,
        uint32_t provider_size
    );

    NRPluginResult(NR_INTERFACE_API *RegisterExecuteProvider)(
        NRPluginHandle handle,
        const NRGlassesExecuteProvider * provider,
        uint32_t provider_size
    );
};

NR_REGISTER_INTERFACE_GUID(0xCAA6989A22834B6CULL, 0xB179BF2C9CA842F5ULL,
                            NRGlassesInterface)

