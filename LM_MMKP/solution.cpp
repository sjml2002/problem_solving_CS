#include "solution.h"
#include <algorithm>
#include <limits>
#include <cmath>
#include <iostream>
#include <set>
#include <functional>

static bool DEBUG_VERBOSE = false;

namespace {

double normWeight(const Instance& inst, int i, int j, int m) {
    return static_cast<double>(inst.classes[i].items[j].weight[m]) / inst.capacity[m];
}

double itemValue(const Instance& inst, int i, int j) {
    return static_cast<double>(inst.classes[i].items[j].value);
}

Solution initializeSolution(const Instance& inst) {
    Solution sol;
    sol.selected.resize(inst.N);
    sol.usage.assign(inst.M, 0.0);
    sol.totalValue = 0.0;

    for (int i = 0; i < inst.N; ++i) {
        const auto& items = inst.classes[i].items;
        int bestJ = 0;
        double bestVal = itemValue(inst, i, 0);
        for (int j = 1; j < static_cast<int>(items.size()); ++j) {
            double v = itemValue(inst, i, j);
            if (v > bestVal) { bestVal = v; bestJ = j; }
        }
        sol.selected[i] = bestJ;
        sol.totalValue += bestVal;
    }

    for (int m = 0; m < inst.M; ++m) {
        double u = 0.0;
        for (int i = 0; i < inst.N; ++i) u += normWeight(inst, i, sol.selected[i], m);
        sol.usage[m] = u;
    }

    if (DEBUG_VERBOSE) {
        double maxUsage = 0.0;
        for (double u : sol.usage) maxUsage = std::max(maxUsage, u);
        std::cout << "  [init] totalValue=" << sol.totalValue
                  << " maxUsage=" << maxUsage << std::endl;
    }

    return sol;
}

void applySwap(const Instance& inst, Solution& sol, int classIdx, int newItem) {
    int oldItem = sol.selected[classIdx];
    if (oldItem == newItem) return;

    for (int m = 0; m < inst.M; ++m) {
        sol.usage[m] += normWeight(inst, classIdx, newItem, m) - normWeight(inst, classIdx, oldItem, m);
    }

    sol.totalValue += itemValue(inst, classIdx, newItem) - itemValue(inst, classIdx, oldItem);
    sol.selected[classIdx] = newItem;
}

void dropPhase(const Instance& inst, Solution& sol, std::vector<double>& lambda) {
    const double EPS = 1e-9;
    int iter = 0;
    std::set<std::vector<int>> seenStates; //이전 상태 감지

    while (true) {
        int I = -1;
        double worst = 1.0;
        for (int m = 0; m < inst.M; ++m) {
            if (sol.usage[m] > worst) { worst = sol.usage[m]; I = m; }
        }
        if (I == -1) break;

         // 사이클 감지: 이미 방문한 selected 상태면 강제 종료
        if (seenStates.count(sol.selected)) {
            std::cerr << "[dropPhase] Cycle detected at iter " << iter
                      << ", forcing termination (infeasible or degenerate case)." << std::endl;
            break;
        }
        seenStates.insert(sol.selected);

        double bestRatio = std::numeric_limits<double>::infinity();
        int bestClass = -1, bestItem = -1;

        for (int i = 0; i < inst.N; ++i) {
            int curItem = sol.selected[i];
            double curW = normWeight(inst, i, curItem, I);
            const auto& items = inst.classes[i].items;

            for (int j = 0; j < static_cast<int>(items.size()); ++j) {
                if (j == curItem) continue;
                double newW = normWeight(inst, i, j, I);
                if (newW >= curW) continue;

                double valueLoss = itemValue(inst, i, curItem) - itemValue(inst, i, j);
                double weightFreed = curW - newW;
                if (weightFreed < EPS) continue;

                double ratio = valueLoss / weightFreed;
                if (ratio < bestRatio) {
                    bestRatio = ratio;
                    bestClass = i;
                    bestItem = j;
                }
            }
        }

        if (bestClass == -1) {
            if (DEBUG_VERBOSE) std::cout << "  [drop] stuck: resource " << I << " still over capacity" << std::endl;
            break;
        }

        lambda[I] += bestRatio;
        applySwap(inst, sol, bestClass, bestItem);
        ++iter;

        if (DEBUG_VERBOSE) {
            std::cout << "  [drop #" << iter << "] resource=" << I
                      << " class=" << bestClass << " item=" << bestItem
                      << " ratio=" << bestRatio
                      << " totalValue=" << sol.totalValue << std::endl;
        }
    }

    if (DEBUG_VERBOSE) std::cout << "  [drop] done after " << iter << " swaps" << std::endl;
}

void addPhase(const Instance& inst, Solution& sol) {
    bool improved = true;
    int iter = 0;

    while (improved) {
        improved = false;

        double bestGain = 0.0;
        int bestClass = -1, bestItem = -1;

        for (int i = 0; i < inst.N; ++i) {
            int curItem = sol.selected[i];
            double curVal = itemValue(inst, i, curItem);
            const auto& items = inst.classes[i].items;

            for (int j = 0; j < static_cast<int>(items.size()); ++j) {
                if (j == curItem) continue;

                double gain = itemValue(inst, i, j) - curVal;
                if (gain <= bestGain) continue;

                bool feasible = true;
                for (int m = 0; m < inst.M; ++m) {
                    double delta = normWeight(inst, i, j, m) - normWeight(inst, i, curItem, m);
                    if (sol.usage[m] + delta > 1.0 + 1e-9) { feasible = false; break; }
                }

                if (feasible) {
                    bestGain = gain;
                    bestClass = i;
                    bestItem = j;
                }
            }
        }

        if (bestClass != -1) {
            applySwap(inst, sol, bestClass, bestItem);
            improved = true;
            ++iter;

            if (DEBUG_VERBOSE) {
                std::cout << "  [add #" << iter << "] class=" << bestClass
                          << " item=" << bestItem << " gain=" << bestGain
                          << " totalValue=" << sol.totalValue << std::endl;
            }
        }
    }

    if (DEBUG_VERBOSE) std::cout << "  [add] done after " << iter << " swaps" << std::endl;
}

} // namespace

void setSolverDebug(bool enabled) {
    DEBUG_VERBOSE = enabled;
}

SolutionResult solve(const Instance& instance) {
    SolutionResult result;
    if (instance.N == 0 || instance.M == 0) return result;

    std::vector<double> lambda(instance.M, 0.0);
    setSolverDebug(1);

    if (DEBUG_VERBOSE) std::cout << "[solve] " << instance.name
                                  << " N=" << instance.N << " M=" << instance.M << std::endl;

    Solution sol = initializeSolution(instance);
    dropPhase(instance, sol, lambda);
    addPhase(instance, sol);

    result.bestValue = sol.totalValue;
    result.selectedItem = sol.selected;
    result.lambda = lambda;

    return result;
}