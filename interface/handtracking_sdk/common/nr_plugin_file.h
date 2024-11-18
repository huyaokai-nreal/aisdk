#pragma once

#include "nr_plugin_interface.h"
#include "nr_plugin_types.h"
typedef struct NRFileProvider {

    NRPluginResult(NR_INTERFACE_API *RemoveUserConfigFiles)(
        NRPluginHandle handle
    );
} NRFileProvider;

NR_DECLARE_INTERFACE(NRFileInterface) {

    NRPluginResult(NR_INTERFACE_API *RegisterProvider)(
        NRPluginHandle handle,
        const NRFileProvider * provider,
        uint32_t provider_size
    );

    NRPluginResult(NR_INTERFACE_API *GetUserConfigDirectory)(
        NRPluginHandle handle,
        const char ** user_config_directory,
        uint32_t * directory_size
    );
	/*
		
		out_buffer_data的内存由调用者创建，尽量创建足够大的内存，以能够存储所有数据。
		out_buffer_size作为入参指定调用者创建的out_buffer_data的内存大小
		函数返回时，out_buffer_size被修改为数据的真正大小
		
	*/
    NRPluginResult(NR_INTERFACE_API *SafeReadBufferFromFile)(
        NRPluginHandle handle,
        const char * file_path,
        uint32_t path_size,
        uint32_t * out_buffer_size,
        char * out_buffer_data
    );

    NRPluginResult(NR_INTERFACE_API *SafeSaveBufferToFile)(
        NRPluginHandle handle,
        const char * buffer_data,
        uint32_t buffer_size,
        const char * file_path,
        uint32_t path_size
    );

    NRPluginResult(NR_INTERFACE_API *SafeRemoveFile)(
        NRPluginHandle handle,
        const char * file_path,
        uint32_t path_size
    );

    NRPluginResult(NR_INTERFACE_API *GetDefaultHomeDirectory)(
        NRPluginHandle handle,
        const char ** default_home_directory,
        uint32_t * directory_size
    );
};

NR_REGISTER_INTERFACE_GUID(0x0CC1B9A74C4044FEULL, 0x9C766DDB0E65F8EDULL,
                            NRFileInterface)

