#include "dataIO.h"
#include "dp_FPLS.h"
#include "original_FPLS.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>

void write_dp_fpls_csv(const std::string &csvPath,
                       const std::vector<FPLSRunResult> &allRuns,
                       const std::vector<std::pair<std::string, FPLSRunResult>> &bestPerInstance,
                       const std::vector<std::pair<std::string, int>> &bestDpIdxPerInstance)
{
    std::ofstream fout(csvPath.c_str());
    if (!fout) {
        std::cerr << "[ERROR] Failed to open CSV: " << csvPath << std::endl;
        return;
    }

    // 헤더
    fout << "instance_id,run_index,LP_opt,best_feasible_CB,"
            "our_LP,our_best_solution,percent_diff_solution,percent_diff_LP,dp_weight\n";

    // 1) 전체 run 기록
    for (const auto &r : allRuns) {
        fout << r.instanceId << ","
             << r.runIndex << ","
             << r.lpOptimum << ","
             << r.bestFeasibleCB << ","
             << r.ourBestLP << ","
             << r.ourBestSolution << ","
             << r.percentDiffSolution << ","
             << r.percentDiffLP << ","
             << r.dpWeight << "\n";
    }

    // 2) "-dp{i}" 별 평균
    struct Agg {
        int       count     = 0;
        double    sumBestSol = 0, sumLP = 0, sumPctSol = 0, sumPctLP = 0;
        double    lpOpt = 0, bestCB = 0;
        long long dpWeight  = 0;  // 같은 dp_i 그룹에선 전부 동일
    };
    std::map<std::string, Agg> byGroup;
    for (const auto &r : allRuns) {
        Agg &a = byGroup[r.instanceId];
        a.count++;
        a.sumBestSol += r.ourBestSolution;
        a.sumLP      += r.ourBestLP;
        a.sumPctSol  += r.percentDiffSolution;
        a.sumPctLP   += r.percentDiffLP;
        a.lpOpt       = r.lpOptimum;
        a.bestCB      = r.bestFeasibleCB;
        a.dpWeight    = r.dpWeight;  // run마다 동일하므로 덮어써도 무방
    }
    for (const auto &kv : byGroup) {
        const Agg &a = kv.second;
        if (a.count == 0) continue;
        fout << kv.first << ",AVG,"
             << a.lpOpt << "," << a.bestCB << ","
             << a.sumLP      / a.count << ","
             << a.sumBestSol / a.count << ","
             << a.sumPctSol  / a.count << ","
             << a.sumPctLP   / a.count << ","
             << a.dpWeight << "\n";
    }

    // 3) 인스턴스별 best run + best_dp_constraint
    for (size_t k = 0; k < bestPerInstance.size(); ++k) {
        const std::string   &baseId = bestPerInstance[k].first;
        const FPLSRunResult &br     = bestPerInstance[k].second;
        int                  bIdx   = bestDpIdxPerInstance[k].second;
        fout << baseId << ",BEST_RUN,"
             << br.lpOptimum << "," << br.bestFeasibleCB << ","
             << br.ourBestLP << "," << br.ourBestSolution << ","
             << br.percentDiffSolution << "," << br.percentDiffLP << ","
             << br.dpWeight
             << ",best_dp_constraint=" << bIdx << "\n";
    }

    std::cerr << "[INFO] CSV written to " << csvPath << std::endl;
}

void process_file_with_dp_fpls(
        const std::string &filePath,
        const std::string &prefixId,
        const MKCBResults &mkcb,
        std::vector<FPLSRunResult> &allRuns,
        std::vector<std::pair<std::string, FPLSRunResult>> &bestPerInstance,
        std::vector<std::pair<std::string, int>> &bestDpIdxPerInstance)
{
    std::ifstream fin(filePath.c_str());
    if (!fin) {
        std::cerr << "[ERROR] Failed to open: " << filePath << std::endl;
        return;
    }

    int numProblems;
    if (!(fin >> numProblems)) return;

    std::cerr << "[INFO] Processing: " << filePath
              << " (" << numProblems << " problems)" << std::endl;

    for (int idx = 0; idx < numProblems; ++idx) {
        MKPInstance inst;
        if (!read_single_instance(fin, inst)) break;

        std::ostringstream ossId;
        ossId << prefixId << "-";
        if (idx < 10) ossId << "0" << idx;
        else ossId << idx;
        std::string instanceId = ossId.str();

        MKCBResultRow mkcbRow;
        bool found = false;
        for (const auto &row : mkcb.rows) {
            if (row.instanceId == instanceId) { mkcbRow = row; found = true; break; }
        }
        if (!found) {
            std::cerr << "[WARN] Not found in mkcbres: " << instanceId << std::endl;
            continue;
        }

        std::cerr << "  [INFO] " << instanceId
                  << " (n=" << inst.numItems
                  << ", m=" << inst.numConstraints << ")" << std::endl;

        FPLSRunResult bestRun;
        int bestDpIdx = -1;
        run_dp_fpls_all(inst, instanceId, mkcbRow, allRuns, bestRun, bestDpIdx);

        bestPerInstance.push_back({instanceId, bestRun});
        bestDpIdxPerInstance.push_back({instanceId, bestDpIdx});
    }
}

int main()
{
    MKCBResults mkcb = read_mkcbres("mkcbres-9.txt");
    if (mkcb.rows.empty()) {
        std::cerr << "[ERROR] No mkcbres rows loaded." << std::endl;
        return 1;
    }

    std::vector<FPLSRunResult> allRuns;
    std::vector<std::pair<std::string, FPLSRunResult>> bestPerInstance;
    std::vector<std::pair<std::string, int>> bestDpIdxPerInstance;

    process_file_with_dp_fpls("mknapcb1-10.txt", "5.100",  mkcb, allRuns, bestPerInstance, bestDpIdxPerInstance);
    process_file_with_dp_fpls("mknapcb2-11.txt", "5.250",  mkcb, allRuns, bestPerInstance, bestDpIdxPerInstance);
    process_file_with_dp_fpls("mknapcb3-12.txt", "5.500",  mkcb, allRuns, bestPerInstance, bestDpIdxPerInstance);
    process_file_with_dp_fpls("mknapcb4-13.txt", "10.100", mkcb, allRuns, bestPerInstance, bestDpIdxPerInstance);
    process_file_with_dp_fpls("mknapcb5-3.txt",  "10.250", mkcb, allRuns, bestPerInstance, bestDpIdxPerInstance);
    process_file_with_dp_fpls("mknapcb6-4.txt",  "10.500", mkcb, allRuns, bestPerInstance, bestDpIdxPerInstance);
    process_file_with_dp_fpls("mknapcb7-5.txt",  "30.100", mkcb, allRuns, bestPerInstance, bestDpIdxPerInstance);
    process_file_with_dp_fpls("mknapcb8-6.txt",  "30.250", mkcb, allRuns, bestPerInstance, bestDpIdxPerInstance);
    process_file_with_dp_fpls("mknapcb9-7.txt",  "30.500", mkcb, allRuns, bestPerInstance, bestDpIdxPerInstance);

    write_dp_fpls_csv("dp_fpls_results.csv", allRuns, bestPerInstance, bestDpIdxPerInstance);

    return 0;
}