#ifndef DP_FPLS_H
#define DP_FPLS_H

#include "dataIO.h"
#include <vector>

// Result of one DP+FPLS hybrid solve for a single fixed resource k.
struct DPFPLSResult {
    double bestValue = -1.0;          // -1 means "no feasible solution found"
    std::vector<int> selectedItem;    // selectedItem[i] = chosen item index in class i
    std::vector<double> lambda;       // Lagrange multipliers (lambda[fixedResource] unused/0)
    std::vector<double> usage;        // raw resource usage, size M
    long long feasibleHits = 0;       // iterations (across all runs) that were feasible
    int fixedResource = -1;           // which resource index (0-based) was solved via DP
};

// Solve one instance with the DP+FPLS hybrid.
DPFPLSResult solveDPFPLS(const Instance& instance, int fixedResource,
                          int N = 30000, int R = 1000, double c = 10.0, unsigned seed = 1);

void setDPFPLSDebug(bool enabled);

// Independently re-verify a DPFPLSResult against the raw instance data.
// Checks: exactly one item chosen per class (selectedItem.size()==N, valid
// indices), recomputed usage <= capacity for EVERY resource (including the
// DP-fixed one), and recomputed total value matches result.bestValue.
// Prints a report and returns true only if every check passes.
bool verifySolution(const Instance& instance, const DPFPLSResult& result, bool verbose = true);

#endif // DP_FPLS_H