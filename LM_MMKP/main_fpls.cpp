#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <iomanip>
#include <sstream>

#include "dataIO.h"
#include "fpls.h"

// main_fpls.cpp
// Runs the FPLS heuristic (fpls.h/fpls.cpp) over a list of MMKP instances,
// reusing the instance/solution IO from dataIO.h/dataIO.cpp exactly as
// main.cpp does for the original DROP/ADD repair heuristic (solution.cpp).
//
// Kept structurally separate from main.cpp / solution.cpp so the two
// heuristics can be built and run independently via the Makefile
// ("make original" vs "make fpls").

static const std::string INSTANCE_DIR = "instances/standard/";
static const std::string SOLUTIONS_CSV = "instances/solutions.csv";
static const std::string RESULTS_DIR = "results/";
static const std::string RESULTS_CSV = RESULTS_DIR + "LM_MMKP_fpls.csv";

// FPLS hyperparameters (see Yoon, Kim, Moon 2012, Table 4 for reference values).
static const int FPLS_N = 30000;      // iterations per run
static const int FPLS_R = 1000;       // independent randomized runs (best-of-R)
static const double FPLS_C = 10.0;    // step-size offset
static const unsigned FPLS_SEED = 1u;

// All 260 standard instance file names (mmkp_a_07_reduced.txt and large/ excluded).
static const std::vector<std::string> FILE_LIST = {
    "mmkp_a_07.txt",
    "mmkp_a_08.txt",
    "mmkp_a_09.txt",
    "mmkp_a_10.txt",
    "mmkp_a_11.txt",
    "mmkp_a_12.txt",
    "mmkp_a_13.txt",
    "mmkp_b_01.txt",
    "mmkp_b_02.txt",
    "mmkp_b_03.txt",
    "mmkp_b_04.txt",
    "mmkp_b_05.txt",
    "mmkp_b_06.txt",
    "mmkp_b_07.txt",
    "mmkp_b_08.txt",
    "mmkp_b_09.txt",
    "mmkp_b_10.txt",
    "mmkp_b_11.txt",
    "mmkp_b_12.txt",
    "mmkp_b_13.txt",
    "mmkp_b_14.txt",
    "mmkp_b_15.txt",
    "mmkp_b_16.txt",
    "mmkp_b_17.txt",
    "mmkp_b_18.txt",
    "mmkp_b_19.txt",
    "mmkp_b_20.txt",
    "mmkp_c_21.txt",
    "mmkp_c_22.txt",
    "mmkp_c_23.txt",
    "mmkp_c_24.txt",
    "mmkp_c_25.txt",
    "mmkp_c_26.txt",
    "mmkp_c_27.txt",
    "mmkp_c_28.txt",
    "mmkp_c_29.txt",
    "mmkp_c_30.txt",
    "mmkp_d_1.txt",
    "mmkp_d_2.txt",
    "mmkp_d_3.txt",
    "mmkp_d_4.txt",
    "mmkp_d_5.txt",
    "mmkp_d_6.txt",
    "mmkp_d_7.txt",
    "mmkp_d_8.txt",
    "mmkp_d_9.txt",
    "mmkp_d_10.txt",
    "mmkp_d_11.txt",
    "mmkp_d_12.txt",
    "mmkp_d_13.txt",
    "mmkp_d_14.txt",
    "mmkp_d_15.txt",
    "mmkp_d_16.txt",
    "mmkp_d_17.txt",
    "mmkp_d_18.txt",
    "mmkp_d_19.txt",
    "mmkp_d_20.txt",
    "mmkp_d_21.txt",
    "mmkp_d_22.txt",
    "mmkp_d_23.txt",
    "mmkp_d_24.txt",
    "mmkp_d_25.txt",
    "mmkp_d_26.txt",
    "mmkp_d_27.txt",
    "mmkp_d_28.txt",
    "mmkp_d_29.txt",
    "mmkp_d_30.txt",
    "mmkp_d_31.txt",
    "mmkp_d_32.txt",
    "mmkp_d_33.txt",
    "mmkp_d_34.txt",
    "mmkp_d_35.txt",
    "mmkp_d_36.txt",
    "mmkp_d_37.txt",
    "mmkp_d_38.txt",
    "mmkp_d_39.txt",
    "mmkp_d_40.txt",
    "mmkp_d_41.txt",
    "mmkp_d_42.txt",
    "mmkp_d_43.txt",
    "mmkp_d_44.txt",
    "mmkp_d_45.txt",
    "mmkp_d_46.txt",
    "mmkp_d_47.txt",
    "mmkp_d_48.txt",
    "mmkp_d_49.txt",
    "mmkp_d_50.txt",
    "mmkp_d_51.txt",
    "mmkp_d_52.txt",
    "mmkp_d_53.txt",
    "mmkp_d_54.txt",
    "mmkp_d_55.txt",
    "mmkp_d_56.txt",
    "mmkp_d_57.txt",
    "mmkp_d_59.txt",
    "mmkp_d_60.txt",
    "mmkp_d_61.txt",
    "mmkp_d_62.txt",
    "mmkp_d_64.txt",
    "mmkp_d_65.txt",
    "mmkp_d_66.txt",
    "mmkp_d_67.txt",
    "mmkp_d_68.txt",
    "mmkp_d_69.txt",
    "mmkp_d_70.txt",
    "mmkp_d_71.txt",
    "mmkp_d_72.txt",
    "mmkp_d_73.txt",
    "mmkp_d_74.txt",
    "mmkp_d_75.txt",
    "mmkp_d_76.txt",
    "mmkp_d_77.txt",
    "mmkp_d_78.txt",
    "mmkp_d_79.txt",
    "mmkp_d_80.txt",
    "mmkp_d_81.txt",
    "mmkp_d_82.txt",
    "mmkp_d_83.txt",
    "mmkp_d_84.txt",
    "mmkp_d_85.txt",
    "mmkp_d_86.txt",
    "mmkp_d_87.txt",
    "mmkp_d_88.txt",
    "mmkp_d_89.txt",
    "mmkp_d_90.txt",
    "mmkp_d_91.txt",
    "mmkp_d_92.txt",
    "mmkp_d_93.txt",
    "mmkp_d_94.txt",
    "mmkp_d_95.txt",
    "mmkp_d_97.txt",
    "mmkp_d_98.txt",
    "mmkp_d_99.txt",
    "mmkp_d_100.txt",
    "mmkp_d_101.txt",
    "mmkp_d_102.txt",
    "mmkp_d_103.txt",
    "mmkp_d_105.txt",
    "mmkp_d_106.txt",
    "mmkp_d_107.txt",
    "mmkp_d_108.txt",
    "mmkp_d_109.txt",
    "mmkp_d_110.txt",
    "mmkp_d_111.txt",
    "mmkp_d_112.txt",
    "mmkp_d_113.txt",
    "mmkp_d_114.txt",
    "mmkp_d_115.txt",
    "mmkp_d_116.txt",
    "mmkp_d_117.txt",
    "mmkp_d_118.txt",
    "mmkp_d_119.txt",
    "mmkp_d_120.txt",
    "mmkp_d_121.txt",
    "mmkp_d_122.txt",
    "mmkp_d_123.txt",
    "mmkp_d_124.txt",
    "mmkp_d_129.txt",
    "mmkp_d_130.txt",
    "mmkp_d_131.txt",
    "mmkp_d_132.txt",
    "mmkp_d_133.txt",
    "mmkp_d_134.txt",
    "mmkp_d_135.txt",
    "mmkp_d_136.txt",
    "mmkp_d_137.txt",
    "mmkp_d_138.txt",
    "mmkp_d_139.txt",
    "mmkp_d_140.txt",
    "mmkp_d_141.txt",
    "mmkp_d_142.txt",
    "mmkp_d_143.txt",
    "mmkp_d_144.txt",
    "mmkp_d_145.txt",
    "mmkp_d_146.txt",
    "mmkp_d_147.txt",
    "mmkp_d_148.txt",
    "mmkp_d_149.txt",
    "mmkp_d_150.txt",
    "mmkp_d_151.txt",
    "mmkp_d_152.txt",
    "mmkp_d_153.txt",
    "mmkp_d_154.txt",
    "mmkp_d_155.txt",
    "mmkp_d_156.txt",
    "mmkp_d_158.txt",
    "mmkp_d_159.txt",
    "mmkp_d_160.txt",
    "mmkp_d_161.txt",
    "mmkp_d_162.txt",
    "mmkp_d_163.txt",
    "mmkp_d_164.txt",
    "mmkp_d_165.txt",
    "mmkp_d_166.txt",
    "mmkp_d_167.txt",
    "mmkp_d_168.txt",
    "mmkp_d_169.txt",
    "mmkp_d_170.txt",
    "mmkp_d_171.txt",
    "mmkp_d_172.txt",
    "mmkp_d_173.txt",
    "mmkp_d_174.txt",
    "mmkp_d_175.txt",
    "mmkp_d_176.txt",
    "mmkp_d_177.txt",
    "mmkp_d_178.txt",
    "mmkp_d_179.txt",
    "mmkp_d_180.txt",
    "mmkp_d_181.txt",
    "mmkp_d_182.txt",
    "mmkp_d_183.txt",
    "mmkp_d_184.txt",
    "mmkp_d_193.txt",
    "mmkp_d_194.txt",
    "mmkp_d_195.txt",
    "mmkp_d_196.txt",
    "mmkp_d_197.txt",
    "mmkp_d_198.txt",
    "mmkp_d_199.txt",
    "mmkp_d_200.txt",
    "mmkp_d_201.txt",
    "mmkp_d_202.txt",
    "mmkp_d_203.txt",
    "mmkp_d_204.txt",
    "mmkp_d_205.txt",
    "mmkp_d_206.txt",
    "mmkp_d_207.txt",
    "mmkp_d_208.txt",
    "mmkp_d_209.txt",
    "mmkp_d_210.txt",
    "mmkp_d_211.txt",
    "mmkp_d_212.txt",
    "mmkp_d_213.txt",
    "mmkp_d_214.txt",
    "mmkp_d_215.txt",
    "mmkp_d_216.txt",
    "mmkp_d_217.txt",
    "mmkp_d_218.txt",
    "mmkp_d_219.txt",
    "mmkp_d_220.txt",
    "mmkp_d_221.txt",
    "mmkp_d_222.txt",
    "mmkp_d_223.txt",
    "mmkp_d_224.txt",
    "mmkp_d_225.txt",
    "mmkp_d_226.txt",
    "mmkp_d_227.txt",
    "mmkp_d_228.txt",
    "mmkp_d_229.txt",
    "mmkp_d_230.txt",
    "mmkp_d_231.txt",
    "mmkp_d_232.txt",
    "mmkp_d_233.txt",
    "mmkp_d_234.txt",
    "mmkp_d_235.txt",
    "mmkp_d_236.txt",
    "mmkp_d_241.txt",
    "mmkp_d_242.txt",
    "mmkp_d_243.txt",
    "mmkp_d_244.txt",
};


