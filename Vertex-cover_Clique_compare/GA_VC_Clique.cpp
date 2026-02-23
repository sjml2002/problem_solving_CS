// vc_clique_ga_onecsv_plus_global_summary.cpp
// Build: g++ -std=c++17 -O2 vc_clique_ga_onecsv_plus_global_summary.cpp -o run
//
// Run:
//   ./run --graph_dir GRAPHLIB --out_dir results --trials 30 --seed 0 --time_limit_ms 1000
//
// Optional GA params:
//   --pop 60 --elite 2 --tourn 3 --pcross 0.9 --pmut 0.01
//
// Progress params:
//   --progress_step 1        (print every trial; set 5/10 to reduce overhead)
//   --progress_detail 1      (0: minimal, 1: show best-so-far values)
//
// Outputs:
//   results/instance_vc_clique_both.csv
//   results/summary_global_vc_clique_both.csv

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace std;

static inline int64_t now_ms() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

struct Graph {
    int n = 0;
    vector<vector<int>> adj;          // sorted unique
    vector<pair<int,int>> edges;      // u < v

    bool hasEdge(int u, int v) const {
        const auto &A = adj[u];
        return binary_search(A.begin(), A.end(), v);
    }
    int64_t m() const { return (int64_t)edges.size(); }
};

// -------- GRAPHLIB parser: "id (x,y) deg nb1 nb2 ..." ----------
static Graph read_graphlib_instance(const string &path) {
    ifstream in(path);
    if (!in) throw runtime_error("Cannot open file: " + path);

    vector<pair<int,int>> rawEdges;
    rawEdges.reserve(1 << 20);

    int maxLabel = 0;

    while (true) {
        int vid;
        if (!(in >> vid)) break;

        string coordTok;
        if (!(in >> coordTok)) break;

        int deg;
        if (!(in >> deg)) break;

        maxLabel = max(maxLabel, vid);

        for (int i = 0; i < deg; i++) {
            int nb;
            if (!(in >> nb)) throw runtime_error("Unexpected EOF while reading neighbors: " + path);
            maxLabel = max(maxLabel, nb);

            if (vid == nb) continue;
            int u = vid - 1, v = nb - 1;
            if (u < 0 || v < 0) continue;
            if (u > v) std::swap(u, v);
            rawEdges.push_back({u, v});
        }
    }

    Graph G;
    G.n = maxLabel;
    G.adj.assign(G.n, {});

    for (auto &e : rawEdges) {
        int u = e.first, v = e.second;
        if (u == v) continue;
        if (u < 0 || v < 0 || u >= G.n || v >= G.n) continue;
        G.adj[u].push_back(v);
        G.adj[v].push_back(u);
    }
    for (int u = 0; u < G.n; u++) {
        auto &A = G.adj[u];
        sort(A.begin(), A.end());
        A.erase(unique(A.begin(), A.end()), A.end());
    }

    G.edges.clear();
    for (int u = 0; u < G.n; u++) {
        for (int v : G.adj[u]) if (u < v) G.edges.push_back({u, v});
    }
    return G;
}

// -------- checks ----------
static bool is_vertex_cover_on_base(const Graph &G, const vector<uint8_t> &inCover) {
    for (auto &e : G.edges) {
        if (!inCover[e.first] && !inCover[e.second]) return false;
    }
    return true;
}

// -------- greedy decoders used as GA repair ----------
static vector<int> greedy_clique_from_candidates_base(const Graph &G, const vector<int> &cand, std::mt19937 &rng) {
    if (cand.empty()) return {};

    vector<int> curCand = cand;
    vector<int> clique;
    clique.reserve(cand.size());

    vector<uint8_t> inCur(G.n, 0);

    while (!curCand.empty()) {
        std::uniform_int_distribution<int> dist(0, (int)curCand.size() - 1);
        int v = curCand[dist(rng)];
        clique.push_back(v);

        std::fill(inCur.begin(), inCur.end(), 0);
        for (int u : curCand) inCur[u] = 1;

        vector<int> next;
        next.reserve(curCand.size());
        for (int nb : G.adj[v]) {
            if (inCur[nb]) next.push_back(nb);
        }
        curCand.swap(next);
    }
    return clique;
}

