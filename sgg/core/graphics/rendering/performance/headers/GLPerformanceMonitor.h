#pragma once
#include <chrono>
#include <thread>
#include <cstdint>
#include "core/utils/headers/CompilerTraits.h"

class GLPerformanceMonitor {
public:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;
    using Duration = std::chrono::duration<double, std::milli>;

    GLPerformanceMonitor() noexcept = default;

    // Call at start of each frame - marked as hot path
    ATTR_GNU_HOT [[msvc::forceinline]] void startFrame() noexcept {
        frameStart = Clock::now();
    }

    // Call at end of each frame; returns ms spent - critical hot path
    ATTR_GNU_HOT double endFrame() noexcept {
        const auto now = Clock::now();
        delta = Duration(now - frameStart).count();

        // FPS calculation with smoothing (no limiting here - handled by GLBackend)
        updateFPSSmoothing();

        return delta;
    }

    // Set a hard FPS cap (0 = uncapped) - precalculate target frame time
    void setLimitFPS(int cap) noexcept {
        limitFPS = cap;
        targetFrameTime = (cap > 0) ? (1000.0 / cap) : 0.0;
    }

    // Get target frame time for external FPS limiting
    [[nodiscard]] double getTargetFrameTime() const noexcept {
        return targetFrameTime;
    }

    // Get current FPS limit setting
    [[nodiscard]] int getFPSLimit() const noexcept {
        return limitFPS;
    }

    // Getters - inline for performance
    ATTR_GNU_HOT [[msvc::forceinline]] double getFPS() const noexcept {
        return smoothedFPS;
    }

    ATTR_GNU_HOT [[msvc::forceinline]] double getFrameTimeMs() const noexcept {
        return delta;
    }

    // Get current frame time (elapsed since startFrame()) - accurate during frame execution
    ATTR_GNU_HOT [[msvc::forceinline]] double getCurrentFrameTimeMs() const noexcept {
        if (frameStart.time_since_epoch().count() == 0) {
            return 0.0;
        }
        const auto now = Clock::now();
        return Duration(now - frameStart).count();
    }

    // Get inter-frame delta time (time since last frame) - for physics/animation
    ATTR_GNU_HOT [[msvc::forceinline]] double getInterFrameDeltaMs() noexcept {
        const auto now = Clock::now();
        static auto lastFrameTime = now; // Initialize to current time
        const double deltaMs = Duration(now - lastFrameTime).count();
        lastFrameTime = now;
        return deltaMs;
    }

    // Get instantaneous FPS (not smoothed) - useful for debugging
    double getInstantaneousFPS() const noexcept {
        return (delta > 0.0) ? (1000.0 / delta) : 0.0;
    }

    // Reset statistics
    void reset() noexcept {
        frameCount = 0;
        timeAccumulator = 0.0;
        smoothedFPS = 0.0;
        delta = 0.0;
        isFirstFrame = true;
        lastUpdateTime = Clock::now();
    }

    // Get frame statistics
    struct FrameStats {
        double avgFPS;
        double avgFrameTime;
        uint32_t totalFrames;
        double totalTime;
    };

    FrameStats getFrameStats() const noexcept {
        return {
            smoothedFPS,
            (smoothedFPS > 0.0) ? (1000.0 / smoothedFPS) : 0.0,
            totalFrameCount,
            totalTimeAccumulator
        };
    }

private:
    // Core timing data
    TimePoint frameStart{};
    TimePoint lastUpdateTime{Clock::now()};
    double delta{0.0};
    bool isFirstFrame{true};

    // FPS limiting
    int limitFPS{0};
    double targetFrameTime{0.0}; // Precalculated to avoid division in hot path

    // FPS smoothing - configurable interval
    uint32_t frameCount{0};
    double timeAccumulator{0.0};
    double smoothedFPS{0.0};

    // Long-term statistics
    uint32_t totalFrameCount{0};
    double totalTimeAccumulator{0.0};

    // Configuration - made adaptive for very high FPS
    static constexpr double minUpdateIntervalMs = 50.0;   // Minimum update interval
    static constexpr double maxUpdateIntervalMs = 200.0;  // Maximum update interval
    static constexpr uint32_t minFramesForUpdate = 10;    // Minimum frames before update

    // Improved FPS calculation with better edge case handling
    ATTR_GNU_HOT void updateFPSSmoothing() noexcept {
        const auto now = Clock::now();

        // Handle first frame
        if (isFirstFrame) [[unlikely]] {
            isFirstFrame = false;
            lastUpdateTime = now;
            smoothedFPS = getInstantaneousFPS();
            return;
        }

        timeAccumulator += delta;
        frameCount++;
        totalFrameCount++;
        totalTimeAccumulator += delta;

        // Calculate time since last update
        const double timeSinceUpdate = Duration(now - lastUpdateTime).count();

        // Update conditions:
        // 1. We've accumulated enough time (adaptive based on FPS)
        // 2. We have minimum number of frames
        // 3. Don't wait too long for very slow framerates
        const bool timeCondition = timeSinceUpdate >= minUpdateIntervalMs;
        const bool frameCondition = frameCount >= minFramesForUpdate;
        const bool maxTimeCondition = timeSinceUpdate >= maxUpdateIntervalMs;

        if ((timeCondition && frameCondition) || maxTimeCondition) [[unlikely]] {
            // Use actual elapsed time for more accuracy
            if (timeSinceUpdate > 0.0 && frameCount > 0) {
                smoothedFPS = frameCount * 1000.0 / timeSinceUpdate;

                // Sanity check - clamp to reasonable values
                if (smoothedFPS > 1000000.0) smoothedFPS = 1000000.0;
                if (smoothedFPS < 0.1) smoothedFPS = 0.1;
            }

            // Reset accumulators
            frameCount = 0;
            timeAccumulator = 0.0;
            lastUpdateTime = now;
        }
    }
};

// RAII frame timer for automatic measurement
class FrameTimer {
public:
    explicit FrameTimer(GLPerformanceMonitor& monitor) noexcept
        : monitor_(monitor) {
        monitor_.startFrame();
    }

    ~FrameTimer() noexcept {
        monitor_.endFrame();
    }

    // Non-copyable, non-movable
    FrameTimer(const FrameTimer&) = delete;
    FrameTimer& operator=(const FrameTimer&) = delete;
    FrameTimer(FrameTimer&&) = delete;
    FrameTimer& operator=(FrameTimer&&) = delete;

private:
    GLPerformanceMonitor& monitor_;
};