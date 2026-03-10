#include <bits/stdc++.h>
#include <filesystem>
using namespace std;

using ll = long long;
namespace fs = std::filesystem;

/* =========================================================
   CONFIG (Windows에서 여기만 바꿔서 사용)
   ========================================================= */

static const int TRIALS = 30;

static const int POP = 2000;
static const int GEN = 5000;
static const int ELITE = 2;
static const double PX = 0.9;
static const double PM = 0.4;
static const int MUT_K = 2;
static const int MUT_K_STAG = 10;
static const int IMMIGRANTS = 20;
static const int STAG_LIMIT = 200;

static const int PRINT_EVERY = 50;

static const int MAX_N = 600;
static const size_t MAX_EDGES = 12000;

static const bool QAP_EVAL_DENSE = true;
static const int DENSE_N_HARD_LIMIT = 4000;

/* ================= RNG ================= */

static uint32_t g_seed = (uint32_t)chrono::steady_clock::now().time_since_epoch().count();
static mt19937 gen(g_seed);

static inline int rndInt(int lo, int hi){
    uniform_int_distribution<int> dis(lo, hi);
    return dis(gen);
}
static inline double rndReal(){
    uniform_real_distribution<double> dis(0.0, 1.0);
    return dis(gen);
}

/* ================= Utils ================= */

static inline string trim(const string& s){
    int i=0, j=(int)s.size()-1;
    while(i<(int)s.size() && isspace((unsigned char)s[i])) i++;
    while(j>=0 && isspace((unsigned char)s[j])) j--;
    if(i>j) return "";
    return s.substr(i, j-i+1);
}

static inline bool parseCoordToken(string tok, double& x, double& y){
    if(!tok.empty() && tok.front()=='(') tok.erase(tok.begin());
    if(!tok.empty() && tok.back()==')') tok.pop_back();
    for(char& c: tok) if(c==',') c=' ';
    istringstream iss(tok);
    return (iss >> x >> y) ? true : false;
}

/* ================= Instances ================= */

struct GPPInstance {
    int n = 0;
    vector<pair<double,double>> coord;
    vector<pair<int,int>> edges;           // undirected unique edges (u<v), 0-indexed
} gpp;

struct QAPDerived {
    int n = 0;
    int k = 0;                             // n/2
    vector<uint8_t> F;                     // dense adjacency (optional)
    vector<pair<int,int>> arcs;            // store each undirected edge ONCE (u<v)
    bool hasDenseF = false;
} qap;

/* ================= GPPLIB Reader (with limits) ================= */

bool readGPPLIB_limited(const string& filename, int maxN, size_t maxEdges, string& reason){
    ifstream in(filename);
    if(!in){
        reason = "open_fail";
        return false;
    }

    unordered_set<uint64_t> edgeSet;
    edgeSet.reserve(1<<16);

    int maxId = 0;
    vector<pair<double,double>> coordTmp(1);

    string line;
    while(getline(in, line)){
        line = trim(line);
        if(line.empty()) continue;
        if(!isdigit((unsigned char)line[0])) continue;

        istringstream iss(line);
        int u, deg;
        string coordTok;
        if(!(iss >> u >> coordTok >> deg)){
            reason = "parse_fail";
            return false;
        }
        maxId = max(maxId, u);

        if(maxN > 0 && maxId > maxN){
            reason = "too_large_n";
            return false;
        }

        if((int)coordTmp.size() <= u) coordTmp.resize(u+1, {0.0,0.0});
        double x=0.0, y=0.0;
        (void)parseCoordToken(coordTok, x, y);
        coordTmp[u] = {x,y};

        for(int t=0;t<deg;t++){
            int v;
            if(!(iss >> v)){
                reason = "parse_fail";
                return false;
            }
            maxId = max(maxId, v);

            if(maxN > 0 && maxId > maxN){
                reason = "too_large_n";
                return false;
            }

            if(u == v) continue;
            int a = min(u,v), b = max(u,v);
            uint64_t key = (uint64_t(a) << 32) | uint32_t(b);
            edgeSet.insert(key);

            if(maxEdges > 0 && edgeSet.size() > maxEdges){
                reason = "too_large_edges";
                return false;
            }
        }
    }

    int n = maxId;
    if(n <= 0){
        reason = "empty_or_bad";
        return false;
    }
    if(maxN > 0 && n > maxN){
        reason = "too_large_n";
        return false;
    }

    gpp.n = n;
    gpp.coord.assign(n, {0.0,0.0});
    for(int i=1;i<=n && i<(int)coordTmp.size();i++) gpp.coord[i-1] = coordTmp[i];

    gpp.edges.clear();
    gpp.edges.reserve(edgeSet.size());
    for(uint64_t key : edgeSet){
        int a = int(key >> 32);
        int b = int(uint32_t(key));
        if(a>=1 && b>=1 && a<=n && b<=n && a!=b){
            int uu = a-1, vv = b-1;
            if(uu < vv) gpp.edges.push_back({uu,vv});
            else gpp.edges.push_back({vv,uu});
        }
    }
    sort(gpp.edges.begin(), gpp.edges.end());
    gpp.edges.erase(unique(gpp.edges.begin(), gpp.edges.end()), gpp.edges.end());

    if(maxEdges > 0 && gpp.edges.size() > maxEdges){
        reason = "too_large_edges";
        return false;
    }

    reason = "ok";
    return true;
}

