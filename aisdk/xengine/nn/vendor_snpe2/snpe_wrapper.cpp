#include "snpe_wrapper.h"

#include <iostream>

#include "aisdk/base/log.h"
using namespace std;

static size_t calcSizeFromDims(const size_t* dims, size_t rank, size_t elementSize) {
    if (rank == 0) return 0;
    size_t size = elementSize;
    while (rank--) {
        size *= *dims;
        dims++;
    }
    return size;
}

void SNPEWrapper::createUserBuffer(Snpe_UserBufferMap_Handle_t userBufferMapHandle,
                                   std::unordered_map<std::string, std::vector<uint8_t>>& applicationBuffers,
                                   std::vector<Snpe_IUserBuffer_Handle_t>& snpeUserBackedBuffersHandle,
                                   Snpe_TensorShape_Handle_t bufferShapeHandle, const char* name,
                                   size_t bufferElementSize) {
    // Calculate the stride based on buffer strides, assuming tightly packed.
    // Note: Strides = Number of bytes to advance to the next element in each
    // dimension. For example, if a float tensor of dimension 2x4x3 is tightly
    // packed in a buffer of 96 bytes, then the strides would be (48,12,4) Note:
    // Buffer stride is usually known and does not need to be calculated.
    std::vector<size_t> strides(snpe2_capi.Snpe_TensorShape_Rank(bufferShapeHandle));
    strides[strides.size() - 1] = sizeof(float);
    size_t stride = strides[strides.size() - 1];
    for (size_t i = snpe2_capi.Snpe_TensorShape_Rank(bufferShapeHandle) - 1; i > 0; i--) {
        stride *= snpe2_capi.Snpe_TensorShape_At(bufferShapeHandle, i);
        strides[i - 1] = stride;
    }
    Snpe_TensorShape_Handle_t stridesHandle =
        snpe2_capi.Snpe_TensorShape_CreateDimsSize(strides.data(), snpe2_capi.Snpe_TensorShape_Rank(bufferShapeHandle));
    size_t bufSize = calcSizeFromDims(snpe2_capi.Snpe_TensorShape_GetDimensions(bufferShapeHandle),
                                      snpe2_capi.Snpe_TensorShape_Rank(bufferShapeHandle), bufferElementSize);

    // set the buffer encoding type
    Snpe_UserBufferEncoding_Handle_t userBufferEncodingFloatHandle = snpe2_capi.Snpe_UserBufferEncodingFloat_Create();
    // create user-backed storage to load input data onto it
    applicationBuffers.emplace(name, std::vector<uint8_t>(bufSize));
    // create SNPE user buffer from the user-backed buffer
    snpeUserBackedBuffersHandle.push_back(snpe2_capi.Snpe_Util_CreateUserBuffer(
        applicationBuffers.at(name).data(), bufSize, stridesHandle, userBufferEncodingFloatHandle));
    // add the user-backed buffer to the inputMap, which is later on fed to the
    // network for execution
    snpe2_capi.Snpe_UserBufferMap_Add(userBufferMapHandle, name, snpeUserBackedBuffersHandle.back());
    snpe2_capi.Snpe_UserBufferEncodingFloat_Delete(userBufferEncodingFloatHandle);
}

SNPEWrapper::SNPEWrapper() {
    int aa = SNPELibWrapper::getInstance(nullptr).getSnpe2CInterface(&snpe2_capi);
    // int aa = LoadSnpe2CInterface("./libSNPE.so",&snpe2_capi);
    if (0 != aa) {
        AISDK_LOG_TRACE("LoadSnpe2CInterface error");
    }

    Snpe_DlVersion_Handle_t versionHandle = snpe2_capi.Snpe_Util_GetLibraryVersion();
    AISDK_LOG_TRACE("Using SNPE version: {}", snpe2_capi.Snpe_DlVersion_ToString(versionHandle));
    snpe2_capi.Snpe_DlVersion_Delete(versionHandle);

    m_container = nullptr;
    m_snpe = nullptr;
    m_runtimeList = nullptr;
    m_set_outputLayers = nullptr;
    m_set_outputTensors = nullptr;
    m_inputUserBufferMap = nullptr;
    m_outputUserBufferMap = nullptr;
    m_inputShapeMapHandle = nullptr;
}

SNPEWrapper::~SNPEWrapper() {}

