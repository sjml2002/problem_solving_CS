#include "libct.h"
# define R 8
# define C 14
# define STARTNUM 1000
# define MAXNUM 9999
# define PSIZE 2
# define MAXSAME 30
using namespace std;
typedef long long int ll;

/**
 * 랜덤 값 생성
 */
static random_device rd;   // 시드값 생성(한 번만)
static mt19937 gen(
    chrono::steady_clock::now().time_since_epoch().count()
); // 매번 시드값 달라짐
//0 ~ end 사이 범위 정수 랜덤값 생성 함수 (0과 end 포함)
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
	int fitness = -1; //STARTNUM~MAXNUM까지 채울 수 있는 점수
	int fitscore = -1; //문제에서의 점수

	Solution() {
		this->fitness = -1;
		this->fitscore = -1;
	}
	Solution(string tmp[]) {
		this->fitness = -1;
		this->fitscore = -1;
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
	*/
	int Fitness() {
		if (this->fitness != -1)
			return (this->fitness);
		
		int score = STARTNUM;
		int fitscoreflag = 0;
		int totalscore = 0; //STARTNUM~MAXNUM까지 채울 수 있는 점수
		while (score <= MAXNUM) {
			string scorestr = to_string(score);
			int flag = 0;
			for(int i=0; i<R; i++) {
				for(int j=0; j<C; j++) {
					if (scorestr[0] == gene[i][j]) {
						int successFlag = Fitness_BFS(scorestr, i, j, 1);
						if (successFlag) //scorestr 만들기 가능
							flag = 1;
					}
				}
			}
			
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

		this->fitness = totalscore;
		return (this->fitness);
	}
	
	int Fitness_BFS(string& scorestr, int i, int j, int idx) {
		if (idx == scorestr.size())
			return (1);
		
		int flag = 0; //flag == 1 이라면 scorestr을 표현할 수 있다는 뜻이다.
		if (!flag && i>0 && gene[i-1][j] == scorestr[idx]) //상 
			flag = flag | Fitness_BFS(scorestr, i-1, j, idx+1);
		if (!flag && i<R-1 && gene[i+1][j] == scorestr[idx]) //하 
			flag = flag | Fitness_BFS(scorestr, i+1, j, idx+1);
		if (!flag && j>0 && gene[i][j-1] == scorestr[idx]) //좌 
			flag = flag | Fitness_BFS(scorestr, i, j-1, idx+1);
		if (!flag && j<C-1 && gene[i][j+1] == scorestr[idx]) //우 
			flag = flag | Fitness_BFS(scorestr, i, j+1, idx+1);
		if (!flag && (i>0 && j>0) && gene[i-1][j-1] == scorestr[idx]) //좌상 
			flag = flag | Fitness_BFS(scorestr, i-1, j-1, idx+1);
		if (!flag && (i>0 && j<C-1) && gene[i-1][j+1] == scorestr[idx]) //우상 
			flag = flag | Fitness_BFS(scorestr, i-1, j+1, idx+1);
		if (!flag && (i<R-1 && j>0) && gene[i+1][j-1] == scorestr[idx]) //좌하 
			flag = flag | Fitness_BFS(scorestr, i+1, j-1, idx+1);
		if (!flag && (i<R-1 && j<C-1) && gene[i+1][j+1] == scorestr[idx]) //우하 
			flag = flag | Fitness_BFS(scorestr, i+1, j+1, idx+1);
		
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


int main() {
	Solution tmps;
    tmps.initSol();

    for(int r=0; r<R; r++) {
        for(int c=0; c<C; c++)
            cout << tmps.gene[r][c];
        cout << "\n";
    }
	
	return (0);
}