/* ================= Build QAP from GPP (bisection) ================= */

bool buildQAP_from_GPP_bisection(bool buildDenseF){
    int n = gpp.n;
    if(n <= 0) return false;
    if(n % 2 != 0) return false;

    qap.n = n;
    qap.k = n/2;

    // 핵심 수정: undirected edge를 한 번만 저장 (u<v)
    qap.arcs = gpp.edges; // already stored as u<v

    qap.hasDenseF = buildDenseF;
    qap.F.clear();

    if(buildDenseF){
        qap.F.assign((size_t)n*n, 0);
        for(auto [u,v] : gpp.edges){
            qap.F[(size_t)u*n + v] = 1;
        }
    }
    return true;
}

/* ================= Cost (same encoding) ================= */

static inline ll costGPP_fromPerm(const vector<int>& p){
    int k = gpp.n/2;
    ll cut = 0;
    for(auto [u,v] : gpp.edges){
        cut += ((p[u] < k) ^ (p[v] < k));
    }
    return cut;
}

// 핵심 수정: arcs가 이제 u<v 한 번만 있으므로 결과가 cut-edge 수와 같은 스케일
static inline ll costQAP_sparse(const vector<int>& p){
    int k = qap.k;
    ll s = 0;
    for(auto [u,v] : qap.arcs){
        s += ((p[u] < k) ^ (p[v] < k));
    }
    return s;
}

// dense를 쓰면 (대칭 F에 대해) 이중합에서 2배가 다시 생길 수 있음.
// (지금 설정은 QAP_EVAL_DENSE=false라 영향 없음)
static inline ll costQAP_dense(const vector<int>& p){
    int n = qap.n;
    int k = qap.k;
    const auto& F = qap.F;
    ll s = 0;
    for(int i=0;i<n;i++){
        int piPart = (p[i] < k);
        const uint8_t* row = &F[(size_t)i*n];
        for(int j=0;j<n;j++){
            if(row[j]) s += (piPart ^ (p[j] < k));
        }
    }
    return s;
}

/* ================= GA (common, identical ops) ================= */

struct Solution {
    vector<int> p;
    ll cost = (1LL<<62);
};

static inline Solution pmxSwapRepair(const Solution& A, const Solution& B){
    int n = (int)A.p.size();
    int l = rndInt(0, n-1);
    int r = rndInt(0, n-1);
    if(l > r) swap(l, r);

    Solution C;
    C.p = B.p;

    vector<int> pos(n);
    for(int i=0;i<n;i++) pos[C.p[i]] = i;

    for(int i=l;i<=r;i++){
        int want = A.p[i];
        int cur  = C.p[i];
        if(want == cur) continue;

        int j = pos[want];
        swap(C.p[i], C.p[j]);

        pos[cur]  = j;
        pos[want] = i;
    }
    return C;
}