bool SNPEWrapper::setInputShape(const std::string& name, const std::vector<size_t>& shape) {
    m_inputShapeList[name] = shape;
    return true;
}

bool SNPEWrapper::init(const std::string& model_path, const std::string& runtime) {
    if (runtime == "CPU") {
        m_runtime = SNPE_RUNTIME_CPU;
    } else if (runtime == "GPU_FP16") {
        m_runtime = SNPE_RUNTIME_GPU_FLOAT16;
    } else if (runtime == "DSP_INT8") {
        m_runtime = SNPE_RUNTIME_DSP;
    } else if (runtime == "AIP") {
        m_runtime = SNPE_RUNTIME_AIP_FIXED8_TF;
    } else {
        AISDK_LOG_ERROR("Unsupported runtime: {}", runtime.c_str());
        return false;
    }

    // 接口正确性
    if (!snpe2_capi.Snpe_Util_IsRuntimeAvailable) {
        return false;
    }

    // if (!snpe2_capi.Snpe_Util_IsRuntimeAvailable(m_runtime)) {
    //     AISDK_LOG_ERROR("Selected runtime not supported. Falling back to CPU.");
    //     // m_runtime = SNPE_RUNTIME_CPU;
    //     m_runtime = SNPE_RUNTIME_DSP;
    // }

    m_container = snpe2_capi.Snpe_DlContainer_Open(model_path.c_str());
    Snpe_SNPEBuilder_Handle_t snpeBuilderHandle = snpe2_capi.Snpe_SNPEBuilder_Create(m_container);
    Snpe_PerformanceProfile_t profile = SNPE_PERFORMANCE_PROFILE_DEFAULT;
    if (nullptr == m_runtimeList) m_runtimeList = snpe2_capi.Snpe_RuntimeList_Create();
    snpe2_capi.Snpe_RuntimeList_Add(m_runtimeList, m_runtime);
    // Snpe_RuntimeList_Add(m_runtimeList, SNPE_RUNTIME_CPU);
    snpe2_capi.Snpe_SNPEBuilder_SetRuntimeProcessorOrder(snpeBuilderHandle, m_runtimeList);
    if (m_set_outputLayers) {
        if (snpe2_capi.Snpe_SNPEBuilder_SetOutputLayers(snpeBuilderHandle, m_set_outputLayers)) {
            AISDK_LOG_ERROR("Snpe_SNPEBuilder_SetOutputLayers failed: {}",
                            snpe2_capi.Snpe_ErrorCode_GetLastErrorString());
            return false;
        }
    }
    if (m_set_outputTensors) {
        if (snpe2_capi.Snpe_SNPEBuilder_SetOutputTensors(snpeBuilderHandle, m_set_outputTensors)) {
            AISDK_LOG_ERROR("Snpe_SNPEBuilder_SetOutputTensors failed: {}",
                            snpe2_capi.Snpe_ErrorCode_GetLastErrorString());
            return false;
        }
    }
    snpe2_capi.Snpe_SNPEBuilder_SetUseUserSuppliedBuffers(snpeBuilderHandle, true);
    snpe2_capi.Snpe_SNPEBuilder_SetPerformanceProfile(snpeBuilderHandle, profile);

    if (!m_inputShapeList.empty()) {
        m_inputShapeMapHandle = snpe2_capi.Snpe_TensorShapeMap_Create();
        for (auto& shape : m_inputShapeList) {
            Snpe_TensorShape_Handle_t inputShapeHandle;
            inputShapeHandle = snpe2_capi.Snpe_TensorShape_CreateDimsSize(shape.second.data(), shape.second.size());
            snpe2_capi.Snpe_TensorShapeMap_Add(m_inputShapeMapHandle, shape.first.c_str(), inputShapeHandle);
        }
        snpe2_capi.Snpe_SNPEBuilder_SetInputDimensions(snpeBuilderHandle, m_inputShapeMapHandle);
    }

    if (0) {
        // test
        m_platformconfig = snpe2_capi.Snpe_PlatformConfig_Create();
        const char* str_platform = snpe2_capi.Snpe_PlatformConfig_GetPlatformOptions(m_platformconfig);
        AISDK_LOG_TRACE("before PlatformConfig {}", str_platform);
        int setok = snpe2_capi.Snpe_PlatformConfig_SetPlatformOptions(m_platformconfig, "unsignedPD:OFF");
        int setok1 = snpe2_capi.Snpe_PlatformConfig_SetPlatformOptionValue(m_platformconfig, "unsignedPD", "OFF");
        const char* str_platform1 = snpe2_capi.Snpe_PlatformConfig_GetPlatformOptions(m_platformconfig);
        AISDK_LOG_TRACE("after PlatformConfig {} setok={}, setok1={}", str_platform1, setok, setok1);
        int SigndPD = snpe2_capi.Snpe_PlatformConfig_IsValid(m_platformconfig);
        AISDK_LOG_TRACE("PlatformConfig_IsValid = {}", SigndPD);
        snpe2_capi.Snpe_SNPEBuilder_SetPlatformConfig(snpeBuilderHandle, m_platformconfig);
        AISDK_LOG_TRACE("Snpe_SNPEBuilder_Build Signed dsp\n");
    }

    m_snpe = snpe2_capi.Snpe_SNPEBuilder_Build(snpeBuilderHandle);
    if (nullptr == m_snpe) {
        const char* errStr = snpe2_capi.Snpe_ErrorCode_GetLastErrorString();
        AISDK_LOG_ERROR("SNPE build failed: {}", errStr);
        return false;
    }

    // get input tensor names of the network that need to be populated
    Snpe_StringList_Handle_t inputNamesHandle = snpe2_capi.Snpe_SNPE_GetInputTensorNames(m_snpe);

    assert(snpe2_capi.Snpe_StringList_Size(inputNamesHandle) > 0);

    // create SNPE user buffers for each application storage buffer
    if (nullptr == m_inputUserBufferMap) m_inputUserBufferMap = snpe2_capi.Snpe_UserBufferMap_Create();
    for (size_t i = 0; i < snpe2_capi.Snpe_StringList_Size(inputNamesHandle); ++i) {
        const char* name = snpe2_capi.Snpe_StringList_At(inputNamesHandle, i);
        // get attributes of buffer by name
        auto bufferAttributesOptHandle = snpe2_capi.Snpe_SNPE_GetInputOutputBufferAttributes(m_snpe, name);
        if (nullptr == bufferAttributesOptHandle) {
            AISDK_LOG_ERROR("Error obtaining attributes for input tensor: {}", name);
            return false;
        }

        auto bufferShapeHandle = snpe2_capi.Snpe_IBufferAttributes_GetDims(bufferAttributesOptHandle);
        std::vector<size_t> tensorShape;
        for (size_t j = 0; j < snpe2_capi.Snpe_TensorShape_Rank(bufferShapeHandle); j++) {
            tensorShape.push_back(snpe2_capi.Snpe_TensorShape_At(bufferShapeHandle, j));
        }
        m_inputShapes.emplace(name, tensorShape);

        // size_t bufferElementSize =
        // Snpe_IBufferAttributes_GetElementSize(bufferAttributesOptHandle);
        createUserBuffer(m_inputUserBufferMap, m_inputTensors, m_inputUserBuffers, bufferShapeHandle, name,
                         sizeof(float));

        snpe2_capi.Snpe_IBufferAttributes_Delete(bufferAttributesOptHandle);
        snpe2_capi.Snpe_TensorShape_Delete(bufferShapeHandle);
    }
    snpe2_capi.Snpe_StringList_Delete(inputNamesHandle);

    // get output tensor names of the network that need to be populated
    if (nullptr == m_outputUserBufferMap) m_outputUserBufferMap = snpe2_capi.Snpe_UserBufferMap_Create();
    Snpe_StringList_Handle_t outputNamesHandle = snpe2_capi.Snpe_SNPE_GetOutputTensorNames(m_snpe);
    if (nullptr == outputNamesHandle) throw std::runtime_error("Error obtaining input tensor names");
    assert(snpe2_capi.Snpe_StringList_Size(outputNamesHandle) > 0);

    // create SNPE user buffers for each application storage buffer
    for (size_t i = 0; i < snpe2_capi.Snpe_StringList_Size(outputNamesHandle); ++i) {
        const char* name = snpe2_capi.Snpe_StringList_At(outputNamesHandle, i);
        // get attributes of buffer by name
        auto bufferAttributesOptHandle = snpe2_capi.Snpe_SNPE_GetInputOutputBufferAttributes(m_snpe, name);
        if (!bufferAttributesOptHandle) {
            AISDK_LOG_ERROR("Error obtaining attributes for input tensor: {}", name);
            return false;
        }

        auto bufferShapeHandle = snpe2_capi.Snpe_IBufferAttributes_GetDims(bufferAttributesOptHandle);
        std::vector<size_t> tensorShape;
        for (size_t j = 0; j < snpe2_capi.Snpe_TensorShape_Rank(bufferShapeHandle); j++) {
            tensorShape.push_back(snpe2_capi.Snpe_TensorShape_At(bufferShapeHandle, j));
        }
        m_outputShapes.emplace(name, tensorShape);

        // size_t bufferElementSize =
        // Snpe_IBufferAttributes_GetElementSize(bufferAttributesOptHandle);
        createUserBuffer(m_outputUserBufferMap, m_outputTensors, m_outputUserBuffers, bufferShapeHandle, name,
                         sizeof(float));

        snpe2_capi.Snpe_IBufferAttributes_Delete(bufferAttributesOptHandle);
        snpe2_capi.Snpe_TensorShape_Delete(bufferShapeHandle);
    }

    snpe2_capi.Snpe_StringList_Delete(outputNamesHandle);
    snpe2_capi.Snpe_SNPEBuilder_Delete(snpeBuilderHandle);

    m_isInit = true;

    return true;
}