// clique in complement(G) == independent set in base(G)
static vector<int> greedy_clique_from_candidates_complement(const Graph &G, const vector<int> &cand, std::mt19937 &rng) {
    vector<int> order = cand;
    std::shuffle(order.begin(), order.end(), rng);

    vector<uint8_t> forbidden(G.n, 0);
    vector<int> indep;
    indep.reserve(order.size());

    for (int v : order) {
        if (forbidden[v]) continue;
        indep.push_back(v);
        for (int nb : G.adj[v]) forbidden[nb] = 1;
    }
    return indep;
}

// -------- GA core ----------
struct GAParams {
    int pop = 60;
    int elite = 2;
    int tourn = 3;
    double pcross = 0.9;
    double pmut = 0.01; // per-bit flip probability
};

struct GAOutcome {
    vector<uint8_t> bestChrom;
    vector<int> bestSet;       // for clique GA
    vector<uint8_t> bestCover; // for cover GA
    int bestObj = 0;           // clique size (maximize) or cover size (minimize)
    int64_t time_ms = 0;
};

static int tournament_select(const vector<double> &fit, std::mt19937 &rng, int k) {
    std::uniform_int_distribution<int> dist(0, (int)fit.size() - 1);
    int best = dist(rng);
    for (int i = 1; i < k; i++) {
        int j = dist(rng);
        if (fit[j] > fit[best]) best = j;
    }
    return best;
}

static void uniform_crossover(const vector<uint8_t> &p1, const vector<uint8_t> &p2,
                              vector<uint8_t> &c1, vector<uint8_t> &c2, std::mt19937 &rng) {
    std::bernoulli_distribution coin(0.5);
    int n = (int)p1.size();
    c1.resize(n); c2.resize(n);
    for (int i = 0; i < n; i++) {
        if (coin(rng)) { c1[i] = p1[i]; c2[i] = p2[i]; }
        else           { c1[i] = p2[i]; c2[i] = p1[i]; }
    }
}

static void bitflip_mutation(vector<uint8_t> &x, std::mt19937 &rng, double pmut) {
    std::bernoulli_distribution flip(pmut);
    for (auto &b : x) if (flip(rng)) b = (uint8_t)(1 - b);
}

