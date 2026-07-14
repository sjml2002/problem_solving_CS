#include "dataIO.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

bool DataIO::validateItemTokens(int tokenCount, int M) {
    return tokenCount == M + 1;
}

bool DataIO::readInstance(const std::string& filePath, Instance& outInstance) {
    std::ifstream fin(filePath);
    if (!fin.is_open()) {
        std::cerr << "[DataIO] Failed to open file: " << filePath << std::endl;
        return false;
    }

    outInstance = Instance{};
    outInstance.name = std::filesystem::path(filePath).filename().string();

    if (!(fin >> outInstance.N >> outInstance.M)) {
        std::cerr << "[DataIO] Failed to read N, M from: " << filePath << std::endl;
        return false;
    }

    int N = outInstance.N;
    int M = outInstance.M;

    outInstance.capacity.resize(M);
    for (int m = 0; m < M; ++m) {
        if (!(fin >> outInstance.capacity[m])) {
            std::cerr << "[DataIO] Failed to read capacity[" << m << "] from: " << filePath << std::endl;
            return false;
        }
    }

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

bool DataIO::readSolutions(const std::string& csvPath,
                            std::unordered_map<std::string, long long>& outMap) {
    std::ifstream fin(csvPath, std::ios::binary);
    if (!fin.is_open()) {
        std::cerr << "[DataIO] Failed to open solutions CSV: " << csvPath << std::endl;
        return false;
    }

    outMap.clear();
    std::string line;
    bool firstLine = true;

    while (std::getline(fin, line)) {
        if (line.empty()) continue;

        // Strip UTF-8 BOM if present on the very first line.
        if (firstLine) {
            if (line.size() >= 3 &&
                static_cast<unsigned char>(line[0]) == 0xEF &&
                static_cast<unsigned char>(line[1]) == 0xBB &&
                static_cast<unsigned char>(line[2]) == 0xBF) {
                line = line.substr(3);
            }
            firstLine = false;
        }

        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        std::stringstream ss(line);
        std::string filename;
        std::string valueStr;

        if (!std::getline(ss, filename, ',')) continue;
        if (!std::getline(ss, valueStr, ',')) continue;

        try {
            long long value = std::stoll(valueStr);
            outMap[filename] = value;
        } catch (const std::exception& e) {
            std::cerr << "[DataIO] Failed to parse solution line: " << line << std::endl;
        }
    }

    return true;
}