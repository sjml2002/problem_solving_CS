// combination_crossover.cpp
#include <algorithm>
#include <vector>
#include <random>
#include <unordered_set>

using std::vector;

extern std::mt19937 gen;  // 기존 코드의 전역 RNG를 그대로 재사용한다고 가정

// 유틸: combination을 정렬 + 중복 제거한 뒤, size == k로 맞추는 repair
static inline void repair_combination(vector<int>& comb, int n, int k) {
    std::sort(comb.begin(), comb.end());
    comb.erase(std::unique(comb.begin(), comb.end()), comb.end());

    if ((int)comb.size() > k) {
        comb.resize(k);
    }

    if ((int)comb.size() < k) {
        std::vector<int> candidates;
        candidates.reserve(n);
        int i = 0;
        for (int v = 0; v < n; ++v) {
            while (i < (int)comb.size() && comb[i] < v) ++i;
            if (i < (int)comb.size() && comb[i] == v) continue;
            candidates.push_back(v);
        }
        std::shuffle(candidates.begin(), candidates.end(), gen);
        int need = k - (int)comb.size();
        for (int i2 = 0; i2 < need && i2 < (int)candidates.size(); ++i2)
            comb.push_back(candidates[i2]);
        std::sort(comb.begin(), comb.end());
    }
}

/* ========== One-point crossover ========== */
// 부모는 정렬된 combination (size=k) 라고 가정
inline vector<int> comb_one_point(const vector<int>& A, const vector<int>& B, int n, int k) {
    std::uniform_int_distribution<int> cutDis(1, k-1);
    int cut = cutDis(gen);

    vector<int> child;
    child.reserve(k);
    for (int i = 0; i < cut; ++i) child.push_back(A[i]);
    for (int i = cut; i < k; ++i) child.push_back(B[i]);

    repair_combination(child, n, k);
    return child;
}

/* ========== Two-point crossover ========== */
inline vector<int> comb_two_point(const vector<int>& A, const vector<int>& B, int n, int k) {
    if (k < 2) return comb_one_point(A, B, n, k);
    std::uniform_int_distribution<int> cutDis(0, k-1);
    int c1 = cutDis(gen), c2 = cutDis(gen);
    if (c1 > c2) std::swap(c1, c2);

    vector<int> child;
    child.reserve(k);
    for (int i = 0; i < c1; ++i) child.push_back(A[i]);
    for (int i = c1; i <= c2; ++i) child.push_back(B[i]);
    for (int i = c2+1; i < k; ++i) child.push_back(A[i]);

    repair_combination(child, n, k);
    return child;
}

/* ========== Three-point crossover ========== */
inline vector<int> comb_three_point(const vector<int>& A, const vector<int>& B, int n, int k) {
    if (k < 3) return comb_two_point(A, B, n, k);
    std::uniform_int_distribution<int> cutDis(0, k-1);
    int c1 = cutDis(gen), c2 = cutDis(gen), c3 = cutDis(gen);
    if (c1 > c2) std::swap(c1, c2);
    if (c2 > c3) std::swap(c2, c3);
    if (c1 > c2) std::swap(c1, c2);

    vector<int> child;
    child.reserve(k);
    for (int i = 0; i < c1; ++i) child.push_back(A[i]);
    for (int i = c1; i <= c2; ++i) child.push_back(B[i]);
    for (int i = c2+1; i <= c3; ++i) child.push_back(A[i]);
    for (int i = c3+1; i < k; ++i) child.push_back(B[i]);

    repair_combination(child, n, k);
    return child;
}

/* ========== Uniform crossover ========== */
inline vector<int> comb_uniform(const vector<int>& A, const vector<int>& B, int n, int k, double pFromA = 0.5) {
    std::bernoulli_distribution pickA(pFromA);
    vector<int> child;
    child.reserve(2*k);

    for (int i = 0; i < k; ++i) {
        if (pickA(gen)) child.push_back(A[i]);
        else            child.push_back(B[i]);
    }

    repair_combination(child, n, k);
    return child;
}