int main(int argc, char** argv) {
    // Optional CLI overrides: main_fpls [N] [R] [c] [seed]
    int N = argc > 1 ? std::atoi(argv[1]) : FPLS_N;
    int R = argc > 2 ? std::atoi(argv[2]) : FPLS_R;
    double c = argc > 3 ? std::atof(argv[3]) : FPLS_C;
    unsigned seed = argc > 4 ? static_cast<unsigned>(std::atoi(argv[4])) : FPLS_SEED;

    std::unordered_map<std::string, long long> solutionMap;
    bool solutionsLoaded = DataIO::readSolutions(SOLUTIONS_CSV, solutionMap);
    if (!solutionsLoaded) {
        std::cerr << "[main_fpls] Warning: could not load " << SOLUTIONS_CSV
                  << ". Percent diff will be reported as N/A." << std::endl;
    }

    std::filesystem::create_directories(RESULTS_DIR);
    std::ofstream fout(RESULTS_CSV);
    if (!fout.is_open()) {
        std::cerr << "[main_fpls] Failed to open output file: " << RESULTS_CSV << std::endl;
        return 1;
    }

    fout << "instance,N,M,our_value,known_optimum,percent_diff,feasible_hits\n";

    int successCount = 0, failCount = 0, matchedCount = 0;

    for (const std::string& fileName : FILE_LIST) {
        std::string fullPath = INSTANCE_DIR + fileName;

        Instance instance;
        bool ok = DataIO::readInstance(fullPath, instance);

        if (!ok) {
            std::cerr << "[main_fpls] Failed to read instance: " << fileName << std::endl;
            ++failCount;
            continue;
        }

        FPLSResult result = solveFPLS(instance, N, R, c, seed);
        ++successCount;

        auto it = solutionMap.find(fileName);
        std::string knownOptStr = "N/A";
        std::string percentDiffStr = "N/A";

        if (it != solutionMap.end()) {
            long long knownOpt = it->second;
            knownOptStr = std::to_string(knownOpt);

            if (knownOpt != 0 && result.bestValue >= 0.0) {
                double percentDiff = (result.bestValue - static_cast<double>(knownOpt))
                                      / static_cast<double>(knownOpt) * 100.0;
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(4) << percentDiff;
                percentDiffStr = oss.str();
            }
            ++matchedCount;
        }

        fout << fileName << "," << instance.N << "," << instance.M << ","
             << std::fixed << std::setprecision(2) << result.bestValue << ","
             << knownOptStr << "," << percentDiffStr << ","
             << result.feasibleHits << "\n";

        std::cout << "[main_fpls] " << fileName
                  << " N=" << instance.N << " M=" << instance.M
                  << " ourValue=" << result.bestValue
                  << " knownOpt=" << knownOptStr
                  << " percentDiff=" << percentDiffStr
                  << " feasibleHits=" << result.feasibleHits << std::endl;

        if (result.bestValue < 0.0) {
            std::cerr << "[main_fpls] Warning: no feasible solution found for "
                      << fileName << " within N=" << N << ", R=" << R << std::endl;
        }
    }

    fout.close();

    std::cout << "\n[main_fpls] Done. Success: " << successCount
              << ", Failed: " << failCount
              << ", Matched with solutions.csv: " << matchedCount
              << ", Total: " << FILE_LIST.size() << std::endl;
    std::cout << "[main_fpls] Results written to: " << RESULTS_CSV << std::endl;

    return 0;
}