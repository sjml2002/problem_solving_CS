#include "libct.h"
# define R 8
# define C 14
# define MAXNUM 8140
using namespace std;

/**
 * <1.> 각 격자별로 숫자 카운팅에 몇번 사용됐는지?
 * <2.> 숫자 n을 표현할 수 있는 곳이 어디인지?
 *      그리고 숫자 n을 표현할 수 있는 곳의 총 개수? -> vector size로 하면 될듯
 */

class SolutionAnalysis {
public:
	string gene[R]; //실제 해
	int fitness = -1;
    int vis[R][C]; //1.
    vector<vector<pair<int, int>>> possibleRepresent[MAXNUM+1]; //[score][path][coord]
	
	SolutionAnalysis() {}
	SolutionAnalysis(string tmp[]) {
		for(int i=0; i<R; i++) {
            gene[i] = tmp[i];
            for(int j=0; j<C; j++)
                vis[i][j] = 0;
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
                            possibleRepresent[score].push_back(path);
                            flag = 1;

                        }
					}
				}
			}
            if (!flag && fitscore == 0)
                fitscore = score;
			score++;
		}
		this->fitness = fitscore-1;
		return (this->fitness);
	}
	
	int Fitness_BFS(vector<pair<int, int>>* path, string& scorestr, int i, int j, int idx) {
        vis[i][j]++; //<1.> i, j가 사용됨
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
	
	void output(ostream& out) {
		out << "Fitness: " << FitnessAnalysis() << "\n";
		for(int i=0; i<R; i++)
			out << gene[i] << "\n";
		out << "------------------------\n";

        out << "counting in vis\n";
        for(int i=0; i<R; i++) {
            for(int j=0; j<C; j++)
                out << vis[i][j] << " ";
            out << "\n";
        }
        out << "------------------------\n";

        out << "use count each score\n";
        for(int score=1; score<=MAXNUM; score++) {
            out << score << ": " << possibleRepresent[score].size() << "\n";
            for(int pi=0; pi<possibleRepresent[score].size(); pi++) {
                //output path
                for(int i=0; i<possibleRepresent[score][pi].size(); i++)  {
                    int y = possibleRepresent[score][pi][i].first;
                    int x = possibleRepresent[score][pi][i].second;
                    out << "(" << y << "," << x << "), ";
                }
                out << "\n";                
            }
            out << "\n";
        }
        out << "------------------------\n";

        out << "use count=0, each score\n";
        for(int score=1; score<=MAXNUM; score++) {
            if (possibleRepresent[score].size() == 0) {
                out << score << ": " << possibleRepresent[score].size() << "\n";
            }
        }
        out << "------------------------\n";

        out << "use count=1 (unique), each score\n";
        int uniqueCnt = 0;
        for(int score=1; score<=MAXNUM; score++) {
            if (possibleRepresent[score].size() == 1) {
                uniqueCnt++;
                out << score << ": " << possibleRepresent[score].size() << "\n";
                for(int pi=0; pi<possibleRepresent[score].size(); pi++) {
                    //output path
                    for(int i=0; i<possibleRepresent[score][pi].size(); i++)  {
                        int y = possibleRepresent[score][pi][i].first;
                        int x = possibleRepresent[score][pi][i].second;
                        out << "(" << y << "," << x << "), ";
                    }
                    out << "\n";                
                }
                out << "\n";
            }
        }
        out << "total uniqueCnt: " << uniqueCnt << "\n";
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
