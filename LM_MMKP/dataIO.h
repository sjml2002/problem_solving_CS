#ifndef DATAIO_H
#define DATAIO_H

#include <string>
#include <vector>

// One item: value + M-dimensional weight vector
struct Item {
    int value;
    std::vector<int> weight; // size = M
};

// One class (group): a set of mutually exclusive items
struct ItemClass {
    std::vector<Item> items; // size = I_n
};

// Full instance parsed from one input file
struct Instance {
    std::string name;               // file name, used for solution matching
    int N = 0;                      // number of classes
    int M = 0;                      // number of resource dimensions
    std::vector<int> capacity;      // size = M
    std::vector<ItemClass> classes; // size = N
};

class DataIO {
public:
    // Reads one MMKP instance file (standard format) into an Instance.
    // Returns true on success, false on failure (file open error, parse error).
    static bool readInstance(const std::string& filePath, Instance& outInstance);

private:
    // Defensive check: verifies token count matches M+1 for an item line.
    static bool validateItemTokens(int tokenCount, int M);
};

#endif // DATAIO_H
