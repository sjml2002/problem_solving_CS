#include "dataIO.h"
#include "dp_FPLS.h"
#include "original_FPLS.h"

#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <iomanip>


struct MKNap1Instance {
    MKPInstance inst;
    double       optimalValue;
};


bool read_mknap1(const std::string &filePath, std::vector<MKNap1Instance> &problems)
{
    std::ifstream fin(filePath.c_str());
    if (!fin) {
        std::cerr << "[ERROR] Failed to open: " << filePath << std::endl;
        return false;
    }

    int K;
    if (!(fin >> K)) return false;

    for (int k = 0; k < K; ++k) {
        MKNap1Instance item;
        MKPInstance &inst = item.inst;

        if (!(fin >> inst.numItems >> inst.numConstraints >> item.optimalValue))
            return false;

        int n = inst.numItems;
        int m = inst.numConstraints;

        inst.profits.resize(n);
        for (int j = 0; j < n; ++j) {
            double tmp;
            fin >> tmp;
            inst.profits[j] = static_cast<int>(std::round(tmp));
        }

        inst.weights.assign(m, std::vector<int>(n));
        for (int i = 0; i < m; ++i)
            for (int j = 0; j < n; ++j)
                fin >> inst.weights[i][j];

        inst.capacities.resize(m);
        for (int i = 0; i < m; ++i)
            fin >> inst.capacities[i];

        problems.push_back(item);
    }
    return true;
}


int main()
{
    std::string fpath = "./MKP_instances/";

    std::vector<MKNap1Instance> problems;
    if (!read_mknap1(fpath + "mknap1.txt", problems)) {
        std::cerr << "[ERROR] Failed to read mknap1.txt" << std::endl;
        return 1;
    }
    std::cerr << "[INFO] Loaded " << problems.size() << " problems from mknap1.txt" << std::endl;

    std::ofstream fout("./results/checkOptimal_results.csv");
    if (!fout) {
        std::cerr << "[ERROR] Failed to open output CSV" << std::endl;
        return 1;
    }

    fout << "instance_id,n,m,optimal,"
            "orig_best_sol,orig_%diff,"
            "dp_best_sol,dp_%diff,best_dp_constraint\n";

    double sumOrigPct = 0.0, sumDpPct = 0.0;
    int N = static_cast<int>(problems.size());

    for (int k = 0; k < N; ++k) {
        const MKPInstance &inst   = problems[k].inst;
        double             optVal = problems[k].optimalValue;

        std::ostringstream ossId;
        ossId << "mknap1-" << (k + 1);
        std::string instanceId = ossId.str();

        std::cerr << "[INFO] " << instanceId
                  << " (n=" << inst.numItems
                  << ", m=" << inst.numConstraints
                  << ", opt=" << optVal << ")" << std::endl;

        // optimal을 lpOptimum/bestFeasible로 사용
        MKCBResultRow mkcbRow;
        mkcbRow.instanceId   = instanceId;
        mkcbRow.lpOptimum    = optVal;
        mkcbRow.bestFeasible = optVal;

        // ── Original FPLS ──────────────────────────────
        FPLSRunResult origBest;
        origBest.ourBestSolution = -1e18;
        for (int r = 0; r < g_numRuns; ++r) {
            FPLSRunResult res = run_fpls_single(inst, instanceId, mkcbRow, r);
            if (res.ourBestSolution > origBest.ourBestSolution)
                origBest = res;
        }
        double origPct = (optVal > 0.0)
            ? 100.0 * (optVal - origBest.ourBestSolution) / optVal : 0.0;

        // ── DP-FPLS ────────────────────────────────────
        std::vector<FPLSRunResult> allRuns;
        FPLSRunResult dpBest;
        int bestDpIdx = -1;
        run_dp_fpls_all(inst, instanceId, mkcbRow, allRuns, dpBest, bestDpIdx);
        double dpPct = (optVal > 0.0)
            ? 100.0 * (optVal - dpBest.ourBestSolution) / optVal : 0.0;

        // 콘솔 출력
        std::cout << instanceId
                  << " | opt=" << optVal
                  << " | orig=" << origBest.ourBestSolution
                  << " (" << std::fixed << std::setprecision(2) << origPct << "%)"
                  << " | dp=" << dpBest.ourBestSolution
                  << " (" << dpPct << "%)"
                  << " | best_dp_constraint=b_" << bestDpIdx
                  << "\n";

        // CSV 행
        fout << instanceId << ","
             << inst.numItems << ","
             << inst.numConstraints << ","
             << optVal << ","
             << origBest.ourBestSolution << ","
             << std::fixed << std::setprecision(4) << origPct << ","
             << dpBest.ourBestSolution << ","
             << dpPct << ","
             << bestDpIdx << "\n";

        sumOrigPct += origPct;
        sumDpPct   += dpPct;
    }

    // 평균 행
    fout << "AVERAGE,,,,,"
         << std::fixed << std::setprecision(4) << sumOrigPct / N << ",,"
         << sumDpPct / N << ",\n";

    std::cerr << "[INFO] CSV written to ./results/checkOptimal_results.csv" << std::endl;
    std::cout << "\n=== AVERAGE %diff ==="
              << "\n  original FPLS : " << std::fixed << std::setprecision(4) << sumOrigPct / N << "%"
              << "\n  dp-FPLS       : " << sumDpPct / N << "%\n";

    return 0;
}