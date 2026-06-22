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
        if (fplsX[ji]) combined[ji] = 1;

    // 목적함수 직접 계산
    long long objVal = 0;
    for (int j = 0; j < n; ++j)
        if (combined[j]) objVal += inst.profits[j];

    std::cout << "\n========== VERIFY: " << instanceId << " ==========\n";
    std::cout << "LP_opt = " << lpOpt << "\n";
    std::cout << "Computed objVal = " << objVal << "\n";
    std::cout << (objVal > lpOpt ? "!!! objVal > LP_opt -> BUG !!!" : "objVal <= LP_opt -> OK") << "\n";

    // dp가 고른 아이템 목록 출력
    std::cout << "\n[DP selected items] (constraint=" << dpConstraintIdx << ")\n  items: ";
    for (int j = 0; j < n; ++j)
        if (dpSelected[j]) std::cout << j << " ";
    std::cout << "\n";

    // fpls가 고른 아이템 목록 출력
    std::cout << "[FPLS selected items]\n  items: ";
    for (int ji = 0; ji < (int)fplsX.size(); ++ji)
        if (fplsX[ji]) std::cout << ji << " ";
    std::cout << "\n";

    // 제약별 사용량 검사
    std::cout << "\n[Constraint feasibility check]\n";
    //1. dp 제약 만족하는지 확인
    bool dpFeasible = true;
    int dpWeightSum = 0;
    for (int i=0; i<(int)dpSelected.size(); i++)
        dpWeightSum += inst.weights[dpConstraintIdx][i] * dpSelected[i];
    dpFeasible = (dpWeightSum <= inst.capacities[dpConstraintIdx]);
    std::cout << "DP Constraint " << dpConstraintIdx << ": used " << dpWeightSum
             << " / capacity " << inst.capacities[dpConstraintIdx]
             << (dpFeasible ? " OK" : " *** INFEASIBLE ***") << "\n";

    //2. fpls 제약 만족하는지 확인
    bool fplsFeasible = true;
    for(int j=0; j<m; j++) {
        if (j == dpConstraintIdx)
            continue ;
        int fplsWeightSum = 0;
        for (int i=0; i<(int)fplsX.size(); i++)
            if (fplsX[i]) fplsWeightSum += inst.weights[j][i];

        if (fplsWeightSum > inst.capacities[j]) {
            fplsFeasible = false;
            break ;
        }
        std::cout << "Constraint " << j << ": used " << fplsWeightSum
             << " / capacity " << inst.capacities[j]
             << (fplsWeightSum > inst.capacities[j] ? " *** INFEASIBLE ***" : " OK") << "\n";
    }

    bool allFeasible = dpFeasible && fplsFeasible;
    std::cout << "\nOverall feasible: " << (allFeasible ? "YES" : "NO *** BUG ***") << "\n";
    std::cout << "=============================================\n\n";
}