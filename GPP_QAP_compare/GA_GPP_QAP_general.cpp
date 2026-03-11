#include <bits/stdc++.h>
#include <filesystem>
#include "combination_crossover.cpp"
#include "permutation_crossover.cpp"

/* ================= Crossover Selection ================= */

// ----- GPP (Combination encoding) -----

//define GPP_CROSS comb_one_point
//#define GPP_CROSS comb_two_point
#define GPP_CROSS comb_three_point
//#define GPP_CROSS(A,B,n,k) comb_uniform(A,B,n,k,0.5)

// ----- QAP (Permutation encoding) -----

//define QAP_CROSS pmx
//#define QAP_CROSS ox
#define QAP_CROSS ox2
//#define QAP_CROSS edge_recombination


using namespace std;

using ll = long long;
namespace fs = std::filesystem;

/* =========================================================
CONFIG
========================================================= */

static const int TRIALS = 5;

static const int POP = 200;
static const int GEN = 200;
static const int ELITE = 2;

static const double PX = 0.9;
static const double PM = 0.4;

static const int IMMIGRANTS = 20;
static const int STAG_LIMIT = 200;

static const int PRINT_EVERY = 50;

static const int MAX_N = 600;
static const size_t MAX_EDGES = 12000;

/* ================= RNG ================= */

static uint32_t g_seed =
(uint32_t)chrono::steady_clock::now().time_since_epoch().count();

mt19937 gen(g_seed);

//static inline int rndInt(int lo,int hi){
//    uniform_int_distribution<int> dis(lo,hi);
//    return dis(gen);
//}

static inline double rndReal(){
    uniform_real_distribution<double> dis(0.0,1.0);
    return dis(gen);
}

/* ================= Utils ================= */

static inline string trim(const string& s){
    int i=0,j=(int)s.size()-1;
    while(i<s.size() && isspace((unsigned char)s[i])) i++;
    while(j>=0 && isspace((unsigned char)s[j])) j--;
    if(i>j) return "";
    return s.substr(i,j-i+1);
}

/* ================= Instances ================= */

struct GPPInstance{
    int n=0;
    vector<pair<int,int>> edges;
} gpp;

struct QAPDerived{
    int n=0;
    int k=0;
    vector<pair<int,int>> arcs;
} qap;

/* ================= Reader ================= */

bool readGPPLIB_limited(
const string& filename,
int maxN,
size_t maxEdges,
string& reason
){

    ifstream in(filename);

    if(!in){
        reason="open_fail";
        return false;
    }

    unordered_set<uint64_t> edgeSet;

    int maxId=0;

    string line;

    while(getline(in,line)){

        line=trim(line);

        if(line.empty()) continue;
        if(!isdigit(line[0])) continue;

        istringstream iss(line);

        int u,deg;
        string coord;

        if(!(iss>>u>>coord>>deg)){
            reason="parse_fail";
            return false;
        }

        maxId=max(maxId,u);

        for(int i=0;i<deg;i++){

            int v;
            iss>>v;

            maxId=max(maxId,v);

            if(u==v) continue;

            int a=min(u,v);
            int b=max(u,v);

            uint64_t key=((uint64_t)a<<32)|b;

            edgeSet.insert(key);

            if(edgeSet.size()>maxEdges){
                reason="too_large_edges";
                return false;
            }
        }
    }

    int n=maxId;

    if(n>maxN){
        reason="too_large_n";
        return false;
    }

    gpp.n=n;

    gpp.edges.clear();

    for(auto key:edgeSet){

        int a=key>>32;
        int b=key&0xffffffff;

        gpp.edges.push_back({a-1,b-1});
    }

    reason="ok";

    return true;
}

/* ================= Build QAP ================= */

bool buildQAP_from_GPP(){

    int n=gpp.n;

    if(n%2) return false;

    qap.n=n;
    qap.k=n/2;

    qap.arcs=gpp.edges;

    return true;
}

/* ================= COST ================= */

ll costGPP_comb(const vector<int>& comb){
    int n=gpp.n;
    vector<uint8_t> inA(n,0);
    for(int v:comb) inA[v]=1;

    ll cut=0;
    for(auto [u,v]:gpp.edges)
        cut+=(inA[u]^inA[v]);

    return cut;
}

ll costQAP(const vector<int>& p){
    int n=qap.n;
    int k=qap.k;

    vector<uint8_t> inA(n,0);
    for(int i=0;i<k;i++)
        inA[p[i]] = 1;

    ll s=0;
    for(auto [u,v]:qap.arcs)
        s += (inA[u] ^ inA[v]);

    return s;
}

/* ================= Solution ================= */

struct SolutionGPP{
    vector<int> comb;
    ll cost;
};

struct SolutionQAP{
    vector<int> p;
    ll cost;
};

/* ================= Mutation ================= */

void mutateCombination(vector<int>& comb,int n,int k){

    static vector<int> mark(MAX_N);
    fill(mark.begin(), mark.begin()+n, 0);

    for(int v:comb) mark[v]=1;

    int idx=rndInt(0,k-1);

    int v;
    do{
        v=rndInt(0,n-1);
    }while(mark[v]);

    comb[idx]=v;
}
void mutatePermutation(vector<int>& p){

    int n=p.size();

    int a=rndInt(0,n-1);
    int b=rndInt(0,n-1);

    swap(p[a],p[b]);
}

