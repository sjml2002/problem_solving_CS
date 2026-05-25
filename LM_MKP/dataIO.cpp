#include "dataIO.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <limits>

// OR-Library mknapcb 파일 포맷을 기준으로 한 인스턴스를 읽는다.
// 파일에는 먼저 문제 개수(보통 30)가 주어지고, 각 문제마다
//   n m tightness
//   m 줄의 weights
//   ...
//   profits (n 개)
//   capacities (m 개)
// 의 순서로 데이터가 주어진다.
bool read_single_instance(std::istream &in, MKPInstance &inst)
{
    int n, m;
    int optimal; // 최적값 (알 수 없으면 0). 지금 실험에는 안 쓰지만 읽어는 둔다.

    // 1) n, m, optimal solution value
    if (!(in >> n >> m >> optimal)) {
        return false; // 더 이상 읽을 인스턴스가 없음
    }

    inst.numItems        = n;
    inst.numConstraints  = m;
    inst.tightnessClass  = optimal; // 필요하면 별도 필드로 optimal을 추가해도 됨

    inst.profits.assign(n, 0);
    inst.weights.assign(m, std::vector<int>(n));
    inst.capacities.assign(m, 0);

    // 2) profits p(j), j = 1..n
    for (int j = 0; j < n; ++j) {
        in >> inst.profits[j];
    }

    // 3) weights r(i,j) for each constraint i = 1..m
    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < n; ++j) {
            in >> inst.weights[i][j];
        }
    }

    // 4) capacities b(i), i = 1..m
    for (int i = 0; i < m; ++i) {
        in >> inst.capacities[i];
    }

    return true;
}

// mkcbres 파일을 읽어서 각 instance ID 에 대해 best feasible, LP optimum 을 저장한다.
// mkcbres-9.txt 는 두 개의 테이블로 구성되어 있는데, 둘 다 같은 ID 순서로 정렬되어 있다고 가정한다.
MKCBResults read_mkcbres(const std::string &filePath)
{
    MKCBResults results;

    std::ifstream fin(filePath.c_str());
    if (!fin) {
        std::cerr << "[ERROR] Failed to open mkcbres file: " << filePath << std::endl;
        return results;
    }

    std::string line;
    int phase = 0; 
    // 0: 아직 첫 번째 테이블 시작 전
    // 1: 첫 번째 테이블(베스트 feasible) 읽는 중
    // 2: 첫 번째 테이블 끝, 두 번째 테이블 헤더 찾는 중
    // 3: 두 번째 테이블(LP optimal) 읽는 중

    std::vector<std::pair<std::string, double> > bestFeasibleRows;
    std::vector<std::pair<std::string, double> > lpRows;

    while (std::getline(fin, line)) {
	// 양 끝 공백 제거
        auto trim = [](std::string &s) {
            size_t p = s.find_first_not_of(" \t\r\n");
            if (p == std::string::npos) { s.clear(); return; }
            size_t q = s.find_last_not_of(" \t\r\n");
            s = s.substr(p, q - p + 1);
        };
        trim(line);
        if (line.empty()) {
            // 빈 줄이면, 첫 테이블에서 두 번째 테이블로 넘어가는 경계일 수 있으므로
            // phase 1 이었던 경우는 2로, phase 3 이면 그대로 둔다.
            if (phase == 1) phase = 2;
            continue;
        }

        // 헤더 줄 감지
        if (line.find("Problem Name") == 0) {
            if (phase == 0) {
                // 첫 번째 테이블 헤더
                phase = 1;
            } else if (phase == 2) {
                // 두 번째 테이블 헤더
                phase = 3;
            }
            continue;
        }

        // 실제 데이터 줄 파싱
        if (phase == 1 || phase == 3) {
            std::istringstream iss(line);
            std::string id;
            double val;
            if (!(iss >> id >> val)) {
                continue; // 형식이 안 맞으면 스킵
            }
            // id 가 "5.100-00" 같은 형태인지 대략 검사
            bool looksLikeId = (id.find('.') != std::string::npos &&
                                id.find('-') != std::string::npos);
            if (!looksLikeId) continue;

            if (phase == 1) {
                bestFeasibleRows.emplace_back(id, val);
            } else { // phase == 3
                lpRows.emplace_back(id, val);
            }
        }
        // phase 0,2 에서는 아무 것도 하지 않음
    }

    // 두 테이블을 매칭
    size_t len = std::min(bestFeasibleRows.size(), lpRows.size());
    results.rows.reserve(len);
    for (size_t i = 0; i < len; ++i) {
        MKCBResultRow row;
        row.instanceId   = bestFeasibleRows[i].first;
        row.bestFeasible = bestFeasibleRows[i].second;
        row.lpOptimum    = lpRows[i].second;
        results.rows.push_back(row);
    }

    std::cerr << "[INFO] Loaded " << results.rows.size()
              << " rows from mkcbres." << std::endl;
    return results;
}



