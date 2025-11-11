#include "libct.h"
# define R 4
# define C 7
# define MAXNUM 8140
# define PSIZE 2
using namespace std;
typedef long long int ll;

/**
 * 랜덤 값 생성
 */
static random_device rd;   // 시드값 생성(한 번만)
static mt19937 gen(
    chrono::steady_clock::now().time_since_epoch().count()
); // 매번 시드값 달라짐
//0 ~ end 사이 범위 정수 랜덤값 생성 함수
int genRandom(int end) { 
	uniform_int_distribution<> dis(0, end);
	return (dis(gen));
}
////////////////


/*
	Simulated Annealing
*/
class Solution {
public:
	string gene[R]; //실제 해
	int fitness = -1;
	int fitscore = -1;
	set<vector<pair<int, int>>> possibleRepresent[MAXNUM+1]; //[score][path][coord]
	int uniquePathCnt = 0;
	double upc = 0.5;

	Solution() {}
	Solution(string tmp[]) {
		for(int i=0; i<R; i++)
			gene[i] = tmp[i];
	}

	
	void initSol() {
		for(int i=0; i<R; i++) {
			string tmp; 
			for(int j=0; j<C; j++) {
				int rv = genRandom(9);
				tmp += ((char)rv+48);
			}
			gene[i] = tmp;
		}
	}

	
	/**
	 * - 적합도 평가 (점수 채점) 
	 * 실제 점수 - (upc)*uniquePathCnt
	 * //upc는 uniquePathCnt 가중치를 얼마나 적용할 것인지에 대한 비율
	 * uniquePathCnt는 score을 만들 수 있는 방법이 단 하나밖에 없는 score들을 카운트한다.
	 * uniquePathCnt는 후에 Fitness에 음수 가중치로 들어간다.
	*/
	int Fitness() {
		if (this->fitness != -1)
			return (this->fitness);
		
		int score = 1;
		int fitscoreflag = 0;
		int totalscore = 0;
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
			
			if (possibleRepresent[score].size() == 1)
				uniquePathCnt++;
			
			if (!flag) { //score을 찾지 못했음 
				if (!fitscoreflag) { //Solution의 fitscore update
					this->fitscore = score-1;
					fitscoreflag = 1;
				}
			}
			else {
				totalscore++;
			}
			score++;
		}

		this->fitness = totalscore - (upc * uniquePathCnt);
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
	
	void output(ostream& out) {
		Fitness();
		out << "Fitness: " << this->fitness << " / score: " << this->fitscore << "\n";
		for(int i=0; i<R; i++)
			out << gene[i] << "\n";
		out << "\n";
	}
};

/**
 * sort cmp
 */
int sortcmp(Solution& s1, Solution& s2) {
	return (s1.Fitness() > s2.Fitness());
}

vector<Solution> bestsol;
/*
 * best sol 업데이트
 */
void updateBest(vector<Solution>& bestsol, const vector<Solution>& cursv) {
    int bssize = bestsol.size();
    vector<Solution> merged = bestsol;
    merged.insert(merged.end(), cursv.begin(), cursv.end()); //bestsol + cursv
	sort(merged.begin(), merged.end(), sortcmp);

    // 상위 bssize만 남기기
    if (merged.size() > bssize)
        merged.resize(bssize);
    // bestsol 갱신
    bestsol = merged;
}




/**
 * Mutation
 * 바로 옆에 있는 숫자와 같은 숫자가 되도록 유도
 * 돌연변이 확률은 generateChild에서 정하기
 */
int selectNum(string s[], int i, int j) {
	int maxv = 0;
	int maxnum = 0;
	for(int num=0; num<10; num++) {
		char numstr = (char)(num+48);
		int flag = 0; //flag는 현재 numstr이 주변에 얼마나 있는지에 대한 count
		if (i>0 && s[i-1][j] == numstr) //상 
			flag++;
		if (i<R-1 && s[i+1][j] == numstr) //하 
			flag++;
		if (j>0 && s[i][j-1] == numstr) //좌 
			flag++;
		if (j<C-1 && s[i][j+1] == numstr) //우 
			flag++;
		if ((i>0 && j>0) && s[i-1][j-1] == numstr) //좌상 
			flag++;
		if ((i>0 && j<C-1) && s[i-1][j+1] == numstr) //우상 
			flag++;
		if ((i<R-1 && j>0) && s[i+1][j-1] == numstr) //좌하 
			flag++;
		if ((i<R-1 && j<C-1) && s[i+1][j+1] == numstr) //우하 
			flag++;
		
		//Rank Based Selection 비스무리 하게
		int rv;
		if (flag == 1) { //같은 숫자 연속적으로 나올 수 있도록
			//만약에 현재 위치가 끝쪽이라면 최대한 다른값 선택할 수 있도록
			if (i==0 || i==R-1 || j==0 || j==C-1)
				rv = genRandom(0);
			else
				rv = genRandom(700); //상한선 설정
		}
		else if (flag >= 4) //num과 같은숫자가 주변에 과도하게 많다면 선택되지 않도록
			rv = genRandom(0);
		else if (flag == 0) //num과 같은 숫자가 주변에 하나도 없다면 선택할 확률 매우 높임
			rv = genRandom(1000);
		else //flag가 적을수록 선택될 확률을 높임
			rv = genRandom(1000/flag + 100); //flag=2 : 500 , flag=3 : 333
		
		if (rv > maxv) {
			maxv = rv;
			maxnum = num;
		}
	}
	return (maxnum);
}