/* ================= GA GPP ================= */

SolutionGPP runGA_GPP(ofstream& genlog){

    int n=gpp.n;
    int k=n/2;

    cout<<"[GPP GA start] n="<<n<<" k="<<k<<"\n";

    vector<SolutionGPP> pop(POP);

    for(auto& s:pop){

        vector<int> perm(n);

        iota(perm.begin(),perm.end(),0);

        shuffle(perm.begin(),perm.end(),gen);

        s.comb.assign(perm.begin(),perm.begin()+k);

        s.cost=costGPP_comb(s.comb);
    }

    SolutionGPP best=pop[0];

    for(int g=0;g<GEN;g++){

        sort(pop.begin(),pop.end(), [](const auto& a, const auto& b){return a.cost < b.cost;});


        if(pop[0].cost<best.cost)
            best=pop[0];

        genlog<<"GPP,"<<g<<","<<best.cost<<"\n";

        if(g % PRINT_EVERY == 0){
            cout<<"GPP Gen "<<g<<" best="<<best.cost<<"\n";
        }

        vector<SolutionGPP> next;

        for(int i=0;i<ELITE;i++)
            next.push_back(pop[i]);

        while(next.size()<POP){

            int a=rndInt(0,POP/2);
            int b=rndInt(0,POP/2);

            SolutionGPP child;

            if(rndReal()<PX)
                child.comb = GPP_CROSS(pop[a].comb, pop[b].comb, n, k);
            else
                child.comb=pop[a].comb;

            if(rndReal()<PM)
                mutateCombination(child.comb,n,k);

            child.cost=costGPP_comb(child.comb);

            next.push_back(child);
        }

        pop.swap(next);
    }

    return best;
}

/* ================= GA QAP ================= */

SolutionQAP runGA_QAP(ofstream& genlog){

    int n=qap.n;

    cout<<"[QAP GA start] n="<<n<<"\n";

    vector<SolutionQAP> pop(POP);

    for(auto& s:pop){

        s.p.resize(n);

        iota(s.p.begin(),s.p.end(),0);

        shuffle(s.p.begin(),s.p.end(),gen);

        s.cost=costQAP(s.p);
    }

    SolutionQAP best=pop[0];

    for(int g=0;g<GEN;g++){

        sort(pop.begin(),pop.end(),
        [](auto&a,auto&b){return a.cost<b.cost;});

        if(pop[0].cost<best.cost)
            best=pop[0];

        genlog<<"QAP,"<<g<<","<<best.cost<<"\n";

        if(g % PRINT_EVERY == 0){
            cout<<"QAP Gen "<<g<<" best="<<best.cost<<"\n";
        }

        vector<SolutionQAP> next;

        for(int i=0;i<ELITE;i++)
            next.push_back(pop[i]);

        while(next.size()<POP){

            int a=rndInt(0,POP/2);
            int b=rndInt(0,POP/2);

            SolutionQAP child;

            if(rndReal()<PX)
                child.p = QAP_CROSS(pop[a].p, pop[b].p);
            else
                child.p=pop[a].p;

            

            if(rndReal()<PM)
                mutatePermutation(child.p);

            child.cost=costQAP(child.p);

            next.push_back(child);
        }

        pop.swap(next);
    }

    return best;
}

/* ================= File utils ================= */

vector<fs::path> list_gpp_files(const fs::path& dir){

    vector<fs::path> files;

    for(auto& e:fs::directory_iterator(dir)){

        if(!e.is_regular_file()) continue;

        files.push_back(e.path());
    }

    sort(files.begin(),files.end());

    return files;
}

/* ================= MAIN ================= */

int main(){

//    ios::sync_with_stdio(false);
//    cin.tie(nullptr);

    fs::create_directories("./results");

    ofstream summary("./results/summary_all.csv");

    summary<<"Instance,n,Trial,GPP_cost,QAP_cost\n";

    auto files=list_gpp_files("./GPPLIB/");

    cout<<"Seed="<<g_seed<<"\n";

    for(auto& path:files){

        string inst=path.filename().string();

        cout<<"\n===== "<<inst<<" =====\n";
        cout<<"Reading instance...\n";

        string reason;

        if(!readGPPLIB_limited(path.string(),MAX_N,MAX_EDGES,reason)){
            cout<<"Skip "<<reason<<"\n";
            continue;
        }

        if(!buildQAP_from_GPP()){
            cout<<"Skip build fail\n";
            continue;
        }

        cout<<"Instance loaded: n="<<gpp.n
            <<" edges="<<gpp.edges.size()<<"\n";

        fs::path genpath="./results/_genlog_"+inst+".csv";

        ofstream genlog(genpath);

        genlog<<"Problem,Generation,BestCost\n";

        for(int t=1;t<=TRIALS;t++){

            cout<<"\nTrial "<<t<<"\n";

            auto gppBest=runGA_GPP(genlog);

            auto qapBest=runGA_QAP(genlog);

            summary<<inst<<","<<gpp.n<<","<<t<<","
            <<gppBest.cost<<","<<qapBest.cost<<"\n";

            cout<<"Trial "<<t
                <<" finished | GPP="<<gppBest.cost
                <<" QAP="<<qapBest.cost<<"\n";
        }
    }

    cout<<"\nAll done\n";
}
