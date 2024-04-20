#pragma once

#include <deque>

#include <opencv2/opencv.hpp>
#include "aisdk/base/type.h"
#include "ofxOneEuroFilter.h"

namespace aisdk::algorithm {

struct BoxData {
    float score;
    cv::Rect bbox_xywh;
};

struct KptData {
    float score;
    Vec2f_t kpt2d;
};

struct HandStepData {
    HandStepData() { kpt = std::vector<KptData>(21, {0, Vec2f_t{0, 0}}); };

    BoxData bbox;
    std::vector<KptData> kpt;
};

struct DynamicData2D {
    DynamicData2D() {
        x.resize(3);
        y.resize(3);
    }
    std::vector<float> x;
    std::vector<float> y;
};

struct DynamicData3D {
    DynamicData3D() {
        x.resize(3);
        y.resize(3);
        z.resize(3);
    }
    std::vector<float> x;
    std::vector<float> y;
    std::vector<float> z;
};

class SeqManager final {
   public:
    SeqManager();
    ~SeqManager(){};
    // EMA
    bool getCurrentHandData(cv::Rect& bbox, float& score, std::vector<Vec2f_t>& kpt_2d,
                            std::vector<float>& score_kpt);
    // 1 Euro
    bool getFilterBoxData(cv::Rect& bbox, float& score);
    bool getFilterKptData(std::vector<Vec2f_t>& kpt_2d, std::vector<float>& score_kpt);
    bool reset();

   private:
    int mSlideWindowSize;
    int mStepCount;
    float mSeqParam[5];
    std::deque<HandStepData> mHandDataSavedSeq;
    std::vector<std::unique_ptr<aisdk::algorithm::OneEuroFilter>> mOneEuroFilterList;
};

class SeqManager2D final {
   public:
    SeqManager2D();
    ~SeqManager2D(){};

    bool updateSeq2D(const std::vector<Vec2f_t>& kpt_2d);
    bool reset();
    std::vector<std::vector<Vec2f_t>> getSeq();

   private:
    std::deque<std::vector<Vec2f_t>> mHandDataSeq;
    int mWindowLength;
};

class DynamicFilter3D final {
   public:
    DynamicFilter3D(int sample_num, int dim, int window, float lambda);
    ~DynamicFilter3D(){};

    bool getDynamicFilterHandData(std::vector<Vec3f_t>& kpt_3d);
    bool reset();

   private:
    void getHandDatafromSeq();
    float mean(std::vector<float> input);
    float var(std::vector<float> input);

    std::deque<std::vector<Vec3f_t>> mHandDataSeq;
    std::vector<Vec3f_t> mPreData;
    std::vector<DynamicData3D> mHandData;
    int mWindowLength;
    int mDim;
    int mSampleNum;
    float mLambda;
};

class DynamicFilter2D final {
   public:
    DynamicFilter2D(int dim, int window, float lambda);
    ~DynamicFilter2D(){};

    bool getDynamicFilterHandData(std::vector<Vec2f_t>& kpt_2d);
    bool reset();

   private:
    void getHandDatafromSeq();
    float mean(std::vector<float> input);
    float var(std::vector<float> input);

    float unexplicit_var(std::vector<float> input, int pnt_index, int coord_index);

    std::deque<std::vector<Vec2f_t>> mHandDataSeq;
    std::vector<Vec2f_t> mPreData;
    std::vector<DynamicData2D> mHandData;
    int mWindowLength;
    int mDim;
    float mLambda;
};

typedef struct OneEuroParams {
    OneEuroParams() {
        mincutoff.resize(3);
        beta.resize(3);
        dcutoff.resize(3);
    }

    float freq;
    std::vector<float> mincutoff;
    std::vector<float> beta;
    std::vector<float> dcutoff;
} OneEuroParams;

class SeqManager3D final {
   public:
    SeqManager3D(int sample_num, const OneEuroParams& params);
    ~SeqManager3D(){};

    bool getFilterHandData(std::vector<Vec3f_t>& kpt_3d);

    bool reset();

   private:
    int mSampleNum;
    std::vector<std::unique_ptr<aisdk::algorithm::OneEuroFilter>> mOneEuroFilterList;
};
class SeqManagerVec final {
   public:
    SeqManagerVec(const OneEuroParams& params);
    ~SeqManagerVec(){};

    bool getFilterVecData(Vec3f_t& velocity);

    bool reset();

   private:
    std::vector<std::unique_ptr<aisdk::algorithm::OneEuroFilter>> mOneEuroFilterList;
};

}  // namespace XrealAI
