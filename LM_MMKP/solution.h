#ifndef SOLUTION_H
#define SOLUTION_H

#include "dataIO.h"

// Placeholder result struct — to be filled in with Lagrangian Multiplier results later.
struct SolutionResult {
    double bestValue = 0.0;
    // TODO: add selected items, multiplier values, gap, iterations, etc.
};

// Skeleton solve function. Currently just returns an empty result.
// Will be implemented with Lagrangian Multiplier relaxation for MMKP.
SolutionResult solve(const Instance& instance);

#endif // SOLUTION_H