static inline void mutateSwapK(Solution& s, int k){
    int n = (int)s.p.size();
    for(int t=0;t<k;t++){
        int i = rndInt(0, n-1);
        int j = rndInt(0, n-1);
        swap(s.p[i], s.p[j]);
    }
}

static inline vector<int> selectRankSUS_sorted(const vector<Solution>& popSorted, int need){
    int m = (int)popSorted.size();
    vector<ll> prefix(m);
    for(int i=0;i<m;i++){
        ll w = (ll)(m - i);
        prefix[i] = w + (i?prefix[i-1]:0);
    }
    ll sumW = prefix.back();

    double step = (double)sumW / need;
    uniform_real_distribution<double> dis(0.0, step);
    double start = dis(gen);

    vector<int> picks;
    picks.reserve(need);

    int idx = 0;
    for(int t=0;t<need;t++){
        double target = start + step * t;
        while(idx < m-1 && prefix[idx] < target) idx++;
        picks.push_back(idx);
    }
    shuffle(picks.begin(), picks.end(), gen);
    return picks;
}

struct GAParams {
    int POP = 1000;
    int GEN = 5000;
    int ELITE = 2;
    double PX = 0.9;
    double PM = 0.4;
    int MUT_K = 2;
    int MUT_K_STAG = 10;
    int IMMIGRANTS = 20;
    int STAG_LIMIT = 200;
    int PRINT_EVERY = 50;
};

using CostFn = function<ll(const vector<int>&)>;

static inline Solution runGA_common(
    const string& tag,
    ofstream& genlog,
    const GAParams& P,
    int n,
    const CostFn& costFn
){
    vector<Solution> pop(P.POP);

    for(auto& s: pop){
        s.p.resize(n);
        iota(s.p.begin(), s.p.end(), 0);
        shuffle(s.p.begin(), s.p.end(), gen);
        s.cost = costFn(s.p);
    }

    Solution best = pop[0];
    int stagnant = 0;
    auto start = chrono::steady_clock::now();

    for(int g=0; g<P.GEN; g++){
        sort(pop.begin(), pop.end(), [](const Solution& a, const Solution& b){ return a.cost < b.cost; });

        if(pop[0].cost < best.cost){
            best = pop[0];
            stagnant = 0;
        } else stagnant++;

        auto now = chrono::steady_clock::now();
        ll tms = chrono::duration_cast<chrono::milliseconds>(now-start).count();
        genlog << tag << "," << g << "," << best.cost << "," << tms << "\n";

        if(P.PRINT_EVERY > 0 && (g % P.PRINT_EVERY == 0)){
            cout << "[" << tag << "] Gen " << g
                 << " Best=" << best.cost
                 << " stagn=" << stagnant
                 << " timeMS=" << tms
                 << "\n";
        }

        vector<Solution> next;
        next.reserve(P.POP);

        for(int i=0;i<P.ELITE && (int)next.size()<P.POP;i++)
            next.push_back(pop[i]);

        for(int k=0;k<P.IMMIGRANTS && (int)next.size()<P.POP;k++){
            Solution s;
            s.p.resize(n);
            iota(s.p.begin(), s.p.end(), 0);
            shuffle(s.p.begin(), s.p.end(), gen);
            s.cost = costFn(s.p);
            next.push_back(std::move(s));
        }

        auto picks = selectRankSUS_sorted(pop, P.POP);
        int ptr = 0;

        bool stag = (stagnant >= P.STAG_LIMIT);
        int mutK = stag ? P.MUT_K_STAG : P.MUT_K;

        while((int)next.size() < P.POP){
            int ra = picks[ptr++ % (int)picks.size()];
            int rb = picks[ptr++ % (int)picks.size()];
            if(ra == rb) rb = picks[ptr++ % (int)picks.size()];

            const Solution& A = pop[ra];
            const Solution& B = pop[rb];

            Solution child;
            if(rndReal() < P.PX) child = pmxSwapRepair(A, B);
            else child = A;

            if(rndReal() < P.PM) mutateSwapK(child, mutK);

            child.cost = costFn(child.p);
            next.push_back(std::move(child));
        }

        pop.swap(next);
    }

    cout << "[" << tag << "] Done. Best=" << best.cost << "\n";
    return best;
}