// -------- GA: MAX-CLIQUE on base or complement ----------
static GAOutcome ga_max_clique(const Graph &G, bool onComplement, uint32_t seed,
                               int64_t time_limit_ms, const GAParams &P) {
    int64_t t0 = now_ms();
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> ur(0.0, 1.0);

    int n = G.n;

    vector<vector<uint8_t>> pop(P.pop, vector<uint8_t>(n, 0));
    for (int i = 0; i < P.pop; i++) {
        for (int v = 0; v < n; v++) pop[i][v] = (ur(rng) < 0.5) ? 1 : 0;
    }

    vector<double> fit(P.pop, -1e18);

    vector<int> bestClique;
    vector<uint8_t> bestChrom;
    int bestSize = -1;

    auto eval = [&](const vector<uint8_t> &chrom, vector<int> &decodedClique) -> double {
        vector<int> cand;
        cand.reserve(n);
        for (int v = 0; v < n; v++) if (chrom[v]) cand.push_back(v);

        if (!onComplement) decodedClique = greedy_clique_from_candidates_base(G, cand, rng);
        else decodedClique = greedy_clique_from_candidates_complement(G, cand, rng);

        return (double)decodedClique.size();
    };

    for (int i = 0; i < P.pop; i++) {
        vector<int> dec;
        fit[i] = eval(pop[i], dec);
        if ((int)dec.size() > bestSize) {
            bestSize = (int)dec.size();
            bestClique = dec;
            bestChrom = pop[i];
        }
    }

    vector<vector<uint8_t>> nextPop;
    nextPop.reserve(P.pop);

    while (now_ms() - t0 < time_limit_ms) {
        vector<int> idx(P.pop);
        for (int i = 0; i < P.pop; i++) idx[i] = i;
        sort(idx.begin(), idx.end(), [&](int a, int b){ return fit[a] > fit[b]; });

        nextPop.clear();
        for (int e = 0; e < P.elite && e < P.pop; e++) nextPop.push_back(pop[idx[e]]);

        std::bernoulli_distribution doCross(P.pcross);
        while ((int)nextPop.size() < P.pop) {
            int a = tournament_select(fit, rng, P.tourn);
            int b = tournament_select(fit, rng, P.tourn);

            vector<uint8_t> c1, c2;
            if (doCross(rng)) uniform_crossover(pop[a], pop[b], c1, c2, rng);
            else { c1 = pop[a]; c2 = pop[b]; }

            bitflip_mutation(c1, rng, P.pmut);
            bitflip_mutation(c2, rng, P.pmut);

            nextPop.push_back(std::move(c1));
            if ((int)nextPop.size() < P.pop) nextPop.push_back(std::move(c2));
        }

        pop.swap(nextPop);

        for (int i = 0; i < P.pop; i++) {
            vector<int> dec;
            fit[i] = eval(pop[i], dec);
            if ((int)dec.size() > bestSize) {
                bestSize = (int)dec.size();
                bestClique = dec;
                bestChrom = pop[i];
            }
        }
    }

    GAOutcome out;
    out.bestChrom = std::move(bestChrom);
    out.bestSet = std::move(bestClique);
    out.bestObj = bestSize;
    out.time_ms = now_ms() - t0;
    return out;
}

