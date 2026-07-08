#include "dataIO.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

bool DataIO::validateItemTokens(int tokenCount, int M) {
    return tokenCount == M + 1; // 1 value + M weights
}

bool DataIO::readInstance(const std::string& filePath, Instance& outInstance) {
    std::ifstream fin(filePath);
    if (!fin.is_open()) {
        std::cerr << "[DataIO] Failed to open file: " << filePath << std::endl;
        return false;
    }

    outInstance = Instance{};
    outInstance.name = std::filesystem::path(filePath).filename().string();

    // Line 1: N M
    if (!(fin >> outInstance.N >> outInstance.M)) {
        std::cerr << "[DataIO] Failed to read N, M from: " << filePath << std::endl;
        return false;
    }

    int N = outInstance.N;
    int M = outInstance.M;

    // Line 2: Q1 Q2 ... QM
    outInstance.capacity.resize(M);
    for (int m = 0; m < M; ++m) {
        if (!(fin >> outInstance.capacity[m])) {
            std::cerr << "[DataIO] Failed to read capacity[" << m << "] from: " << filePath << std::endl;
            return false;
        }
    }

    // N classes, each: I_n, then I_n lines of (V + M weights)
    outInstance.classes.resize(N);
    for (int i = 0; i < N; ++i) {
        int Ii;
        if (!(fin >> Ii)) {
            std::cerr << "[DataIO] Failed to read I_" << i << " from: " << filePath << std::endl;
            return false;
        }

        ItemClass& cls = outInstance.classes[i];
        cls.items.resize(Ii);

        for (int j = 0; j < Ii; ++j) {
            Item& item = cls.items[j];
            item.weight.resize(M);

            if (!(fin >> item.value)) {
                std::cerr << "[DataIO] Failed to read value at class " << i
                          << ", item " << j << " in: " << filePath << std::endl;
                return false;
            }

            for (int m = 0; m < M; ++m) {
                if (!(fin >> item.weight[m])) {
                    std::cerr << "[DataIO] Failed to read weight[" << m << "] at class " << i
                              << ", item " << j << " in: " << filePath << std::endl;
                    return false;
                }
            }
        }
    }

    return true;
}
