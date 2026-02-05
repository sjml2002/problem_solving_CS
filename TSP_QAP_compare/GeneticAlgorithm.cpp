#include <bits/stdc++.h>
#include <filesystem>

using namespace std;
using ll = long long;
namespace fs = std::filesystem;

static mt19937 gen((uint32_t)chrono::steady_clock::now().time_since_epoch().count());

/* ================= Instance ================= */

struct TSPInstance {
    int n = 0;
    vector<pair<double,double>> coord;
    vector<vector<ll>> dist;
} tsp;

struct QAPInstance {
    int n = 0;
    vector<vector<ll>> flow;
    vector<vector<ll>> dist;
} qap;

/* ================= RNG Utils ================= */

static inline int rndInt(int lo, int hi){
    uniform_int_distribution<int> dis(lo, hi);
    return dis(gen);
}
static inline double rndReal(){
    uniform_real_distribution<double> dis(0.0, 1.0);
    return dis(gen);
}

/* ================= Math Utils ================= */

static inline ll euc2d(double x1,double y1,double x2,double y2){
    return (ll)(sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2)) + 0.5);
}

/* ================= TSPLIB Reader (EUC_2D only) ================= */

static inline string trim(const string& s){
    int i=0, j=(int)s.size()-1;
    while(i<(int)s.size() && isspace((unsigned char)s[i])) i++;
    while(j>=0 && isspace((unsigned char)s[j])) j--;
    if(i>j) return "";
    return s.substr(i, j-i+1);
}

static inline bool parseKeyVal(const string& s, string& key, string& val){
    auto pos = s.find(':');
    if(pos == string::npos) return false;
    key = trim(s.substr(0, pos));
    val = trim(s.substr(pos+1));
    return true;
}

bool readTSPLIB(const string& filename){
    ifstream in(filename);
    if(!in) return false;

    string line;
    int dimension = -1;
    string edgeType;
    bool coordSection = false;

    while(getline(in, line)){
        line = trim(line);
        if(line.empty()) continue;

        if(line == "NODE_COORD_SECTION"){
            coordSection = true;
            break;
        }
        if(line == "EOF") break;

        string key, val;
        if(parseKeyVal(line, key, val)){
            if(key == "DIMENSION") dimension = stoi(val);
            else if(key == "EDGE_WEIGHT_TYPE") edgeType = val;
        }
    }

    if(!coordSection) return false;
    if(dimension <= 0) return false;
    if(dimension >= 1000) return false;
    if(edgeType != "EUC_2D") return false;

    tsp.n = dimension;
    tsp.coord.assign(dimension, {0,0});

    int readCount = 0;
    while(readCount < dimension && getline(in, line)){
        line = trim(line);
        if(line.empty()) continue;
        if(line == "EOF") break;

        istringstream iss(line);
        int id; double x,y;
        if(!(iss >> id >> x >> y)) return false;
        if(id < 1 || id > dimension) return false;

        tsp.coord[id-1] = {x,y};
        readCount++;
    }
    if(readCount != dimension) return false;

    tsp.dist.assign(dimension, vector<ll>(dimension, 0));
    for(int i=0;i<dimension;i++){
        for(int j=0;j<dimension;j++){
            tsp.dist[i][j] = euc2d(
                tsp.coord[i].first, tsp.coord[i].second,
                tsp.coord[j].first, tsp.coord[j].second
            );
        }
    }

    return true;
}

/* ================= TSP -> QAP (directed cycle flow) ================= */

void buildQAP_from_TSP_directedCycleFlow(){
    int n = tsp.n;
    qap.n = n;
    qap.dist = tsp.dist;

    qap.flow.assign(n, vector<ll>(n, 0));
    for(int i=0;i<n-1;i++) qap.flow[i][i+1] = 1;
    qap.flow[n-1][0] = 1;
}

/* ================= Cost ================= */

ll computeTSPCost(const vector<int>& tour){
    ll s = 0;
    int n = (int)tour.size();
    for(int i=0;i+1<n;i++) s += tsp.dist[tour[i]][tour[i+1]];
    s += tsp.dist[tour.back()][tour[0]];
    return s;
}

ll computeQAPCost(const vector<int>& perm){
    ll s = 0;
    int n = qap.n;
    for(int i=0;i<n;i++){
        for(int j=0;j<n;j++){
            s += qap.flow[i][j] * qap.dist[perm[i]][perm[j]];
        }
    }
    return s;
}

/* ================= Solution ================= */

struct Solution {
    vector<int> p;
    ll cost = (1LL<<62);
};

/* ================= PMX (pos-swap) ================= */