bool SNPEWrapper::init(const uint8_t* buffer, const size_t size, const std::string& runtime, bool support_SigndPD) {
    if (runtime == "CPU") {
        m_runtime = SNPE_RUNTIME_CPU;
    } else if (runtime == "GPU_FP16") {
        m_runtime = SNPE_RUNTIME_GPU_FLOAT16;
    } else if (runtime == "DSP_INT8") {
        m_runtime = SNPE_RUNTIME_DSP;
    } else if (runtime == "AIP") {
        m_runtime = SNPE_RUNTIME_AIP_FIXED8_TF;
    } else {
        AISDK_LOG_ERROR("Unsupported runtime: {}", runtime.c_str());
        return false;
    }
    AISDK_LOG_TRACE("setting runtime: {}", m_runtime);

    // 接口正确性
    if (!snpe2_capi.Snpe_Util_IsRuntimeAvailable) {
        return false;
    }

    // if (!snpe2_capi.Snpe_Util_IsRuntimeAvailable(m_runtime)) {
    //     AISDK_LOG_ERROR("Selected runtime not supported. Falling back to CPU.");
    //     m_runtime = SNPE_RUNTIME_CPU;
    // }
    // AISDK_LOG_TRACE("runtime avaliable!: {}", m_runtime);

    // AISDK_LOG_TRACE("buffer ptr: {}, size: {}", buffer, size);

    m_container = snpe2_capi.Snpe_DlContainer_OpenBuffer(buffer, size);
    if (nullptr == m_container) {
        const char* errStr = snpe2_capi.Snpe_ErrorCode_GetLastErrorString();
        AISDK_LOG_ERROR("SNPE build failed: {}", errStr);
        return false;
    }
    AISDK_LOG_TRACE("model open success! ");
    Snpe_SNPEBuilder_Handle_t snpeBuilderHandle = snpe2_capi.Snpe_SNPEBuilder_Create(m_container);
    AISDK_LOG_TRACE("builder create success! ");
    Snpe_PerformanceProfile_t profile = SNPE_PERFORMANCE_PROFILE_DEFAULT;
    if (nullptr == m_runtimeList) m_runtimeList = snpe2_capi.Snpe_RuntimeList_Create();
    snpe2_capi.Snpe_RuntimeList_Add(m_runtimeList, m_runtime);
    // Snpe_RuntimeList_Add(m_runtimeList, SNPE_RUNTIME_CPU);
    snpe2_capi.Snpe_SNPEBuilder_SetRuntimeProcessorOrder(snpeBuilderHandle, m_runtimeList);
    AISDK_LOG_TRACE("set runtime success! ");
    if (m_set_outputLayers) {
        if (snpe2_capi.Snpe_SNPEBuilder_SetOutputLayers(snpeBuilderHandle, m_set_outputLayers)) {
            AISDK_LOG_ERROR("Snpe_SNPEBuilder_SetOutputLayers failed: {}",
                            snpe2_capi.Snpe_ErrorCode_GetLastErrorString());
            return false;
        }
    }
    if (m_set_outputTensors) {
        if (snpe2_capi.Snpe_SNPEBuilder_SetOutputTensors(snpeBuilderHandle, m_set_outputTensors)) {
            AISDK_LOG_ERROR("Snpe_SNPEBuilder_SetOutputTensors failed: {}",
                            snpe2_capi.Snpe_ErrorCode_GetLastErrorString());
            return false;
        }
    }
    snpe2_capi.Snpe_SNPEBuilder_SetUseUserSuppliedBuffers(snpeBuilderHandle, true);
    AISDK_LOG_TRACE("set userbuffer success! ");
    snpe2_capi.Snpe_SNPEBuilder_SetPerformanceProfile(snpeBuilderHandle, profile);

    if (!m_inputShapeList.empty()) {
        m_inputShapeMapHandle = snpe2_capi.Snpe_TensorShapeMap_Create();
        for (auto& shape : m_inputShapeList) {
            Snpe_TensorShape_Handle_t inputShapeHandle;
            inputShapeHandle = snpe2_capi.Snpe_TensorShape_CreateDimsSize(shape.second.data(), shape.second.size());
            snpe2_capi.Snpe_TensorShapeMap_Add(m_inputShapeMapHandle, shape.first.c_str(), inputShapeHandle);
        }
        snpe2_capi.Snpe_SNPEBuilder_SetInputDimensions(snpeBuilderHandle, m_inputShapeMapHandle);
    }

    if (support_SigndPD) {
        // test
        m_platformconfig = snpe2_capi.Snpe_PlatformConfig_Create();
        const char* str_platform = snpe2_capi.Snpe_PlatformConfig_GetPlatformOptions(m_platformconfig);
        AISDK_LOG_TRACE("before PlatformConfig {}", str_platform);
        int setok = snpe2_capi.Snpe_PlatformConfig_SetPlatformOptions(m_platformconfig, "unsignedPD:OFF");
        int setok1 = snpe2_capi.Snpe_PlatformConfig_SetPlatformOptionValue(m_platformconfig, "unsignedPD", "OFF");
        const char* str_platform1 = snpe2_capi.Snpe_PlatformConfig_GetPlatformOptions(m_platformconfig);
        AISDK_LOG_TRACE("after PlatformConfig {} setok={}, setok1={}", str_platform1, setok, setok1);
        int SigndPD = snpe2_capi.Snpe_PlatformConfig_IsValid(m_platformconfig);
        AISDK_LOG_TRACE("PlatformConfig_IsValid = {}", SigndPD);
        snpe2_capi.Snpe_SNPEBuilder_SetPlatformConfig(snpeBuilderHandle, m_platformconfig);
        AISDK_LOG_TRACE("Snpe_SNPEBuilder_Build Signed dsp\n");
    }

    m_snpe = snpe2_capi.Snpe_SNPEBuilder_Build(snpeBuilderHandle);
    if (nullptr == m_snpe) {
        const char* errStr = snpe2_capi.Snpe_ErrorCode_GetLastErrorString();
        AISDK_LOG_ERROR("SNPE build failed: {}", errStr);
        return false;
    }
    AISDK_LOG_TRACE("build success! ");

    // get input tensor names of the network that need to be populated
    Snpe_StringList_Handle_t inputNamesHandle = snpe2_capi.Snpe_SNPE_GetInputTensorNames(m_snpe);

    assert(snpe2_capi.Snpe_StringList_Size(inputNamesHandle) > 0);

    AISDK_LOG_TRACE("Creating input userbuffer");

    // create SNPE user buffers for each application storage buffer
    if (nullptr == m_inputUserBufferMap) m_inputUserBufferMap = snpe2_capi.Snpe_UserBufferMap_Create();
    for (size_t i = 0; i < snpe2_capi.Snpe_StringList_Size(inputNamesHandle); ++i) {
        const char* name = snpe2_capi.Snpe_StringList_At(inputNamesHandle, i);
        // get attributes of buffer by name
        auto bufferAttributesOptHandle = snpe2_capi.Snpe_SNPE_GetInputOutputBufferAttributes(m_snpe, name);
        if (nullptr == bufferAttributesOptHandle) {
            AISDK_LOG_ERROR("Error obtaining attributes for input tensor: {}", name);
            return false;
        }

        auto bufferShapeHandle = snpe2_capi.Snpe_IBufferAttributes_GetDims(bufferAttributesOptHandle);
        std::vector<size_t> tensorShape;
        for (size_t j = 0; j < snpe2_capi.Snpe_TensorShape_Rank(bufferShapeHandle); j++) {
            tensorShape.push_back(snpe2_capi.Snpe_TensorShape_At(bufferShapeHandle, j));
        }
        m_inputShapes.emplace(name, tensorShape);

        // size_t bufferElementSize =
        // Snpe_IBufferAttributes_GetElementSize(bufferAttributesOptHandle);
        createUserBuffer(m_inputUserBufferMap, m_inputTensors, m_inputUserBuffers, bufferShapeHandle, name,
                         sizeof(float));

        snpe2_capi.Snpe_IBufferAttributes_Delete(bufferAttributesOptHandle);
        snpe2_capi.Snpe_TensorShape_Delete(bufferShapeHandle);
    }
    snpe2_capi.Snpe_StringList_Delete(inputNamesHandle);

    AISDK_LOG_TRACE("Creating output userbuffer");

    // get output tensor names of the network that need to be populated
    if (nullptr == m_outputUserBufferMap) m_outputUserBufferMap = snpe2_capi.Snpe_UserBufferMap_Create();
    Snpe_StringList_Handle_t outputNamesHandle = snpe2_capi.Snpe_SNPE_GetOutputTensorNames(m_snpe);
    if (nullptr == outputNamesHandle) {
        AISDK_LOG_TRACE("Error obtaining input tensor names");
        throw std::runtime_error("Error obtaining input tensor names");
    }
    assert(snpe2_capi.Snpe_StringList_Size(outputNamesHandle) > 0);

    // create SNPE user buffers for each application storage buffer
    for (size_t i = 0; i < snpe2_capi.Snpe_StringList_Size(outputNamesHandle); ++i) {
        const char* name = snpe2_capi.Snpe_StringList_At(outputNamesHandle, i);
        // get attributes of buffer by name
        auto bufferAttributesOptHandle = snpe2_capi.Snpe_SNPE_GetInputOutputBufferAttributes(m_snpe, name);
        if (!bufferAttributesOptHandle) {
            AISDK_LOG_ERROR("Error obtaining attributes for input tensor: {}", name);
            return false;
        }

        auto bufferShapeHandle = snpe2_capi.Snpe_IBufferAttributes_GetDims(bufferAttributesOptHandle);
        std::vector<size_t> tensorShape;
        for (size_t j = 0; j < snpe2_capi.Snpe_TensorShape_Rank(bufferShapeHandle); j++) {
            tensorShape.push_back(snpe2_capi.Snpe_TensorShape_At(bufferShapeHandle, j));
        }
        m_outputShapes.emplace(name, tensorShape);

        // size_t bufferElementSize =
        // Snpe_IBufferAttributes_GetElementSize(bufferAttributesOptHandle);
        createUserBuffer(m_outputUserBufferMap, m_outputTensors, m_outputUserBuffers, bufferShapeHandle, name,
                         sizeof(float));

        snpe2_capi.Snpe_IBufferAttributes_Delete(bufferAttributesOptHandle);
        snpe2_capi.Snpe_TensorShape_Delete(bufferShapeHandle);
    }

    snpe2_capi.Snpe_StringList_Delete(outputNamesHandle);
    snpe2_capi.Snpe_SNPEBuilder_Delete(snpeBuilderHandle);

    AISDK_LOG_TRACE("SNPE init complete!");

    m_isInit = true;

    return true;
}

