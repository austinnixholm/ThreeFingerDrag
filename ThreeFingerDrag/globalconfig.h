#pragma once
#include <iostream>

constexpr auto DEFAULT_ACCELERATION_FACTOR = 75.0;
constexpr auto DEFAULT_NUM_SKIPPED_FRAMES = 3;

class GlobalConfig {
private:
    double gesture_speed_;
    int skipped_gesture_frames_;
    static GlobalConfig* instance;

    // Private constructor
    GlobalConfig() {
        // Set default values
        gesture_speed_ = DEFAULT_ACCELERATION_FACTOR;
        skipped_gesture_frames_ = DEFAULT_NUM_SKIPPED_FRAMES;
    }

public:
    // Singleton instance
    static GlobalConfig* GetInstance() {
        if (instance == nullptr) 
            instance = new GlobalConfig();
        return instance;
    }

    int GetSkippedGestureFrames() const {
        return skipped_gesture_frames_;
    }

    void SetSkippedGestureFrames(int frames) {
        skipped_gesture_frames_ = frames;
    }

    double GetGestureSpeed() const {
        return gesture_speed_;
    }

    void SetGestureSpeed(double speed) {
        gesture_speed_ = speed;
    }


};

GlobalConfig* GlobalConfig::instance = nullptr;
