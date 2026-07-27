#ifndef FPLS_H
#define FPLS_H

#include "dataIO.h"
#include <vector>

// Result of the Feasibility-Pursuing Lagrangian Search (FPLS).
struct FPLSResult {
    double bestValue = -1.0;          // -1 means "no feasible solution found"
    std::vector<int> selectedItem;    // selectedItem[i] = chosen item index in class i
    std::vector<double> lambda;       // Lagrange multipliers at the best feasible point found
    std::vector<double> usage;        // resource usage (raw units) at the best solution
    long long feasibleHits = 0;       // number of iterations (across all runs) that were feasible
};

// Solve one instance with FPLS.
//   N : number of iterations per run (Yoon et al. 2012 default N=30000)
//   R : number of independent randomized runs, best-of-R (default 1000)
//   c : step-size offset, delta_t = 1 / (t + c - 1) (default 10)
//   seed : RNG seed for reproducibility
FPLSResult solveFPLS(const Instance& instance, int N = 30000, int R = 1000,
                      double c = 10.0, unsigned seed = 1);

void setFPLSDebug(bool enabled);

// Independently re-verify a FPLS solution against the raw instance data.
// Checks: exactly one item chosen per class (selectedItem.size()==N, valid
// indices), recomputed resource usage <= capacity for every dimension, and
// recomputed total value matches result.bestValue. Prints a report and
// returns true only if every check passes.
bool verifySolution(const Instance& instance, const FPLSResult& result, bool verbose = true);

#endif // FPLS_H