/*
 * @Author: jszhang jszhang@nreal.ai
 * @Date: 2023-04-10 03:27:57
 * @LastEditors: jszhang jszhang@nreal.ai
 * @LastEditTime: 2023-04-10 06:32:28
 * @FilePath: /SNPE_demo/src/snpe/snpe_lib_wrapper.h
 */
#include <dlfcn.h>

#include <iostream>
#include <string>
#include <vector>

#include "LoadInterface.h"

class SNPELibWrapper {
   public:
    static SNPELibWrapper& getInstance() {
        static SNPELibWrapper instance;
        return instance;
    }

    SNPELibWrapper(const SNPELibWrapper&) = delete;
    SNPELibWrapper& operator=(const SNPELibWrapper&) = delete;

    int getSnpe2CInterface(SnpeCInterface* api) const {
        if (snpe2_handle_ == nullptr) {
            return -1;
        }
        *api = snpe2_provider_;
        return 0;
    }

    int getSnpe2PlatformCInterface(SnpePlatformCInterface* api) const {
        if (snpe2_platform_handle_ == nullptr) {
            return -1;
        }
        *api = snpe2_platform_provider_;
        return 0;
    }

    int UnloadSnpe2CInterface();
    int UnloadSnpe2PlatformCInterface();
   private:
    SNPELibWrapper();
    ~SNPELibWrapper();

    int LoadSnpe2CInterface(const char* snpe_soname, SnpeCInterface* api);
    int LoadSnpe2PlatformCInterface(const char* snpe_soname, SnpePlatformCInterface* api);

    void* snpe2_handle_ = nullptr;
    SnpeCInterface snpe2_provider_;

    void* snpe2_platform_handle_ = nullptr;
    SnpePlatformCInterface snpe2_platform_provider_;
};