// 모든 run 결과를 하나의 CSV 로 저장한다.
// 형식: instance_id,run_index,LP_opt,best_feasible_CB,our_LP,our_best_solution,percent_diff_solution,percent_diff_LP
void write_results_csv(const std::string &csvPath,
                       const std::vector<FPLSRunResult> &allRuns)
{
    std::ofstream fout(csvPath.c_str());
    if (!fout) {
        std::cerr << "[ERROR] Failed to open CSV for writing: " << csvPath << std::endl;
        return;
    }

    fout << "instance_id,run_index,LP_opt,best_feasible_CB,our_LP,our_best_solution,"
            "percent_diff_solution,percent_diff_LP\n";

    // 먼저 모든 run 을 그대로 기록
    for (const auto &r : allRuns) {
        fout << r.instanceId << ","
             << r.runIndex << ","
             << r.lpOptimum << ","
             << r.bestFeasibleCB << ","
             << r.ourBestLP << ","
             << r.ourBestSolution << ","
             << r.percentDiffSolution << ","
             << r.percentDiffLP << "\n";
    }

    // 인스턴스별 평균과 전체 평균 계산
    struct Agg {
        int count = 0;
        double sumBestSol = 0.0;
        double sumLP = 0.0;
        double sumPctSol = 0.0;
        double sumPctLP = 0.0;
        double lpOpt = 0.0;
        double bestCB = 0.0;
    };

    std::map<std::string, Agg> byInst;

    for (const auto &r : allRuns) {
        Agg &a = byInst[r.instanceId];
        a.count += 1;
        a.sumBestSol += r.ourBestSolution;
        a.sumLP += r.ourBestLP;
        a.sumPctSol += r.percentDiffSolution;
        a.sumPctLP += r.percentDiffLP;
        a.lpOpt = r.lpOptimum;
        a.bestCB = r.bestFeasibleCB;
    }

    double totalPctSol = 0.0, totalPctLP = 0.0;
    int totalCount = 0;

    for (const auto &kv : byInst) {
        const std::string &id = kv.first;
        const Agg &a = kv.second;
        if (a.count == 0) continue;
        double avgBestSol = a.sumBestSol / a.count;
        double avgLP = a.sumLP / a.count;
        double avgPctSol = a.sumPctSol / a.count;
        double avgPctLP = a.sumPctLP / a.count;

        totalPctSol += avgPctSol;
        totalPctLP += avgPctLP;
        totalCount += 1;

        fout << id << ",AVG,"
             << a.lpOpt << ","
             << a.bestCB << ","
             << avgLP << ","
             << avgBestSol << ","
             << avgPctSol << ","
             << avgPctLP << "\n";
    }

    if (totalCount > 0) {
        double globalAvgPctSol = totalPctSol / totalCount;
        double globalAvgPctLP = totalPctLP / totalCount;
        fout << "GLOBAL_AVG,NA,0,0,0,0,"
             << globalAvgPctSol << ","
             << globalAvgPctLP << "\n";
    }

    std::cerr << "[INFO] CSV written to " << csvPath << std::endl;
}