/* ================= Files ================= */

static inline vector<fs::path> list_gpp_files(const fs::path& dir){
    vector<fs::path> files;
    if(!fs::exists(dir)) return files;
    for(const auto& e : fs::directory_iterator(dir)){
        if(!e.is_regular_file()) continue;
        auto name = e.path().filename().string();
        if(name == "README") continue;
        if(!name.empty() && name[0]=='.') continue;
        files.push_back(e.path());
    }
    sort(files.begin(), files.end());
    return files;
}

/* ================= Main ================= */

int main(){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    fs::create_directories("./results");
    ofstream skiplog("./results/skip_list.txt", ios::out | ios::trunc);

    ofstream allcsv("./results/summary_all.csv", ios::out | ios::trunc);
    allcsv << "Instance,n,Trials,"
           << "GPP_mean,GPP_std,GPP_best,GPP_worst,GPP_timeMS_mean,GPP_timeMS_std,GPP_timeMS_best,GPP_timeMS_worst,"
           << "QAP_mean,QAP_std,QAP_best,QAP_worst,QAP_timeMS_mean,QAP_timeMS_std,QAP_timeMS_best,QAP_timeMS_worst\n";

    vector<fs::path> files = list_gpp_files("./GPPLIB/");
    if(files.empty()){
        cerr << "No files in ./GPPLIB/\n";
        return 0;
    }

    GAParams P;
    P.POP = POP; P.GEN = GEN; P.ELITE = ELITE; P.PX = PX; P.PM = PM;
    P.MUT_K = MUT_K; P.MUT_K_STAG = MUT_K_STAG;
    P.IMMIGRANTS = IMMIGRANTS; P.STAG_LIMIT = STAG_LIMIT;
    P.PRINT_EVERY = PRINT_EVERY;

    cout << "Seed=" << g_seed
         << " Trials=" << TRIALS
         << " POP=" << P.POP
         << " GEN=" << P.GEN
         << " max_n=" << MAX_N
         << " max_edges=" << MAX_EDGES
         << " QAP_eval=" << (QAP_EVAL_DENSE ? "dense" : "sparse")
         << " print_every=" << P.PRINT_EVERY
         << "\n";

    for(const auto& path : files){
        string inst = path.filename().string();
        string full = path.string();

        cout << "\n===== Instance: " << inst << " =====\n";

        string reason;
        if(!readGPPLIB_limited(full, MAX_N, MAX_EDGES, reason)){
            skiplog << inst << "\t" << full << "\t" << reason << "\n";
            cout << "Skip (" << reason << ")\n";
            continue;
        }

        cout << "n=" << gpp.n << " edges=" << gpp.edges.size() << "\n";

        if(gpp.n % 2 != 0){
            skiplog << inst << "\t" << full << "\todd_n\n";
            cout << "Skip (odd_n)\n";
            continue;
        }

        if(QAP_EVAL_DENSE && gpp.n > DENSE_N_HARD_LIMIT){
            skiplog << inst << "\t" << full << "\ttoo_large_for_dense_qap\n";
            cout << "Skip (too_large_for_dense_qap)\n";
            continue;
        }

        if(!buildQAP_from_GPP_bisection(QAP_EVAL_DENSE)){
            skiplog << inst << "\t" << full << "\tqap_build_fail\n";
            cout << "Skip (qap_build_fail)\n";
            continue;
        }

        fs::path instPath = fs::path("./results") / ("summary_" + inst + ".csv");
        ofstream instcsv(instPath.string(), ios::out | ios::trunc);
        instcsv << "Instance,n,Trial,"
                << "GPP_cost,GPP_timeMS,"
                << "QAP_cost,QAP_timeMS\n";

        fs::path genlogPath = fs::path("./results") / ("_genlog_" + inst + ".csv");
        ofstream genlog(genlogPath.string(), ios::out | ios::trunc);
        genlog << "Problem,Generation,BestCost,TimeMS\n";

        long double gppSum=0, gppSq=0, qapSum=0, qapSq=0;
        long double tGppSum=0, tGppSq=0, tQapSum=0, tQapSq=0;
        ll gppBest=(1LL<<62), gppWorst=-1, qapBest=(1LL<<62), qapWorst=-1;
        ll tGppBest=(1LL<<62), tGppWorst=-1, tQapBest=(1LL<<62), tQapWorst=-1;

        CostFn qapCostFn = QAP_EVAL_DENSE
            ? CostFn([](const vector<int>& p){ return costQAP_dense(p); })
            : CostFn([](const vector<int>& p){ return costQAP_sparse(p); });

        for(int ti=1; ti<=TRIALS; ti++){
            cout << "\n--- Trial " << ti << "/" << TRIALS << " (GPP) ---\n";
            auto t0 = chrono::steady_clock::now();
            Solution bestGPP = runGA_common("GPP", genlog, P, gpp.n,
                                           [](const vector<int>& p){ return costGPP_fromPerm(p); });
            auto t1 = chrono::steady_clock::now();
            ll tmsGPP = chrono::duration_cast<chrono::milliseconds>(t1-t0).count();
            cout << "[GPP] Trial done. Best=" << bestGPP.cost << " timeMS=" << tmsGPP << "\n";

            cout << "\n--- Trial " << ti << "/" << TRIALS << " (QAP) ---\n";
            auto t2 = chrono::steady_clock::now();
            Solution bestQAP = runGA_common("QAP", genlog, P, qap.n, qapCostFn);
            auto t3 = chrono::steady_clock::now();
            ll tmsQAP = chrono::duration_cast<chrono::milliseconds>(t3-t2).count();
            cout << "[QAP] Trial done. Best=" << bestQAP.cost << " timeMS=" << tmsQAP << "\n";

            instcsv << inst << "," << gpp.n << "," << ti << ","
                    << bestGPP.cost << "," << tmsGPP << ","
                    << bestQAP.cost << "," << tmsQAP << "\n";

            gppSum += bestGPP.cost; gppSq += (long double)bestGPP.cost * bestGPP.cost;
            qapSum += bestQAP.cost; qapSq += (long double)bestQAP.cost * bestQAP.cost;
            tGppSum += tmsGPP;      tGppSq += (long double)tmsGPP * tmsGPP;
            tQapSum += tmsQAP;      tQapSq += (long double)tmsQAP * tmsQAP;

            gppBest = min(gppBest, bestGPP.cost);
            gppWorst = max(gppWorst, bestGPP.cost);
            qapBest = min(qapBest, bestQAP.cost);
            qapWorst = max(qapWorst, bestQAP.cost);

            tGppBest = min(tGppBest, tmsGPP);
            tGppWorst = max(tGppWorst, tmsGPP);
            tQapBest = min(tQapBest, tmsQAP);
            tQapWorst = max(tQapWorst, tmsQAP);
        }

        auto mean = [&](long double s){ return (double)(s / TRIALS); };
        auto stdev = [&](long double s, long double ss){
            long double m = s / TRIALS;
            long double var = (ss / TRIALS) - m*m;
            if(var < 0) var = 0;
            return sqrt((double)var);
        };

        allcsv << inst << "," << gpp.n << "," << TRIALS << ","
               << fixed << setprecision(6)
               << mean(gppSum) << "," << stdev(gppSum, gppSq) << "," << gppBest << "," << gppWorst << ","
               << mean(tGppSum) << "," << stdev(tGppSum, tGppSq) << "," << tGppBest << "," << tGppWorst << ","
               << mean(qapSum) << "," << stdev(qapSum, qapSq) << "," << qapBest << "," << qapWorst << ","
               << mean(tQapSum) << "," << stdev(tQapSum, tQapSq) << "," << tQapBest << "," << tQapWorst
               << "\n";

        cout << "\n===== Instance done: " << inst << " =====\n";
        cout << "GPP best=" << gppBest << " QAP best=" << qapBest << "\n";
    }

    cout << "\nAll done.\n";
    return 0;
}
