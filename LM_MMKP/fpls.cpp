#include "fpls.h"
#include <algorithm>
#include <limits>
#include <random>
#include <iostream>

// Feasibility-Pursuing Lagrangian Search (FPLS) for the Multiple-choice
// Multidimensional Knapsack Problem (MMKP).
//
// Extended from Yoon, Kim, Moon (2012). The 0-1 MKP relaxation decides each
// item independently by the sign of (c_j - sum_i u_i a_ij). MMKP additionally
// requires exactly one item per class (multiple-choice constraint). We keep
// that constraint intact and only relax the M resource constraints, so the
// Lagrangian subproblem decomposes per class into an argmax over that
// class's items:
//
//   x_i* = argmax_{j in class i} ( value_ij - sum_m lambda_m * weight_ijm )
//
// Classes are mutually independent given lambda, so this is solved exactly
// in O(N*M) per evaluation (LM_MMKP below). Feasibility of the M capacity
// constraints is then pursued by FPLS's randomized coordinate search on
// lambda (Theorem 2 monotonicity: raising lambda_m weakly shrinks the
// Lagrangian capacity usage of resource m).

const double deltaDefault = 10;

static bool DEBUG_VERBOSE = false;

void setFPLSDebug(bool enabled) {
    DEBUG_VERBOSE = enabled;
}

namespace {

struct LMResult {
    std::vector<int> selected;   // selected[i] = chosen item index in class i
    std::vector<double> usage;   // raw resource usage, size M
    double value = 0.0;          // true objective value (sum of item values)
};

// LM_MMKP: exactly solve the Lagrangian relaxation for a given multiplier
// vector lambda (size M, lambda >= 0). Each class picks the single item
// maximizing (value - lambda . weight), independent of other classes.
LMResult LM_MMKP(const Instance& inst, const std::vector<double>& lambda) {
    LMResult res;
    res.selected.resize(inst.N);
    res.usage.assign(inst.M, 0.0);
    res.value = 0.0;

    for (int i = 0; i < inst.N; ++i) {
        const auto& items = inst.classes[i].items;
        int bestJ = 0;
        double bestScore = -std::numeric_limits<double>::infinity();

        for (int j = 0; j < static_cast<int>(items.size()); ++j) {
            double score = static_cast<double>(items[j].value);
            for (int m = 0; m < inst.M; ++m) {
                score -= (lambda[m] * static_cast<double>(items[j].weight[m]));
            }
            if (score > bestScore) {
                bestScore = score;
                bestJ = j;
            }
        }

        res.selected[i] = bestJ;
        res.value += items[bestJ].value;
        for (int m = 0; m < inst.M; ++m) {
            res.usage[m] += items[bestJ].weight[m];
        }
    }

    return res;
}

bool isFeasible(const Instance& inst, const std::vector<double>& usage) {
    for (int m = 0; m < inst.M; ++m) {
        if (usage[m] > static_cast<double>(inst.capacity[m])) return false;
    }
    return true;
}

// One FPLS run of N iterations starting from lambda = 0.
struct RunResult {
    double bestValue = -1.0;
    std::vector<int> selected;
    std::vector<double> lambda;
    std::vector<double> usage;
    long long feasibleHits = 0;
};

RunResult fplsRun(const Instance& inst, int N, double c, std::mt19937& rng) {
    std::vector<double> lambda(inst.M, 0.0);
    RunResult best;

    std::vector<int> violated;
    violated.reserve(inst.M);
    std::uniform_int_distribution<int> pickAny(0, inst.M - 1);

    for (int t = 1; t <= N; ++t) {
        LMResult r = LM_MMKP(inst, lambda);
        double delta = deltaDefault / (static_cast<double>(t) + c - 1.0);

        if (isFeasible(inst, r.usage)) {
            ++best.feasibleHits;
            if (r.value > best.bestValue) {
                best.bestValue = r.value;
                best.selected = r.selected;
                best.lambda = lambda;
                best.usage = r.usage;
            }
            // Relax a random multiplier to keep exploring for better solutions.
            int k = pickAny(rng);
            lambda[k] = std::max(0.0, lambda[k] - delta);
        } else {
            violated.clear();
            for (int m = 0; m < inst.M; ++m) {
                if (r.usage[m] > static_cast<double>(inst.capacity[m])) {
                    violated.push_back(m);
                }
            }
            std::uniform_int_distribution<int> pickViol(0, static_cast<int>(violated.size()) - 1);
            int k = violated[pickViol(rng)];
            lambda[k] += delta;
        }

        if (DEBUG_VERBOSE && t % 5000 == 0) {
            std::cout << "  [fpls t=" << t << "] bestValue=" << best.bestValue
                      << " feasibleHits=" << best.feasibleHits << std::endl;
        }
    }

    return best;
}

} // namespace

FPLSResult solveFPLS(const Instance& instance, int N, int R, double c, unsigned seed) {
    std::mt19937 rng(seed);
    FPLSResult overall;

    for (int run = 0; run < R; ++run) {
        RunResult r = fplsRun(instance, N, c, rng);
        overall.feasibleHits += r.feasibleHits;

        if (r.bestValue > overall.bestValue) {
            overall.bestValue = r.bestValue;
            overall.selectedItem = r.selected;
            overall.lambda = r.lambda;
            overall.usage = r.usage;
        }

        if (DEBUG_VERBOSE) {
            std::cout << " [fpls run " << run << "] runBest=" << r.bestValue
                      << " overallBest=" << overall.bestValue << std::endl;
        }
    }

    return overall;
}