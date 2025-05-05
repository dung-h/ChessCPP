#pragma once
#include <cstdint>
#include <random>
#include <chrono>

extern uint64_t zobristTable[2][6][64];
extern uint64_t zobristSide;

inline void initializeZobrist() {
    std::mt19937_64 rng((unsigned)std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<uint64_t> dist;
    for (int c = 0; c < 2; c++) {
        for (int pt = 0; pt < 6; pt++) {
            for (int sq = 0; sq < 64; sq++) {
                zobristTable[c][pt][sq] = dist(rng);
            }
        }
    }
    zobristSide = dist(rng);
}