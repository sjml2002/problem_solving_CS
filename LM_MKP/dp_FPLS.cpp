#include "dp_FPLS.h"
#include "verify_solution.h"

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

    std::vector<std::vector<long long>> dp(n + 1, std::vector<long long>(cap + 1, 0LL));


    for (int j=0; j < n; j++) {
        for (int c=0; c <= cap; c++) {
            if (j == 0) {
		if (c > 0)
		    dp[j][c] = dp[j][c-1];
                if (c >= w[j])
                    dp[j][c] = std::max(dp[j][c], (long long)p[j]);
            }
            else {
                dp[j][c] = std::max(dp[j][c], dp[j-1][c]);
		if (c > 0)
		    dp[j][c] = std::max(dp[j][c], dp[j][c-1]);
                if (c >= w[j])
                    dp[j][c] = std::max(dp[j][c], dp[j-1][c - w[j]] + (long long)p[j]);
            }
        }
    }

    //최종값은 dp[n-1][cap] 에 담겨져 있음.
    // 역추적
    selected.assign(n, 0);
    int rem = cap;
    for (int j = n - 1; j > 0; --j) {
        //현재 아이템 j와 j-1이 다르므로 현재 아이템 j를 넣었다는 것을 알 수 있음
        if (dp[j][rem] != dp[j-1][rem]) {
            selected[j] = 1;
            rem -= w[j];
        }

        if (rem == 0 || dp[j][rem] == 0)
            break ;
    }
}

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

    long long dpObj = 0;
    long long dpWeight = 0;
    std::vector<int> fplsItems;
    for (int j = 0; j < n; ++j) {
        if (selected[j]) {
            dpObj += inst.profits[j];
            std::cout << j << ": " << inst.profits[j] << "\n"; //debug
            dpWeight += inst.weights[dpConstraintIdx][j];
        }
        else //dp로 고른 아이템들은 제외
            fplsItems.push_back(j);
    }
    int nFpls = static_cast<int>(fplsItems.size()); //dp로 고른 아이템 제외
    std::cout << "\n"; //debug

    std::vector<int> fplsConstraints;
    for (int i = 0; i < m; ++i)
        if (i != dpConstraintIdx) fplsConstraints.push_back(i);
    int mFpls = static_cast<int>(fplsConstraints.size()); //m-1

    FPLSRunResult runRes;
    runRes.instanceId      = instanceId;
    runRes.runIndex        = runIndex;
    runRes.lpOptimum       = mkcbRow.lpOptimum;
    runRes.bestFeasibleCB  = mkcbRow.bestFeasible;
    runRes.ourBestSolution = 0.0;
    runRes.ourBestLP       = -std::numeric_limits<double>::infinity();
    runRes.dpWeight        = dpWeight;
    runRes.dpOpt           = dpObj;

    if (nFpls == 0 || mFpls == 0) {
        runRes.ourBestSolution = static_cast<double>(dpObj);
        double LP = runRes.lpOptimum;
        if (LP > 0.0) {
            runRes.percentDiffSolution = 100.0 * (LP - runRes.ourBestSolution) / LP;
            runRes.percentDiffLP       = 0.0;
        }
        return runRes;
    }

    // DP 아이템들의 m-1개 제약 소비량 (penalty 보정용)
    std::vector<long long> dpUsage(mFpls, 0LL);
    for (int ki = 0; ki < mFpls; ++ki) {
        int ci = fplsConstraints[ki];
        for (int j = 0; j < n; ++j)
            if (selected[j]) dpUsage[ki] += inst.weights[ci][j];
    }

    // Step 2: FPLS 루프
    std::mt19937 rng(123456789u
                     + static_cast<unsigned int>(runIndex)
                     + static_cast<unsigned int>(dpConstraintIdx) * 1000007u);

    std::vector<double> u(mFpls, 0.0);

    for (int t = 1; t <= g_numIterations; ++t) {
        double delta = 1.0 / static_cast<double>(t + g_gamma - 1);

        // LMMKP: Lagrangian 비용 기반 greedy 선택
        std::vector<int> x(nFpls, 0);
        for (int ji = 0; ji < nFpls; ++ji) {
            int j = fplsItems[ji];
            double lagCost = 0.0;
            for (int ki = 0; ki < mFpls; ++ki)
                lagCost += u[ki] * static_cast<double>(inst.weights[fplsConstraints[ki]][j]);
            if (static_cast<double>(inst.profits[j]) > lagCost)
                x[ji] = 1;
        }

        // bStar: FPLS 아이템의 제약별 사용량
        std::vector<long long> bStar(mFpls, 0LL);
        for (int ki = 0; ki < mFpls; ++ki) {
            int ci = fplsConstraints[ki];
            for (int ji = 0; ji < nFpls; ++ji)
                if (x[ji]) bStar[ki] += inst.weights[ci][fplsItems[ji]];
        }

        // L(u) = dpObj + fpls목적 - sum_k u_k*(dpUsage[k] + bStar[k] - b_k)
        double fplsObj = 0.0;
        for (int ji = 0; ji < nFpls; ++ji)
            if (x[ji]) fplsObj += inst.profits[fplsItems[ji]];

        double Lu = static_cast<double>(dpObj) + fplsObj;
        for (int ki = 0; ki < mFpls; ++ki) {
            int ci = fplsConstraints[ki];
            double violation = static_cast<double>(dpUsage[ki] + bStar[ki]
                                                   - inst.capacities[ci]);
            Lu -= u[ki] * violation;
        }

        if (Lu > runRes.ourBestLP)
            runRes.ourBestLP = Lu;

        // I, J 분류
        std::vector<int> I, J;
        for (int ki = 0; ki < mFpls; ++ki) {
            int ci = fplsConstraints[ki];
            // dpUsage + bStar 합산해서 실제 사용량으로 판단
            if (dpUsage[ki] + bStar[ki] <= static_cast<long long>(inst.capacities[ci]))
                I.push_back(ki);
            else
                J.push_back(ki);
        }

        if (J.empty()) {
            double totalObj = static_cast<double>(dpObj) + fplsObj;
            if (totalObj > runRes.ourBestSolution)
                runRes.ourBestSolution = totalObj;
            
            // DEBUG: LP_opt 초과 시 출력
            if (totalObj > runRes.lpOptimum) {
                verify_and_print(inst, instanceId, selected, x, fplsItems,
                                dpConstraintIdx, runRes.lpOptimum);
            }

            if (!I.empty()) {
                std::uniform_int_distribution<int> dist(0, static_cast<int>(I.size()) - 1);
                int ki = I[dist(rng)];
                u[ki] = std::max(0.0, u[ki] - delta);
            }
        } else {
            std::uniform_int_distribution<int> dist(0, static_cast<int>(J.size()) - 1);
            int ki = J[dist(rng)];
            u[ki] += delta;
        }
    }

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
