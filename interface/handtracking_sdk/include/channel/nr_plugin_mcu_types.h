#pragma once

#include "nr_plugin_types.h"

#ifdef NRSDK

#include "nr_plugin_mcu_types.inc"

#else

NR_PLUGIN_ENUM(NRReportLevel) {
    NR_REPORT_LEVEL_SERVICE = 1,
    NR_REPORT_LEVEL_FATAL,
    NR_REPORT_LEVEL_ERROR,
    NR_REPORT_LEVEL_WARN,
    NR_REPORT_LEVEL_EVENT,
};


NR_PLUGIN_ENUM(NRMcuStatus) {
    NR_MCU_STATUS_NEAR = 1,
    NR_MCU_STATUS_AWAY,
    NR_MCU_STATUS_CABLEOFF,
    NR_MCU_STATUS_KEY_BRIGHTNESS_UP_ONE_CLICK,
    NR_MCU_STATUS_KEY_BRIGHTNESS_DOWN_ONE_CLICK,
    NR_MCU_STATUS_7211_UPDATE_RUNNING,
    NR_MCU_STATUS_7211_UPDATE_FINISHED,
    NR_MCU_STATUS_7211_UPDATE_FAILED,
    NR_MCU_STATUS_BRIGHTNESS_NOTIFY,
    NR_MCU_STATUS_GO_TO_SLEEP,
    NR_MCU_STATUS_LIGHT_INTENSITY_NOTIFY,
    NR_MCU_STATUS_SCREEN_STATUS_NOTIFY,
    NR_MCU_STATUS_OVER_TEMPERATURE_NOTIFY,
    NR_MCU_STATUS_IMU_NOT_SUBMIT,
    NR_MCU_STATUS_HARDWARE_ERROR = 10000,
};


NR_PLUGIN_ENUM(NRTemperatureType) {
    NR_TEMPERATURE_TYPE_GLASSES_FOREHEAD = 1,
    NR_TEMPERATURE_TYPE_GLASSES_TEMPLE,
};


NR_PLUGIN_ENUM(NRMcuKeyFunction) {
    NR_MCU_KEY_FUNC_CLICK = 1,
    NR_MCU_KEY_FUNC_DOUBLE_CLICK,
    NR_MCU_KEY_FUNC_LONG_PRESS,
    NR_MCU_KEY_FUNC_OPEN_DISPLAY,
    NR_MCU_KEY_FUNC_CLOSE_DISPLAY,
    NR_MCU_KEY_FUNC_INCREASE_BRIGHTNESS,
    NR_MCU_KEY_FUNC_DECREASE_BRIGHTNESS,
};


NR_PLUGIN_ENUM(NRMcuKeyType) {
    NR_MCU_KEY_TYPE_MULTI_KEY = 1,
    NR_MCU_KEY_TYPE_INCREASE_KEY,
    NR_MCU_KEY_TYPE_DECREASE_KEY,
};


