#include "dp_fpls.h"
#include <algorithm>
#include <limits>
#include <random>
#include <iostream>
#include <cmath>

// DP + FPLS hybrid for MMKP.
//
// Same Lagrangian idea as fpls.cpp, except ONE resource (fixedResource = k)
// is not relaxed. Instead, that resource's capacity constraint is enforced
// EXACTLY via a multiple-choice knapsack DP:
//
//   dp[i][w] = best achievable score using classes 0..i-1, with resource-k
//              usage AT MOST w (0 <= w <= B_k).
//
//   dp[i][w] = max_{j in class i-1} ( dp[i-1][w - A_{i-1,j,k}] + score_{i-1,j} )
//              for all w - A_{i-1,j,k} >= 0
//
//   score_{i,j} = value_{i,j} - sum_{m != k} lambda_m * weight_{i,j,m}
//
// Answer = max over w in [0, B_k] of dp[N][w].
//
// This is an exact solve of resource k's constraint jointly with the
// Lagrangian relaxation of the other M-1 resources, using the classic
// multiple-choice knapsack DP structure (state = class prefix index,
// NOT a full 2^N bitmask), giving O(N * B_k * N_i) time per evaluation
// (N_i = items per class).
//
// A lightweight parent-pointer array (choice[i][w] = j) is kept purely to
// recover which item was chosen per class -- this is required so the outer
// FPLS loop can compute the REAL usage of the other M-1 resources and the
// REAL objective value (without the lambda penalty), which drives
// feasibility checks and multiplier updates. It is not a separate/expensive
// backtracking pass -- it falls out of the DP table itself.

static bool DEBUG_VERBOSE = true;

void setDPFPLSDebug(bool enabled) {
    DEBUG_VERBOSE = enabled;
}

const double deltaDefault = 1000.0;

