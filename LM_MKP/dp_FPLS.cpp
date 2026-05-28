#include "dp_fpls.h"

#include <random>
#include <algorithm>
#include <iostream>
#include <limits>

// 1-D 0/1 knapsack DP
void solve_dp(const MKPInstance &inst,
              int dpConstraintIdx,
              std::vector<int> &selected)
{
    int n   = inst.numItems;
    int cap = inst.capacities[dpConstraintIdx];
    const std::vector<int> &w = inst.weights[dpConstraintIdx];
    const std::vector<int> &p = inst.profits;

    // dp[c] = 용량 c 이하에서의 최대 이득
    std::vector<long long> dp(cap + 1, 0LL);

    for (int j = 0; j < n; ++j) {
        if (w[j] <= 0 || w[j] > cap) continue;
        for (int c = cap; c >= w[j]; --c) {
            dp[c] = std::max(dp[c], dp[c - w[j]] + static_cast<long long>(p[j]));
        }
    }

    // 역추적으로 선택된 물건 복원
    selected.assign(n, 0);
    int rem = cap;
    for (int j = n - 1; j >= 0; --j) {
        if (rem < w[j]) continue;
        if (dp[rem] == dp[rem - w[j]] + static_cast<long long>(p[j])) {
            selected[j] = 1;
            rem -= w[j];
        }
    }
}

// DP+FPLS 단일 run 실행
FPLSRunResult run_dp_fpls_single(const MKPInstance &inst,
                                 const std::string &instanceId,
                                 const MKCBResultRow &mkcbRow,
                                 int runIndex)
{
    int n = inst.numItems;
    int m = inst.numConstraints;

    FPLSRunResult runRes;
    runRes.instanceId      = instanceId;
    runRes.runIndex        = runIndex;
    runRes.lpOptimum       = mkcbRow.lpOptimum;
    runRes.bestFeasibleCB  = mkcbRow.bestFeasible;
    runRes.ourBestSolution = 0.0;
    runRes.ourBestLP       = -std::numeric_limits<double>::infinity();

    // -------------------------------------------------------
    // Step 1: 모든 m개 제약에 대해 각각 DP 실행
    //         feasible한 해들 중 가장 좋은 것을 ourBestSolution 초기값으로 세팅
    // -------------------------------------------------------
    for (int dpIdx = 0; dpIdx < m; ++dpIdx) {
        std::vector<int> x_dp;
        solve_dp(inst, dpIdx, x_dp);

        // 목적함수 값 계산
        long long dpObj = 0;
        for (int j = 0; j < n; ++j) {
            if (x_dp[j]) dpObj += inst.profits[j];
        }

        // 전체 m개 제약 feasibility 확인
        bool dpFeasible = true;
        for (int i = 0; i < m; ++i) {
            long long use = 0;
            for (int j = 0; j < n; ++j) {
                if (x_dp[j]) use += inst.weights[i][j];
            }
            if (use > inst.capacities[i]) {
                dpFeasible = false;
                break;
            }
        }

        // feasible하고 지금까지 best보다 좋으면 갱신
        if (dpFeasible && static_cast<double>(dpObj) > runRes.ourBestSolution) {
            runRes.ourBestSolution = static_cast<double>(dpObj);
        }
    }

    // -------------------------------------------------------
    // Step 2: Original FPLS 루프 그대로 실행
    // -------------------------------------------------------
    static const unsigned int FIXED_SEED = 123456789u;
    std::mt19937 rng(FIXED_SEED + static_cast<unsigned int>(runIndex));

    LagrangianState state;
    state.u.assign(m, 0.0);

    const std::vector<int> &b = inst.capacities;

    for (int t = 1; t <= g_numIterations; ++t) {
        double delta = 1.0 / static_cast<double>(t + g_gamma - 1);

        LMMKPResult lmRes = run_lmmkp(inst, state);

        // theta(u) = mu^* + u^T(b - b^*)
        double theta = lmRes.mu;
        for (int i = 0; i < m; ++i) {
            theta += state.u[i] * static_cast<double>(b[i] - lmRes.bStar[i]);
        }
        if (theta > runRes.ourBestLP) {
            runRes.ourBestLP = theta;
        }

        // I, J 분류
        std::vector<int> I, J;
        I.reserve(m);
        J.reserve(m);
        for (int i = 0; i < m; ++i) {
            if (lmRes.bStar[i] <= b[i]) I.push_back(i);
            else                        J.push_back(i);
        }

        if (J.empty()) {
            // feasible -> best solution 갱신 후보
            if (lmRes.mu > runRes.ourBestSolution) {
                runRes.ourBestSolution = lmRes.mu;
            }
            std::uniform_int_distribution<int> dist(0, static_cast<int>(I.size()) - 1);
            int i = I[dist(rng)];
            state.u[i] = std::max(0.0, state.u[i] - delta);
        } else {
            std::uniform_int_distribution<int> dist(0, static_cast<int>(J.size()) - 1);
            int k = J[dist(rng)];
            state.u[k] += delta;
        }
    }

    // percent difference 계산
    double LP = runRes.lpOptimum;
    if (LP > 0.0) {
        runRes.percentDiffSolution = 100.0 * (LP - runRes.ourBestSolution) / LP;
        runRes.percentDiffLP       = 100.0 * (LP - runRes.ourBestLP) / LP;
    } else {
        runRes.percentDiffSolution = 0.0;
        runRes.percentDiffLP       = 0.0;
    }

    return runRes;
}