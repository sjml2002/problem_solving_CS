#ifndef DATA_IO_H
#define DATA_IO_H

#include <string>
#include <vector>
#include <utility>
#include <istream>

// MKP 인스턴스 정보를 저장하기 위한 구조체
struct MKPInstance {
    int numItems;              // 물건 개수 n
    int numConstraints;        // 제약 개수 m
    int tightnessClass;        // 타이트니스 클래스 (예: 100, 250, 500)
    std::vector<int> profits;  // 각 물건의 이득 c_j
    std::vector< std::vector<int> > weights; // 각 제약 i에 대한 a_ij, 크기 m x n
    std::vector<int> capacities;             // 각 제약의 용량 b_i
};

// mkcbres 에서 읽어온 기준 값들을 저장
struct MKCBResultRow {
    std::string instanceId; // 예: "5.100-00"
    double bestFeasible;    // Chu-Beasley GA 의 best solution 값
    double lpOptimum;       // LP relaxation optimum (upper bound)
};

// 한 번의 FPLS 실행 결과 (어떤 FPLS 변형에도 공통으로 사용 가능하도록 정의)
struct FPLSRunResult {
    std::string instanceId; // 문제 ID
    int runIndex;           // 0..R-1
    double lpOptimum;       // mkcbres 에서 가져온 LP optimal 값
    double bestFeasibleCB;  // mkcbres 에서 가져온 CB-GA best feasible 값
    double ourBestSolution; // 이번 run 에서 FPLS 가 찾은 best feasible 값
    double ourBestLP;       // 이번 run 에서 FPLS 가 찾은 최고 Lagrangian bound θ(u)
    double percentDiffSolution; // 100 * (LP_opt - ourBestSolution) / LP_opt
    double percentDiffLP;       // 100 * (LP_opt - ourBestLP) / LP_opt
};

// mkcbres 결과 전체를 저장 (두 테이블을 파싱한 후 매핑한 구조)
struct MKCBResults {
    std::vector<MKCBResultRow> rows;
};

// --------- 데이터 입출력 함수 선언 ---------

// OR-Library mknapcb 파일에서 단일 인스턴스를 읽어오는 함수
bool read_single_instance(std::istream &in, MKPInstance &inst);

// mkcbres 파일을 읽어서 MKCBResults 구조체에 채우는 함수
MKCBResults read_mkcbres(const std::string &filePath);

// 모든 run 결과를 하나의 CSV 로 저장하는 함수
void write_results_csv(const std::string &csvPath,
                       const std::vector<FPLSRunResult> &allRuns);

#endif // DATA_IO_H