Solution pmxSwapRepair(const Solution& A, const Solution& B){
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

/* ================= Mutation ================= */

void mutateSwapK(Solution& s, int k){
    int n = (int)s.p.size();
    for(int t=0;t<k;t++){
        int i = rndInt(0, n-1);
        int j = rndInt(0, n-1);
        swap(s.p[i], s.p[j]);
    }
}

/* ================= Rank-based SUS Selection ================= */

vector<int> selectRankSUS(const vector<Solution>& pop, int need){
    int m = (int)pop.size();
    vector<ll> prefix(m);
    for(int k=0;k<m;k++){
        ll w = (ll)(m - k);
        prefix[k] = w + (k?prefix[k-1]:0);
    }
    ll sumW = prefix.back();

    uniform_real_distribution<double> dis(0.0, (double)sumW);
    double start = dis(gen) / need;
    double step  = (double)sumW / need;

    vector<int> picks;
    picks.reserve(need);

    int idx = 0;
    for(int t=0;t<need;t++){
        double target = start + step * t;
        while(idx < m-1 && prefix[idx] < target) idx++;
        picks.push_back(idx);
    }
    return picks;
}

/* ================= GA Runner ================= */

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
};

Solution runGA(bool isQAP, ofstream& report, const GAParams& P){
    vector<Solution> pop(P.POP);

    for(auto& s: pop){
        s.p.resize(tsp.n);
        iota(s.p.begin(), s.p.end(), 0);
        shuffle(s.p.begin(), s.p.end(), gen);
        s.cost = isQAP ? computeQAPCost(s.p) : computeTSPCost(s.p);
    }

    Solution best = pop[0];
    int stagnant = 0;
    auto start = chrono::steady_clock::now();

    for(int g=0; g<P.GEN; g++){
        sort(pop.begin(), pop.end(),
             [](const Solution& a, const Solution& b){ return a.cost < b.cost; });

        if(pop[0].cost < best.cost){
            best = pop[0];
            stagnant = 0;
        } else stagnant++;

        auto now = chrono::steady_clock::now();
        ll tms = chrono::duration_cast<chrono::milliseconds>(now-start).count();

        report << (isQAP?"QAP":"TSP") << ","
               << g << ","
               << best.cost << ","
               << tms << "\n";

        if(g % 50 == 0){
            cout << (isQAP?"[QAP] ":"[TSP] ")
                 << "Gen " << g
                 << " Best=" << best.cost
                 << " stagn=" << stagnant
                 << "\n";
        }

        vector<Solution> next;
        next.reserve(P.POP);

        for(int i=0;i<P.ELITE && (int)next.size()<P.POP;i++)
            next.push_back(pop[i]);

        for(int k=0;k<P.IMMIGRANTS && (int)next.size()<P.POP;k++){
            Solution s;
            s.p.resize(tsp.n);
            iota(s.p.begin(), s.p.end(), 0);
            shuffle(s.p.begin(), s.p.end(), gen);
            s.cost = isQAP ? computeQAPCost(s.p) : computeTSPCost(s.p);
            next.push_back(std::move(s));
        }

        int needParents = P.POP;
        auto parentRanks = selectRankSUS(pop, needParents);

        bool stag = (stagnant >= P.STAG_LIMIT);
        int mutK = stag ? P.MUT_K_STAG : P.MUT_K;

        while((int)next.size() < P.POP){
            int ra = parentRanks[rndInt(0, (int)parentRanks.size()-1)];
            int rb = parentRanks[rndInt(0, (int)parentRanks.size()-1)];
            const Solution& A = pop[ra];
            const Solution& B = pop[rb];

            Solution child;
            if(rndReal() < P.PX) child = pmxSwapRepair(A, B);
            else child = A;

            if(rndReal() < P.PM) mutateSwapK(child, mutK);

            child.cost = isQAP ? computeQAPCost(child.p) : computeTSPCost(child.p);
            next.push_back(std::move(child));
        }

        pop.swap(next);
    }

    return best;
}

/* ================= List .tsp files ================= */

vector<fs::path> list_tsp_files(const fs::path& dir){
    vector<fs::path> files;
    for(const auto& entry : fs::directory_iterator(dir)){
        if(!entry.is_regular_file()) continue;
        if(entry.path().extension() == ".tsp") files.push_back(entry.path());
    }
    sort(files.begin(), files.end());
    return files;
}

/* ================= Main ================= */