bool SNPEWrapper::release() {
    if (nullptr != m_runtimeList) snpe2_capi.Snpe_RuntimeList_Delete(m_runtimeList);
    for (auto& input : m_inputUserBuffers) {
        if (nullptr != input) snpe2_capi.Snpe_IUserBuffer_Delete(input);
    }
    m_inputUserBuffers.clear();
    for (auto& output : m_outputUserBuffers) {
        if (nullptr != output) snpe2_capi.Snpe_IUserBuffer_Delete(output);
    }
    m_outputUserBuffers.clear();

    if (nullptr != m_inputUserBufferMap) snpe2_capi.Snpe_UserBufferMap_Delete(m_inputUserBufferMap);
    if (nullptr != m_outputUserBufferMap) snpe2_capi.Snpe_UserBufferMap_Delete(m_outputUserBufferMap);
    if (nullptr != m_snpe) snpe2_capi.Snpe_SNPE_Delete(m_snpe);
    if (nullptr != m_container) snpe2_capi.Snpe_DlContainer_Delete(m_container);
    if (nullptr != m_inputShapeMapHandle) snpe2_capi.Snpe_TensorShape_Delete(m_inputShapeMapHandle);
    return true;
}

bool SNPEWrapper::setOutputLayers(std::vector<std::string>& outputLayers) {
    if (nullptr == m_set_outputLayers) m_set_outputLayers = snpe2_capi.Snpe_StringList_Create();

    for (size_t i = 0; i < outputLayers.size(); i++) {
        if (SNPE_SUCCESS != snpe2_capi.Snpe_StringList_Append(m_set_outputLayers, outputLayers[i].c_str())) {
            AISDK_LOG_ERROR("Append output name: {} failed: {}.", outputLayers[i].c_str(),
                            snpe2_capi.Snpe_ErrorCode_GetLastErrorString());
            return false;
        }
    }

    return true;
}

