#ifndef DP_FPLS_H
#define DP_FPLS_H

#include "dataIO.h"
#include "original_FPLS.h"

// 1-D 0/1 knapsack DP
// dpConstraintIdx 번 제약(weights[dpConstraintIdx], capacities[dpConstraintIdx])만 사용.
// selected[j] = 1 이면 물건 j가 DP 최적해에 포함됨.
void solve_dp(const MKPInstance &inst,
              int dpConstraintIdx,
              std::vector<int> &selected);

// DP+FPLS 단일 run 실행
// 알고리즘:
//   1. 모든 m개의 제약에 대해 각각 DP를 풀어서 m개의 후보해를 만든다.
//   2. 각 후보해에 대해 전체 m개 제약 feasibility 확인 후,
//      feasible한 해들 중 목적함수 값이 가장 큰 것을 ourBestSolution 초기값으로 세팅.
//   3. Original FPLS 루프를 그대로 실행.
FPLSRunResult run_dp_fpls_single(const MKPInstance &inst,
                                 const std::string &instanceId,
                                 const MKCBResultRow &mkcbRow,
                                 int runIndex);

#endif // DP_FPLS_H