#include "NR_Seq_Manager.h"

#include <memory>
#include <numeric>

#include "aisdk/base/log.h"
#include "aisdk/base/type.h"

namespace aisdk::algorithm {

SeqManager::SeqManager() : mSlideWindowSize(5), mStepCount(0) {
    mSeqParam[0] = 0.0;
    mSeqParam[1] = 0.0;
    mSeqParam[2] = 0.1;
    mSeqParam[3] = 0.2;
    mSeqParam[4] = 0.7;

    mOneEuroFilterList.resize(46);
    for (int i = 0; i < 46; i++) {
        mOneEuroFilterList[i] = std::unique_ptr<OneEuroFilter>(new OneEuroFilter(30, 4.0, 0.005, 5.0));
    }
}

bool SeqManager::reset() {
    mHandDataSavedSeq.clear();
    return true;
}

bool SeqManager::getCurrentHandData(cv::Rect& bbox, float& score, std::vector<Vec2f_t>& kpt_2d,
                                    std::vector<float>& score_kpt) {
    HandStepData temp;
    if (mHandDataSavedSeq.size() < mSlideWindowSize) {
        temp.bbox.bbox_xywh = bbox;
        temp.bbox.score = score;

        for (int i = 0; i < 21; i++) {
            temp.kpt[i].kpt2d = kpt_2d[i];
            temp.kpt[i].score = score_kpt[i];
        }

        mHandDataSavedSeq.push_back(temp);
    }

    else {
        mHandDataSavedSeq.pop_front();

        int x1, y1, x2, y2;

        x1 = mSeqParam[0] * mHandDataSavedSeq[0].bbox.bbox_xywh.x +
             mSeqParam[1] * mHandDataSavedSeq[1].bbox.bbox_xywh.x +
             mSeqParam[2] * mHandDataSavedSeq[2].bbox.bbox_xywh.x +
             mSeqParam[3] * mHandDataSavedSeq[3].bbox.bbox_xywh.x + mSeqParam[4] * bbox.x;

        y1 = mSeqParam[0] * mHandDataSavedSeq[0].bbox.bbox_xywh.y +
             mSeqParam[1] * mHandDataSavedSeq[1].bbox.bbox_xywh.y +
             mSeqParam[2] * mHandDataSavedSeq[2].bbox.bbox_xywh.y +
             mSeqParam[3] * mHandDataSavedSeq[3].bbox.bbox_xywh.y + mSeqParam[4] * bbox.y;

        x2 = mSeqParam[0] * (mHandDataSavedSeq[0].bbox.bbox_xywh.x + mHandDataSavedSeq[0].bbox.bbox_xywh.width) +
             mSeqParam[1] * (mHandDataSavedSeq[1].bbox.bbox_xywh.x + mHandDataSavedSeq[1].bbox.bbox_xywh.width) +
             mSeqParam[2] * (mHandDataSavedSeq[2].bbox.bbox_xywh.x + mHandDataSavedSeq[2].bbox.bbox_xywh.width) +
             mSeqParam[3] * (mHandDataSavedSeq[3].bbox.bbox_xywh.x + mHandDataSavedSeq[3].bbox.bbox_xywh.width) +
             mSeqParam[4] * (bbox.x + bbox.width);

        y2 = mSeqParam[0] * (mHandDataSavedSeq[0].bbox.bbox_xywh.y + mHandDataSavedSeq[0].bbox.bbox_xywh.height) +
             mSeqParam[1] * (mHandDataSavedSeq[1].bbox.bbox_xywh.y + mHandDataSavedSeq[1].bbox.bbox_xywh.height) +
             mSeqParam[2] * (mHandDataSavedSeq[2].bbox.bbox_xywh.y + mHandDataSavedSeq[2].bbox.bbox_xywh.height) +
             mSeqParam[3] * (mHandDataSavedSeq[3].bbox.bbox_xywh.y + mHandDataSavedSeq[3].bbox.bbox_xywh.height) +
             mSeqParam[4] * (bbox.y + bbox.height);

        bbox = {x1, y1, x2 - x1, y2 - y1};
        temp.bbox.bbox_xywh = bbox;
        temp.bbox.score = score;

        for (int i = 0; i < 21; i++) {
            Vec2f_t temp_kpt;
            temp_kpt = mSeqParam[0] * mHandDataSavedSeq[0].kpt[i].kpt2d +
                       mSeqParam[1] * mHandDataSavedSeq[1].kpt[i].kpt2d +
                       mSeqParam[2] * mHandDataSavedSeq[2].kpt[i].kpt2d +
                       mSeqParam[3] * mHandDataSavedSeq[3].kpt[i].kpt2d + mSeqParam[4] * kpt_2d[i];
            temp.kpt[i].kpt2d = temp_kpt;
            temp.kpt[i].score = score_kpt[i];
            kpt_2d[i] = temp_kpt;
        }

        mHandDataSavedSeq.push_back(temp);
    }
    return true;
}

bool SeqManager::getFilterBoxData(DetectRect& bbox, float& score) {
    int x1, y1, x2, y2;
    int x1_r, y1_r, x2_r, y2_r;

    x1 = bbox.x;
    x2 = bbox.x + bbox.w;
    y1 = bbox.y;
    y2 = bbox.y + bbox.h;

    x1_r = mOneEuroFilterList[42]->filter(x1);
    y1_r = mOneEuroFilterList[43]->filter(y1);
    x2_r = mOneEuroFilterList[44]->filter(x2);
    y2_r = mOneEuroFilterList[45]->filter(y2);

    bbox.x = x1_r;
    bbox.y = y1_r;
    bbox.w = x2_r - x1_r;
    bbox.h = y2_r - y1_r;

    return true;
}

bool SeqManager::getFilterKptData(std::vector<Vec2f_t>& kpt_2d, std::vector<float>& score_kpt) {
    for (int i = 0; i < 21; i++) {
        kpt_2d[i][0] = mOneEuroFilterList[i * 2]->filter(kpt_2d[i][0]);
        kpt_2d[i][1] = mOneEuroFilterList[i * 2 + 1]->filter(kpt_2d[i][1]);
    }
    return true;
}

SeqManager2D::SeqManager2D() : mWindowLength(16) {}

bool SeqManager2D::reset() {
    mHandDataSeq.clear();
    return true;
}

bool SeqManager2D::updateSeq2D(const std::vector<Vec2f_t>& kpt_2d) {
    if (mHandDataSeq.size() == mWindowLength) {
        mHandDataSeq.pop_front();
        mHandDataSeq.push_back(kpt_2d);
    } else {
        mHandDataSeq.clear();
        for (size_t i = 0; i < mWindowLength; i++) {
            mHandDataSeq.push_back(kpt_2d);
        }
    }
    return true;
}

std::vector<std::vector<Vec2f_t>> SeqManager2D::getSeq() {
    return {mHandDataSeq[15], mHandDataSeq[14], mHandDataSeq[13], mHandDataSeq[12], mHandDataSeq[11], mHandDataSeq[10]};
}

DynamicFilter3D::DynamicFilter3D(int sample_num, int dim, int window, float lambda)
    : mWindowLength(window), mDim(dim), mLambda(lambda) {
    mSampleNum = sample_num;
    mHandData.resize(mSampleNum);
}

bool DynamicFilter3D::reset() {
    mHandDataSeq.clear();
    for (int i = 0; i < mHandData.size(); i++) {
        mHandData[i].x.clear();
        mHandData[i].y.clear();
        mHandData[i].z.clear();
        mHandData[i].x.resize(mWindowLength);
        mHandData[i].y.resize(mWindowLength);
        mHandData[i].z.resize(mWindowLength);
    }
    return true;
}

void DynamicFilter3D::getHandDatafromSeq() {
    for (size_t s = 0; s < 3; s++) {
        for (size_t k = 0; k < mSampleNum; k++) {
            mHandData[k].x[s] = mHandDataSeq[s][k][0];
            mHandData[k].y[s] = mHandDataSeq[s][k][1];
            mHandData[k].z[s] = mHandDataSeq[s][k][2];
        }
    }
}

float DynamicFilter3D::mean(std::vector<float> input) {
    float sum = std::accumulate(std::begin(input), std::end(input), 0.0);
    return sum / input.size();
}

float DynamicFilter3D::var(std::vector<float> input) {
    float sum = std::accumulate(std::begin(input), std::end(input), 0.0);
    float mean = sum / input.size();

    float variance = 0.0;
    for (size_t i = 0; i < input.size(); i++) {
        variance = variance + pow(input[i] - mean, 2);
    }
    return variance / input.size();
}

bool DynamicFilter3D::getDynamicFilterHandData(std::vector<Vec3f_t>& kpt_3d) {
    mHandDataSeq.push_back(kpt_3d);
    if (mHandDataSeq.size() > mWindowLength) {
        mHandDataSeq.pop_front();
    }

    if (mHandDataSeq.size() == mWindowLength) {
        getHandDatafromSeq();

        std::vector<float> x_datas, y_datas, z_datas;

        for (size_t i = 0; i < mSampleNum; i++) {
            x_datas.push_back(mHandData[i].x[2]);
            y_datas.push_back(mHandData[i].y[2]);
            z_datas.push_back(mHandData[i].z[2]);
        }

        float hand_norm =
            (*std::max_element(x_datas.begin(), x_datas.end()) - *std::min_element(x_datas.begin(), x_datas.end())) *
            (*std::max_element(y_datas.begin(), y_datas.end()) - *std::min_element(y_datas.begin(), y_datas.end())) *
            (*std::max_element(z_datas.begin(), z_datas.end()) - *std::min_element(z_datas.begin(), z_datas.end()));
        hand_norm = hand_norm * mDim * mLambda * 1e-4;

        for (size_t i = 0; i < mSampleNum; i++) {
            float var_x = this->var(mHandData[i].x);
            float var_y = this->var(mHandData[i].y);
            float var_z = this->var(mHandData[i].z);

            if (var_x < 1e-6 || var_y < 1e-6 || var_z < 1e-6) {
                continue;
            }

            float current_weight_x = var_x / hand_norm < 1.0 ? var_x / hand_norm : 1.0;
            float current_weight_y = var_y / hand_norm < 1.0 ? var_y / hand_norm : 1.0;
            float current_weight_z = var_z / hand_norm < 1.0 ? var_z / hand_norm : 1.0;

            AISDK_LOG_INFO("current_weight_3d_x var: {}, hand_norm: {}, current_weight: {}", var_x, hand_norm,
                           current_weight_x);
            AISDK_LOG_INFO("current_weight_3d_y var: {}, hand_norm: {}, current_weight: {}", var_y, hand_norm,
                           current_weight_y);
            AISDK_LOG_INFO("current_weight_3d_z var: {}, hand_norm: {}, current_weight: {}", var_z, hand_norm,
                           current_weight_z);

            kpt_3d[i][0] = kpt_3d[i][0] * current_weight_x + mPreData[i][0] * (1 - current_weight_x);
            kpt_3d[i][1] = kpt_3d[i][1] * current_weight_y + mPreData[i][1] * (1 - current_weight_y);
            kpt_3d[i][2] = kpt_3d[i][2] * current_weight_z + mPreData[i][2] * (1 - current_weight_z);
        }
    }
    mPreData = kpt_3d;
    return true;
}

DynamicFilter2D::DynamicFilter2D(int dim, int window, float lambda)
    : mWindowLength(window), mDim(dim), mLambda(lambda) {
    mHandData.resize(21);
}

bool DynamicFilter2D::reset() {
    mHandDataSeq.clear();
    for (int i = 0; i < mHandData.size(); i++) {
        mHandData[i].x.clear();
        mHandData[i].y.clear();
        mHandData[i].x.resize(mWindowLength);
        mHandData[i].y.resize(mWindowLength);
    }
    return true;
}

void DynamicFilter2D::getHandDatafromSeq() {
    for (size_t s = 0; s < 3; s++) {
        for (size_t k = 0; k < 21; k++) {
            mHandData[k].x[s] = mHandDataSeq[s][k][0];
            mHandData[k].y[s] = mHandDataSeq[s][k][1];
        }
    }
}

float DynamicFilter2D::mean(std::vector<float> input) {
    float sum = std::accumulate(std::begin(input), std::end(input), 0.0);
    return sum / input.size();
}

float DynamicFilter2D::var(std::vector<float> input) {
    float sum = std::accumulate(std::begin(input), std::end(input), 0.0);
    float mean = sum / input.size();

    float variance = 0.0;
    for (size_t i = 0; i < input.size(); i++) {
        variance = variance + pow(input[i] - mean, 2);
    }
    return variance / input.size();
}

float DynamicFilter2D::unexplicit_var(std::vector<float> input, int pnt_index, int coord_index) {
    float mean = mPreData[pnt_index][coord_index];

    float variance = 0.0;
    for (size_t i = 0; i < input.size(); i++) {
        variance = variance + pow(input[i] - mean, 2);
    }
    return variance / input.size();
}

bool DynamicFilter2D::getDynamicFilterHandData(std::vector<Vec2f_t>& kpt_2d) {
    mHandDataSeq.push_back(kpt_2d);
    if (mHandDataSeq.size() > mWindowLength) {
        mHandDataSeq.pop_front();
    }

    if (mHandDataSeq.size() == mWindowLength) {
        getHandDatafromSeq();

        std::vector<float> x_datas, y_datas;

        for (size_t i = 0; i < 21; i++) {
            x_datas.push_back(mHandData[i].x[2]);
            y_datas.push_back(mHandData[i].y[2]);
        }

        float hand_norm =
            (*std::max_element(x_datas.begin(), x_datas.end()) - *std::min_element(x_datas.begin(), x_datas.end())) *
            (*std::max_element(y_datas.begin(), y_datas.end()) - *std::min_element(y_datas.begin(), y_datas.end()));
        hand_norm = hand_norm * mDim * mLambda * 1e-4;

        for (size_t i = 0; i < 21; i++) {
            // float var_x = this->var(mHandData[i].x);
            // float var_y = this->var(mHandData[i].y);
            float var_x = this->unexplicit_var(mHandData[i].x, i, 0);
            float var_y = this->unexplicit_var(mHandData[i].y, i, 1);

            // std::cout << "var: " << var_x << " " << var_y << std::endl;

            if (var_x < 1e-6 || var_y < 1e-6) {
                continue;
            }

            float current_weight_x = var_x / hand_norm < 1.0 ? var_x / hand_norm : 1.0;
            float current_weight_y = var_y / hand_norm < 1.0 ? var_y / hand_norm : 1.0;

            // AISDK_LOG_INFO("current_weight_2d_x length: {}, {}, {}",
            // var_x, hand_norm, current_weight_x);
            // AISDK_LOG_INFO("current_weight_2d_y length: {}, {}, {}",
            // var_y, hand_norm, current_weight_y);

            kpt_2d[i][0] = kpt_2d[i][0] * current_weight_x + mPreData[i][0] * (1 - current_weight_x);
            kpt_2d[i][1] = kpt_2d[i][1] * current_weight_y + mPreData[i][1] * (1 - current_weight_y);
        }
    }
    mPreData = kpt_2d;

    return true;
}

SeqManager3D::SeqManager3D(int sample_num, const OneEuroParams& params) : mSampleNum(sample_num) {
    mOneEuroFilterList.resize(sample_num * 3);
    for (int i = 0; i < sample_num; i++) {  // 30, 0.0005, 20.0, 3.0
        mOneEuroFilterList[i * 3] =
            std::make_unique<OneEuroFilter>(params.freq, params.mincutoff[0], params.beta[0], params.dcutoff[0]);
        mOneEuroFilterList[i * 3 + 1] =
            std::make_unique<OneEuroFilter>(params.freq, params.mincutoff[1], params.beta[1], params.dcutoff[1]);
        mOneEuroFilterList[i * 3 + 2] =
            std::make_unique<OneEuroFilter>(params.freq, params.mincutoff[2], params.beta[2], params.dcutoff[2]);
    }
}

bool SeqManager3D::reset() {
    for (auto& filter : mOneEuroFilterList) {
        filter->reset();
    }
    return true;
}

bool SeqManager3D::getFilterHandData(std::vector<Vec3f_t>& kpt_3d) {
    for (int i = 0; i < mSampleNum; i++) {
        kpt_3d[i][0] = mOneEuroFilterList[i * 3]->filter(kpt_3d[i][0]);
        kpt_3d[i][1] = mOneEuroFilterList[i * 3 + 1]->filter(kpt_3d[i][1]);
        kpt_3d[i][2] = mOneEuroFilterList[i * 3 + 2]->filter(kpt_3d[i][2]);
    }
    return true;
}

SeqManagerVec::SeqManagerVec(const OneEuroParams& params) {
    mOneEuroFilterList.resize(3);
    mOneEuroFilterList[0] = std::unique_ptr<OneEuroFilter>(
        new OneEuroFilter(params.freq, params.mincutoff[0], params.beta[0], params.dcutoff[0]));
    mOneEuroFilterList[1] = std::unique_ptr<OneEuroFilter>(
        new OneEuroFilter(params.freq, params.mincutoff[1], params.beta[1], params.dcutoff[1]));
    mOneEuroFilterList[2] = std::unique_ptr<OneEuroFilter>(
        new OneEuroFilter(params.freq, params.mincutoff[2], params.beta[2], params.dcutoff[2]));
}

bool SeqManagerVec::reset() { return true; }

bool SeqManagerVec::getFilterVecData(Vec3f_t& velocity_3d) {
    velocity_3d[0] = mOneEuroFilterList[0]->filter(velocity_3d[0]);
    velocity_3d[1] = mOneEuroFilterList[1]->filter(velocity_3d[1]);
    velocity_3d[2] = mOneEuroFilterList[2]->filter(velocity_3d[2]);
    return true;
}

}  // namespace aisdk::algorithm