bool SNPEWrapper::setOutputTensors(std::vector<std::string>& outputTensors) {
    if (nullptr == m_set_outputTensors) m_set_outputTensors = snpe2_capi.Snpe_StringList_Create();

    for (size_t i = 0; i < outputTensors.size(); i++) {
        if (SNPE_SUCCESS != snpe2_capi.Snpe_StringList_Append(m_set_outputTensors, outputTensors[i].c_str())) {
            AISDK_LOG_ERROR("Append output name: {} failed: {}.", outputTensors[i].c_str(),
                            snpe2_capi.Snpe_ErrorCode_GetLastErrorString());
            return false;
        }
    }

    return true;
}

std::map<std::string, std::vector<size_t>> SNPEWrapper::getInputTensorAttrs() { return m_inputShapes; }

std::map<std::string, std::vector<size_t>> SNPEWrapper::getOutputTensorAttrs() { return m_outputShapes; }

std::vector<size_t> SNPEWrapper::getInputShape(const std::string& name) {
    if (isInit()) {
        if (m_inputShapes.find(name) != m_inputShapes.end()) {
            return m_inputShapes.at(name);
        }
        AISDK_LOG_ERROR("Can't find any input layer named {}", name.c_str());
        return {};
    } else {
        AISDK_LOG_ERROR(
            "The getInputShape() needs to be called after SNPE2 is "
            "initialized!");
        return {};
    }
}

