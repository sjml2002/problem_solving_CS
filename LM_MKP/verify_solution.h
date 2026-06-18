#pragma once
#include "dataIO.h"
#include <vector>
#include <string>

void verify_and_print(
    const MKPInstance      &inst,
    const std::string      &instanceId,
    const std::vector<int> &dpSelected,   // DP가 선택한 아이템 (크기 n)
    const std::vector<int> &fplsX,        // FPLS x (크기 nFpls)
    const std::vector<int> &fplsItems,    // fplsX 인덱스 → 실제 아이템 인덱스
    int                     dpConstraintIdx,
    double                  lpOpt
);