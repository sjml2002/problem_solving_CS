#ifndef DP_FPLS_H
#define DP_FPLS_H

#include "dataIO.h"
#include "original_FPLS.h"
#include <vector>

// 1-D 0/1 knapsack DP
// dpConstraintIdx 번 제약만 사용.
// selected[j] = 1 이면 물건 j가 DP 최적해에 포함됨.
long long solve_dp(const MKPInstance &inst,
              int dpConstraintIdx,
              std::vector<int> &selected);

// b_i 하나를 고정해서 돌리는 단일 run
// - dpConstraintIdx: DP에 사용할 제약 인덱스 i
// - runIndex: 0..g_numRuns-1
// - FPLS는 x_dp를 제외한 아이템, b_i를 제외한 m-1개 제약으로 실행
FPLSRunResult run_dp_fpls_single(const MKPInstance &inst,
                                 const std::string &instanceId,
                                 const MKCBResultRow &mkcbRow,
                                 int dpConstraintIdx,
                                 int runIndex);

// b_0 ~ b_{m-1} 각각에 대해 g_numRuns번씩 전부 실행
// allRuns    : 모든 run 결과 (m * g_numRuns 개)
// bestRun    : 전체 중 가장 좋은 단일 run
// bestDpIdx  : best가 나온 제약 인덱스
void run_dp_fpls_all(const MKPInstance &inst,
                     const std::string &instanceId,
                     const MKCBResultRow &mkcbRow,
                     std::vector<FPLSRunResult> &allRuns,
                     FPLSRunResult &bestRun,
                     int &bestDpIdx);

#endif // DP_FPLS_H