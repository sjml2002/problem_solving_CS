#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>
#include <fstream>
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
	int fitness = -1;
	
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
		if (this->fitness != -1)
			return (this->fitness);
		
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
		this->fitness = score-1;
		return (this->fitness);
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
	
	void output(ostream& out) {
		out << "Fitness: " << Fitness() << "\n";
		for(int i=0; i<R; i++)
			out << gene[i] << "\n";
		out << "\n";
	}
};

Solution bestsol;


/**
	- 우수한 개체를 다음 세대의 부모를 선택
	=> 일단 부모는 2개로
*/
int cmp(Solution& s1, Solution& s2) {
	return (s1.Fitness() > s2.Fitness());
}
vector<Solution> selectExcellentParent(vector<Solution>* cursv) {
	sort(cursv->begin(), cursv->end(), cmp);
	
	int psize = 1;
	vector<Solution> nextParent;
	//nextParent.push_back(bestsol);
	for(int i=0; i<cursv->size(); i++) {
		nextParent.push_back((*cursv)[i]);
	}
	return (nextParent);
}

/**
	- 부모로 자식 생성 (돌연변이 포함)
	생성한 자식은 dest에 할당하기
	=> Adjacency Preserving Crossover 비슷하게

	1. 현재 숫자는 주변 8개의 숫자랑 다를 수록 확률이 높아짐.
		0~9까지 숫자에서 상한선을 정해서 랜덤값 생성한 다음에 가장 높은거 선택 ㄱㄱ 
	1-1. 단 자신과 같다면 상한선 조금 높게 
		=> Failed. 200 이상 넘지가 않는다.

	2. 현재 최상의 부모에서 하나만 선택해서 변경
*/
int selectNum(string s[], int i, int j) {
	int maxv = 0;
	int maxnum = 0;
	for(int num=0; num<10; num++) {
		char numstr = (char)(num+48);
		int flag = 0;
		if (i>0 && s[i-1][j] == numstr) //상 
			flag = 1;
		if (i<R-1 && s[i+1][j] == numstr) //하 
			flag = 1;
		if (j>0 && s[i][j-1] == numstr) //좌 
			flag = 1;
		if (j<C-1 && s[i][j+1] == numstr) //우 
			flag = 1;
		if ((i>0 && j>0) && s[i-1][j-1] == numstr) //좌상 
			flag = 1;
		if ((i>0 && j<C-1) && s[i-1][j+1] == numstr) //우상 
			flag = 1;
		if ((i<R-1 && j>0) && s[i+1][j-1] == numstr) //좌하 
			flag = 1;
		if ((i<R-1 && j<C-1) && s[i+1][j+1] == numstr) //우하 
			flag = 1;
		
		int rv;
		if (flag == 1 && numstr == s[i][j])
			rv = genRandom(800); //상한선 900
		else if (flag == 1)
			rv = genRandom(600); //상한선 800
		else
			rv = genRandom(1000); //상한선 1000
		
		if (rv > maxv) {
			maxv = rv;
			maxnum = num;
		}
	}
	return (maxnum);
}
void generateChild(vector<Solution>* dest, vector<Solution>* parent, int genes) {
	string original[R];
	for (int k = 0; k < R; k++)
		original[k] = (*parent)[0].gene[k];
	
	int gi = 0;
	int num = 0;
	while (num < 10) {
	//while(gi < genes) {
		//1. Uniform CrossOver
		for(int i=0; i<R; i++) {
			for(int j=0; j<C; j++) {
				char oc = original[i][j];
				//int num = selectNum((*parent)[0].gene, i, j);
				original[i][j] = (char)(num+48);
				dest->push_back(Solution(original));

				original[i][j] = oc; //다시 원래대로
			}
			//돌연변이?
		}
		//gi++;
		num++;
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
	}

	int gi = 0;
	while (gi < generation) {
		cout << "GEN" << gi << endl; //debug
		// 2. 우수한 개체를 다음 세대의 부모로 선택
		vector<Solution> parent = selectExcellentParent(&cursv);
		// 2-1. best Solution 판단
		if (parent[0].Fitness() > bestsol.Fitness())
			bestsol = parent[0];

		// 3. Cross over : 두 부모로 새로운 자식 염색체 생성
		cursv.clear(); //기존 자식 삭제
		generateChild(&cursv, &parent, genes);
		gi++;
	}


	sort(cursv.begin(), cursv.end(), cmp);
	// best Solution 판단
	if (cursv[0].Fitness() > bestsol.Fitness())
		bestsol = cursv[0];
	
	//이번 프로그램에서 가장 좋았던 5개 출력
	cout << endl << "RESULT" << endl; //debug
	for(int i=0; i<5; i++)
		cursv[i].output(cout);
	
	return (0);
} 

int main() {
	int generation = 3000; // generation LOOP
	int genes = 100; //number of genes

	// //임시 검증
	// string tmp[R];
	// for(int i=0; i<R; i++)
	// 	cin >> tmp[i];
	// Solution tmpsol = Solution(tmp);
	// tmpsol.output(cout);

	// 0. init bestsol (Prev GA's best)
	ifstream ifs("./best.txt");
	if (!ifs.is_open()) {
		cout << "Error opening file" << endl;
		return (1);
	}
	
	string line;
	getline(ifs, line); //first line is Fitness
	int ri = 0;
	string prevbest[R];
	while (getline(ifs, line)) {
		if (line.size() <= 1)
			break ;
		prevbest[ri] = line;
		ri++;
	}
	ifs.close();

	bestsol = Solution(prevbest);
	bestsol.output(cout);
	
	int res = GeneticAlgorithm(generation, genes);

	// write best solution in "best.txt"
	ofstream ofs("./best.txt");
	if (ofs.is_open()) {
		bestsol.output(ofs);
		ofs.close();
	}
	
	return (0);
}
