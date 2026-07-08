#include "original_FPLS.h"

#include <random>
#include <algorithm>
#include <iostream>
#include <limits>

// 전역 파라미터 기본값 설정
int g_numIterations = 50; // N
int g_gamma = 10;            // \gamma
int g_numRuns = 10;          // R

// 고정 난수 시드
static const unsigned int FIXED_SEED = 123456789u;

// LMMKP(u, c, A)의 구현
// 논문 의사코드:
//   for j <- 1 to n
//       if c_j > sum_{i=1}^m u_i a_ij then x_j^* <- 1
//       else x_j^* <- 0
//   b^* <- A x^*
//   mu^* <- c^T x^*
//   return (mu^*, x^*, b^*)
// 여기서 mu^* = c^T x^* 는 라그랑주 용량에 대응하는 해의 목적함수 값이다.
LMMKPResult run_lmmkp(const MKPInstance &inst,
                      const LagrangianState &state)
{
    LMMKPResult result;
    int n = inst.numItems;
    int m = inst.numConstraints;

    result.x.assign(n, 0);
    result.bStar.assign(m, 0);

    const std::vector<int> &c = inst.profits;
    const std::vector< std::vector<int> > &A = inst.weights;
    const std::vector<double> &u = state.u;

    // 각 물건 j에 대해 라그랑주 비용 \sum_i u_i a_ij 를 계산한 뒤,
    // 원래 이득 c_j 와 비교하여 선택 여부를 결정한다.
    //  - c_j > \sum_i u_i a_ij 면 x_j = 1
    //  - 아니면 x_j = 0
    // 이는 라그랑주 이완 문제의 해를 닫힌형태로 구하는 규칙이다.
    for (int j = 0; j < n; ++j) {
        double lagCost = 0.0; // \sum_i u_i a_ij
        for (int i = 0; i < m; ++i) {
            lagCost += u[i] * static_cast<double>(A[i][j]);
        }
        if (static_cast<double>(c[j]) > lagCost) {
            result.x[j] = 1;
        } else {
            result.x[j] = 0;
        }
    }

    // b^* = A x^*
    for (int i = 0; i < m; ++i) {
        long long sum = 0;
        for (int j = 0; j < n; ++j) {
            if (result.x[j]) {
                sum += static_cast<long long>(A[i][j]);
            }
        }
        result.bStar[i] = static_cast<int>(sum);
    }

    // mu^* = c^T x^*
    long long muSum = 0;
    for (int j = 0; j < n; ++j) {
        if (result.x[j]) {
            muSum += static_cast<long long>(c[j]);
        }
    }
    result.mu = static_cast<double>(muSum);

    return result;
}

// FPLS 한 번 실행
// 의사코드 (논문 Fig. 4)
//   u <- 0
//   for t <- 1 to N
//       delta <- 1 / (t + gamma - 1)
//       (mu^*, x^*, b^*) <- LMMKP(u, c, A)
//       I <- { i : b_i^* <= b_i }
//       J <- { i : b_i^* >  b_i }
//       if I = {1,...,m} then
//           // 모든 제약을 만족하므로 feasible solution
//           best solution 업데이트
//           임의의 i \in I 를 골라 u_i <- u_i - delta
//       else
//           임의의 k \in J 를 골라 u_k <- u_k + delta
//   return best solution
//
// 여기서 우리는 추가로 Lagrangian bound
//   theta(u) = mu^* + u^T (b - b^*)
// 를 계산하여, 그 최대값을 ourBestLP 로 저장한다.
FPLSRunResult run_fpls_single(const MKPInstance &inst,
                              const std::string &instanceId,
                              const MKCBResultRow &mkcbRow,
                              int runIndex)
{
    int n = inst.numItems;
    int m = inst.numConstraints;

    FPLSRunResult runRes;
    runRes.instanceId = instanceId;
    runRes.runIndex = runIndex;
    runRes.lpOptimum = mkcbRow.lpOptimum;
    runRes.bestFeasibleCB = mkcbRow.bestFeasible;
    runRes.dpWeight = 0LL;   // original FPLS는 DP 없음

    // 초기 best 값들: 아직 아무 해도 없으므로 매우 나쁜 값으로 설정
    runRes.ourBestSolution = 0.0; // feasible 한 해가 없으면 0 으로 남는다 (MKP 는 비음수 이득)
    runRes.ourBestLP = -std::numeric_limits<double>::infinity();

    // 라그랑주 승수 초기화: u = 0
    LagrangianState state;
    state.u.assign(m, 0.0);

    // 고정 난수 생성기: runIndex 를 씨앗에 섞어서 run 간 독립성은 유지하되 재현성 확보
    std::mt19937 rng(FIXED_SEED + static_cast<unsigned int>(runIndex));

    const std::vector<int> &b = inst.capacities;

    for (int t = 1; t <= g_numIterations; ++t) {
        double delta = 1.0 / static_cast<double>(t + g_gamma - 1); // step size

        // 현재 u 에 대해 LMMKP 를 풀어 (mu^*, x^*, b^*) 를 얻는다.
        LMMKPResult lmRes = run_lmmkp(inst, state);

        // Lagrangian bound theta(u) = mu^* + u^T (b - b^*) 계산
        double theta = lmRes.mu;
        for (int i = 0; i < m; ++i) {
            double diff = static_cast<double>(b[i] - lmRes.bStar[i]);
            theta += state.u[i] * diff;
        }
        if (theta > runRes.ourBestLP) {
            runRes.ourBestLP = theta;
        }

        // 제약 만족 여부에 따라 I, J 집합을 만든다.
        std::vector<int> I; // b_i^* <= b_i 인 제약들의 인덱스 집합
        std::vector<int> J; // b_i^* >  b_i 인 제약들의 인덱스 집합
        I.reserve(m);
        J.reserve(m);
        bool allSatisfied = true;
        for (int i = 0; i < m; ++i) {
            if (lmRes.bStar[i] <= b[i]) {
                I.push_back(i);
            } else {
                J.push_back(i);
                allSatisfied = false;
            }
        }

        // 모든 제약이 만족되면 feasible solution 이므로 best solution 갱신 후보
        if (allSatisfied) {
            if (lmRes.mu > runRes.ourBestSolution) {
                runRes.ourBestSolution = lmRes.mu;
            }
            // I 에서 임의의 인덱스를 하나 골라 u_i 를 감소시킨다.
            std::uniform_int_distribution<int> dist(0, static_cast<int>(I.size()) - 1);
            int idx = dist(rng);
            int i = I[idx];
            state.u[i] = std::max(0.0, state.u[i] - delta); // u >= 0 를 유지하기 위해 0 이하로는 내리지 않는다.
        } else {
            // 위반된 제약들의 집합 J 에서 임의의 인덱스를 골라 u_k 를 증가시킨다.
            std::uniform_int_distribution<int> dist(0, static_cast<int>(J.size()) - 1);
            int idx = dist(rng);
            int k = J[idx];
            state.u[k] += delta;
        }
    }

    // 마지막에 percent difference 들을 계산
    double LP = runRes.lpOptimum;
    if (LP > 0.0) {
        runRes.percentDiffSolution = 100.0 * (LP - runRes.ourBestSolution) / LP;
        runRes.percentDiffLP = 100.0 * (LP - runRes.ourBestLP) / LP;
    } else {
        runRes.percentDiffSolution = 0.0;
        runRes.percentDiffLP = 0.0;
    }

    return runRes;
}