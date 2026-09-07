#include "dataIO.h"
#include "dp_FPLS.h"
#include "original_FPLS.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <iomanip>
#include <cstdlib>


// CSV 파일 열기 + 헤더 쓰기
std::ofstream open_dp_fpls_csv(const std::string &csvPath)
{
    std::ofstream fout(csvPath.c_str());
    if (!fout) {
        std::cerr << "[ERROR] Failed to open CSV: " << csvPath << std::endl;
        return fout;
    }

    fout << "instance_id,run_index,LP_opt,best_feasible_CB,"
            "our_LP,our_best_solution,percent_diff_solution,percent_diff_LP,dp_weight\n";

    return fout;
}


// CSV 한 행 추가
void append_dp_fpls_csv(std::ofstream &fout, const FPLSRunResult &r)
{
    if (!fout) return;

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


void process_file_with_dp_fpls(
        const std::string &filePath,
        const std::string &prefixId,
        const MKCBResults &mkcb,
        std::ofstream &csvOut,
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
        else          ossId << idx;
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

        int m = inst.numConstraints;
        for (int dpIdx = 0; dpIdx < m; ++dpIdx) {
            for (int r = 0; r < g_numRuns; ++r) {
                FPLSRunResult res = run_dp_fpls_single(inst, instanceId, mkcbRow, dpIdx, r);

                std::ostringstream ossRunId;
                ossRunId << instanceId << "-dp" << dpIdx;
                res.instanceId = ossRunId.str();
                res.runIndex   = r;

                // CSV 에 즉시 쓰기
                append_dp_fpls_csv(csvOut, res);

                if (res.ourBestSolution > bestRun.ourBestSolution) {
                    bestRun   = res;
                    bestDpIdx = dpIdx;
                }
            }
        }

        std::cout << "\n=== " << instanceId
                  << " | LP_opt=" << mkcbRow.lpOptimum
                  << " | best_CB=" << mkcbRow.bestFeasible
                  << " ===\n";
        std::cout << "  >> BEST: " << bestRun.instanceId
                  << " | dpOpt=" << bestRun.dpOpt
                  << " | bestSol=" << bestRun.ourBestSolution
                  << " | %diff_sol=" << bestRun.percentDiffSolution
                  << " | best_dp_constraint=b_" << bestDpIdx
                  << "\n";

        bestPerInstance.push_back({instanceId, bestRun});
        bestDpIdxPerInstance.push_back({instanceId, bestDpIdx});
    }
}


int main(int argc, char *argv[])
{
    // 인자 처리: N (FPLS 반복 횟수)
    if (argc >= 2) {
        int N = std::atoi(argv[1]);
        if (N > 0) {
            g_numIterations = N;
            std::cerr << "[INFO] Set g_numIterations = "
                      << g_numIterations << std::endl;
        }
    }

    std::string fpath = "./MKP_instances/";

    MKCBResults mkcb = read_mkcbres(fpath + "mkcbres.txt");
    if (mkcb.rows.empty()) {
        std::cerr << "[ERROR] No mkcbres rows loaded. Exiting." << std::endl;
        return 1;
    }

    struct InputFile {
        std::string filename;
        std::string prefixId;
    };

    const std::vector<InputFile> inputFiles = {
        // // 기존 Chu-Beasley benchmark
        // {"mknapcb1.txt", "5.100"},
        // {"mknapcb2.txt", "5.250"},
        // {"mknapcb3.txt", "5.500"},
        // {"mknapcb4.txt", "10.100"},
        // {"mknapcb5.txt", "10.250"},
        // {"mknapcb6.txt", "10.500"},
        // {"mknapcb7.txt", "30.100"},
        // {"mknapcb8.txt", "30.250"},
        // {"mknapcb9.txt", "30.500"},

        // // m=3 생성 인스턴스
        // {"mknapcb1_m3.txt", "5.100"},

        // m=2 생성 인스턴스
        {"mknapcb10_m=2_a=0.25.txt", "5.100"},
        {"mknapcb10_m=2_a=0.5.txt",  "5.250"},
        {"mknapcb10_m=2_a=0.75.txt", "5.500"}
    };

    for (const auto &input : inputFiles) {
        // 확장자 제거: mknapcb10_m=2_a=0.25.txt
        //            -> mknapcb10_m=2_a=0.25
        std::string instanceName = input.filename;
        const std::string extension = ".txt";

        if (instanceName.size() >= extension.size() &&
            instanceName.compare(
                instanceName.size() - extension.size(),
                extension.size(),
                extension
            ) == 0) {
            instanceName.erase(instanceName.size() - extension.size());
        }

        // 예: ./results/mkp_dp_mknapcb10_m=2_a=0.25.csv
        std::string csvPath =
            "./results/mkp_dp_" + instanceName + ".csv";

        std::ofstream csvOut = open_dp_fpls_csv(csvPath);
        if (!csvOut) {
            std::cerr << "[ERROR] Skip file due to CSV open failure: "
                      << input.filename << std::endl;
            continue;
        }

        std::vector<std::pair<std::string, FPLSRunResult>> bestPerInstance;
        std::vector<std::pair<std::string, int>> bestDpIdxPerInstance;

        process_file_with_dp_fpls(
            fpath + input.filename,
            input.prefixId,
            mkcb,
            csvOut,
            bestPerInstance,
            bestDpIdxPerInstance
        );

        csvOut.close();

        std::cerr << "[INFO] CSV written to " << csvPath << std::endl;
    }

    return 0;
}