#ifndef ORIGINAL_FPLS_H
#define ORIGINAL_FPLS_H

#include <vector>
#include "dataIO.h"

// 전역 파라미터들 (논문에서 사용한 값)
extern int g_numIterations; // N: FPLS 반복 횟수
extern int g_gamma;         // \gamma: step size 를 조절하는 상수
extern int g_numRuns;       // R: 한 인스턴스에 대해 독립적으로 반복하는 run 수

// 라그랑주 곱 u 와 관련된 계산을 편하게 하기 위한 타입
struct LagrangianState {
    std::vector<double> u;  // 현재 라그랑주 승수 벡터 u \in R^m, u >= 0
};

// LMMKP(u, c, A)
//  - 입력: 라그랑주 승수 u, 인스턴스 정보
//  - 출력: (mu, x, b_star)
//    * mu      = c^T x
//    * x       = 선택된 0-1 해
//    * b_star  = A x (각 제약의 실제 사용량)
struct LMMKPResult {
    double mu;                      // 목적함수 값 c^T x
    std::vector<int> x;             // 0-1 해 벡터
    std::vector<int> bStar;         // 각 제약의 사용량 A x
};

LMMKPResult run_lmmkp(const MKPInstance &inst,
                      const LagrangianState &state);

// FPLS(A, b, c)
//  - 입력: 인스턴스
//  - 출력: 한 번의 FPLS 실행 결과 (ourBestSolution, ourBestLP 채워서 반환)
FPLSRunResult run_fpls_single(const MKPInstance &inst,
                              const std::string &instanceId,
                              const MKCBResultRow &mkcbRow,
                              int runIndex);

#endif // ORIGINAL_FPLS_H