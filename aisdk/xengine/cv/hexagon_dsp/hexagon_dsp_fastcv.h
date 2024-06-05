
#ifndef _HEXAGON_DSP_FASTCV_H_
#define _HEXAGON_DSP_FASTCV_H_

namespace aisdk::xengine {

typedef int (*xrealai_calculator_test)(int runLocal, int domain_id, int num, bool is_unsignedpd_enabled);

class HexagonDspInterface {
public:
    HexagonDspInterface() {}
    ~HexagonDspInterface() {}
    bool Init();
    // 这里一定要区分adsp还是cdsp
    // adsp：V66架构   cdsp：V73架构
    // 其中Stub和Skel的so是不易兼容，混用会造成崩溃阻塞等问题。 
    bool DspSupport(int domain_id, bool is_unsignedpd);
private:
    void* so_handle = nullptr;
    xrealai_calculator_test calculator_test = nullptr;
};

}

#endif