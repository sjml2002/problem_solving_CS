#ifndef SOLUTION_H
#define SOLUTION_H

#include "dataIO.h"
#include <vector>

// selected[i] = index of the item currently chosen in class i (this IS x).
struct Solution {
    std::vector<int> selected;
    std::vector<double> usage;   // normalized 0..1 per resource
    double totalValue = 0.0;
};

struct SolutionResult {
    double bestValue = 0.0;
    std::vector<int> selectedItem;
    std::vector<double> lambda;
};

SolutionResult solve(const Instance& instance);

void setSolverDebug(bool enabled);

#endif // SOLUTION_H