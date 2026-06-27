#include "dp_FPLS.h"
#include "verify_solution.h"

#include <random>
#include <algorithm>
#include <iostream>
#include <limits>
#include <sstream>

FPLSRunResult run_dp_fpls_single(const MKPInstance &inst,
                                 const std::string &instanceId,
                                 const MKCBResultRow &mkcbRow,
                                 int dpConstraintIdx,
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

    // Step 1: FPLS + DP
    std::mt19937 rng(123456789u
                     + static_cast<unsigned int>(runIndex)
                     + static_cast<unsigned int>(dpConstraintIdx) * 1000007u);

    std::vector<double> u(m, 0.0);

    for (int t = 1; t <= g_numIterations; ++t) {
        double delta = 1.0 / static_cast<double>(t + g_gamma - 1);

        int cap = inst.capacities[dpConstraintIdx];
        std::vector<std::vector<double>> dp(n + 1, std::vector<double>(cap + 1, 0LL)); //dp[i][w] = w의 무게로 i번째 아이템까지 넣었을 때 크기

        for(int j=0; j<n; j++) {
            for(int w=0; w<=cap; w++) { //j : index of Items
                //우선 dpConstraints를 제외하고 나머지의 LM값 계산
                double lagCost = 0.0;
                for (int ki=0; ki < m; ki++) { //ki : index of Constraints
                    if (ki == dpConstraintIdx)
                        continue ;
                    lagCost += u[ki] * static_cast<double>(inst.weights[ki][j]);
                }
                //Lagrangian 조건에 부합하면 그때 dp를 돌린다.
                double profits = 0;
                if (static_cast<double>(inst.profits[j]) > lagCost)
                    profits = static_cast<double>(inst.profits[j]) - lagCost;

                //dpConstraint에 대해서만 knapsack dp 시작
                if (j == 0) {
                    if (w > 0)
                        dp[j][w] = dp[j][w-1];
                    if (w >= inst.weights[dpConstraintIdx][j])
                        dp[j][w] = std::max(dp[j][w], profits);
                }
                else {
                    dp[j][w] = std::max(dp[j][w], dp[j-1][w]);
                    if (w > 0)
                        dp[j][w] = std::max(dp[j][w], dp[j][w-1]);
                    if (w >= inst.weights[dpConstraintIdx][j])
                        dp[j][w] = std::max(dp[j][w], dp[j-1][w - inst.weights[dpConstraintIdx][j]] + profits);
                }   
            }
        }
        
        //dp 역추적해서 x값 결정하기 (최종값은 dp[n-1][cap]에 존재)
        std::vector<int> x(n, 0);
        int rem = cap;
        const double EPS = 1e-9;
        for (int j = n - 1; j > 0; --j) {
            if (dp[j][rem] - dp[j-1][rem] > EPS) { //dp[j][rem] != dp[j-1][rem]
                x[j] = 1;
                rem -= inst.weights[dpConstraintIdx][j];
            }
            if (rem == 0 || dp[j][rem] == 0)
                break ;
        }
        if (rem >= inst.weights[dpConstraintIdx][0] && dp[0][rem] > 0) //j=0일때도 포함
            x[0] = 1;

        // bStar: FPLS 아이템의 제약별 사용량
        std::vector<long long> bStar(m, 0LL);
        for (int ki = 0; ki < m; ++ki) { //ki : index of Constraints
            for (int ji = 0; ji < n; ++ji) { //ji : index of Items
                if (x[ji]) bStar[ki] += inst.weights[ki][ji];
            }
        }

        // totalObj = 최종 목적함수 값
        double totalObj = 0.0;
        for (int ji = 0; ji < n; ++ji) {
            if (x[ji]) {
                totalObj += inst.profits[ji];
            }
        }
            

        // if (totalObj > runRes.ourBestLP)
        //     runRes.ourBestLP = totalObj;

        // I, J 분류
        std::vector<int> I, J;
        for (int ki = 0; ki < m; ++ki) { //ki : index of Constraints
            if (bStar[ki] <= static_cast<long long>(inst.capacities[ki]))
                I.push_back(ki);
            else
                J.push_back(ki);
        }

        // //DEBUG
        // if (t%1000 == 0) {
        //     std::cout << "\nt: " << t << " / dpConstraintIdx: " << dpConstraintIdx << " delta: " << delta << "\n";
        // }

        if (J.empty()) {
            if (totalObj > runRes.ourBestSolution)
                runRes.ourBestSolution = totalObj;
            
            // //debug
            // std::cout << "\nt: " << t << " / dpConstraintIdx: " << dpConstraintIdx << "\n";
            // std::cout << dp[n-1][cap] << " => " << I.size() << " " << J.size() << "\n";
            // for(int i=0; i<I.size(); i++)
            //     std::cout << I[i] << ", ";
            // std::cout << "\n" << totalObj << " , " << runRes.ourBestSolution << "\n";
            

            // //DEBUG
            // int wsum = 0;
            // for (int i=0; i<m; i++) {
            //     int wsum = 0;
            //     for(int j=0; j<n; j++) {
            //         if (x[j]) wsum += inst.weights[i][j];
            //     }
            //     if (inst.capacities[i] < wsum) { //Error
            //         std::cout << "Error! : " << inst.capacities[i] << " < " << wsum << ": ";
            //         for(int j=0; j<n; j++)
            //             if (x[j]) std::cout << j << ", ";
            //         std::cout << "\n";
            //     }
            // }

            if (!I.empty()) {
                std::uniform_int_distribution<int> dist(0, static_cast<int>(I.size()) - 1);
                int i = I[dist(rng)];
                u[i] = std::max(0.0, u[i] - delta);
            }
        } else {
            std::uniform_int_distribution<int> dist(0, static_cast<int>(J.size()) - 1);
            int i = J[dist(rng)];
            u[i] += delta;
        }
    }

    double LP = runRes.lpOptimum;
    double bestFeasibleCB = runRes.bestFeasibleCB;
    if (bestFeasibleCB > 0.0)
        runRes.percentDiffSolution = 100.0 * (bestFeasibleCB - runRes.ourBestSolution) / bestFeasibleCB;
    else
        runRes.percentDiffSolution = 0.0;
    if (LP > 0.0)
        runRes.percentDiffLP = 100.0 * (LP - runRes.ourBestLP) / LP;
    else
        runRes.percentDiffLP       = 0.0;

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

            std::cout << dpIdx << ", " << r << " - " << bestRun.ourBestSolution << ", " << bestDpIdx << "\n"; //DEBUG
        }
    }
}
