// permutation_crossover.cpp
#include <algorithm>
#include <vector>
#include <random>
#include <unordered_map>
#include <unordered_set>

using std::vector;

extern std::mt19937 gen;

static inline int rndInt(int lo,int hi){
    std::uniform_int_distribution<int> dis(lo,hi);
    return dis(gen);
}

void repairPermutation(vector<int>& p){

    int n = p.size();

    vector<int> cnt(n,0);

    for(int x:p)
        if(0<=x && x<n)
            cnt[x]++;

    vector<int> missing;

    for(int i=0;i<n;i++)
        if(cnt[i]==0)
            missing.push_back(i);

    int ptr=0;

    for(int i=0;i<n;i++){
        if(cnt[p[i]]>1){
            cnt[p[i]]--;
            p[i] = missing[ptr++];
        }
    }
}


/* ========== PMX ========== */
vector<int> pmx(const vector<int>& p1, const vector<int>& p2){

    int n = p1.size();

    vector<int> child(n, -1);

    int l = rndInt(0, n-2);
    int r = rndInt(l+1, n-1);

    // 1. copy segment
    for(int i=l;i<=r;i++)
        child[i] = p1[i];

    // 2. mapping
    for(int i=l;i<=r;i++){

        int val = p2[i];

        if(find(child.begin()+l, child.begin()+r+1, val) != child.begin()+r+1)
            continue;

        int pos = i;

        while(true){

            int mapped = p1[pos];

            pos = find(p2.begin(), p2.end(), mapped) - p2.begin();

            if(child[pos] == -1){
                child[pos] = val;
                break;
            }
        }
    }

    // 3. fill remaining
    for(int i=0;i<n;i++)
        if(child[i] == -1)
            child[i] = p2[i];

    
    //repairPermutation(child);

    return child;
}

/* ========== OX (Order Crossover) ========== */
inline vector<int> ox(const vector<int>& A, const vector<int>& B) {
    int n = (int)A.size();
    std::uniform_int_distribution<int> cutDis(0, n-1);
    int c1 = cutDis(gen), c2 = cutDis(gen);
    if (c1 > c2) std::swap(c1, c2);

    vector<int> child(n, -1);
    std::unordered_set<int> used;
    used.reserve(c2-c1+1);

    for (int i = c1; i <= c2; ++i) {
        child[i] = A[i];
        used.insert(A[i]);
    }

    int idxB = (c2 + 1) % n;
    int idxC = (c2 + 1) % n;
    while ((int)used.size() < n) {
        int v = B[idxB];
        if (!used.count(v)) {
            child[idxC] = v;
            used.insert(v);
            idxC = (idxC + 1) % n;
        }
        idxB = (idxB + 1) % n;
    }

    repairPermutation(child);

    return child;
}

/* ========== OX2 (Order-based Crossover 2) ========== */
// 한 정의: B에서 선택된 위치의 값 순서를 A의 같은 위치에 반영
inline vector<int> ox2(const vector<int>& A, const vector<int>& B) {
    int n = (int)A.size();
    std::uniform_int_distribution<int> cutDis(1, n); // 선택할 위치 수
    int m = cutDis(gen);

    std::uniform_int_distribution<int> posDis(0, n-1);
    vector<int> positions;
    positions.reserve(m);
    std::unordered_set<int> posSet;
    while ((int)positions.size() < m) {
        int p = posDis(gen);
        if (posSet.insert(p).second) positions.push_back(p);
    }

    // B에서 positions에 해당하는 값들을 순서대로 뽑기
    vector<int> vals;
    vals.reserve(m);
    std::unordered_set<int> posInB(positions.begin(), positions.end());
    for (int i = 0; i < n; ++i) {
        if (posInB.count(i))
            vals.push_back(B[i]);
    }

    vector<int> child = A;
    for (int i = 0; i < m; ++i) {
        int pos = positions[i];
        child[pos] = vals[i];
    }

    repairPermutation(child);

    return child;
}

/* ========== Fast Edge Recombination Crossover (ERX) ========== */

inline vector<int> edge_recombination(const vector<int>& A, const vector<int>& B) {

    int n = (int)A.size();

    vector<vector<int>> adj(n);
    adj.reserve(n);

    auto add_edge = [&](int u, int v) {
        if (u == v) return;
        adj[u].push_back(v);
    };

    auto build_adj = [&](const vector<int>& P) {
        for (int i = 0; i < n; i++) {
            int v = P[i];
            int l = P[(i - 1 + n) % n];
            int r = P[(i + 1) % n];

            add_edge(v, l);
            add_edge(v, r);
        }
    };

    build_adj(A);
    build_adj(B);

    vector<bool> used(n, false);

    vector<int> child;
    child.reserve(n);

    std::uniform_int_distribution<int> dis(0, n - 1);
    int current = A[dis(gen)];

    child.push_back(current);
    used[current] = true;

    for (int step = 1; step < n; step++) {

        // 후보 중에서 아직 사용 안한 것만
        int next = -1;
        int best_deg = 1e9;

        for (int v : adj[current]) {

            if (used[v]) continue;

            int deg = 0;

            for (int x : adj[v])
                if (!used[x])
                    deg++;

            if (deg < best_deg) {
                best_deg = deg;
                next = v;
            }
        }

        // 후보가 없으면 랜덤 선택
        if (next == -1) {
            vector<int> remaining;
            remaining.reserve(n);

            for (int i = 0; i < n; i++)
                if (!used[i])
                    remaining.push_back(i);

            std::uniform_int_distribution<int> rdis(0, (int)remaining.size() - 1);
            next = remaining[rdis(gen)];
        }

        current = next;
        used[current] = true;
        child.push_back(current);
    }

    return child;
}