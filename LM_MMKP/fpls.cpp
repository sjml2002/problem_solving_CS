
// fpls.cpp
// Feasibility-Pursuing Lagrangian Search (FPLS) for the Multiple-choice
// Multidimensional Knapsack Problem (MMKP), extended from Yoon, Kim, Moon (2012)
// "A theoretical and empirical investigation on the Lagrangian capacities
//  of the 0-1 multidimensional knapsack problem".
//
// Core idea:
//   - LM_MMKP(u): given Lagrange multipliers u (>=0, size m), solve the
//     Lagrangian-relaxed problem exactly in O(n*m) by picking, independently
//     for each group, the single item maximizing (profit - u . weight).
//     This always satisfies the multiple-choice constraint but may violate
//     the m resource (capacity) constraints.
//   - FPLS: randomized search over u that nudges multipliers up/down
//     depending on which capacity constraints are violated, tracking the
//     best feasible solution found (Theorem 2 monotonicity in the paper).
//
// Input file format (instances/standard/*.txt , instances/large/*.txt):
//   line 1      : G m           (number of groups, number of resource dims)
//   line 2      : b_1 ... b_m   (capacities)
//   for each group g = 1..G:
//     line      : k_g                       (number of items in group g)
//     k_g lines : profit  w_1 ... w_m        (one item per line)
//
// solutions.csv format (with possible UTF-8 BOM on first line):
//   filename,best_value
//   ...
//
//
// Usage:
//   ./fpls <instance_file> [N=30000] [R=1000] [c=10] [seed=1] [solutions_csv]
//
//   N : number of FPLS iterations per run
//   R : number of independent randomized runs (best-of-R)
//   c : denominator offset for step size delta_t = 1/(t + c - 1)
//   solutions_csv : optional path to solutions.csv for gap reporting

#include <bits/stdc++.h>
using namespace std;

struct Item {
    long long profit;
    vector<long long> weight; // size m
};

struct Instance {
    int G = 0;                 // number of groups
    int m = 0;                 // number of resource dimensions
    vector<long long> cap;     // size m
    vector<vector<Item>> groups; // groups[g] = list of items
    string name;
};

