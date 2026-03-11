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

    
    repairPermutation(child);

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

/* ========== Edge Recombination Crossover (ERX) ========== */
inline vector<int> edge_recombination(const vector<int>& A, const vector<int>& B) {
    int n = (int)A.size();
    // adjacency list for each gene
    std::unordered_map<int, std::unordered_set<int>> adj;
    adj.reserve(n);

    auto add_edge = [&](int u, int v){
        if (u == v) return;
        adj[u].insert(v);
        adj[v].insert(u);
    };

    auto build_adj = [&](const vector<int>& P) {
        for (int i = 0; i < n; ++i) {
            int v = P[i];
            int left = P[(i - 1 + n) % n];
            int right = P[(i + 1) % n];
            add_edge(v, left);
            add_edge(v, right);
        }
    };
    build_adj(A);
    build_adj(B);

    vector<int> child;
    child.reserve(n);

    std::uniform_int_distribution<int> idxDis(0, n-1);
    int current = A[idxDis(gen)];  // 시작점은 임의로 A에서 선택
    child.push_back(current);

    for (int step = 1; step < n; ++step) {
        // 현재 노드를 인접 리스트에서 제거
        for (auto& [node, nbrs] : adj) {
            nbrs.erase(current);
        }

        // 후보 이웃들 중에서 인접 리스트 크기가 가장 작은 것 선택
        int next = -1;
        int bestDeg = 1e9;
        if (!adj.count(current)) {
            // 인접 정보가 없으면, 아직 방문 안 한 어떤 노드나 랜덤 선택
            std::vector<int> remaining;
            remaining.reserve(adj.size());
            for (auto& kv : adj) remaining.push_back(kv.first);
            if (remaining.empty()) break;
            std::uniform_int_distribution<int> rDis(0, (int)remaining.size()-1);
            next = remaining[rDis(gen)];
        } else {
            auto& nbrs = adj[current];
            if (nbrs.empty()) {
                std::vector<int> remaining;
                remaining.reserve(adj.size());
                for (auto& kv : adj) remaining.push_back(kv.first);
                if (remaining.empty()) break;
                std::uniform_int_distribution<int> rDis(0, (int)remaining.size()-1);
                next = remaining[rDis(gen)];
            } else {
                for (int v : nbrs) {
                    int d = adj.count(v) ? (int)adj[v].size() : 0;
                    if (d < bestDeg) {
                        bestDeg = d;
                        next = v;
                    }
                }
            }
        }

        adj.erase(current);

        current = next;
        child.push_back(current);
    }

    // 혹시 모자라면 남은 노드들 추가 (방문 안 한 것들)
    if ((int)child.size() < n) {
        std::unordered_set<int> used(child.begin(), child.end());
        for (int v : A) {
            if (!used.count(v)) child.push_back(v);
        }
    }

    repairPermutation(child);

    return child;
}