int main(){
    fs::create_directories("./results");
    ofstream skiplog("./results/skip_list.txt", ios::out | ios::trunc);

    // summary_all.csv는 "인스턴스당 1줄 요약"으로 유지
    ofstream allcsv("./results/summary_all.csv", ios::out | ios::trunc);
    allcsv << "Instance,n,Trials,"
           << "TSP_mean,TSP_std,TSP_best,TSP_worst,TSP_timeMS_mean,TSP_timeMS_std,TSP_timeMS_best,TSP_timeMS_worst,"
           << "QAP_mean,QAP_std,QAP_best,QAP_worst,QAP_timeMS_mean,QAP_timeMS_std,QAP_timeMS_best,QAP_timeMS_worst\n";

    vector<fs::path> files = list_tsp_files("./TSPLIB/");
    int testcase = 100;

    for(const auto& p : files){
        string inst = p.stem().string();
        string full = p.string();

        cout << "Instance: " << inst << "\n";

        if(!readTSPLIB(full)){
            skiplog << inst << "\t" << full << "\n";
            continue;
        }
        buildQAP_from_TSP_directedCycleFlow();

        // 인스턴스별 CSV: testcase마다 1줄
        fs::path instPath = fs::path("./results") / ("summary_" + inst + ".csv");
        ofstream instcsv(instPath.string(), ios::out | ios::trunc);
        instcsv << "Instance,n,Trial,"
                << "TSP_cost,TSP_timeMS,"
                << "QAP_cost,QAP_timeMS\n";

        // 인스턴스 요약(전체 CSV에 쓸 통계)
        long double tspSum=0, tspSq=0, qapSum=0, qapSq=0, tTspSum=0, tTspSq=0, tQapSum=0, tQapSq=0;
        ll tspBest=(1LL<<62), tspWorst=-1, qapBest=(1LL<<62), qapWorst=-1;
        ll tTspBest=(1LL<<62), tTspWorst=-1, tQapBest=(1LL<<62), tQapWorst=-1;

        // GA 로그(세대별)는 인스턴스당 1개 파일로만(append) 저장
        fs::path genlogPath = fs::path("./results") / ("_genlog_" + inst + ".csv");
        ofstream genlog(genlogPath.string(), ios::out | ios::trunc);
        genlog << "Problem,Generation,BestCost,TimeMS\n";

        GAParams P;
        // 필요하면 여기서 P 조정
        // P.POP = 1000; P.GEN = 5000;
        P.POP = 1000;
        P.GEN = 5000;

        for(int ti=1; ti<=testcase; ti++){
            auto t0 = chrono::steady_clock::now();
            Solution bestTSP = runGA(false, genlog, P);
            auto t1 = chrono::steady_clock::now();
            ll tmsTSP = chrono::duration_cast<chrono::milliseconds>(t1-t0).count();

            auto t2 = chrono::steady_clock::now();
            Solution bestQAP = runGA(true, genlog, P);
            auto t3 = chrono::steady_clock::now();
            ll tmsQAP = chrono::duration_cast<chrono::milliseconds>(t3-t2).count();

            // 인스턴스별: trial별 1줄 기록
            instcsv << inst << "," << tsp.n << "," << ti << ","
                    << bestTSP.cost << "," << tmsTSP << ","
                    << bestQAP.cost << "," << tmsQAP << "\n";

            // 통계 누적
            tspSum += bestTSP.cost; tspSq += (long double)bestTSP.cost * bestTSP.cost;
            qapSum += bestQAP.cost; qapSq += (long double)bestQAP.cost * bestQAP.cost;
            tTspSum += tmsTSP;      tTspSq += (long double)tmsTSP * tmsTSP;
            tQapSum += tmsQAP;      tQapSq += (long double)tmsQAP * tmsQAP;

            tspBest = min(tspBest, bestTSP.cost);
            tspWorst = max(tspWorst, bestTSP.cost);
            qapBest = min(qapBest, bestQAP.cost);
            qapWorst = max(qapWorst, bestQAP.cost);

            tTspBest = min(tTspBest, tmsTSP);
            tTspWorst = max(tTspWorst, tmsTSP);
            tQapBest = min(tQapBest, tmsQAP);
            tQapWorst = max(tQapWorst, tmsQAP);
        }

        auto mean = [&](long double s){ return (double)(s / testcase); };
        auto stdev = [&](long double s, long double ss){
            long double m = s / testcase;
            long double var = (ss / testcase) - m*m;
            if(var < 0) var = 0;
            return sqrt((double)var);
        };

        // 전체 summary_all.csv에 인스턴스당 1줄
        allcsv << inst << "," << tsp.n << "," << testcase << ","
               << fixed << setprecision(6)
               << mean(tspSum) << "," << stdev(tspSum, tspSq) << "," << tspBest << "," << tspWorst << ","
               << mean(tTspSum) << "," << stdev(tTspSum, tTspSq) << "," << tTspBest << "," << tTspWorst << ","
               << mean(qapSum) << "," << stdev(qapSum, qapSq) << "," << qapBest << "," << qapWorst << ","
               << mean(tQapSum) << "," << stdev(tQapSum, tQapSq) << "," << tQapBest << "," << tQapWorst
               << "\n";
    }

    return 0;
}