std::vector<size_t> SNPEWrapper::getOutputShape(const std::string& name) {
    if (isInit()) {
        if (m_outputShapes.find(name) != m_outputShapes.end()) {
            return m_outputShapes.at(name);
        }
        AISDK_LOG_ERROR("Can't find any ouput layer named {}", name.c_str());
        return {};
    } else {
        AISDK_LOG_ERROR(
            "The getOutputShape() needs to be called after SNPE2 is "
            "initialized!");
        return {};
    }
}

float* SNPEWrapper::getInputTensor(const std::string& name) {
    if (isInit()) {
        if (m_inputTensors.find(name) != m_inputTensors.end()) {
            return reinterpret_cast<float*>(m_inputTensors.at(name).data());
        }
        AISDK_LOG_ERROR("Can't find any input tensor named {}", name.c_str());
        return nullptr;
    } else {
        AISDK_LOG_ERROR(
            "The getInputTensor() needs to be called after SNPE2 is "
            "initialized!");
        return nullptr;
    }
}

float* SNPEWrapper::getOutputTensor(const std::string& name) {
    if (isInit()) {
        if (m_outputTensors.find(name) != m_outputTensors.end()) {
            return reinterpret_cast<float*>(m_outputTensors.at(name).data());
        }
        AISDK_LOG_ERROR("Can't find any output tensor named {}", name.c_str());
        return nullptr;
    } else {
        AISDK_LOG_ERROR(
            "The getOutputTensor() needs to be called after SNPE2 is "
            "initialized!");
        return nullptr;
    }
}

bool SNPEWrapper::execute() {
    if (SNPE_SUCCESS != snpe2_capi.Snpe_SNPE_ExecuteUserBuffers(m_snpe, m_inputUserBufferMap, m_outputUserBufferMap)) {
        AISDK_LOG_ERROR("SNPE2 execute failed: {}", snpe2_capi.Snpe_ErrorCode_GetLastErrorString());
        return false;
    }

    return true;
}
