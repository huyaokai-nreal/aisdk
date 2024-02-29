/*
 * @Author: jszhang jszhang@nreal.ai
 * @Date: 2023-03-23 05:58:54
 * @LastEditors: jszhang jszhang@nreal.ai
 * @LastEditTime: 2023-04-14 06:35:39
 * @FilePath: /nreal_hand_demo_android/src/core/pipeline/impl/handtracking/function/hand_state.h
 */
#pragma once

#include <algorithm>
#include <deque>
#include <functional>
#include <iostream>
#include <map>

namespace aisdk::algorithm {

enum class HandState { Tracking, Uncertain, Lost };

class HandStateMachine {
   public:
    HandStateMachine(float threshold, size_t threshold_count) : threshold(threshold), threshold_count(threshold_count) {
        current_state = HandState::Lost;
    }

    void handle_score(float score) {
        std::lock_guard<std::mutex> lock(m_mutex);
        score_history.push_back(score);
        if (score_history.size() > threshold_count) {
            score_history.pop_front();
        }

        switch (current_state) {
            case HandState::Tracking:
                if (score < threshold) {
                    current_state = HandState::Uncertain;
                }
                break;
            case HandState::Uncertain:
                if (is_consecutive_above_threshold()) {
                    current_state = HandState::Tracking;
                } else if (is_consecutive_below_threshold()) {
                    current_state = HandState::Lost;
                }
                break;
            case HandState::Lost:
                if (is_consecutive_above_threshold()) {
                    current_state = HandState::Tracking;
                }
                break;
        }
    }
    HandState get_current_state() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return current_state;
    }

   private:
    bool is_consecutive_above_threshold() const {
        return std::all_of(score_history.begin(), score_history.end(),
                           [this](float score) { return score > threshold; });
    }

    bool is_consecutive_below_threshold() const {
        return std::all_of(score_history.begin(), score_history.end(),
                           [this](float score) { return score <= threshold; });
    }

    float threshold;
    size_t threshold_count;
    std::deque<float> score_history;
    HandState current_state;

    mutable std::mutex m_mutex;
};
}  // namespace aisdk::algorithm