NR_PLUGIN_ENUM(NRMcuCommand) {
    NR_R_BRIGHTNESS,
    NR_W_BRIGHTNESS,
    NR_R_DISPLAY_2D_3D,
    NR_W_DISPLAY_2D_3D,
    NR_R_HOST_ID,
    NR_W_HOST_ID,
    NR_R_GLASSID,
    NR_R_PSENSOR_CLOSED,
    NR_W_PSENSOR_CLOSED,
    NR_R_PSENSOR_NOCLOSED,
    NR_W_PSENSOR_NOCLOSED,
    NR_R_TEMPERATURE,
    NR_R_DISPLAY_DUTY,
    NR_W_DISPLAY_DUTY,
    NR_R_POWER_FUCTION,
    NR_W_POWER_FUCTION,
    NR_R_MAGNETIC_FUCTION,
    NR_W_MAGNETIC_FUCTION,
    NR_R_VSYNC_FUCTION,
    NR_W_VSYNC_FUCTION,
    NR_R_ENV_LIGHT,
    NR_W_ENV_LIGHT,
    NR_R_WORLD_LED,
    NR_W_WORLD_LED,
    NR_R_SLEEP_TIME,
    NR_W_SLEEP_TIME,
    NR_R_DP_FW_VERSION,
    NR_W_7211_UPDATE,
    NR_W_REBOOT,
    NR_W_UPDATE_FLAG,
    NR_W_FW_UPDATE,
    NR_W_OV580_RESET,
    NR_R_BRIGHTNESS_EXT,
    NR_W_BRIGHTNESS_EXT,
    NR_R_TEMPERATURE_FUNCTION,
    NR_W_TEMPERATURE_FUNCTION,
    NR_W_TRY_CTRL_DISPLAY_STATUS,
    NR_W_HEARTBEAT,
    NR_W_SDK_VERSION,
    NR_R_MCU_FW_VERSION ,
    NR_R_TEMPERATURE_EXT,
    NR_R_MACHINE_ID,
    NR_R_GLASSES_RUN_STATUS,
    NR_R_DISPLAY_INFO,
    NR_R_OLED_BRIGHTNESS,
    NR_W_OLED_BRIGHTNESS,
    NR_R_ACTIVATION,
    NR_W_ACTIVATION,
    NR_R_ACTIVATION_TIME,
    NR_W_ACTIVATION_TIME,
    NR_R_PRIVILEGATION,
    NR_W_PRIVILEGATION,
    NR_R_RGB_SWITCH,
    NR_W_RGB_SWITCH,
    NR_R_DEV_STATUS,
    NR_W_LOG_TRIGGER,
    NR_R_SUPPORT_DEVICES,
    NR_R_SUPPORT_DISPLAYS,
    NR_R_BRIGHTNESS_FUCTION,
    NR_W_BRIGHTNESS_FUCTION,
    NR_R_DEVICE_PARAMETERS,
    NR_W_CANCEL_ACTIVATION,
    NR_R_MAG_CALIBR_DATA,
    NR_W_MAG_CALIBR_DATA,
    NR_R_OLED_COORDINATE,
    NR_W_OLED_COORDINATE,
    NR_W_FORCE_CTRL_OLED,
    NR_R_PSENSOR_VALUE,
    NR_R_RESERVED_SN0,
    NR_R_RESERVED_SN1,
    NR_W_IMU_FREQ_DIVIDE,
    NR_R_DISPLAY_DEFAULT_START_MODE,
    NR_W_DISPLAY_DEFAULT_START_MODE,
    NR_R_PSENSOR_SWITCH,
    NR_W_PSENSOR_SWITCH,
    NR_START_ERRORS_AND_EVENTS_REPORT,
    NR_W_SWITCH_OLED,
    NR_R_ENABLE_PHYSICAL_BUTTON_SWITCH_DISPLAY_MODE_FLAG,
    NR_W_ENABLE_PHYSICAL_BUTTON_SWITCH_DISPLAY_MODE_FLAG,
    NR_R_DISPLAY_BYPASS_PSENSOR_FLAG,
    NR_W_DISPLAY_BYPASS_PSENSOR_FLAG,
    NR_W_DUMP_DP_REGISTERS,
    NR_R_GLASSES_DISPLAY_STATUS,
    NR_R_PSENSOR_IS_WEARING,
    NR_R_BRIGHTNESS_LEVEL_NUMBER,
    NR_W_DP_ESD_PARAM,
    NR_W_DP_HDCP_ENABLE,
    NR_R_DSP_VERSION,
    NR_W_DISPLAY_MAP_PARAMS,
    NR_R_DISPLAY_MAP_PARAMS,
    NR_W_DP_LEVEL,
    NR_R_SCREEN_STATUS,
    NR_W_TOGGLE_KEY,
    NR_R_IMU_INTERRUPT_COUNT,
    NR_R_SWITCH_CAL_OLED,
    NR_W_SWITCH_CAL_OLED,
    NR_R_ELECTROCHROMIC_CURRENT_LEVEL,
    NR_R_ELECTROCHROMIC_TOTAL_LEVEL,
    NR_W_ELECTROCHROMIC_LEVEL,
    NR_W_HOST_TYPE,
    NR_R_HW_VERSION,
    NR_R_TEMPERATURE_LEVEL,
    NR_W_FORCE_ENTER_SLEEP,
    NR_R_LED_WORK_MODE,
    NR_W_LED_WORK_MODE,
    NR_R_SERIAL_LOG_STATUS,
    NR_W_SERIAL_LOG_STATUS,
    NR_W_LED_RGB_AND_MODE,
    NR_R_LED_RGB_AND_MODE,
};


#pragma pack(1)
typedef struct NRMcuEventsReportData {
    union {
        struct {
            uint32_t category_id;
            uint32_t event_id;
            uint32_t time_offset;
            uint32_t info_1;
            uint32_t info_2;
            uint32_t description_length;
            char description[48];
        };
        uint8_t padding[96];
    };

} NRMcuEventsReportData;

typedef struct NRMcuStatusData {
    union {
        struct {
            NRMcuStatus status;
            uint64_t hmd_time_nanos;
            uint64_t hmd_hw_time_nanos;
            int32_t notify_data;
        };
        uint8_t padding[32];
    };

} NRMcuStatusData;

typedef struct NRMcuTemperatureData {
    union {
        struct {
            uint64_t hmd_time_nanos;
            uint64_t hmd_hw_time_nanos;
            int32_t temperature;
            NRTemperatureType glasses_parts;
        };
        uint8_t padding[64];
    };

} NRMcuTemperatureData;

typedef struct NRMcuHeatbeatData {
    union {
        struct {
            uint64_t sync_hmd_time_nanos;
            uint64_t sync_hmd_hw_time_nanos;
            uint64_t hmd_time_nanos;
            uint64_t hmd_hw_time_nanos;
        };
        uint8_t padding[64];
    };

} NRMcuHeatbeatData;

typedef struct NRMcuKeyClickData {
    union {
        struct {
            uint32_t key_type;
            uint32_t key_func;
            uint32_t key_param;
            uint64_t hmd_time_nanos;
            uint64_t hmd_hw_time_nanos;
        };
        uint8_t padding[64];
    };

} NRMcuKeyClickData;

#pragma pack()

#endif // NRSDK