/**
 * 기존 solution을 가지고 이웃해 생성
 * 바꿀 Row와 Column을 randomly하게 선택하게 하고 그 수를 selectNum으로 선택
 * @param{sol} : 기존 solution
 */

Solution generateNeibor(Solution* sol) {
	Solution neighbor(sol->gene);
	int r = genRandom(R-1);
	int c = genRandom(C-1);

	char newnum = (char)(selectNum(neighbor.gene, r, c) + 48);
	neighbor.gene[r][c] = newnum;
	return (neighbor);
}



/**
 * @param{generation} : 세대 수 (Loop 수)
 * @param{genes} : 1세대 당 유전자의 수 
 * @return : 최고 점수 
*/
int SimulatedAnnealing(int ti) {
	// 1. 초기해 (bestsol)
	vector<Solution> cursv;
	int randomSize = 0;
	for(int i=0; i<PSIZE-randomSize; i++) {
		cursv.push_back(bestsol[i]);
	}
	// 1-1. 초기해 랜덤
	for(int i=0; i<randomSize; i++) {
		Solution tmp;
		tmp.initSol();
		cursv.push_back(tmp);
	}

    double selectp = 0.8; //selectp보다 p가 높으면 선택함.
	double r = 0.99; //냉각률
	double T = 100000; //온도
	double limit = 0.00000001;

	int gi = 0;
	while (T > limit) {
		// 2. 이웃해 생성
		for(int i=0; i<cursv.size(); i++) {
			Solution neighbor = generateNeibor(&cursv[i]);

			/* neighbor 이 더 크다면 costdiff >= 1 이 되고 p는 1 이상이 되어서 무조건 선택
				neighbor이 더 작다면 costdiff < 1 이 되고 p는 0~1  사이 값
					=> 온도에 따라서 선택하게 됨. (온도 ↑, p ↑)
			*/
			double costdiff = neighbor.Fitness() - cursv[i].Fitness();
			double p = exp(costdiff / T);

			if (p >= selectp) //2-1. 이웃해 채택
				cursv[i] = neighbor;
		}
		
		if (gi % 10 == 0) {
			cout << ti << " - SA" << gi << " T: " << T << " => " << cursv[0].Fitness() << "(" << cursv[0].fitscore << ") / ";
			cout << bestsol[0].Fitness() << "(" << bestsol[0].fitscore << ")" << endl; //debug : 현재 세대 Fitness
		}

		// 3. best Solution 판단
		updateBest(bestsol, cursv);
		gi++;
		T = r*T;
	}
	//마지막 한번 더
	updateBest(bestsol, cursv); // best Solution 판단
	
	return (0);
}


int main() {
	int testcase = 25;
	int ti = 0;
	while (ti < testcase) {
		bestsol.clear();
		cout << "========= testcase " << ti << "=========" << endl;

		int bestsolsize = PSIZE;

		// 0. init bestsol (Prev GA's best)
		ifstream ifs("./best4x7.txt");
		if (!ifs.is_open()) {
			cout << "Error opening file" << endl;
			return (1);
		}

		string line;
		for(int i=0; i<bestsolsize; i++) {
			getline(ifs, line); //first line is Fitness
			int ri = 0;
			string prevbest[R];
			while (getline(ifs, line)) {
				if (line.size() <= 1)
					break ;
				prevbest[ri] = line;
				ri++;
			}
			bestsol.push_back(Solution(prevbest));

			bestsol[i].output(cout); //debug
		}
		ifs.close();
		
		int res = SimulatedAnnealing(ti);

		// write best solution in "best.txt"
		ofstream ofs("./best4x7.txt");
		if (ofs.is_open()) {
			/** 만약 best.txt에 기록되는 해를 늘리고 싶다면 bestsol.size()로 바꾸기 */
			// for(int i=0; i<bestsolsize; i++) {
			// 	bestsol[i].output(ofs);
			// }
			for(int i=0; i<bestsol.size(); i++) {
				bestsol[i].output(ofs);
			}
			ofs.close();
		}
		ti++;
	}
	
	return (0);
}
