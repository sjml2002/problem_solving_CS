#include "dp_FPLS.h"

#include <random>
#include <algorithm>
#include <iostream>
#include <limits>
#include <sstream>

// -------------------------------------------------------
// 1-D 0/1 knapsack DP
// -------------------------------------------------------
void solve_dp(const MKPInstance &inst,
              int dpConstraintIdx,
              std::vector<int> &selected)
{
    int n   = inst.numItems;
    int cap = inst.capacities[dpConstraintIdx];
    const std::vector<int> &w = inst.weights[dpConstraintIdx];
    const std::vector<int> &p = inst.profits;

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
        if (w[j] <= 0 || w[j] > cap) continue;  // DP와 동일한 조건 추가
        if (rem >= w[j] && dp[rem] == dp[rem - w[j]] + static_cast<long long>(p[j])) {
            selected[j] = 1;
            rem -= w[j];
        }
    }
}

// -------------------------------------------------------
// DP+FPLS 단일 run
// -------------------------------------------------------
FPLSRunResult run_dp_fpls_single(const MKPInstance &inst,
                                 const std::string &instanceId,
                                 const MKCBResultRow &mkcbRow,
                                 int dpConstraintIdx,
                                 int runIndex)
{
    int n = inst.numItems;
    int m = inst.numConstraints;

    // Step 1: DP 실행
    std::vector<int> selected;
    solve_dp(inst, dpConstraintIdx, selected);

    // DP가 선택한 아이템의 목적함수 기여값
    long long dpObj = 0;
    for (int j = 0; j < n; ++j) {
        if (selected[j]) dpObj += inst.profits[j];
    }

    // DP로 선택된 아이템들의 b_i 기준 총 무게 W
    long long dpWeight = 0;
    for (int j = 0; j < n; ++j) {
        if (selected[j]) dpWeight += inst.weights[dpConstraintIdx][j];
    }

    // FPLS에서 쓸 아이템: DP가 선택 안 한 것들
    std::vector<int> fplsItems;
    for (int j = 0; j < n; ++j) {
        if (!selected[j]) fplsItems.push_back(j);
    }
    int nFpls = static_cast<int>(fplsItems.size());

    // FPLS에서 쓸 제약: b_i 제외한 m-1개
    std::vector<int> fplsConstraints;
    for (int i = 0; i < m; ++i) {
        if (i != dpConstraintIdx) fplsConstraints.push_back(i);
    }
    int mFpls = static_cast<int>(fplsConstraints.size()); // m-1

    // 결과 구조체 초기화
    FPLSRunResult runRes;
    runRes.instanceId      = instanceId;
    runRes.runIndex        = runIndex;
    runRes.lpOptimum       = mkcbRow.lpOptimum;
    runRes.bestFeasibleCB  = mkcbRow.bestFeasible;
    runRes.ourBestSolution = 0.0;
    runRes.ourBestLP       = -std::numeric_limits<double>::infinity();
    runRes.dpWeight        = dpWeight;

    // 엣지 케이스: FPLS 아이템이나 제약이 없으면 DP 해 그대로 반환
    if (nFpls == 0 || mFpls == 0) {
        runRes.ourBestSolution = static_cast<double>(dpObj);
        double LP = runRes.lpOptimum;
        if (LP > 0.0) {
            runRes.percentDiffSolution = 100.0 * (LP - runRes.ourBestSolution) / LP;
            runRes.percentDiffLP       = 0.0;
        }
        return runRes;
    }

    // Step 2: FPLS 루프
    static const unsigned int FIXED_SEED = 123456789u;
    std::mt19937 rng(FIXED_SEED
                     + static_cast<unsigned int>(runIndex)
                     + static_cast<unsigned int>(dpConstraintIdx) * 1000007u);

    std::vector<double> u(mFpls, 0.0);

    for (int t = 1; t <= g_numIterations; ++t) {
        double delta = 1.0 / static_cast<double>(t + g_gamma - 1);

        // LMMKP
        std::vector<int> x(nFpls, 0);
        for (int ji = 0; ji < nFpls; ++ji) {
            int j = fplsItems[ji];
            double lagCost = 0.0;
            for (int ki = 0; ki < mFpls; ++ki) {
                int ci = fplsConstraints[ki];
                lagCost += u[ki] * static_cast<double>(inst.weights[ci][j]);
            }
            if (static_cast<double>(inst.profits[j]) > lagCost) {
                x[ji] = 1;
            }
        }

        // mu^* = FPLS 아이템 부분 목적함수
        double mu = 0.0;
        for (int ji = 0; ji < nFpls; ++ji) {
            if (x[ji]) mu += inst.profits[fplsItems[ji]];
        }

        // b^* = FPLS 제약 부분 사용량
        std::vector<long long> bStar(mFpls, 0LL);
        for (int ki = 0; ki < mFpls; ++ki) {
            int ci = fplsConstraints[ki];
            for (int ji = 0; ji < nFpls; ++ji) {
                if (x[ji]) bStar[ki] += inst.weights[ci][fplsItems[ji]];
            }
        }

        // theta(u) = mu^* + u^T(b - b^*)  ← dpObj 더하지 않음
        double theta = mu;
        for (int ki = 0; ki < mFpls; ++ki) {
            int ci = fplsConstraints[ki];
            theta += u[ki] * static_cast<double>(inst.capacities[ci] - bStar[ki]);
        }
        if (theta > runRes.ourBestLP) {
            runRes.ourBestLP = theta;
        }

        // I, J 분류
        std::vector<int> I, J;
        for (int ki = 0; ki < mFpls; ++ki) {
            int ci = fplsConstraints[ki];
            if (bStar[ki] <= static_cast<long long>(inst.capacities[ci])) I.push_back(ki);
            else                                                           J.push_back(ki);
        }

        if (J.empty()) {
            // FPLS 아이템들의 b_i 사용량 체크
            long long fplsWeightOnDp = 0;
            for (int ji = 0; ji < nFpls; ++ji) {
                if (x[ji]) {
                    fplsWeightOnDp += inst.weights[dpConstraintIdx][fplsItems[ji]];
                }
            }

            // DP 사용량 + FPLS 사용량 <= b_i 용량이어야 진짜 feasible
            if (dpWeight + fplsWeightOnDp
                    <= static_cast<long long>(inst.capacities[dpConstraintIdx])) {
                double totalObj = static_cast<double>(dpObj) + mu;
                if (totalObj > runRes.ourBestSolution) {
                    runRes.ourBestSolution = totalObj;
                }
            }

            if (!I.empty()) {
                std::uniform_int_distribution<int> dist(0, static_cast<int>(I.size()) - 1);
                int ki = I[dist(rng)];
                u[ki] = std::max(0.0, u[ki] - delta);
            }
        }
    }

    // percent diff: ourBestLP는 FPLS 부분만이므로 dpObj 합산해서 비교
    double LP = runRes.lpOptimum;
    if (LP > 0.0) {
        runRes.percentDiffSolution = 100.0 * (LP - runRes.ourBestSolution) / LP;
        double ourTotalLP = runRes.ourBestLP + static_cast<double>(dpObj);
        runRes.percentDiffLP = 100.0 * (LP - ourTotalLP) / LP;
    } else {
        runRes.percentDiffSolution = 0.0;
        runRes.percentDiffLP       = 0.0;
    }

    return runRes;
}

// -------------------------------------------------------
// b_0 ~ b_{m-1} 각각에 대해 g_numRuns번씩 전부 실행
// -------------------------------------------------------
void run_dp_fpls_all(const MKPInstance &inst,
                     const std::string &instanceId,
                     const MKCBResultRow &mkcbRow,
                     std::vector<FPLSRunResult> &allRuns,
                     FPLSRunResult &bestRun,
                     int &bestDpIdx)
{
    int m = inst.numConstraints;

    bestRun.ourBestSolution = -std::numeric_limits<double>::infinity();
    bestDpIdx = -1;

    for (int dpIdx = 0; dpIdx < m; ++dpIdx) {
        for (int r = 0; r < g_numRuns; ++r) {
            FPLSRunResult res = run_dp_fpls_single(inst, instanceId, mkcbRow, dpIdx, r);

            std::ostringstream ossId;
            ossId << instanceId << "-dp" << dpIdx;
            res.instanceId = ossId.str();
            res.runIndex   = r;

            allRuns.push_back(res);

            if (res.ourBestSolution > bestRun.ourBestSolution) {
                bestRun   = res;
                bestDpIdx = dpIdx;
            }
        }
    }
}