// -------- GA: MIN-VERTEX-COVER on base or complement (implicit) ----------
// onComplement=false: repair to cover all base edges
// onComplement=true : cover in complement <-> V\C is clique in base; repair by extracting clique from V\C and rebuilding C
static GAOutcome ga_min_vertex_cover(const Graph &G, bool onComplement, uint32_t seed,
                                     int64_t time_limit_ms, const GAParams &P) {
    int64_t t0 = now_ms();
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> ur(0.0, 1.0);

    int n = G.n;

    vector<vector<uint8_t>> pop(P.pop, vector<uint8_t>(n, 0));
    for (int i = 0; i < P.pop; i++) {
        for (int v = 0; v < n; v++) pop[i][v] = (ur(rng) < 0.5) ? 1 : 0;
    }

    vector<double> fit(P.pop, -1e18);

    vector<uint8_t> bestCover;
    vector<uint8_t> bestChrom;
    int bestSize = 1e9;

    auto repair_base_cover = [&](vector<uint8_t> C) -> vector<uint8_t> {
        for (auto &e : G.edges) {
            int u = e.first, v = e.second;
            if (!C[u] && !C[v]) {
                if (G.adj[u].size() >= G.adj[v].size()) C[u] = 1;
                else C[v] = 1;
            }
        }
        vector<uint8_t> C2 = C;
        for (int v = 0; v < n; v++) {
            if (!C2[v]) continue;
            C2[v] = 0;
            if (!is_vertex_cover_on_base(G, C2)) C2[v] = 1;
        }
        return C2;
    };

    auto repair_complement_cover = [&](vector<uint8_t> C) -> vector<uint8_t> {
        vector<int> U;
        U.reserve(n);
        for (int v = 0; v < n; v++) if (!C[v]) U.push_back(v);

        vector<int> clique = greedy_clique_from_candidates_base(G, U, rng);

        vector<uint8_t> C2(n, 1);
        for (int v : clique) C2[v] = 0;
        return C2;
    };

    auto eval = [&](const vector<uint8_t> &chrom, vector<uint8_t> &decodedCover, int &coverSize) -> double {
        decodedCover = chrom;
        if (!onComplement) decodedCover = repair_base_cover(decodedCover);
        else decodedCover = repair_complement_cover(decodedCover);

        coverSize = 0;
        for (int v = 0; v < n; v++) coverSize += decodedCover[v] ? 1 : 0;
        return -(double)coverSize; // maximize negative size
    };

    for (int i = 0; i < P.pop; i++) {
        vector<uint8_t> dec;
        int sz = 0;
        fit[i] = eval(pop[i], dec, sz);
        if (sz < bestSize) {
            bestSize = sz;
            bestCover = dec;
            bestChrom = pop[i];
        }
    }

    vector<vector<uint8_t>> nextPop;
    nextPop.reserve(P.pop);

    while (now_ms() - t0 < time_limit_ms) {
        vector<int> idx(P.pop);
        for (int i = 0; i < P.pop; i++) idx[i] = i;
        sort(idx.begin(), idx.end(), [&](int a, int b){ return fit[a] > fit[b]; });

        nextPop.clear();
        for (int e = 0; e < P.elite && e < P.pop; e++) nextPop.push_back(pop[idx[e]]);

        std::bernoulli_distribution doCross(P.pcross);
        while ((int)nextPop.size() < P.pop) {
            int a = tournament_select(fit, rng, P.tourn);
            int b = tournament_select(fit, rng, P.tourn);

            vector<uint8_t> c1, c2;
            if (doCross(rng)) uniform_crossover(pop[a], pop[b], c1, c2, rng);
            else { c1 = pop[a]; c2 = pop[b]; }

            bitflip_mutation(c1, rng, P.pmut);
            bitflip_mutation(c2, rng, P.pmut);

            nextPop.push_back(std::move(c1));
            if ((int)nextPop.size() < P.pop) nextPop.push_back(std::move(c2));
        }

        pop.swap(nextPop);

        for (int i = 0; i < P.pop; i++) {
            vector<uint8_t> dec;
            int sz = 0;
            fit[i] = eval(pop[i], dec, sz);
            if (sz < bestSize) {
                bestSize = sz;
                bestCover = dec;
                bestChrom = pop[i];
            }
        }
    }

    GAOutcome out;
    out.bestChrom = std::move(bestChrom);
    out.bestCover = std::move(bestCover);
    out.bestObj = bestSize;
    out.time_ms = now_ms() - t0;
    return out;
}

// -------- stats ----------
struct Stat4 { double mean=NAN, stdev=NAN, best=NAN, worst=NAN; };

static Stat4 compute_stats(const vector<double> &vals, bool minimize) {
    Stat4 s;
    if (vals.empty()) return s;

    double sum = 0.0;
    s.best = vals[0];
    s.worst = vals[0];

    for (double x : vals) {
        sum += x;
        if (minimize) { s.best = min(s.best, x); s.worst = max(s.worst, x); }
        else          { s.best = max(s.best, x); s.worst = min(s.worst, x); }
    }
    s.mean = sum / (double)vals.size();

    double var = 0.0;
    for (double x : vals) {
        double d = x - s.mean;
        var += d * d;
    }
    var /= (double)vals.size(); // population std
    s.stdev = sqrt(var);
    return s;
}

// -------- file listing ----------
static vector<string> list_instance_files(const string &dir, int limit) {
    namespace fs = std::filesystem;
    vector<string> files;
    if (!fs::exists(dir)) return files;

    for (auto const& entry : fs::recursive_directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        auto p = entry.path();
        auto name = p.filename().string();
        if (name == "README" || name == "readme" || name == "Readme") continue;
        if (!name.empty() && name[0] == '.') continue;
        files.push_back(p.string());
    }
    sort(files.begin(), files.end());
    if (limit > 0 && (int)files.size() > limit) files.resize(limit);
    return files;
}

// -------- CLI helpers ----------
static bool starts_with(const string &s, const string &p) {
    return s.size() >= p.size() && std::equal(p.begin(), p.end(), s.begin());
}

