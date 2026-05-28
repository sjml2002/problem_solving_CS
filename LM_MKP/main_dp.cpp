#include "dataIO.h"
#include "dp_fpls.h"
#include "original_FPLS.h"   // g_numRuns, g_numIterations, g_gamma 사용

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

void process_file_with_dp_fpls(const std::string &filePath,
                               const std::string &prefixId,
                               const MKCBResults &mkcb,
                               std::vector<FPLSRunResult> &allRuns)
{
    std::ifstream fin(filePath.c_str());
    if (!fin) {
        std::cerr << "[ERROR] Failed to open data file: " << filePath << std::endl;
        return;
    }

    int numProblems;
    if (!(fin >> numProblems)) {
        std::cerr << "[ERROR] Failed to read problem count in: " << filePath << std::endl;
        return;
    }

    std::cerr << "[INFO] Processing file: " << filePath
              << " with " << numProblems << " problems." << std::endl;

    for (int idx = 0; idx < numProblems; ++idx) {
        MKPInstance inst;
        if (!read_single_instance(fin, inst)) {
            std::cerr << "[ERROR] Failed to read instance index " << idx
                      << " in file: " << filePath << std::endl;
            break;
        }

        // instance ID 생성: 예) "5.100-07"
        std::ostringstream ossId;
        ossId << prefixId << "-";
        if (idx < 10) ossId << "0" << idx;
        else ossId << idx;
        std::string instanceId = ossId.str();

        // mkcbres 에서 기준 값 찾기
        MKCBResultRow mkcbRow;
        bool found = false;
        for (const auto &row : mkcb.rows) {
            if (row.instanceId == instanceId) {
                mkcbRow = row;
                found = true;
                break;
            }
        }
        if (!found) {
            std::cerr << "[WARN] mkcbres row not found for instanceId = "
                      << instanceId << std::endl;
            continue;
        }

        std::cerr << "  [INFO] Instance " << instanceId
                  << " (n=" << inst.numItems
                  << ", m=" << inst.numConstraints << ")" << std::endl;

        // DP+FPLS 실행
        for (int r = 0; r < g_numRuns; ++r) {
            FPLSRunResult runRes = run_dp_fpls_single(inst, instanceId, mkcbRow, r);
            allRuns.push_back(runRes);
        }
    }
}

int main()
{
    std::string fpath = "./MKP_instances/";
    
    /* dp FPLS */
    MKCBResults mkcb = read_mkcbres(fpath + "mkcbres.txt");
    if (mkcb.rows.empty()) {
        std::cerr << "[ERROR] No mkcbres rows loaded. Exiting." << std::endl;
        return 1;
    }

    std::vector<FPLSRunResult> allRuns;
    allRuns.reserve(9 * 30 * g_numRuns);

    process_file_with_dp_fpls(fpath + "mknapcb1.txt", "5.100", mkcb, allRuns);
    process_file_with_dp_fpls(fpath + "mknapcb2.txt", "5.250", mkcb, allRuns);
    process_file_with_dp_fpls(fpath + "mknapcb3.txt", "5.500", mkcb, allRuns);
    process_file_with_dp_fpls(fpath + "mknapcb4.txt", "10.100", mkcb, allRuns);
    process_file_with_dp_fpls(fpath + "mknapcb5.txt",  "10.250", mkcb, allRuns);
    process_file_with_dp_fpls(fpath + "mknapcb6.txt",  "10.500", mkcb, allRuns);
    process_file_with_dp_fpls(fpath + "mknapcb7.txt",  "30.100", mkcb, allRuns);
    process_file_with_dp_fpls(fpath + "mknapcb8.txt",  "30.250", mkcb, allRuns);
    process_file_with_dp_fpls(fpath + "mknapcb9.txt",  "30.500", mkcb, allRuns);

    write_results_csv("./results/dp_fpls_results.csv", allRuns);

    return 0;
}