// combination_crossover.cpp
#include <algorithm>
#include <vector>
#include <random>
#include <unordered_set>

using std::vector;

extern std::mt19937 gen;

static inline void repair_combination(vector<int>& comb, int n, int k)
{
    // 1. 중복 제거
    std::sort(comb.begin(), comb.end());
    comb.erase(std::unique(comb.begin(), comb.end()), comb.end());
    // 2. 너무 많으면 truncate
    if ((int)comb.size() > k)
        comb.resize(k);
    // 3. 현재 사용된 값 표시
    vector<char> used(n, 0);
    for (int v : comb)
        if (0 <= v && v < n)
            used[v] = 1;
    // 4. 부족하면 랜덤으로 채움
    std::uniform_int_distribution<int> dis(0, n - 1);
    while ((int)comb.size() < k) {
        int v = dis(gen);
        if (!used[v]) {
            used[v] = 1;
            comb.push_back(v);
        }
    }
    // 5. 정렬 유지
    std::sort(comb.begin(), comb.end());
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