static string get_arg(int argc, char **argv, const string &key, const string &def) {
    for (int i = 1; i < argc; i++) {
        string a = argv[i];
        if (a == key && i + 1 < argc) return argv[i + 1];
        if (starts_with(a, key + "=")) return a.substr(key.size() + 1);
    }
    return def;
}

static int get_arg_int(int argc, char **argv, const string &key, int def) {
    string s = get_arg(argc, argv, key, "");
    if (s.empty()) return def;
    return stoi(s);
}

static int64_t get_arg_i64(int argc, char **argv, const string &key, int64_t def) {
    string s = get_arg(argc, argv, key, "");
    if (s.empty()) return def;
    return stoll(s);
}

static double get_arg_double(int argc, char **argv, const string &key, double def) {
    string s = get_arg(argc, argv, key, "");
    if (s.empty()) return def;
    return stod(s);
}

// -------- progress printing (same-line update + flush) ----------
static void print_progress_line(const string& inst, int inst_i, int inst_total,
                                int trial_i, int trials, int64_t inst_start_ms,
                                int progress_detail,
                                int best_vc_base, int best_vc_fromClq,
                                int best_clq_base, int best_clq_fromVC) {
    int64_t elapsed = now_ms() - inst_start_ms;
    std::cout << "\r"
              << "(" << inst_i << "/" << inst_total << ") "
              << inst << " | trial " << (trial_i + 1) << "/" << trials
              << " | elapsed " << elapsed << " ms";
    if (progress_detail) {
        std::cout << " | VC base " << best_vc_base
                  << " | VC<-Clq " << best_vc_fromClq
                  << " | Clq base " << best_clq_base
                  << " | Clq<-VC " << best_clq_fromVC;
    }
    std::cout << "          " << std::flush;
}

// -------- CSV headers ----------
static void write_header_instance(ofstream &out) {
    out
    << "Instance,n,m,Trials,"
    // VC direct
    << "VC_mean,VC_std,VC_best,VC_worst,"
    << "VC_timeMS_mean,VC_timeMS_std,VC_timeMS_best,VC_timeMS_worst,"
    // VC from Clique (VC->Clique xform then map back)
    << "VC_fromClique_mean,VC_fromClique_std,VC_fromClique_best,VC_fromClique_worst,"
    << "VC_fromClique_timeMS_mean,VC_fromClique_timeMS_std,VC_fromClique_timeMS_best,VC_fromClique_timeMS_worst,"
    // Clique direct
    << "Clique_mean,Clique_std,Clique_best,Clique_worst,"
    << "Clique_timeMS_mean,Clique_timeMS_std,Clique_timeMS_best,Clique_timeMS_worst,"
    // Clique from VC (Clique->VC xform then map back)
    << "Clique_fromVC_mean,Clique_fromVC_std,Clique_fromVC_best,Clique_fromVC_worst,"
    << "Clique_fromVC_timeMS_mean,Clique_fromVC_timeMS_std,Clique_fromVC_timeMS_best,Clique_fromVC_timeMS_worst\n";
}

static void write_header_summary(ofstream &out) {
    out
    << "TotalSamples,NumInstances,"
    // pooled graph stats across instances (n,m are per-instance, not per-sample)
    << "n_mean,n_std,n_best,n_worst,"
    << "m_mean,m_std,m_best,m_worst,"
    // global pooled (all instance×trial samples)
    << "VC_mean,VC_std,VC_best,VC_worst,"
    << "VC_timeMS_mean,VC_timeMS_std,VC_timeMS_best,VC_timeMS_worst,"
    << "VC_fromClique_mean,VC_fromClique_std,VC_fromClique_best,VC_fromClique_worst,"
    << "VC_fromClique_timeMS_mean,VC_fromClique_timeMS_std,VC_fromClique_timeMS_best,VC_fromClique_timeMS_worst,"
    << "Clique_mean,Clique_std,Clique_best,Clique_worst,"
    << "Clique_timeMS_mean,Clique_timeMS_std,Clique_timeMS_best,Clique_timeMS_worst,"
    << "Clique_fromVC_mean,Clique_fromVC_std,Clique_fromVC_best,Clique_fromVC_worst,"
    << "Clique_fromVC_timeMS_mean,Clique_fromVC_timeMS_std,Clique_fromVC_timeMS_best,Clique_fromVC_timeMS_worst\n";
}

