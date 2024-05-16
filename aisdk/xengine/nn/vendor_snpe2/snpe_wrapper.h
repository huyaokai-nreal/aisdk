/*
 * @Author: jszhang jszhang@nreal.ai
 * @Date: 2023-03-31 03:09:21
 * @LastEditors: jszhang jszhang@nreal.ai
 * @LastEditTime: 2023-04-10 07:38:42
 * @FilePath: /SNPE2_demo/src/snpe/snpe_wrapper.h
 */
#ifndef __SNPE_WRAPPER__
#define __SNPE_WRAPPER__

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "aisdk/xengine/nr_snpe_header.h"
#include "snpe_lib_wrapper.h"

class SNPEWrapper {
   public:
    SNPEWrapper();
    ~SNPEWrapper();

    bool init(const std::string& model_path, const std::string& runtime);
    bool init(const uint8_t* buffer, const size_t size, const std::string& runtime);
    bool release();
    bool setInputShape(const std::string& name, const std::vector<size_t>& shape);
    bool setOutputLayers(std::vector<std::string>& outputLayers);
    bool setOutputTensors(std::vector<std::string>& outputTensors);

    std::map<std::string, std::vector<size_t>> getInputTensorAttrs();
    std::map<std::string, std::vector<size_t>> getOutputTensorAttrs();

    std::vector<size_t> getInputShape(const std::string& name);
    std::vector<size_t> getOutputShape(const std::string& name);

    float* getInputTensor(const std::string& name);
    float* getOutputTensor(const std::string& name);

    bool isInit() { return m_isInit; }

    bool execute();

   private:
    bool m_isInit = false;

    std::map<std::string, std::vector<size_t>> m_inputShapeList;

    Snpe_DlContainer_Handle_t m_container;
    Snpe_PlatformConfig_Handle_t m_platformconfig;
    Snpe_SNPE_Handle_t m_snpe;
    Snpe_Runtime_t m_runtime;
    Snpe_RuntimeList_Handle_t m_runtimeList;
    Snpe_StringList_Handle_t m_set_outputLayers;
    Snpe_StringList_Handle_t m_set_outputTensors;

    std::map<std::string, std::vector<size_t>> m_inputShapes;
    std::map<std::string, std::vector<size_t>> m_outputShapes;

    std::vector<Snpe_IUserBuffer_Handle_t> m_inputUserBuffers;
    std::vector<Snpe_IUserBuffer_Handle_t> m_outputUserBuffers;
    Snpe_UserBufferMap_Handle_t m_inputUserBufferMap;
    Snpe_UserBufferMap_Handle_t m_outputUserBufferMap;

    Snpe_TensorShapeMap_Handle_t m_inputShapeMapHandle;

    std::unordered_map<std::string, std::vector<uint8_t>> m_inputTensors;
    std::unordered_map<std::string, std::vector<uint8_t>> m_outputTensors;

    void createUserBuffer(Snpe_UserBufferMap_Handle_t userBufferMapHandle,
                          std::unordered_map<std::string, std::vector<uint8_t>>& applicationBuffers,
                          std::vector<Snpe_IUserBuffer_Handle_t>& snpeUserBackedBuffersHandle,
                          Snpe_TensorShape_Handle_t bufferShapeHandle, const char* name, size_t bufferElementSize);

    SnpeCInterface snpe2_capi;
};

#endif  // __SNPE_WRAPPER__