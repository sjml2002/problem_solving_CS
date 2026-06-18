#include "verify_solution.h"
#include <iostream>
#include <numeric>

void verify_and_print(
    const MKPInstance      &inst,
    const std::string      &instanceId,
    const std::vector<int> &dpSelected,
    const std::vector<int> &fplsX,
    const std::vector<int> &fplsItems,
    int                     dpConstraintIdx,
    double                  lpOpt)
{
    int n = inst.numItems;
    int m = inst.numConstraints;

    // 전체 선택 벡터 합산 (dp + fpls)
    std::vector<int> combined(n, 0);
    for (int j = 0; j < n; ++j)
        if (dpSelected[j]) combined[j] = 1;
    for (int ji = 0; ji < (int)fplsX.size(); ++ji)
        if (fplsX[ji]) combined[fplsItems[ji]] = 1;

    // 목적함수 직접 계산
    long long objVal = 0;
    for (int j = 0; j < n; ++j)
        if (combined[j]) objVal += inst.profits[j];

    std::cout << "\n========== VERIFY: " << instanceId << " ==========\n";
    std::cout << "LP_opt = " << lpOpt << "\n";
    std::cout << "Computed objVal = " << objVal << "\n";
    std::cout << (objVal > lpOpt ? "!!! objVal > LP_opt -> BUG !!!" : "objVal <= LP_opt -> OK") << "\n";

    // 선택 아이템 목록 출력
    std::cout << "\n[DP selected items] (constraint=" << dpConstraintIdx << ")\n  items: ";
    for (int j = 0; j < n; ++j)
        if (dpSelected[j]) std::cout << j << " ";
    std::cout << "\n";

    std::cout << "[FPLS selected items]\n  items: ";
    for (int ji = 0; ji < (int)fplsX.size(); ++ji)
        if (fplsX[ji]) std::cout << fplsItems[ji] << " ";
    std::cout << "\n";

    // 제약별 사용량 검사
    std::cout << "\n[Constraint feasibility check]\n";
    bool allFeasible = true;
    for (int i = 0; i < m; ++i) {
        long long dpUse   = 0;
        long long fplsUse = 0;
        long long total   = 0;

        for (int j = 0; j < n; ++j)
            if (dpSelected[j]) dpUse += inst.weights[i][j];
        for (int ji = 0; ji < (int)fplsX.size(); ++ji)
            if (fplsX[ji]) fplsUse += inst.weights[i][fplsItems[ji]];
        total = dpUse + fplsUse;

        bool ok = (total <= inst.capacities[i]);
        if (!ok) allFeasible = false;

        std::cout << "  b[" << i << "]"
                  << (i == dpConstraintIdx ? "(DP)" : "    ")
                  << " : dp=" << dpUse
                  << " fpls=" << fplsUse
                  << " total=" << total
                  << " / cap=" << inst.capacities[i]
                  << (ok ? " OK" : " *** VIOLATION ***")
                  << "\n";
    }

    std::cout << "\nOverall feasible: " << (allFeasible ? "YES" : "NO *** BUG ***") << "\n";
    std::cout << "=============================================\n\n";
}