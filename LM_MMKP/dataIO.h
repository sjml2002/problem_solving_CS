#ifndef DATAIO_H
#define DATAIO_H

#include <string>
#include <vector>
#include <unordered_map>

struct Item {
    int value;
    std::vector<int> weight; // size = M
};

struct ItemClass {
    std::vector<Item> items; // size = I_n
};

struct Instance {
    std::string name;
    int N = 0;
    int M = 0;
    std::vector<int> capacity;
    std::vector<ItemClass> classes;
};

class DataIO {
public:
    static bool readInstance(const std::string& filePath, Instance& outInstance);

    // solutions.csv: filename,best_value (no header, UTF-8 BOM tolerant)
    static bool readSolutions(const std::string& csvPath,
                               std::unordered_map<std::string, long long>& outMap);

private:
    static bool validateItemTokens(int tokenCount, int M);
};

#endif // DATAIO_H