static string trim(const string &s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

static void stripBOM(string &s) {
    if (s.size() >= 3 &&
        (unsigned char)s[0] == 0xEF &&
        (unsigned char)s[1] == 0xBB &&
        (unsigned char)s[2] == 0xBF) {
        s.erase(0, 3);
    }
}

Instance readInstance(const string &path) {
    ifstream fin(path);
    if (!fin) {
        throw runtime_error("cannot open instance file: " + path);
    }
    Instance inst;
    inst.name = path;
    // strip directory for matching against solutions.csv
    {
        size_t pos = inst.name.find_last_of("/\\");
        if (pos != string::npos) inst.name = inst.name.substr(pos + 1);
    }

    fin >> inst.G >> inst.m;
    inst.cap.resize(inst.m);
    for (int i = 0; i < inst.m; i++) fin >> inst.cap[i];

    inst.groups.resize(inst.G);
    for (int g = 0; g < inst.G; g++) {
        int k;
        fin >> k;
        inst.groups[g].resize(k);
        for (int j = 0; j < k; j++) {
            Item &it = inst.groups[g][j];
            it.weight.resize(inst.m);
            fin >> it.profit;
            for (int i = 0; i < inst.m; i++) fin >> it.weight[i];
        }
    }
    if (!fin) {
        throw runtime_error("malformed instance file (unexpected EOF): " + path);
    }
    return inst;
}

unordered_map<string, double> readSolutions(const string &path) {
    unordered_map<string, double> sol;
    ifstream fin(path);
    if (!fin) return sol; // optional file
    string line;
    bool first = true;
    while (getline(fin, line)) {
        if (first) { stripBOM(line); first = false; }
        if (line.empty()) continue;
        size_t comma = line.find(',');
        if (comma == string::npos) continue;
        string fname = trim(line.substr(0, comma));
        string val = trim(line.substr(comma + 1));
        if (fname.empty() || val.empty()) continue;
        try {
            sol[fname] = stod(val);
        } catch (...) {
            // skip malformed row
        }
    }
    return sol;
}

// Result of one LM_MMKP evaluation.
struct LMResult {
    vector<int> choice;      // choice[g] = selected item index in group g
    vector<long long> usage; // resource usage vector (size m)
    long long profit = 0;    // true objective value (sum of profits), NOT the Lagrangian value
};

// LM_MMKP: solve the Lagrangian-relaxed problem exactly for given u.
// For each group, independently pick argmax_j ( profit_j - sum_i u_i * weight_ij ).
LMResult LM_MMKP(const Instance &inst, const vector<double> &u) {
    LMResult res;
    res.choice.resize(inst.G);
    res.usage.assign(inst.m, 0);
    res.profit = 0;

    for (int g = 0; g < inst.G; g++) {
        const auto &items = inst.groups[g];
        int bestJ = 0;
        double bestVal = -numeric_limits<double>::infinity();
        for (size_t j = 0; j < items.size(); j++) {
            double val = (double)items[j].profit;
            for (int i = 0; i < inst.m; i++) val -= u[i] * (double)items[j].weight[i];
            if (val > bestVal) {
                bestVal = val;
                bestJ = (int)j;
            }
        }
        res.choice[g] = bestJ;
        res.profit += items[bestJ].profit;
        for (int i = 0; i < inst.m; i++) res.usage[i] += items[bestJ].weight[i];
    }
    return res;
}

static bool isFeasible(const Instance &inst, const vector<long long> &usage) {
    for (int i = 0; i < inst.m; i++) {
        if (usage[i] > inst.cap[i]) return false;
    }
    return true;
}

struct FPLSResult {
    long long bestProfit = -1;
    vector<int> bestChoice;
    vector<long long> bestUsage;
    long long feasibleFound = 0; // count of iterations that produced a feasible solution
};

// One FPLS run: N iterations of randomized multiplier search.
FPLSResult fplsRun(const Instance &inst, int N, double c, mt19937 &rng) {
    vector<double> u(inst.m, 0.0);
    FPLSResult best;

    vector<int> violated; violated.reserve(inst.m);
    uniform_int_distribution<int> pickAny(0, inst.m - 1);

    for (int t = 1; t <= N; t++) {
        LMResult r = LM_MMKP(inst, u);
        double delta = 1.0 / (double)(t + c - 1.0);

        if (isFeasible(inst, r.usage)) {
            best.feasibleFound++;
            if (r.profit > best.bestProfit) {
                best.bestProfit = r.profit;
                best.bestChoice = r.choice;
                best.bestUsage = r.usage;
            }
            // relax a random multiplier to keep exploring for better solutions
            int k = pickAny(rng);
            u[k] = max(0.0, u[k] - delta);
        } else {
            violated.clear();
            for (int i = 0; i < inst.m; i++) {
                if (r.usage[i] > inst.cap[i]) violated.push_back(i);
            }
            uniform_int_distribution<int> pickViol(0, (int)violated.size() - 1);
            int k = violated[pickViol(rng)];
            u[k] += delta;
        }
    }
    return best;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0]
             << " <instance_file> [N=30000] [R=1000] [c=10] [seed=1] [solutions_csv]\n";
        return 1;
    }

    string instPath = argv[1];
    int N   = argc > 2 ? atoi(argv[2]) : 30000;
    int R   = argc > 3 ? atoi(argv[3]) : 1000;
    double c = argc > 4 ? atof(argv[4]) : 10.0;
    unsigned seed = argc > 5 ? (unsigned)atoi(argv[5]) : 1u;
    string solPath = argc > 6 ? argv[6] : "";

    Instance inst;
    try {
        inst = readInstance(instPath);
    } catch (const exception &e) {
        cerr << "Error reading instance: " << e.what() << "\n";
        return 1;
    }

    cerr << "Loaded instance '" << inst.name << "': G=" << inst.G
         << " m=" << inst.m << "\n";

    mt19937 rng(seed);
    FPLSResult overallBest;

    auto t0 = chrono::steady_clock::now();
    for (int run = 0; run < R; run++) {
        FPLSResult r = fplsRun(inst, N, c, rng);
        if (r.bestProfit > overallBest.bestProfit) {
            overallBest = r;
        }
    }
    auto t1 = chrono::steady_clock::now();
    double elapsed = chrono::duration<double>(t1 - t0).count();

    cout << "instance,profit,feasible,time_sec";
    unordered_map<string, double> solutions;
    if (!solPath.empty()) {
        solutions = readSolutions(solPath);
        cout << ",known_best,gap_percent";
    }
    cout << "\n";

    cout << inst.name << "," << overallBest.bestProfit << ","
         << (overallBest.bestProfit >= 0 ? 1 : 0) << ","
         << fixed << setprecision(3) << elapsed;

    if (!solPath.empty()) {
        auto it = solutions.find(inst.name);
        if (it != solutions.end() && overallBest.bestProfit >= 0) {
            double known = it->second;
            double gap = known > 0 ? 100.0 * (known - overallBest.bestProfit) / known : 0.0;
            cout << "," << known << "," << fixed << setprecision(3) << gap;
        } else {
            cout << ",NA,NA";
        }
    }
    cout << "\n";

    if (overallBest.bestProfit < 0) {
        cerr << "No feasible solution found within given N, R.\n";
        return 2;
    }

    cerr << "Best profit: " << overallBest.bestProfit << "\n";
    cerr << "Resource usage: ";
    for (int i = 0; i < inst.m; i++) {
        cerr << overallBest.bestUsage[i] << "/" << inst.cap[i] << " ";
    }
    cerr << "\n";

    return 0;
}
