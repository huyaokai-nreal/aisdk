#include "handtracking_sdk_interface.h"

#include <cstdint>

#include "aisdk/base/file.h"
#include "aisdk/base/log.h"
#include "aisdk/base/profiling.h"
#if (defined(ANDROID) || defined(__ANDROID__))
#include <jni.h>
#endif
#include <json/json.h>

#include <Eigen/Dense>
#include <opencv2/opencv.hpp>

#include "aisdk/base/dlutil.h"
#include "aisdk/task/handtracking/handtracking_mediapipe_graph.h"
#include "framework/util/android_globals.h"
#include "framework/util/fileutil.h"
#include "framework/util/os_time.h"

#if defined(XENGINE_SHARED_LIB) && defined(FORCE_USE_PUSH)
// 生产so库，手动强制推so库
#define USE_EXTRA_PUSH_LIB
#endif

namespace aisdk::interface {

#if (defined(ANDROID) || defined(__ANDROID__))
// 几个不同handtracking.apk默认的sdcard路径有变化
// std::string apk_copydir = "/sdcard/Android/data/ai.nreal.sdk.unity.handtracking/files/";
// std::string apk_copydir = "/sdcard/Android/data/ai.nreal.sdk.unity.hellomr/files/";
// std::string apk_copydir = "/sdcard/Android/data/com.Joseph.MRTKHandTracking/files/";
std::string apk_copydir = "/sdcard/Android/data/ai.nreal.sdk.unity.HandTracking/files/";
#else
std::string apk_copydir = "./handTracking/";
#endif

std::string dlopen_with_classloader_dir = "";
const std::string default_libdir = "./handTracking/";

#if defined(XENGINE_SHARED_LIB)
const std::string netalgo_so_name = XENGINE_LIB_NAME;
#endif

HandTracking::~HandTracking() { UnLoadDlsym(true); }

NRPluginResult HandTracking::GetAvailableGestureType(NRPluginHandle handle, uint64_t* out_available_gesture_type_mask) {
    if (handle != Plugin::GetInstance()->GetHandle()) {
        AISDK_LOG_TRACE("HandTracking: GetAvailableGestureType handle error!");
        return NR_PLUGIN_RESULT_FAILURE;
    }
    *out_available_gesture_type_mask = GESTURE_TYPE_MASK_OPEN_HAND | GESTURE_TYPE_MASK_GRAB | GESTURE_TYPE_MASK_PINCH |
                                       GESTURE_TYPE_MASK_POINT | GESTURE_TYPE_MASK_VICTORY | GESTURE_TYPE_MASK_CALL |
                                       GESTURE_TYPE_MASK_SYSTEM | GESTURE_TYPE_MASK_THUMBS_UP;
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult HandTracking::GetAvailableHandJoint(NRPluginHandle handle, uint64_t* out_available_hand_joint_mask) {
    if (handle != Plugin::GetInstance()->GetHandle()) {
        AISDK_LOG_TRACE("HandTracking: GetAvailableHandJoint handle error!");
        return NR_PLUGIN_RESULT_FAILURE;
    }
    *out_available_hand_joint_mask =
        HAND_JOINT_TYPE_MASK_THUMB_1 | HAND_JOINT_TYPE_MASK_THUMB_2 | HAND_JOINT_TYPE_MASK_THUMB_3 |
        HAND_JOINT_TYPE_MASK_INDEX_1 | HAND_JOINT_TYPE_MASK_INDEX_2 | HAND_JOINT_TYPE_MASK_INDEX_3 |
        HAND_JOINT_TYPE_MASK_INDEX_4 | HAND_JOINT_TYPE_MASK_MIDDLE_1 | HAND_JOINT_TYPE_MASK_MIDDLE_2 |
        HAND_JOINT_TYPE_MASK_MIDDLE_3 | HAND_JOINT_TYPE_MASK_MIDDLE_4 | HAND_JOINT_TYPE_MASK_RING_1 |
        HAND_JOINT_TYPE_MASK_RING_2 | HAND_JOINT_TYPE_MASK_RING_3 | HAND_JOINT_TYPE_MASK_RING_4 |
        HAND_JOINT_TYPE_MASK_PINKY_1 | HAND_JOINT_TYPE_MASK_PINKY_2 | HAND_JOINT_TYPE_MASK_PINKY_3 |
        HAND_JOINT_TYPE_MASK_PINKY_4 | HAND_JOINT_TYPE_MASK_PALM_CENTER | HAND_JOINT_TYPE_MASK_WRIST_END |
        HAND_JOINT_TYPE_MASK_THUMB_0 | HAND_JOINT_TYPE_MASK_PINKY_0;
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult HandTracking::GetSupportedFunctions(NRPluginHandle handle, uint64_t* out_supported_function_mask) {
    if (handle != Plugin::GetInstance()->GetHandle()) {
        AISDK_LOG_TRACE("HandTracking: GetSupportedFunctions handle error!");
        return NR_PLUGIN_RESULT_FAILURE;
    }
    *out_supported_function_mask =
        HAND_TRACKING_SUPPORT_MASK_HAND_JOINT_POSITION | HAND_TRACKING_SUPPORT_MASK_HAND_JOINT_ROTATION;
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult HandTracking::UpdateNRHandData() { return NR_PLUGIN_RESULT_SUCCESS; }

NRPluginResult HandTracking::GetHandData(NRPluginHandle handle, uint64_t hmd_time_nanos, HandData* out_hand_array,
                                         uint32_t* out_hand_num) {
    (void)hmd_time_nanos;
    (void)out_hand_array;
    (void)out_hand_num;

    if (handle != Plugin::GetInstance()->GetHandle()) {
        AISDK_LOG_TRACE("HandTracking: GetHandData handle error!");
        return NR_PLUGIN_RESULT_FAILURE;
    }
    auto* ins = Plugin::GetInstance();
    auto& pipeline = ins->GetPipeline();
    if (ins->pipeline_name == "handtracking_bino_graph_v2.0.0") {
        std::shared_ptr<task::HandTrackingMediaPipeGraph> impl =
            std::dynamic_pointer_cast<task::HandTrackingMediaPipeGraph>(pipeline.Impl());
        aisdk::algorithm::Status status = impl->PopResult(hmd_time_nanos, out_hand_num, out_hand_array);
        if (status == aisdk::algorithm::Status::SUCCESS) {
            return NR_PLUGIN_RESULT_SUCCESS;
            AISDK_LOG_TRACE("HandTracking: pop result success!");
        } else {
            *out_hand_num = 0;
            // AISDK_LOG_TRACE("HandTracking: pop result failed!");
        }
    }

    return NR_PLUGIN_RESULT_FAILURE;
}

int HandTracking::GetHandTrackingMidExecInfo(ProfilingInfo* info) {
    (void)info;
    auto* ins = Plugin::GetInstance();
    auto& pipline = ins->GetPipeline();
    if (ins->pipeline_name == "handtracking_bino_graph_v2.0.0") {
        std::shared_ptr<task::HandTrackingMediaPipeGraph> impl =
            std::dynamic_pointer_cast<task::HandTrackingMediaPipeGraph>(pipline.Impl());
        // NrCore::Status status = impl->PopExecInfo(tmp);
        // if (status == NrCore::Status::SUCCESS) {
        //     info->timestamp = result->timestamp;
        //     uint32_t lens = result->noderesult_jsonstring.size();
        //     // 使用者释放
        //     info->noderesult_jsonstring = (char*)malloc(lens + 1);
        //     memcpy(info->noderesult_jsonstring, result->noderesult_jsonstring.data(), lens);
        //     info->noderesult_jsonstring[lens] = '\0';
        //     return 0;
        // }
    }
    return -1;
}

bool HandTracking::LoadDlsym(const std::string& full_path) {
    AISDK_LOG_TRACE("LoadDlsym getplatform={:s}", full_path.c_str());
    bool ret = false;
    if (nullptr == m_dlhandle) {
        m_dlhandle = dlopen(full_path.c_str(), RTLD_NOW);
        if (m_dlhandle) {
            m_funcs.m_getplatform = (GetPlatformStatusFunc)dlsym(m_dlhandle, "_ZN2NR200TK7FUNC001E");
            // AISDK_LOG_TRACE("getplatform={:p}", fmt::ptr(m_funcs.m_getplatform));
            m_funcs.m_createnetalgo = (CreateNetAlgoFunc)dlsym(m_dlhandle, "_ZN2NR200TK7FUNC002E");
            // AISDK_LOG_TRACE("createnetalgo={:p}", fmt::ptr(m_funcs.m_createnetalgo));
            m_funcs.m_destorynetalgo = (DestoryNetAlgoFunc)dlsym(m_dlhandle, "_ZN2NR200TK7FUNC003E");
            // AISDK_LOG_TRACE("destorynetalgo={:p}", fmt::ptr(m_funcs.m_destorynetalgo));
            // m_funcs.m_getincbininfo = (GetIncbinInfoFunc)dlsym(m_dlhandle, "_ZN2NR200TK7FUNC004E");
            // AISDK_LOG_TRACE("getincbininfo={:p}", fmt::ptr(m_funcs.m_getincbininfo));
            m_funcs.m_syncdebugprofiling = (DebugProfilingOptionFunc)dlsym(m_dlhandle, "_ZN2NR200TK7FUNC005E");
            // AISDK_LOG_TRACE("syncdebugprofiling={:p}", fmt::ptr(m_funcs.m_syncdebugprofiling));
            if (m_funcs.m_getplatform && m_funcs.m_createnetalgo && m_funcs.m_destorynetalgo &&
                m_funcs.m_syncdebugprofiling) {
                ret = true;
            } else {
                const char* dlsym_error = dlerror();
                AISDK_LOG_ERROR("HandTracking: Load algo module error!");
                AISDK_LOG_ERROR("HandTracking: dlsym_error={:s}", dlsym_error);
                dlclose(m_dlhandle);
                m_dlhandle = nullptr;
                ret = false;
            }
        } else {
            const char* dlsym_error = dlerror();
            AISDK_LOG_ERROR("HandTracking: Load algo module error!");
            AISDK_LOG_ERROR("HandTracking: dlsym_error={:s}", dlsym_error);
        }
    } else {
        // 第2次Plugin::Initialize，复用so句柄
        ret = true;
        AISDK_LOG_TRACE("HandTracking: repeat used m_dlhandle");
    }

    return ret;
}

void HandTracking::UnLoadDlsym(bool need) {
    if (m_dlhandle && need) {
        // 2023.02.22 notes:
        // 重复执行Plugin::Initialize/Plugin::Release情况下
        // 可以dlclose，也可以不dlclose。
        // 目前libnreal_hand_test.so，不能热更新，所以这里我们选择不dlclose，
        dlclose(m_dlhandle);
        m_dlhandle = nullptr;
    }
}

void SetAdspLibraryPath(const std::string& native_lib_path) {
    (void)native_lib_path;
#if defined(HAVE_HAL_SNPE)
    AISDK_LOG_INFO("SNPE_VERSION={:d}", SNPE_VERSION);
#if (defined(ANDROID) || defined(__ANDROID__))
    std::stringstream path;
    path << native_lib_path << ";/system/lib/rfsa/adsp;/system/vendor/lib/rfsa/adsp;/dsp";
    int set_res = setenv("ADSP_LIBRARY_PATH", path.str().c_str(), 0 /*override*/);
    if (set_res != 0) {
        AISDK_LOG_TRACE("SetAdspLibraryPath failed!");
    }
    const char* p;
    if ((p = getenv("ADSP_LIBRARY_PATH"))) {
        AISDK_LOG_TRACE("GetAdspLibraryPath success! ADSP_LIBRARY_PATH={}", p);
    } else {
        AISDK_LOG_TRACE("GetAdspLibraryPath failed!");
    }

    AISDK_LOG_TRACE("HandTracking: SetAdspLibraryPath ok");
#endif
#endif
}

bool HandTracking::GetApkStorePath() {
#if (defined(ANDROID) || defined(__ANDROID__))
    JavaVM* java_vm_ = nullptr;
    jobject obj_activity_ = nullptr;
    void* class_loader = nullptr;

    Plugin::GetInstance()->m_generic.m_interface->GetActivityInfo(
        reinterpret_cast<void**>(&java_vm_), reinterpret_cast<void**>(&obj_activity_), &class_loader);

    if (java_vm_ && obj_activity_) {
        // note: 使用framework::log功能需要以下2句话先做初始化再打印日志，否则会异常崩溃
        if (framework::util::android::GetJavaVM() == nullptr) {
            framework::util::android::StoreJavaVM(java_vm_);
        }
        if (framework::util::android::GetActivity() == nullptr) {
            framework::util::android::StoreActivity(obj_activity_);
        }
        if (class_loader && framework::util::android::GetExtraClassLoader() == nullptr) {
            framework::util::android::StoreExtraClassLoader(class_loader);
        }
        AISDK_LOG_TRACE("HandTracking: framework loaded");

        const char* global_lib_path = nullptr;
        uint32_t global_path_length = 0;
        if (NR_PLUGIN_RESULT_SUCCESS ==
            Plugin::GetInstance()->m_generic.m_interface->GetDynamicLibraryPath(
                Plugin::GetInstance()->GetHandle(), &global_lib_path, &global_path_length)) {
            if (global_lib_path && global_path_length) {
                std::string dynlib_str(global_lib_path, global_path_length);
                dlopen_with_classloader_dir = dynlib_str + "/";
                AISDK_LOG_TRACE("HandTracking: global_lib_path={:s}", dlopen_with_classloader_dir.c_str());
                AISDK_LOG_TRACE("HandTracking: class_loader return path, running on loader mode.");
            } else {
                AISDK_LOG_TRACE("HandTracking: global_lib_path={:s}", dlopen_with_classloader_dir.c_str());
                AISDK_LOG_TRACE("HandTracking: class_loader return null, fall back to basic mode.");
            }
        } else {
            AISDK_LOG_TRACE("GetDynamicLibraryPath failed");
        }

        std::string ext_path;
        if (framework::util::GetExternalFileDir(ext_path)) {
            AISDK_LOG_TRACE("HandTracking: ext_lib_path={:s}", ext_path.c_str());
            apk_copydir = ext_path + "/";
        } else {
            AISDK_LOG_TRACE("HandTracking: GetExternalFileDir failed");
        }

        // 在apk中通过接口获取
        JNIEnv* jni_env;
        java_vm_->GetEnv((void**)&jni_env, JNI_VERSION_1_6);
        jclass cls_Context = jni_env->GetObjectClass(obj_activity_);
        jmethodID mid_getApplicationInfo =
            jni_env->GetMethodID(cls_Context, "getApplicationInfo", "()Landroid/content/pm/ApplicationInfo;");
        jobject obj_ApplicationInfo = jni_env->CallObjectMethod(obj_activity_, mid_getApplicationInfo);

        jclass cls_ApplicationInfo = jni_env->GetObjectClass(obj_ApplicationInfo);
        jfieldID fld_nativeLibraryDir =
            jni_env->GetFieldID(cls_ApplicationInfo, "nativeLibraryDir", "Ljava/lang/String;");
        jstring jstr_dir = (jstring)jni_env->GetObjectField(obj_ApplicationInfo, fld_nativeLibraryDir);

        const char* nativeString = jni_env->GetStringUTFChars(jstr_dir, 0);
        mNativeLibDir = nativeString;

        //////////////////////////////////////////

        jclass j_context_wrapper_class = jni_env->FindClass("android/content/ContextWrapper");
        if (!j_context_wrapper_class) {
            return false;
        }
        jmethodID j_get_dir_method = jni_env->GetMethodID(j_context_wrapper_class, "getDir",
                                                          "(Ljava/lang/String;I)Ljava/io/File;");  // find method
        if (!j_get_dir_method) {
            return false;
        }
        jclass j_file_class = jni_env->FindClass("java/io/File");
        if (!j_file_class) {
            return false;
        }
        jmethodID j_path_method = jni_env->GetMethodID(j_file_class, "getAbsolutePath",
                                                       "()Ljava/lang/String;");  // find method
        if (!j_path_method) {
            return false;
        }

        jstring jniLibs_String = jni_env->NewStringUTF("jniLibs");

        jobject j_file_object = jni_env->CallObjectMethod(obj_activity_, j_get_dir_method, jniLibs_String, 0);
        if (!j_file_object) {
            return false;
        }
        jobject j_path = jni_env->CallObjectMethod(j_file_object, j_path_method);
        if (!j_path) {
            return false;
        }
        mSharedLibCopyDir = jni_env->GetStringUTFChars(jstring(j_path), 0);

#if defined(USE_EXTRA_PUSH_LIB)
        AISDK_LOG_TRACE("HandTracking: netalgo_so_name={:s}", netalgo_so_name.c_str());
        // 推库调试模式下，需要移动库
        if (never_dlopen_so) {
            // 2023.02.22 notes:
            // 问题现象：在apk中，先stop/后start handTracking接口（其实是Plugin::Initialize/Plugin::Release），会崩溃。
            // 崩溃在第2次dlopen "libnreal_hand_test.so" 后 GetPlatformStatus 接口异常，不能调用
            // 找到的原因：第2次copyFile动作引起，怀疑破坏了系统什么东西，或者so中的静态全局符号的初始化等。
            // 规避办法：libnreal_hand_test.so 在dlopen正常后，不要再次执行copyFile动作。
            // copyFile只能在apk首次启动中，从来没有dlopen 算法库的情况下执行

            std::string old_so = mSharedLibCopyDir + "/" + netalgo_so_name;
            if (aisdk::base::IsFileExist(old_so.c_str())) {
                int bbb = unlink(old_so.c_str());
                AISDK_LOG_TRACE("GetApkStorePath unlink_old_so=%s bbb=%d", old_so.c_str(), bbb);
            }

            aisdk::base::copyFile(apk_copydir + netalgo_so_name, mSharedLibCopyDir + "/" + netalgo_so_name);
            never_dlopen_so = false;
        }
#endif
        // 指定获取相机参数的方式类型，其他分支由debug接口设置
        Plugin::GetInstance()->m_hmd.m_generate_method = 1;

        // 系统调试配置解析,需要时手动开启
        auto& prof = aisdk::base::DebugProfiling::Get().GetOpt();
        // prof.aisdk_init_report = true;
        // prof.pipeline_debug = true;
        // prof.pipeline_node_time_statistics = true;
        // prof.developer_test_all = true;
        prof.local_data_record_rootpath = apk_copydir;
        // 目前仅支持在apk推库模式下使用
        std::string data_record_configf = apk_copydir + "record_configs.json";
        if (aisdk::base::IsFileExist(data_record_configf.c_str())) {
            prof.pipeline_debug = true;
            prof.local_pipeline_node_data_record = true;
            aisdk::base::ReadFromFile(data_record_configf, prof.local_data_record_jsonconfig);
        }
    } else {
        // 终端测试，可在环境变量中获取
        const char* snpe_library = getenv("ADSP_LIBRARY_PATH");
        if (snpe_library) {
            mNativeLibDir = std::string(snpe_library);
        } else {
            mNativeLibDir = default_libdir;
        }

        // 系统调试配置解析,需要时手动开启
        auto& prof = aisdk::base::DebugProfiling::Get().GetOpt();
        // prof.aisdk_init_report = true;
        // prof.pipeline_debug = true;
        // prof.pipeline_node_time_statistics = true;
        // prof.developer_test_all = true;
        prof.local_data_record_rootpath = default_libdir;
    }
#elif defined(__linux__)
    // 终端测试，可在环境变量中获取
    const char* snpe_library = getenv("ADSP_LIBRARY_PATH");
    if (snpe_library) {
        mNativeLibDir = std::string(snpe_library);
    } else {
        mNativeLibDir = default_libdir;
    }
#elif defined(__APPLE__)
    // 终端测试，可在环境变量中获取
    mNativeLibDir = default_libdir;
#endif

    AISDK_LOG_TRACE("HandTracking: GetApkStorePath NativeLibDir={:s}", mNativeLibDir.c_str());
    AISDK_LOG_TRACE("HandTracking: GetApkStorePath SharedLibCopyDir={:s}", mSharedLibCopyDir.c_str());
    AISDK_LOG_TRACE("HandTracking: GetApkStorePath apk_copydir={:s}", apk_copydir.c_str());
    SetAdspLibraryPath(mNativeLibDir);

#if defined(XENGINE_SHARED_LIB)
    // 动态加载so，并获取核心接口
    bool ret = false;
#if defined(FORCE_USE_PUSH)
    // 开启推库情况下，优先此逻辑
    AISDK_LOG_TRACE("HandTracking: LoadDlsym from ext shared lib copy dir {:s}",
                    mSharedLibCopyDir + "/" + netalgo_so_name);
    if (false == ret && mSharedLibCopyDir.size()) {
        ret = LoadDlsym(mSharedLibCopyDir + "/" + netalgo_so_name);
    }
#endif
    // 若不推库的情况下，走native路径
    if (false == ret) {
        // case1：apk后台，尝试读取mNativeLibDir路径下
        // case2：终端调试，尝试读取LD_LIBRARY_PATH环境变量路径下
        AISDK_LOG_TRACE("HandTracking: LoadDlsym from default or classloader {:s}", netalgo_so_name);
        ret = LoadDlsym(dlopen_with_classloader_dir + netalgo_so_name);
    }

    if (false == ret) {
        // 尝试读取默认路径
        AISDK_LOG_TRACE("HandTracking: dlopen default libdir");
        bool ret1 = LoadDlsym(default_libdir + "/" + netalgo_so_name);
        if (false == ret1) {
            return false;
        }
    }
#endif

#if defined(XENGINE_STATIC_LIB)
    m_funcs = *aisdk::xengine::GetXengineCapiStaticSymbol();
    if (!(m_funcs.m_getplatform && m_funcs.m_createnetalgo && m_funcs.m_destorynetalgo && m_funcs.m_createanalysistar &&
          m_funcs.m_destoryanalysistar && m_funcs.m_syncdebugprofiling && m_funcs.m_gethalbackend)) {
        AISDK_LOG_ERROR("HandTracking: GetXengineCapiStaticSymbol error!");
        return false;
    }
#endif

    return true;
}

void Hmd::GetCamerasInformation() {
    NRSize2i resolution_[2];
    NRMat3f intrinsic_mat_[2];
    NRCameraDistortion distortion_params_[2];
    NRTransform extrinsics_lr, extrinsics_lh;

    auto handle = Plugin::GetInstance()->GetHandle();

    m_interface->GetComponentResolution(handle, NR_COMPONENT_GRAYSCALE_CAMERA_LEFT, &resolution_[0]);
    m_interface->GetComponentResolution(handle, NR_COMPONENT_GRAYSCALE_CAMERA_RIGHT, &resolution_[1]);
    m_interface->GetComponentIntrinsic(handle, NR_COMPONENT_GRAYSCALE_CAMERA_LEFT, &intrinsic_mat_[0]);
    m_interface->GetComponentIntrinsic(handle, NR_COMPONENT_GRAYSCALE_CAMERA_RIGHT, &intrinsic_mat_[1]);
    m_interface->GetComponentDistortion(handle, NR_COMPONENT_GRAYSCALE_CAMERA_LEFT, &distortion_params_[0]);
    m_interface->GetComponentDistortion(handle, NR_COMPONENT_GRAYSCALE_CAMERA_RIGHT, &distortion_params_[1]);

    m_cam_param.m_params.clear();
    std::vector<float> cam_resolution = {(float)resolution_[0].width, (float)resolution_[0].height};
    m_cam_param.m_params["cam_resolution"] = std::move(cam_resolution);
    if (resolution_[0].width > resolution_[0].height) {
        cam_is_horizontal = true;
    } else {
        cam_is_horizontal = false;
    }

    AISDK_LOG_TRACE("GetComponentResolution LEFT: w: {}, h: {}", resolution_[0].width, resolution_[0].height);
    AISDK_LOG_TRACE("GetComponentIntrinsic LEFT: fc: {}, {}; cc: {}, {}", intrinsic_mat_[0].column0.x,
                    intrinsic_mat_[0].column1.y, intrinsic_mat_[0].column2.x, intrinsic_mat_[0].column2.y);

    std::vector<float> _l_kc;
    std::vector<float> _r_kc;

    if (distortion_params_[0].camera_model == NRCameraModel::NR_CAMERA_MODEL_RADIAL) {
        AISDK_LOG_TRACE("GetComponentDistortion Pinhole LEFT: {}, {}, {}, {}, {}", distortion_params_[0].radial_k1,
                        distortion_params_[0].radial_k2, distortion_params_[0].radial_p1,
                        distortion_params_[0].radial_p2, distortion_params_[0].radial_k3);
        _l_kc = {distortion_params_[0].radial_k1, distortion_params_[0].radial_k2, distortion_params_[0].radial_p1,
                 distortion_params_[0].radial_p2, distortion_params_[0].radial_k3};
        _r_kc = {distortion_params_[1].radial_k1, distortion_params_[1].radial_k2, distortion_params_[1].radial_p1,
                 distortion_params_[1].radial_p2, distortion_params_[1].radial_k3};
        // m_camera_model = 1;
    } else if (distortion_params_[0].camera_model == NRCameraModel::NR_CAMERA_MODEL_FISHEYE) {
        AISDK_LOG_TRACE("GetComponentDistortion Fisheye LEFT: {}, {}, {}, {}", distortion_params_[0].fisheye_k1,
                        distortion_params_[0].fisheye_k2, distortion_params_[0].fisheye_k3,
                        distortion_params_[0].fisheye_k4);
        _l_kc = {distortion_params_[0].fisheye_k1, distortion_params_[0].fisheye_k2, distortion_params_[0].fisheye_k3,
                 distortion_params_[0].fisheye_k4};
        _r_kc = {distortion_params_[1].fisheye_k1, distortion_params_[1].fisheye_k2, distortion_params_[1].fisheye_k3,
                 distortion_params_[1].fisheye_k4};
        // m_camera_model = 2;
    } else if (distortion_params_[0].camera_model == NRCameraModel::NR_CAMERA_MODEL_FISHEYE624) {
        AISDK_LOG_TRACE(
            "GetComponentDistortion Fisheye624 LEFT: {}, {}, {}, {}, {}, {} / {}, {} /  {}, {}, {}, {}",
            distortion_params_[0].fisheye_k1, distortion_params_[0].fisheye_k2, distortion_params_[0].fisheye_k3,
            distortion_params_[0].fisheye_k4, distortion_params_[0].fisheye_k5, distortion_params_[0].fisheye_k6,
            distortion_params_[0].fisheye_p1, distortion_params_[0].fisheye_p2, distortion_params_[0].fisheye_s1,
            distortion_params_[0].fisheye_s2, distortion_params_[0].fisheye_s3, distortion_params_[0].fisheye_s4);
        _l_kc = {distortion_params_[0].fisheye_k1, distortion_params_[0].fisheye_k2, distortion_params_[0].fisheye_k3,
                 distortion_params_[0].fisheye_k4, distortion_params_[0].fisheye_k5, distortion_params_[0].fisheye_k6,
                 distortion_params_[0].fisheye_p1, distortion_params_[0].fisheye_p2, distortion_params_[0].fisheye_s1,
                 distortion_params_[0].fisheye_s2, distortion_params_[0].fisheye_s3, distortion_params_[0].fisheye_s4};
        _r_kc = {distortion_params_[1].fisheye_k1, distortion_params_[1].fisheye_k2, distortion_params_[1].fisheye_k3,
                 distortion_params_[1].fisheye_k4, distortion_params_[1].fisheye_k5, distortion_params_[1].fisheye_k6,
                 distortion_params_[1].fisheye_p1, distortion_params_[1].fisheye_p2, distortion_params_[1].fisheye_s1,
                 distortion_params_[1].fisheye_s2, distortion_params_[1].fisheye_s3, distortion_params_[1].fisheye_s4};
        // 这里确定相机模式
        m_camera_model = 3;
    }

    std::vector<float> _l_fc = {intrinsic_mat_[0].column0.x, intrinsic_mat_[0].column1.y};
    std::vector<float> _l_cc = {intrinsic_mat_[0].column2.x, intrinsic_mat_[0].column2.y};

    std::vector<float> _r_fc = {intrinsic_mat_[1].column0.x, intrinsic_mat_[1].column1.y};
    std::vector<float> _r_cc = {intrinsic_mat_[1].column2.x, intrinsic_mat_[1].column2.y};

    m_cam_param.m_params["cam_l_fc"] = std::move(_l_fc);
    m_cam_param.m_params["cam_l_cc"] = std::move(_l_cc);
    m_cam_param.m_params["cam_l_kc"] = std::move(_l_kc);
    m_cam_param.m_params["cam_r_fc"] = std::move(_r_fc);
    m_cam_param.m_params["cam_r_cc"] = std::move(_r_cc);
    m_cam_param.m_params["cam_r_kc"] = std::move(_r_kc);

    m_interface->GetComponentExtrinsic(handle, NR_COMPONENT_GRAYSCALE_CAMERA_LEFT, NR_COMPONENT_GRAYSCALE_CAMERA_RIGHT,
                                       &extrinsics_lr);

    m_cam_param.m_params["glL_R_glR"] = {extrinsics_lr.rotation.qw, extrinsics_lr.rotation.qx,
                                         extrinsics_lr.rotation.qy, extrinsics_lr.rotation.qz};

    m_cam_param.m_params["glL_t_glR"] = {extrinsics_lr.position.x, extrinsics_lr.position.y, extrinsics_lr.position.z};

    AISDK_LOG_TRACE("glL_R_glR q: {}, {}, {}, {}", extrinsics_lr.rotation.qw, extrinsics_lr.rotation.qx,
                    extrinsics_lr.rotation.qy, extrinsics_lr.rotation.qz);

    AISDK_LOG_TRACE("glL_R_glR p: {}, {}, {}", extrinsics_lr.position.x, extrinsics_lr.position.y,
                    extrinsics_lr.position.z);

    m_interface->GetComponentExtrinsic(handle, NR_COMPONENT_HEAD, NR_COMPONENT_GRAYSCALE_CAMERA_LEFT, &extrinsics_lh);

    m_cam_param.m_params["glH_R_glL"] = {extrinsics_lh.rotation.qw, extrinsics_lh.rotation.qx,
                                         extrinsics_lh.rotation.qy, extrinsics_lh.rotation.qz};

    m_cam_param.m_params["glH_t_glL"] = {extrinsics_lh.position.x, extrinsics_lh.position.y, extrinsics_lh.position.z};

    AISDK_LOG_TRACE("glH_R_glL q: {}, {}, {}, {}", extrinsics_lh.rotation.qw, extrinsics_lh.rotation.qx,
                    extrinsics_lh.rotation.qy, extrinsics_lh.rotation.qz);

    AISDK_LOG_TRACE("glH_t_glL p: {}, {}, {}", extrinsics_lh.position.x, extrinsics_lh.position.y,
                    extrinsics_lh.position.z);

    m_cam_param.m_params["camera_model"] = {(float)m_camera_model};
    m_cam_param.m_params["generate_method"] = {(float)m_generate_method};
}

bool isHeadPoseValid(const NRTransform& headpose) {
    Eigen::Quaternion<float> q(headpose.rotation.qw, headpose.rotation.qx, headpose.rotation.qy, headpose.rotation.qz);
    const float max_float = std::numeric_limits<float>::max();
    const float min_float = std::numeric_limits<float>::lowest();

    if (q.w() < min_float || q.w() > max_float || q.x() < min_float || q.x() > max_float || q.y() < min_float ||
        q.y() > max_float || q.z() < min_float || q.z() > max_float) {
        return false;
    }

    float length_squared = q.squaredNorm();
    const float epsilon = 1e-6f;
    if (std::abs(length_squared - 1.0f) > epsilon) {
        return false;
    }

    return true;
}

NRPluginResult HandTracking::ParseAllCameraData(const NRGrayscaleCameraFrameData* data) {
    // uint32_t camera_raw_data_size[4];
    // const uint8_t* camera_raw_data_[2];
    uint64_t nano_time_[2];
    NRTransform head_pose;
    DevicePose headpose_proto;
    NRPluginResult errorcode = NR_PLUGIN_RESULT_SUCCESS;

    // We only use cam0 and cam1 now (coresponding to leftcam and rightcam on Light/Air Pro)
    std::vector<cv::Mat> image(2);
    auto ins = Plugin::GetInstance();
    std::shared_ptr<aisdk::base::XrMem> leftmem =
        ins->m_picbuf->RequestMemBlob(data->cameras[0].width * data->cameras[0].height);

    std::shared_ptr<aisdk::base::XrMem> rightmem =
        ins->m_picbuf->RequestMemBlob(data->cameras[1].width * data->cameras[1].height);

    if (!leftmem || !rightmem) {
        AISDK_LOG_ERROR("HandTracking::ParseAllCameraData RequestMemBlob error");
        return NRPluginResult::NR_PLUGIN_RESULT_FAILURE;
    }

    image[0] = cv::Mat(cv::Size(data->cameras[0].width, data->cameras[0].height), CV_8UC1,
                       reinterpret_cast<uint8_t*>(leftmem->addr));
    image[1] = cv::Mat(cv::Size(data->cameras[1].width, data->cameras[1].height), CV_8UC1,
                       reinterpret_cast<uint8_t*>(rightmem->addr));

    // AISDK_LOG_TRACE("ParseAllCameraData input rcam size: %d, %d", image[0].rows, image[0].cols);

    // AISDK_LOG_TRACE("ParseAllCameraData input rcam size: %d, %d", image[1].rows, image[1].cols);
    static uint64_t last_time_nanos = 0;
    AISDK_LOG_TRACE("get image time: {}", data->cameras[0].hmd_time_nanos);
    AISDK_LOG_TRACE("elapsed_time: {}", (data->cameras[0].hmd_time_nanos - last_time_nanos) / 1e9f);
    last_time_nanos = data->cameras[0].hmd_time_nanos;
    for (int cam_id = 0; cam_id < 2; cam_id++) {
        nano_time_[cam_id] = data->cameras[cam_id].hmd_time_nanos;
        const uint8_t* image_buffer = ((uint8_t*)data->data) + data->cameras[cam_id].offset;
        for (uint32_t row = 0; row < data->cameras[cam_id].height; row++) {
            memcpy(static_cast<void*>(image[cam_id].data + data->cameras[cam_id].width * row),
                   image_buffer + data->cameras[cam_id].step * row, data->cameras[cam_id].width);
        }
    }

    aisdk::algorithm::Image d1(image[0], leftmem);
    aisdk::algorithm::Image d2(image[1], rightmem);

    AISDK_LOG_TRACE("ParseAllCameraData input rcam size: h={}, w={}", d1.m_mat.rows, d1.m_mat.cols);
    AISDK_LOG_TRACE("ParseAllCameraData input rcam size: h={}, w={}", d2.m_mat.rows, d2.m_mat.cols);

    // left or right timestamp should be the same
    errorcode = ins->m_handtracking.m_interface->GetDevicePose(ins->GetHandle(), &headpose_proto, nano_time_[0]);
    head_pose = headpose_proto.transform;

    // 赋值相机参数
    aisdk::algorithm::CamInfo input_cam_info;

    auto cam_param = ins->m_hmd.m_cam_param.m_params;

    // 1. cvL_T_cvR
    Eigen::Isometry3f glL_T_glR = Eigen::Isometry3f::Identity();
    glL_T_glR.rotate(Eigen::Quaternionf(cam_param["glL_R_glR"][0], cam_param["glL_R_glR"][1], cam_param["glL_R_glR"][2],
                                        cam_param["glL_R_glR"][3]));
    glL_T_glR.pretranslate(
        Eigen::Vector3f(cam_param["glL_t_glR"][0], cam_param["glL_t_glR"][1], cam_param["glL_t_glR"][2]));
    if (ins->m_hmd.m_generate_method == 1) {
        // 输入是GL系
        Eigen::Matrix3f gl_R_cv;
        gl_R_cv << 1, 0, 0, 0, -1, 0, 0, 0, -1;
        Eigen::Isometry3f gl_T_cv = Eigen::Isometry3f::Identity();
        gl_T_cv.rotate(gl_R_cv);
        input_cam_info.cvL_T_cvR = gl_T_cv * glL_T_glR * gl_T_cv;
    } else {
        // 输入是cv系
        input_cam_info.cvL_T_cvR = glL_T_glR;
    }

    // 2. 左目内参 lcam_intrinsics
    cv::Mat l_K = cv::Mat::eye(3, 3, CV_32FC1);
    l_K.at<float>(0, 0) = cam_param["cam_l_fc"][0];
    l_K.at<float>(1, 1) = cam_param["cam_l_fc"][1];
    l_K.at<float>(0, 2) = cam_param["cam_l_cc"][0];
    l_K.at<float>(1, 2) = cam_param["cam_l_cc"][1];
    input_cam_info.lcam_intrinsics = l_K;

    // 3. 右目内参 rcam_intrinsics
    cv::Mat r_K = cv::Mat::eye(3, 3, CV_32FC1);
    r_K.at<float>(0, 0) = cam_param["cam_r_fc"][0];
    r_K.at<float>(1, 1) = cam_param["cam_r_fc"][1];
    r_K.at<float>(0, 2) = cam_param["cam_r_cc"][0];
    r_K.at<float>(1, 2) = cam_param["cam_r_cc"][1];
    input_cam_info.rcam_intrinsics = r_K;

    // 4. 全部按照最多的参数存储
    input_cam_info.lcam_dist_coeffs = cv::Mat::eye(1, 12, CV_32FC1);
    for (uint32_t k = 0; k < cam_param["cam_l_kc"].size(); k++) {
        input_cam_info.lcam_dist_coeffs.at<float>(0, k) = cam_param["cam_l_kc"][k];
    }

    input_cam_info.rcam_dist_coeffs = cv::Mat::eye(1, 12, CV_32FC1);
    for (uint32_t k = 0; k < cam_param["cam_r_kc"].size(); k++) {
        input_cam_info.rcam_dist_coeffs.at<float>(0, k) = cam_param["cam_r_kc"][k];
    }

    input_cam_info.camera_type = (int)cam_param["camera_model"][0];

    input_cam_info.video_width = (uint32_t)cam_param["cam_resolution"][0];
    input_cam_info.video_height = (uint32_t)cam_param["cam_resolution"][1];

    auto& pipeline = ins->GetPipeline();
    if (ins->pipeline_name == "handtracking_bino_graph_v2.0.0") {
        std::shared_ptr<task::HandTrackingMediaPipeGraph> impl =
            std::dynamic_pointer_cast<task::HandTrackingMediaPipeGraph>(pipeline.Impl());
        std::vector<aisdk::algorithm::Image> images;
        images.emplace_back(std::move(d1));
        images.emplace_back(std::move(d2));
        impl->PushData(nano_time_[0], images, head_pose, input_cam_info);
        AISDK_LOG_TRACE("interface HandTrackingMediaPipeGraph::PushData");
    }

    return errorcode;
}

void HandTracking::NotifyData(NRPluginHandle handle, NRChannelDataType channel_data_type, const void* data,
                              uint32_t data_size) {
    // AISDK_LOG_TRACE("NotifyData in HandTracking: Start!!!!!!!!!!!");
    if (handle != Plugin::GetInstance()->GetHandle()) {
        AISDK_LOG_ERROR("NotifyData handle error!");
        return;
    }
    switch (channel_data_type) {
        case NR_CHANNEL_DATA_TYPE_GLASSES_GRAYSCALE_CAMERA:
            if (data_size != sizeof(NRGrayscaleCameraFrameData)) {
                AISDK_LOG_ERROR("NotifyData Failed: data_size error, the interface is not compatible!");
                return;
            }
            if (Plugin::GetInstance()->isStart()) {
                ParseAllCameraData((const NRGrayscaleCameraFrameData*)data);
            }
            break;
        default:
            break;
    }
    // AISDK_LOG_TRACE("NotifyData in HandTracking: Complete!");
}

Plugin* Plugin::m_ins = nullptr;
Plugin* Plugin::GetInstance() {
    if (m_ins == nullptr) {
        static Plugin instance;
        m_ins = &instance;
    }
    return m_ins;
}

void Plugin::DestoryInstance() {
    if (m_ins) {
        // delete m_ins;
        m_ins = nullptr;
    }
}

Plugin::~Plugin() {
    // m_pipeline = nullptr;
}

bool Plugin::Init(NRPluginHandle handle, NRInterfaces* interfaces) {
    if (m_ins) {
        unsigned long long hmd_interface_size, generic_interface_size;
        hmd_interface_size = sizeof(NRHMDInterface);
        generic_interface_size = sizeof(NRGenericInterface);
        m_hmd.m_interface = interfaces->Get<NRHMDInterface>(&hmd_interface_size);
        m_generic.m_interface = interfaces->Get<NRGenericInterface>(&generic_interface_size);
        if (nullptr == m_hmd.m_interface || m_generic.m_interface == nullptr) {
            AISDK_LOG_TRACE("Plugin::Initialize get invalid interface !!! hmd: {}, generic: {}",
                            nullptr == m_hmd.m_interface, m_generic.m_interface == nullptr);
            return false;
        }

        // unique handle
        Plugin::GetInstance()->SetHandle(handle);

        AISDK_LOG_TRACE("Plugin::Initialize get handle: {}", handle);

        bool ret = m_ins->m_handtracking.GetApkStorePath();
        if (false == ret) {
            AISDK_LOG_TRACE("Plugin::Initialize GetApkStorePath error!!!");
            return false;
        }

        // 注册NRPluginLifecycleProvider接口函数
        unsigned long long handtracking_interface_size;
        handtracking_interface_size = sizeof(HandTrackingInterface);
        m_handtracking.m_interface = interfaces->Get<HandTrackingInterface>(&handtracking_interface_size);
        auto ht = m_handtracking.m_interface;
        if (nullptr == ht) {
            return false;
        }

        // AISDK_LOG_TRACE("Plugin::Init HandTracking_interface={}", ht);
        // AISDK_LOG_INFO("Plugin::Init HandTracking_interface={:p}", fmt::ptr(ht));
        NRPluginLifecycleProvider provider = {&Plugin::Register, &Plugin::Initialize, &Plugin::Start,
                                              &Plugin::Update,   &Plugin::Pause,      &Plugin::Resume,
                                              &Plugin::Stop,     &Plugin::Release,    &Plugin::Unregister};
        ht->RegisterLifecycleProvider(handle, "nr_handtracking_id", "version1.0", &provider, sizeof(provider));
        return true;
    }
    return false;
}

task::Pipeline& Plugin::GetPipeline() {
    if (nullptr == m_pipeline) {
        m_pipeline = std::make_unique<task::Pipeline>();
    }
    return *m_pipeline;
}

void Plugin::ReleasePipeline() { m_pipeline = nullptr; }

bool Plugin::AnalysisTar() {
    bool is_ok = false;
    auto ins = Plugin::GetInstance();
    std::string tar_name(NAME_TO_STRING(DEFAULT_PIPELINE_TAR_NAME));

// 优先加载外部模型
#if (defined(ANDROID) || defined(__ANDROID__))
    std::string external_modeltar_path = apk_copydir + tar_name + ".tar";
    if (ins->m_load_external_modeltar) {
        AISDK_LOG_TRACE("AnalysisTar::TarFile = {:s}", external_modeltar_path.c_str());
        if (aisdk::base::IsFileExist(external_modeltar_path.c_str())) {
            m_tar_handle = ins->m_handtracking.m_funcs.m_createanalysistar();
            is_ok = m_tar_handle->TarFile(external_modeltar_path.c_str());
            AISDK_LOG_TRACE("AnalysisTar::TarFile = {:s} is_ok={}", external_modeltar_path.c_str(), is_ok);
        }
    }
#endif

    // 默认情况下
    if (false == is_ok) {
        m_tar_handle = ins->m_handtracking.m_funcs.m_createanalysistar();
        is_ok = m_tar_handle->TarMem(tar_name.c_str());
        AISDK_LOG_TRACE("AnalysisTar::TarMem = {:s} is_ok={}", tar_name.c_str(), is_ok);
    }

    return is_ok;
}

std::vector<int> SelectPipeline(std::vector<aisdk::xengine::PipelineConfig>& pipelines,
                                aisdk::xengine::PlatformStatus& plat, bool cam_is_horizontal) {
    // 这里简单实现：多条满足条件的pipeline的优先级，以定义pipeline中的顺序作为优先级
    std::vector<int> pipeline_policy;
    for (uint32_t i = 0; i < pipelines.size(); i++) {
        auto& config = pipelines[i];
        if (cam_is_horizontal && config.related_feature.bind_sensor_orientation == "horizontal") {
            if (plat.is_snpe_support && config.related_feature.bind_runtime == "snpe_dsp") {
                pipeline_policy.push_back(i);
                continue;
            }

            if (config.related_feature.bind_runtime == "cpu") {
                pipeline_policy.push_back(i);
                continue;
            }
        } else if (false == cam_is_horizontal && config.related_feature.bind_sensor_orientation == "vertical") {
            if (plat.is_snpe_support && config.related_feature.bind_runtime == "snpe_dsp") {
                pipeline_policy.push_back(i);
                continue;
            }

            if (config.related_feature.bind_runtime == "cpu") {
                pipeline_policy.push_back(i);
                continue;
            }
        }
    }

    return pipeline_policy;
}

NRPluginResult Plugin::Initialize(NRPluginHandle handle) {
    const std::string git_version = AISK_GIT_VERSION;
    AISDK_LOG_INFO("HandTracking: git_version={:s}", git_version.c_str());

    if (handle != Plugin::GetInstance()->GetHandle()) {
        AISDK_LOG_ERROR("HandTracking::Initialize failed: get wrong handle!");
        return NRPluginResult::NR_PLUGIN_RESULT_FAILURE;
    }
    AISDK_LOG_INFO("HandTracking::Initialize");
    // bool ret = false;
    auto ins = Plugin::GetInstance();

    auto plugin_handle = Plugin::GetInstance()->GetHandle();
    // 输入图像缓存
    ins->m_picbuf = std::make_unique<aisdk::base::FixedMembuffer>(std::string("grayscale_pic"), 640 * 480 * 10);

    // 注册NRHandTrackingProvider接口函数
    HandTrackingProvider provider = {
        &HandTracking::GetAvailableGestureType,
        &HandTracking::GetAvailableHandJoint,
        &HandTracking::GetSupportedFunctions,
        &HandTracking::GetHandData,
        &HandTracking::NotifyData,
    };

    ins->m_handtracking.m_interface->RegisterProvider(plugin_handle, &provider, sizeof(provider));
    // 获取全局sdk_global.json的配置
    const char* global_json_data = nullptr;
    uint32_t global_json_size = 0;
    ins->m_generic.m_interface->GetGlobalConfig(plugin_handle, &global_json_data, &global_json_size);

    if (global_json_size && global_json_data) {
        AISDK_LOG_TRACE("Plugin::Initialize sdk_global={}", global_json_data);
        Json::Reader reader;
        Json::Value root;
        std::string root_str(global_json_data, global_json_size);
        if (reader.parse(root_str, root)) {
            AISDK_LOG_INFO("HandTracking: External config loaded.");
            if (root.isMember("sdk")) {
                if (root["sdk"].isMember("log_level")) {
                    std::string log_level = root["sdk"]["log_level"].asString();
                    if (log_level == "1") {
                        aisdk::base::Logger::GetInstance()->SetLogAllLevel(true);
                        aisdk::base::DebugProfiling::Get().GetOpt().loglevel_trace = true;
                        AISDK_LOG_INFO("HandTracking: Loglevel set to All");
                    }
                }
            }

            if (root.isMember("aisdk_developer_options")) {
                if (root["aisdk_developer_options"].isMember("load_external_modeltar")) {
                    std::string load_external_modeltar =
                        root["aisdk_developer_options"]["load_external_modeltar"].asString();
                    if (load_external_modeltar == "1") {
                        ins->m_load_external_modeltar = true;
                        AISDK_LOG_TRACE("HandTracking: load_external_modeltar is true!!!");
                    }
                }
            }
        }
    } else {
        AISDK_LOG_TRACE("HandTracking: Normal config loaded.");
    }

    NRDeviceType device_type;

    ins->m_generic.m_interface->GetDeviceType(plugin_handle, &device_type);

    if (device_type == NRDeviceType::NR_DEVICE_TYPE_LIGHT) {
        ins->m_hmd.m_camera_model = 1;
        AISDK_LOG_TRACE("HandTracking: Get Device Type: Light!");
    } else if (device_type == NRDeviceType::NR_DEVICE_TYPE_FLORA) {
        ins->m_hmd.m_camera_model = 2;
        AISDK_LOG_TRACE("HandTracking: Get Device Type: Flora!");
    } else {
        AISDK_LOG_ERROR("HandTracking:  Get Invalid Device Type code {}", device_type);
        return NR_PLUGIN_RESULT_FAILURE;
    }

    Plugin::GetInstance()->setDeviceType(device_type);

    AISDK_LOG_INFO("HandTracking: setDeviceType to {}!", device_type);

#if defined(FORCE_USE_PUSH)
    AISDK_LOG_TRACE("HandTracking: init with external libs!");
#else
    AISDK_LOG_TRACE("HandTracking: init with default libs!");
#endif

    // 获取camera参数
    ins->m_hmd.GetCamerasInformation();

    // 算法库中设置调试选项
    auto& profcnf = aisdk::base::DebugProfiling::Get().GetOpt();
    AISDK_LOG_TRACE("Plugin::Initialize loglevel_trace={}", profcnf.loglevel_trace);
    AISDK_LOG_TRACE("Plugin::Initialize aisdk_init_report={}", profcnf.aisdk_init_report);
    AISDK_LOG_TRACE("Plugin::Initialize pipeline_debug={}", profcnf.pipeline_debug);
    AISDK_LOG_TRACE("Plugin::Initialize export_pipeline_exec_info_jsonstring={}",
                    profcnf.export_pipeline_exec_info_jsonstring);
    AISDK_LOG_TRACE("Plugin::Initialize local_pipeline_node_data_record={}", profcnf.local_pipeline_node_data_record);
    AISDK_LOG_TRACE("Plugin::Initialize developer_test_all={}", profcnf.developer_test_all);
    AISDK_LOG_TRACE("Plugin::Initialize handtracking_pipeline_exec_enable_detect_boxtracker={}",
                    profcnf.handtracking_pipeline_exec_enable_detect_boxtracker);
    AISDK_LOG_TRACE("Plugin::Initialize handtracking_pipeline_exec_enable_detect_boxsmooth={}",
                    profcnf.handtracking_pipeline_exec_enable_detect_boxsmooth);
    ins->m_handtracking.m_funcs.m_syncdebugprofiling(profcnf);

    AISDK_LOG_TRACE("HandTracking: AnalysisTar");
    // 解析pipeline_tar包
    bool is_ok = ins->AnalysisTar();
    if (is_ok) {
        // 多条pipeline的项目，这里需要指定pipeline的运行策略
        std::vector<aisdk::xengine::PipelineConfig> tmp = ins->m_tar_handle->GetPipelineConfig();
        AISDK_LOG_TRACE("HandTracking: pipeline size={}", tmp.size());
        if (tmp.size() > 0) {
            aisdk::xengine::PlatformStatus* plat = ins->m_handtracking.m_funcs.m_getplatform();
            AISDK_LOG_INFO("HandTracking: dsp_support={}", plat->is_snpe_support);
            AISDK_LOG_TRACE("Plugin::Initialize is_snpe_support={},is_hexagon_dsp={},is_hexagon_unsignedPD_dsp={}",
                            (int)plat->is_snpe_support, (int)plat->is_hexagon_dsp,
                            (int)plat->is_hexagon_unsignedPD_dsp);
            std::vector<int> pipeline_policy = SelectPipeline(tmp, *plat, ins->m_hmd.cam_is_horizontal);

            for (uint32_t i = 0; i < pipeline_policy.size(); i++) {
                uint32_t pipeline_index = pipeline_policy[i];
                if (pipeline_index < tmp.size()) {
                    AISDK_LOG_TRACE("Plugin::Initialize pipline.Init index={} pipline.name={:s}", pipeline_index,
                                    tmp[pipeline_index].pipeline_name.c_str());
                    auto& pipline = ins->GetPipeline();
                    // 需要指定具体的实现
                    aisdk::algorithm::Status status;
                    if (tmp[pipeline_index].pipeline_name == "handtracking_bino_graph_v2.0.0") {
                        status = pipline.Init<task::HandTrackingMediaPipeGraph>(
                            ins->m_handtracking.m_funcs, tmp[pipeline_index], ins->m_hmd.m_cam_param);
                    } else {
                        AISDK_LOG_INFO("HandTracking: not found MediaPipeGraph!");
                        continue;
                    }
                    if (status == aisdk::algorithm::Status::SUCCESS) {
                        ins->pipeline_name = tmp[pipeline_index].pipeline_name;
                        AISDK_LOG_INFO("HandTracking: Initialized!");
                        return NR_PLUGIN_RESULT_SUCCESS;
                    } else {
                        AISDK_LOG_ERROR("HandTracking: init failed since NrCore::Status: {}!",
                                        static_cast<int>(status));
                    }
                } else {
                    AISDK_LOG_TRACE("Plugin::Initialize error: pipeline_index={} < tmp.size={}", pipeline_index,
                                    tmp.size());
                }
            }
        }
    } else {
        AISDK_LOG_TRACE("HandTracking: AnalysisTar failed, but why?");
    }

    return NR_PLUGIN_RESULT_FAILURE;
}

NRPluginResult Plugin::Start(NRPluginHandle handle) {
    if (handle != Plugin::GetInstance()->GetHandle()) {
        AISDK_LOG_ERROR("HandTracking::Start failed: get wrong handle!");
        return NRPluginResult::NR_PLUGIN_RESULT_FAILURE;
    }
    AISDK_LOG_INFO("HandTracking: Start");
    auto ins = Plugin::GetInstance();
    auto pipeline = ins->GetPipeline().Impl();
    auto ret = pipeline->Start();
    if (ret != aisdk::algorithm::Status::SUCCESS) {
        AISDK_LOG_INFO("HandTracking: Started Failure");
        return NR_PLUGIN_RESULT_FAILURE;
    }
    ins->Start();
    AISDK_LOG_INFO("HandTracking: Started");
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult Plugin::Update(NRPluginHandle handle) {
    if (handle != Plugin::GetInstance()->GetHandle()) {
        return NRPluginResult::NR_PLUGIN_RESULT_FAILURE;
    }
    AISDK_LOG_INFO("HandTracking: Updated");
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult Plugin::Pause(NRPluginHandle handle) {
    if (handle != Plugin::GetInstance()->GetHandle()) {
        AISDK_LOG_ERROR("HandTracking::Pause failed: get wrong handle!");
        return NRPluginResult::NR_PLUGIN_RESULT_FAILURE;
    }
    AISDK_LOG_INFO("Plugin::Pause");
    AISDK_LOG_INFO("HandTracking: Pause");
    auto ins = Plugin::GetInstance();
    auto pipline = ins->GetPipeline().Impl();
    pipline->Stop();
    ins->Stop();
    AISDK_LOG_INFO("HandTracking: Paused");
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult Plugin::Resume(NRPluginHandle handle) {
    if (handle != Plugin::GetInstance()->GetHandle()) {
        AISDK_LOG_ERROR("HandTracking::Resume failed: get wrong handle!");
        return NRPluginResult::NR_PLUGIN_RESULT_FAILURE;
    }
    AISDK_LOG_INFO("HandTracking: Resume");
    auto ins = Plugin::GetInstance();
    auto pipline = ins->GetPipeline().Impl();
    auto ret = pipline->Start();
    if (ret != aisdk::algorithm::Status::SUCCESS) {
        AISDK_LOG_INFO("HandTracking: Resume Failure");
        return NR_PLUGIN_RESULT_FAILURE;
    }
    ins->Start();
    AISDK_LOG_INFO("HandTracking: Resumed");
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult Plugin::Stop(NRPluginHandle handle) {
    if (handle != Plugin::GetInstance()->GetHandle()) {
        AISDK_LOG_ERROR("HandTracking::Stop failed: get wrong handle!");
        return NRPluginResult::NR_PLUGIN_RESULT_FAILURE;
    }
    AISDK_LOG_INFO("HandTracking: Stop");
    auto ins = Plugin::GetInstance();
    auto pipline = ins->GetPipeline().Impl();
    pipline->Stop();
    ins->Stop();
    AISDK_LOG_INFO("HandTracking: Stoped");
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult Plugin::Release(NRPluginHandle handle) {
    if (handle != Plugin::GetInstance()->GetHandle()) {
        AISDK_LOG_ERROR("HandTracking::Release failed: get wrong handle!");
        return NRPluginResult::NR_PLUGIN_RESULT_FAILURE;
    }
    AISDK_LOG_INFO("HandTracking: Release");
    auto ins = Plugin::GetInstance();
    ins->ReleasePipeline();
#if defined(XENGINE_SHARED_LIB)
    ins->m_handtracking.UnLoadDlsym(false);
#endif
    ins->m_picbuf = nullptr;
    AISDK_LOG_INFO("HandTracking: Released");
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult Plugin::Register(NRPluginHandle handle) {
    if (handle != Plugin::GetInstance()->GetHandle()) {
        AISDK_LOG_ERROR("HandTracking::Register failed: get wrong handle!");
        return NRPluginResult::NR_PLUGIN_RESULT_FAILURE;
    }
    AISDK_LOG_INFO("HandTracking: Registered");
    return NR_PLUGIN_RESULT_SUCCESS;
}

NRPluginResult Plugin::Unregister(NRPluginHandle handle) {
    if (handle != Plugin::GetInstance()->GetHandle()) {
        AISDK_LOG_ERROR("HandTracking::Unregister failed: get wrong handle!");
        return NRPluginResult::NR_PLUGIN_RESULT_FAILURE;
    }
    AISDK_LOG_INFO("HandTracking: Unregistered");
    return NR_PLUGIN_RESULT_SUCCESS;
}

}  // namespace aisdk::interface

// 插件初始化接口
#ifdef HANDTRACKING_SHARED_LIBS
extern "C" void NR_INTERFACE_EXPORT NR_INTERFACE_API NRPluginCreate(NRPluginHandle handle, NRInterfaces* interfaces) {
#else
extern "C" void NRPluginCreate_HANDTRACKING(NRPluginHandle handle, NRInterfaces* interfaces) {
#endif
    AISDK_LOG_INFO("NRPluginCreate");
    auto ins = aisdk::interface::Plugin::GetInstance();
    ins->Init(handle, interfaces);
}

#ifdef HANDTRACKING_SHARED_LIBS
extern "C" void NR_INTERFACE_EXPORT NR_INTERFACE_API NRPluginDestroy() {
#else
extern "C" void NRPluginDestroy_HANDTRACKING() {
#endif
    AISDK_LOG_INFO("NRPluginDestroy");
    aisdk::interface::Plugin::DestoryInstance();
}

#ifdef HANDTRACKING_SHARED_LIBS
// 以下2个api是新增为profiling使用，不是对外公知接口
extern "C" void NR_INTERFACE_EXPORT NR_INTERFACE_API NRPluginSetProfilingOption(uint32_t flag, void* c_bytestruct) {
    AISDK_LOG_TRACE("NRPluginSetProfiling");
    auto& prof = aisdk::base::DebugProfiling::Get().GetOpt();
    if (PROFILING_FLAG == flag && c_bytestruct) {
        aisdk::interface::ProfilingOption* tmp = (aisdk::interface::ProfilingOption*)c_bytestruct;
        prof.aisdk_init_report = (tmp->aisdk_init_report > 0);
        prof.pipeline_debug = (tmp->pipeline_debug > 0);
        if (prof.pipeline_debug) {
            prof.pipeline_node_time_statistics = (tmp->pipeline_node_time_statistics > 0);
            prof.export_pipeline_exec_info_jsonstring = (tmp->export_pipeline_exec_info_jsonstring > 0);
            prof.handtracking_pipeline_exec_enable_detect_boxtracker =
                (tmp->handtracking_pipeline_exec_enable_detect_boxtracker > 0);
            prof.handtracking_pipeline_exec_enable_detect_boxsmooth =
                (tmp->handtracking_pipeline_exec_enable_detect_boxsmooth > 0);
            prof.handtracking_pipeline_exec_enable_sync_kfpredictor =
                (tmp->handtracking_pipeline_exec_enable_sync_kfpredictor > 0);
            prof.handtracking_pipeline_exec_enable_sync_kfpredictor_timems =
                tmp->handtracking_pipeline_exec_enable_sync_kfpredictor_timems;
            prof.handtracking_pipeline_exec_enable_sync_world_seqfilter =
                (tmp->handtracking_pipeline_exec_enable_sync_world_seqfilter > 0);
            prof.developer_test_all = (tmp->developer_test_all > 0);
        }

        auto ins = aisdk::interface::Plugin::GetInstance();
        ins->m_hmd.m_camera_model = tmp->camera_model;
        ins->m_hmd.m_generate_method = tmp->generate_method;
    }
}

extern "C" int NR_INTERFACE_EXPORT NR_INTERFACE_API NRPluginGetProfilingInfo(uint32_t flag, void* c_bytestruct) {
    // AISDK_LOG_TRACE("NRPluginGetProfilingInfo");
    if (PROFILING_FLAG == flag && c_bytestruct) {
        aisdk::interface::ProfilingInfo* tmp = (aisdk::interface::ProfilingInfo*)c_bytestruct;
        return aisdk::interface::HandTracking::GetHandTrackingMidExecInfo(tmp);
    }

    return -1;
}
#endif
