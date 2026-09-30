#pragma once
#include <atomic>

class Mk64GameRuntime {
public:
    bool initialize();
    void run_ticks(unsigned ticks);
    void render();
    bool initialized() const { return initialized_.load(); }
private:
    std::atomic<bool> initialized_{false};
    unsigned long long tick_count_ = 0;
};

Mk64GameRuntime& mk64_game_runtime();
