#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>
# define R 8
# define C 14
using namespace std;

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
	유전 알고리즘으로 풀어보자. 
*/
class Solution {
public:
	string gene[R]; //실제 해 
	
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
		- 적합도 평가 (점수 채점) 
		그냥 단순히 BFS써서 현재 점수를 계산하는 방식으로 하시죠? 
	*/
	int Fitness() {
		int score = 1;
		while (1) {
			string scorestr = to_string(score);
			int successFlag = 0;
			for(int i=0; i<R; i++) {
				for(int j=0; j<C; j++) {
					if (scorestr[0] == gene[i][j]) {
						successFlag = Fitness_BFS(scorestr, i, j, 1);
						if (successFlag)
							break ;
					}
				}
				if (successFlag)
					break ;
			}
			
			if (!successFlag) //score을 찾지 못했음 
				break ;
			score++;
		}
		return (score-1);
	}
	
	int Fitness_BFS(string& scorestr, int i, int j, int idx) {
		if (idx == scorestr.size())
			return (1);
		
		int flag = 0;
		if (i>0 && gene[i-1][j] == scorestr[idx]) //상 
			flag = flag | Fitness_BFS(scorestr, i-1, j, idx+1);
		if (i<R-1 && gene[i+1][j] == scorestr[idx]) //하 
			flag = flag | Fitness_BFS(scorestr, i+1, j, idx+1);
		if (j>0 && gene[i][j-1] == scorestr[idx]) //좌 
			flag = flag | Fitness_BFS(scorestr, i, j-1, idx+1);
		if (j<C-1 && gene[i][j+1] == scorestr[idx]) //우 
			flag = flag | Fitness_BFS(scorestr, i, j+1, idx+1);
		if ((i>0 && j>0) && gene[i-1][j-1] == scorestr[idx]) //좌상 
			flag = flag | Fitness_BFS(scorestr, i-1, j-1, idx+1);
		if ((i>0 && j<C-1) && gene[i-1][j+1] == scorestr[idx]) //우상 
			flag = flag | Fitness_BFS(scorestr, i-1, j+1, idx+1);
		if ((i<R-1 && j>0) && gene[i+1][j-1] == scorestr[idx]) //좌하 
			flag = flag | Fitness_BFS(scorestr, i+1, j-1, idx+1);
		if ((i<R-1 && j<C-1) && gene[i+1][j+1] == scorestr[idx]) //우하 
			flag = flag | Fitness_BFS(scorestr, i+1, j+1, idx+1);
		
		return (flag);
	}
	
	void output() {
		cout << "Fitness: " << Fitness() << "\n";
		for(int i=0; i<R; i++)
			cout << gene[i] << "\n";
		cout << "\n";
	}
}; 


/**
	- 우수한 개체를 다음 세대의 부모를 선택
	=> 일단 부모는 2개로
*/
int cmp(Solution& s1, Solution& s2) {
	return (s1.Fitness() > s2.Fitness());
}
vector<Solution> selectExcellentParent(vector<Solution>* cursv) {
	sort(cursv->begin(), cursv->end(), cmp);
	
	int psize = 2;
	vector<Solution> nextParent;
	for(int i=0; i<psize; i++) {
		nextParent.push_back((*cursv)[i]);
	}
	return (nextParent);
}

/**
	- 부모로 자식 생성 (돌연변이 포함)
	생성한 자식은 dest에 할당하기

	1. Uniform cross over
		결과 : 
	
	2. PMX
		결과 : 

	3. AdAdjacency Preserving Crossover
		결과 : 
*/
void generateChild(vector<Solution>* dest, vector<Solution>* parent, int genes) {
	int gi = 0;
	while(gi < genes) {
		//1. Uniform CrossOver
		string tmpr[R];
		for(int i=0; i<R; i++) {
			string tmpc;
			for(int j=0; j<C; j++) {
				int rv = genRandom(parent->size()-1); //랜덤값 생성
				tmpc += (*parent)[rv].gene[i][j];
			}
			tmpr[i] = tmpc;

			//돌연변이?
		}
		cout << endl; //debug
		Solution news = Solution(tmpr);
		news.output(); //debug
		dest->push_back(news);
		gi++;
	}
}

/**
 * @param{generation} : 세대 수 (Loop 수)
 * @param{genes} : 1세대 당 유전자의 수 
 * @return : 최고 점수 
*/
int GeneticAlgorithm(int generation, int genes) {
	// 1. genes 만큼 초기해 생성
	vector<Solution> cursv;
	for(int i=0; i<genes; i++) {
		Solution tmp;
		tmp.initSol();
		cursv.push_back(tmp);
		tmp.output(); //debug
	}

	int gi = 0;
	while (gi < generation) {
		cout << "GEN" << gi << endl; //debug
		// 2. 우수한 개체를 다음 세대의 부모로 선택
		vector<Solution> parent = selectExcellentParent(&cursv);
		// 3. Cross over : 두 부모로 새로운 자식 염색체 생성
		// 이거 cursv에 넣을거임
		cursv.clear(); //기존 자식 삭제
		generateChild(&cursv, &parent, genes);
		gi++;
	}

	// 마무리 : 가장 높은 상위 10개를 출력.
	
	cout << "RESULT" << endl; //debug
	for(int i=0; i<genes; i++)
		cursv[i].output();
	
	return (0);
} 

int main() {

	int generation = 10; //100 세대 진행
	int genes = 10; //100개의 유전자들
	
//	string tmp[R];
//	for(int i=0; i<R; i++)
//		cin >> tmp[i];
//	Solution tmpsol = Solution(tmp);
//	tmpsol.output();
	
	int res = GeneticAlgorithm(generation, genes); 
	
	return (0);
}