int main(int argc, char **argv) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    string graph_dir = get_arg(argc, argv, "--graph_dir", "GRAPHLIB");
    string out_dir   = get_arg(argc, argv, "--out_dir", "results");

    int trials         = get_arg_int(argc, argv, "--trials", 30);
    uint32_t seed0     = (uint32_t)get_arg_int(argc, argv, "--seed", 0);
    int64_t time_ms    = get_arg_i64(argc, argv, "--time_limit_ms", 1000);
    int limit          = get_arg_int(argc, argv, "--limit", 0);

    int progress_step   = max(1, get_arg_int(argc, argv, "--progress_step", 1));
    int progress_detail = get_arg_int(argc, argv, "--progress_detail", 1);

    GAParams P;
    P.pop    = get_arg_int(argc, argv, "--pop", 60);
    P.elite  = get_arg_int(argc, argv, "--elite", 2);
    P.tourn  = get_arg_int(argc, argv, "--tourn", 3);
    P.pcross = get_arg_double(argc, argv, "--pcross", 0.9);
    P.pmut   = get_arg_double(argc, argv, "--pmut", 0.01);

    auto files = list_instance_files(graph_dir, limit);
    if (files.empty()) {
        cerr << "No instance files found under: " << graph_dir << "\n";
        return 1;
    }

    std::filesystem::create_directories(out_dir);

    string out_instance = (std::filesystem::path(out_dir) / "instance_vc_clique_both.csv").string();
    string out_summary  = (std::filesystem::path(out_dir) / "summary_global_vc_clique_both.csv").string();

    ofstream csv(out_instance);
    write_header_instance(csv);

    // ---- Global pooled samples (B) ----
    vector<double> G_vc_base_obj, G_vc_base_time;
    vector<double> G_vc_fromClq_obj, G_vc_fromClq_time;
    vector<double> G_clq_base_obj, G_clq_base_time;
    vector<double> G_clq_fromVC_obj, G_clq_fromVC_time;

    // per-instance n,m stats (not per sample)
    vector<double> I_n, I_m;

    int inst_total = (int)files.size();
    int inst_i = 0;
    int parsed_instances = 0;

    for (const auto &path : files) {
        inst_i++;
        const string inst = std::filesystem::path(path).filename().string();
        std::cout << "\n=== Start (" << inst_i << "/" << inst_total << ") " << inst << " ===\n" << std::flush;

        int64_t inst_start = now_ms();

        Graph G;
        try {
            G = read_graphlib_instance(path);
        } catch (const std::exception &e) {
            std::cout << "[SKIP] parse error: " << e.what() << "\n" << std::flush;
            continue;
        }

        parsed_instances++;
        I_n.push_back((double)G.n);
        I_m.push_back((double)G.m());

        // per-instance trial samples
        vector<double> vc_base_obj, vc_base_time;
        vector<double> vc_fromClq_obj, vc_fromClq_time;
        vector<double> clq_base_obj, clq_base_time;
        vector<double> clq_fromVC_obj, clq_fromVC_time;

        vc_base_obj.reserve(trials); vc_base_time.reserve(trials);
        vc_fromClq_obj.reserve(trials); vc_fromClq_time.reserve(trials);
        clq_base_obj.reserve(trials); clq_base_time.reserve(trials);
        clq_fromVC_obj.reserve(trials); clq_fromVC_time.reserve(trials);

        int best_vc_base = 1e9, best_vc_fromClq = 1e9;
        int best_clq_base = -1, best_clq_fromVC = -1;

        for (int t = 0; t < trials; t++) {
            if (t % progress_step == 0 || t == trials - 1) {
                print_progress_line(inst, inst_i, inst_total, t, trials, inst_start,
                                    progress_detail, best_vc_base, best_vc_fromClq, best_clq_base, best_clq_fromVC);
            }

            uint32_t seed = seed0 + (uint32_t)t;

            // (1) VC direct on G (minimize cover size)
            auto vc = ga_min_vertex_cover(G, /*onComplement=*/false, seed, time_ms, P);
            vc_base_obj.push_back((double)vc.bestObj);
            vc_base_time.push_back((double)vc.time_ms);
            best_vc_base = min(best_vc_base, vc.bestObj);

            // (1') VC -> Clique transform: solve clique on complement(G), map back VC size = n - |S|
            auto clqComp = ga_max_clique(G, /*onComplement=*/true, seed, time_ms, P);
            int mappedVC = G.n - (int)clqComp.bestSet.size();
            vc_fromClq_obj.push_back((double)mappedVC);
            vc_fromClq_time.push_back((double)clqComp.time_ms);
            best_vc_fromClq = min(best_vc_fromClq, mappedVC);

            // (2) Clique direct on G (maximize clique size)
            auto clq = ga_max_clique(G, /*onComplement=*/false, seed, time_ms, P);
            clq_base_obj.push_back((double)clq.bestObj);
            clq_base_time.push_back((double)clq.time_ms);
            best_clq_base = max(best_clq_base, clq.bestObj);

            // (2') Clique -> VC transform: solve VC on complement(G), map back clique size = n - |C'|
            auto vcComp = ga_min_vertex_cover(G, /*onComplement=*/true, seed, time_ms, P);
            int mappedClique = G.n - vcComp.bestObj;
            clq_fromVC_obj.push_back((double)mappedClique);
            clq_fromVC_time.push_back((double)vcComp.time_ms);
            best_clq_fromVC = max(best_clq_fromVC, mappedClique);

            // Global pooled append (B): append raw trial sample (instance×trial)
            G_vc_base_obj.push_back((double)vc.bestObj);
            G_vc_base_time.push_back((double)vc.time_ms);
            G_vc_fromClq_obj.push_back((double)mappedVC);
            G_vc_fromClq_time.push_back((double)clqComp.time_ms);

            G_clq_base_obj.push_back((double)clq.bestObj);
            G_clq_base_time.push_back((double)clq.time_ms);
            G_clq_fromVC_obj.push_back((double)mappedClique);
            G_clq_fromVC_time.push_back((double)vcComp.time_ms);
        }

        std::cout << "\n=== Done " << inst << " (n=" << G.n << ", m=" << G.m() << ") ===\n" << std::flush;

        // per-instance stats across trials
        Stat4 s_vc_base      = compute_stats(vc_base_obj, true);
        Stat4 s_vc_base_t    = compute_stats(vc_base_time, true);
        Stat4 s_vc_fromClq   = compute_stats(vc_fromClq_obj, true);
        Stat4 s_vc_fromClq_t = compute_stats(vc_fromClq_time, true);

        Stat4 s_clq_base      = compute_stats(clq_base_obj, false);
        Stat4 s_clq_base_t    = compute_stats(clq_base_time, true);
        Stat4 s_clq_fromVC    = compute_stats(clq_fromVC_obj, false);
        Stat4 s_clq_fromVC_t  = compute_stats(clq_fromVC_time, true);

        csv << inst << "," << G.n << "," << G.m() << "," << trials << ","
            << s_vc_base.mean << "," << s_vc_base.stdev << "," << s_vc_base.best << "," << s_vc_base.worst << ","
            << s_vc_base_t.mean << "," << s_vc_base_t.stdev << "," << s_vc_base_t.best << "," << s_vc_base_t.worst << ","

            << s_vc_fromClq.mean << "," << s_vc_fromClq.stdev << "," << s_vc_fromClq.best << "," << s_vc_fromClq.worst << ","
            << s_vc_fromClq_t.mean << "," << s_vc_fromClq_t.stdev << "," << s_vc_fromClq_t.best << "," << s_vc_fromClq_t.worst << ","

            << s_clq_base.mean << "," << s_clq_base.stdev << "," << s_clq_base.best << "," << s_clq_base.worst << ","
            << s_clq_base_t.mean << "," << s_clq_base_t.stdev << "," << s_clq_base_t.best << "," << s_clq_base_t.worst << ","

            << s_clq_fromVC.mean << "," << s_clq_fromVC.stdev << "," << s_clq_fromVC.best << "," << s_clq_fromVC.worst << ","
            << s_clq_fromVC_t.mean << "," << s_clq_fromVC_t.stdev << "," << s_clq_fromVC_t.best << "," << s_clq_fromVC_t.worst
            << "\n";
        csv.flush();
    }

    // ---- Global summary (B): pooled across ALL instance×trial samples ----
    ofstream sum(out_summary);
    write_header_summary(sum);

    Stat4 sn = compute_stats(I_n, true);
    Stat4 sm = compute_stats(I_m, true);

    Stat4 g_vc_base      = compute_stats(G_vc_base_obj, true);
    Stat4 g_vc_base_t    = compute_stats(G_vc_base_time, true);
    Stat4 g_vc_fromClq   = compute_stats(G_vc_fromClq_obj, true);
    Stat4 g_vc_fromClq_t = compute_stats(G_vc_fromClq_time, true);

    Stat4 g_clq_base      = compute_stats(G_clq_base_obj, false);
    Stat4 g_clq_base_t    = compute_stats(G_clq_base_time, true);
    Stat4 g_clq_fromVC    = compute_stats(G_clq_fromVC_obj, false);
    Stat4 g_clq_fromVC_t  = compute_stats(G_clq_fromVC_time, true);

    int totalSamples = (int)G_vc_base_obj.size(); // = parsed_instances * trials
    sum << totalSamples << "," << parsed_instances << ","
        << sn.mean << "," << sn.stdev << "," << sn.best << "," << sn.worst << ","
        << sm.mean << "," << sm.stdev << "," << sm.best << "," << sm.worst << ","

        << g_vc_base.mean << "," << g_vc_base.stdev << "," << g_vc_base.best << "," << g_vc_base.worst << ","
        << g_vc_base_t.mean << "," << g_vc_base_t.stdev << "," << g_vc_base_t.best << "," << g_vc_base_t.worst << ","

        << g_vc_fromClq.mean << "," << g_vc_fromClq.stdev << "," << g_vc_fromClq.best << "," << g_vc_fromClq.worst << ","
        << g_vc_fromClq_t.mean << "," << g_vc_fromClq_t.stdev << "," << g_vc_fromClq_t.best << "," << g_vc_fromClq_t.worst << ","

        << g_clq_base.mean << "," << g_clq_base.stdev << "," << g_clq_base.best << "," << g_clq_base.worst << ","
        << g_clq_base_t.mean << "," << g_clq_base_t.stdev << "," << g_clq_base_t.best << "," << g_clq_base_t.worst << ","

        << g_clq_fromVC.mean << "," << g_clq_fromVC.stdev << "," << g_clq_fromVC.best << "," << g_clq_fromVC.worst << ","
        << g_clq_fromVC_t.mean << "," << g_clq_fromVC_t.stdev << "," << g_clq_fromVC_t.best << "," << g_clq_fromVC_t.worst
        << "\n";
    sum.flush();

    std::cout << "\nWrote per-instance CSV: " << out_instance << "\n" << std::flush;
    std::cout << "Wrote global summary CSV: " << out_summary << "\n" << std::flush;
    std::cout << "All done.\n" << std::flush;

    return 0;
}
