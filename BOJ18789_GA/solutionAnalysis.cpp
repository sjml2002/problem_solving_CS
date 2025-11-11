#include "libct.h"
# define R 8
# define C 14
# define MAXNUM 8140
using namespace std;

/**
 * <1.> 격자의 각 원소가 unique path로 몇 개의 score을 감당하고 있는지
 * <2.> 숫자 n을 표현할 수 있는 곳이 어디인지?
 *      그리고 숫자 n을 표현할 수 있는 곳의 총 개수? -> vector size로 하면 될듯
 */

class SolutionAnalysis {
public:
	string gene[R]; //실제 해
	int fitness = -1;
    int visUniquePath[R][C]; //1.
    set<vector<pair<int, int>>> possibleRepresent[MAXNUM+1]; //[score][path][coord]
    
	
	SolutionAnalysis() {}
	SolutionAnalysis(string tmp[]) {
		for(int i=0; i<R; i++) {
            gene[i] = tmp[i];
            for(int j=0; j<C; j++)
                visUniquePath[i][j] = 0;
        }
			
	}

	
	/**
		- 적합도 평가 (점수 채점) 
		그냥 단순히 BFS써서 현재 점수를 계산하는 방식으로 하시죠? 
	*/
	int FitnessAnalysis() {
		if (this->fitness != -1)
			return (this->fitness);
		
        int fitscore = 0;
		int score = 1;
		while (score <= MAXNUM) {
			string scorestr = to_string(score);
			int flag = 0;
			for(int i=0; i<R; i++) {
				for(int j=0; j<C; j++) {
					if (scorestr[0] == gene[i][j]) {
                        vector<pair<int, int>> path;
						int successFlag = Fitness_BFS(&path, scorestr, i, j, 1);
						if (successFlag) {
                            possibleRepresent[score].insert(path);
                            flag = 1;

                        }
					}
				}
			}
            if (!flag && fitscore == 0)
                fitscore = score;
			score++;
		}

        calc_visUniquePath();

		this->fitness = fitscore-1;
		return (this->fitness);
	}
	
	int Fitness_BFS(vector<pair<int, int>>* path, string& scorestr, int i, int j, int idx) {
        path->push_back(make_pair(i,j));

		if (idx == scorestr.size())
			return (1);
		
		int flag = 0;
		if (!flag && i>0 && gene[i-1][j] == scorestr[idx]) //상 
			flag = flag | Fitness_BFS(path, scorestr, i-1, j, idx+1);
		if (!flag && i<R-1 && gene[i+1][j] == scorestr[idx]) //하 
			flag = flag | Fitness_BFS(path, scorestr, i+1, j, idx+1);
		if (!flag && j>0 && gene[i][j-1] == scorestr[idx]) //좌 
			flag = flag | Fitness_BFS(path, scorestr, i, j-1, idx+1);
		if (!flag && j<C-1 && gene[i][j+1] == scorestr[idx]) //우 
			flag = flag | Fitness_BFS(path, scorestr, i, j+1, idx+1);
		if (!flag && (i>0 && j>0) && gene[i-1][j-1] == scorestr[idx]) //좌상 
			flag = flag | Fitness_BFS(path, scorestr, i-1, j-1, idx+1);
		if (!flag && (i>0 && j<C-1) && gene[i-1][j+1] == scorestr[idx]) //우상 
			flag = flag | Fitness_BFS(path, scorestr, i-1, j+1, idx+1);
		if (!flag && (i<R-1 && j>0) && gene[i+1][j-1] == scorestr[idx]) //좌하 
			flag = flag | Fitness_BFS(path, scorestr, i+1, j-1, idx+1);
		if (!flag && (i<R-1 && j<C-1) && gene[i+1][j+1] == scorestr[idx]) //우하 
			flag = flag | Fitness_BFS(path, scorestr, i+1, j+1, idx+1);

        if (flag == 0)
            path->pop_back();
		return (flag);
	}


    /**
     * 전부 계산된 possibleRepresent를 가지고
     * visUniquePath 계산하기 (각 격자의 원소가 unique path로 몇 개의 score을 감당하고 있는지?)
     */
    void calc_visUniquePath() {
        for(int score=0; score<=MAXNUM; score++) {
            if (possibleRepresent[score].size() == 1) {
                //path 은 score에서 unique한 path임.
                for(auto path=possibleRepresent[score].begin(); path != possibleRepresent[score].end(); path++) {
                    int y = (*path)[0].first;
                    int x = (*path)[0].second;
                    visUniquePath[y][x]++;
                }
            }
        }
    }

	
	void output(ostream& out) {
		out << "Fitness: " << FitnessAnalysis() << "\n";
		for(int i=0; i<R; i++)
			out << gene[i] << "\n";
		out << "------------------------\n";

        out << "visUniquePath : counting uniquePath each score\n";
        for(int i=0; i<R; i++) {
            for(int j=0; j<C; j++)
                out << visUniquePath[i][j] << " ";
            out << "\n";
        }
        out << "------------------------\n";

        out << "use count each score\n";
        for(int score=1; score<=MAXNUM; score++) {
            out << score << ": " << possibleRepresent[score].size() << "\n";
            for(auto path=possibleRepresent[score].begin(); path != possibleRepresent[score].end(); path++) {
                //output path
                for(int i=0; i<path->size(); i++)  {
                    int y = (*path)[i].first;
                    int x = (*path)[i].second;
                    out << "(" << y << "," << x << "), ";
                }
                out << "\n";                
            }
            out << "\n";
        }
        out << "------------------------\n";

        out << "use count=1 (unique), each score\n";
        int uniqueCnt = 0;
        for(int score=1; score<=MAXNUM; score++) {
            if (possibleRepresent[score].size() == 1) {
                uniqueCnt++;
                out << score << ": " << possibleRepresent[score].size() << "\n";
                for(auto path=possibleRepresent[score].begin(); path != possibleRepresent[score].end(); path++) {
                    //output path
                    for(int i=0; i<path->size(); i++)  {
                        int y = (*path)[i].first;
                        int x = (*path)[i].second;
                        out << "(" << y << "," << x << "), ";
                    }
                    out << "\n";                
                }
                out << "\n";
            }
        }
        out << "total uniqueCnt: " << uniqueCnt << "\n";

        out << "------------------------\n";
        out << "use count=0 (zero), each score\n";
        int zeroCnt = 0;
        for(int score=1; score<=MAXNUM; score++) {
            if (possibleRepresent[score].size() == 0) {
                zeroCnt++;
                out << score << ": " << possibleRepresent[score].size() << "\n";
                for(auto path=possibleRepresent[score].begin(); path != possibleRepresent[score].end(); path++) {
                    //output path
                    for(int i=0; i<path->size(); i++)  {
                        int y = (*path)[i].first;
                        int x = (*path)[i].second;
                        out << "(" << y << "," << x << "), ";
                    }
                    out << "\n";                
                }
                out << "\n";
            }
        }
        out << "total zeroCnt: " << zeroCnt << "\n";


	}
};



int main() {
    ios_base::sync_with_stdio(false);
    cout.tie(0);
    cin.tie(0);

    string tmp[R];
    for(int i=0; i<R; i++)
        cin >> tmp[i];
    SolutionAnalysis tmpsol = SolutionAnalysis(tmp);

    ofstream ofs("./analysis.txt");
    if (ofs.is_open()) {
        tmpsol.output(ofs);
        ofs.close();
    }

    return (0);
}