namespace {

struct DPResult {
    std::vector<int> selected;   // selected[i] = chosen item index in class i
    std::vector<double> usage;   // raw resource usage, size M (includes resource k)
    double value = 0.0;          // true objective value (sum of item values, no lambda)
};

// Exact multiple-choice knapsack DP for resource `k`, with the other M-1
// resources folded into the per-item score via the Lagrangian multipliers.
DPResult LM_MMKP_DP(const Instance& inst, const std::vector<double>& lambda, int k) {
    const int N = inst.N;
    const int Bk = inst.capacity[k];
    const double NEG_INF = -std::numeric_limits<double>::infinity();

    // dp[i][w]: best score using classes 0..i-1, resource-k usage <= w.
    std::vector<std::vector<double>> dp(N + 1, std::vector<double>(Bk + 1, NEG_INF));
    // choice[i][w]: item index chosen for class i-1 to reach dp[i][w] (-1 if unreachable/unused).
    std::vector<std::vector<int>> choice(N + 1, std::vector<int>(Bk + 1, -1));

    for (int w = 0; w <= Bk; ++w) dp[0][w] = 0.0; // no classes selected yet, any usage bound holds

    for (int i = 0; i < N; ++i) {
        const auto& items = inst.classes[i].items;

        for (int w = 0; w <= Bk; ++w) {
            if (dp[i][w] == NEG_INF) continue;

            for (int j = 0; j < static_cast<int>(items.size()); ++j) {
                int aijk = items[j].weight[k];
                int nw = w + aijk;
                if (nw > Bk) continue;

                double score = static_cast<double>(items[j].value);
                for (int m = 0; m < inst.M; ++m) {
                    if (m == k) continue;
                    score -= lambda[m] * static_cast<double>(items[j].weight[m]);
                }

                double cand = dp[i][w] + score;
                if (cand > dp[i + 1][nw]) {
                    dp[i + 1][nw] = cand;
                    choice[i + 1][nw] = j;
                }
            }
        }
    }

    // Find the best reachable usage level for resource k.
    int bestW = -1;
    double bestScore = NEG_INF;
    for (int w = 0; w <= Bk; ++w) {
        if (dp[N][w] > bestScore) {
            bestScore = dp[N][w];
            bestW = w;
        }
    }

    DPResult res;
    res.selected.assign(N, -1);
    res.usage.assign(inst.M, 0.0);
    res.value = 0.0;

    if (bestW == -1) {
        // No feasible assignment even ignoring the other M-1 resources
        // (should not normally happen since dp[0][*] is always 0 and every
        // class has at least one item, unless an item's weight[k] alone
        // exceeds Bk for every choice in some class).
        return res;
    }

    // Reconstruct selected items from the parent pointers.
    int w = bestW;
    for (int i = N; i >= 1; --i) {
        int j = choice[i][w];
        res.selected[i - 1] = j;
        int aijk = inst.classes[i - 1].items[j].weight[k];
        w -= aijk;
    }

    // Recompute the REAL value and usage (all M resources) from the
    // recovered selection, independent of the lambda-weighted score.
    for (int i = 0; i < N; ++i) {
        const Item& item = inst.classes[i].items[res.selected[i]];
        res.value += item.value;
        for (int m = 0; m < inst.M; ++m) {
            res.usage[m] += item.weight[m];
        }
    }

    return res;
}

bool isFeasibleExceptK(const Instance& inst, const std::vector<double>& usage, int k) {
    for (int m = 0; m < inst.M; ++m) {
        if (m == k) continue; // resource k is always satisfied by construction of the DP
        if (usage[m] > static_cast<double>(inst.capacity[m])) return false;
    }
    return true;
}

struct RunResult {
    double bestValue = -1.0;
    std::vector<int> selected;
    std::vector<double> lambda;
    std::vector<double> usage;
    long long feasibleHits = 0;
};

RunResult fplsRunDP(const Instance& inst, int fixedResource, int N, double c, std::mt19937& rng) {
    std::vector<double> lambda(inst.M, 0.0);
    RunResult best;

    std::vector<int> violated;
    violated.reserve(inst.M);

    // Candidate resources for lambda updates exclude fixedResource.
    std::vector<int> others;
    for (int m = 0; m < inst.M; ++m) {
        if (m != fixedResource) others.push_back(m);
    }
    std::uniform_int_distribution<int> pickAnyOther(0, static_cast<int>(others.size()) - 1);

    for (int t = 1; t <= N; ++t) {
        DPResult r = LM_MMKP_DP(inst, lambda, fixedResource);
        double delta = deltaDefault / (static_cast<double>(t) + c - 1.0);

        if (isFeasibleExceptK(inst, r.usage, fixedResource)) {
            ++best.feasibleHits;
            if (r.value > best.bestValue) {
                best.bestValue = r.value;
                best.selected = r.selected;
                best.lambda = lambda;
                best.usage = r.usage;
            }
            // Relax a random non-fixed multiplier to keep exploring.
            int k = others[pickAnyOther(rng)];
            lambda[k] = std::max(0.0, lambda[k] - delta);
        } else {
            violated.clear();
            for (int m : others) {
                if (r.usage[m] > static_cast<double>(inst.capacity[m])) {
                    violated.push_back(m);
                }
            }
            std::uniform_int_distribution<int> pickViol(0, static_cast<int>(violated.size()) - 1);
            int k = violated[pickViol(rng)];
            lambda[k] += delta;
        }

        if (DEBUG_VERBOSE && t % 5000 == 0) {
            std::cout << "  [dp_fpls k=" << fixedResource << " t=" << t
                      << "] bestValue=" << best.bestValue
                      << " feasibleHits=" << best.feasibleHits << std::endl;
        }
    }

    return best;
}

} // namespace

DPFPLSResult solveDPFPLS(const Instance& instance, int fixedResource,
                          int N, int R, double c, unsigned seed) {
    std::mt19937 rng(seed);
    DPFPLSResult overall;
    overall.fixedResource = fixedResource;

    for (int run = 0; run < R; ++run) {
        RunResult r = fplsRunDP(instance, fixedResource, N, c, rng);
        overall.feasibleHits += r.feasibleHits;

        if (r.bestValue > overall.bestValue) {
            overall.bestValue = r.bestValue;
            overall.selectedItem = r.selected;
            overall.lambda = r.lambda;
            overall.usage = r.usage;
        }

        if (DEBUG_VERBOSE) {
            std::cout << " [dp_fpls k=" << fixedResource << " run " << run
                      << "] runBest=" << r.bestValue
                      << " overallBest=" << overall.bestValue << std::endl;
        }
    }

    return overall;
}

/**
 * Independently re-verify a DP+FPLS solution against the raw instance data.
 * Unlike the internal DP bookkeeping, this recomputes usage/value from
 * scratch for ALL M resources (not just the M-1 that were Lagrangian-
 * relaxed), so it also confirms the DP-fixed resource k was truly kept
 * within capacity -- i.e. that LM_MMKP_DP's exactness claim actually holds.
 */
bool verifySolution(const Instance& instance, const DPFPLSResult& result, bool verbose) {
    bool ok = true;

    // 1. Check exactly one selection per class.
    if (static_cast<int>(result.selectedItem.size()) != instance.N) {
        if (verbose) {
            std::cout << "[verify][dp_fpls] FAIL: selectedItem.size()=" << result.selectedItem.size()
                      << " but instance.N=" << instance.N << std::endl;
        }
        return false;
    }

    for (int i = 0; i < instance.N; ++i) {
        int j = result.selectedItem[i];
        int classSize = static_cast<int>(instance.classes[i].items.size());
        if (j < 0 || j >= classSize) {
            ok = false;
            if (verbose) {
                std::cout << "[verify][dp_fpls] FAIL: class " << i << " selected index " << j
                          << " out of range [0, " << classSize << ")" << std::endl;
            }
        }
    }
    if (!ok) return false;

    // 2. Recompute usage and value directly from raw instance data.
    std::vector<double> recomputedUsage(instance.M, 0.0);
    double recomputedValue = 0.0;

    for (int i = 0; i < instance.N; ++i) {
        int j = result.selectedItem[i];
        const Item& item = instance.classes[i].items[j];
        recomputedValue += item.value;
        for (int m = 0; m < instance.M; ++m) {
            recomputedUsage[m] += item.weight[m];
        }
    }

    // 3. Check capacity constraints for ALL M resources, including the
    //    DP-fixed one. This is the key extra check vs. plain FPLS: it
    //    confirms LM_MMKP_DP actually enforced resource k exactly.
    for (int m = 0; m < instance.M; ++m) {
        if (recomputedUsage[m] > static_cast<double>(instance.capacity[m])) {
            ok = false;
            if (verbose) {
                std::cout << "[verify][dp_fpls] FAIL: resource " << m << " usage="
                          << recomputedUsage[m] << " > capacity=" << instance.capacity[m]
                          << (m == result.fixedResource ? "  <-- this was the DP-fixed resource!" : "")
                          << std::endl;
            }
        }
    }

    // 4. Check that the recomputed value matches result.bestValue.
    const double EPS = 1e-6;
    if (std::abs(recomputedValue - result.bestValue) > EPS) {
        ok = false;
        if (verbose) {
            std::cout << "[verify][dp_fpls] FAIL: recomputed value=" << recomputedValue
                       << " but result.bestValue=" << result.bestValue << std::endl;
        }
    }

    if (verbose) {
        if (ok) {
            std::cout << "[verify][dp_fpls] PASS: N=" << instance.N << " classes, "
                      << instance.M << " resources, fixedResource=" << result.fixedResource
                      << ", value=" << recomputedValue << " (matches bestValue)" << std::endl;
        }
        std::cout << "[verify][dp_fpls] usage/capacity breakdown:" << std::endl;
        for (int m = 0; m < instance.M; ++m) {
            std::cout << "    resource " << m << ": usage=" << recomputedUsage[m]
                       << " / capacity=" << instance.capacity[m]
                       << (m == result.fixedResource ? "  [DP-fixed]" : "")
                       << (recomputedUsage[m] > instance.capacity[m] ? "  <-- VIOLATED" : "")
                       << std::endl;
        }
    }

    return ok;
}