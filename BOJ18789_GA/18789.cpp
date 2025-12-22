#include "libct.h"
# define R 8
# define C 14
# define STARTNUM 1000
# define MAXNUM 9999
# define PSIZE 2
# define MAXSAME 25
using namespace std;
typedef long long int ll;

/*
	0. 이번에는 기존의 점수를 계산하는 유일한 길을 줄이는 방법 대신 다른 방법으로 풀 계획

	1. 점수 계산은 1~9999 까지 그대로 가져간다.

	2. Mutation의 방법을 2가지로 한다.
	2-1. 0 <= x,y < 10 , x!=y 에 대해서 모든 x,y의 위치를 변경한다.
		N번 반복했음에도 최고 점수가 변하지 않을 때 수행
	2-2. 각 위치에 있는 원소에서 M%의 확률로 다른 수로 변이 한다.
		단, 2-1이 실행될 때는 2-2가 실행되지 않는다.


	3. 나머지의 parent selection이나, cross over 같은 부분은
		세간에 나와있는 일반적인 방법을 사용한다.
		(Roulette Wheel 방식, matrix cross over 방식 채택)
	
	4. 최적화를 위해 threading 방식을 고안한다.
*/

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


/**
 * sort cmp
 */
int sortcmp(Solution& s1, Solution& s2) {
	if (s1.Fitness() == s2.Fitness())
		return (s1.fitscore > s2.fitscore);
	return (s1.Fitness() > s2.Fitness());
}

vector<Solution> bestsol;
/**
 * best sol은 업데이트 함
 */
void updateBest(vector<Solution>& bestsol, vector<Solution>& cursv) {
	map<int, Solution> nextbs; //next bestsolution <-Fitness, Solution>
	//내림차순 정렬을 위해 Fitness는 내림차순 정렬을 진행한다.
	for(int i=0; i<bestsol.size(); i++) {
		nextbs.insert(make_pair(-bestsol[i].Fitness(), bestsol[i]));
	}
	for(int i=0; i<cursv.size(); i++) {
		int key = -cursv[i].Fitness();

		auto it = nextbs.find(key);
		auto endit = --nextbs.end();
		//중복되지 않았고 현재 가장 작은 점수보다 높으므로 insert
		if (it == nextbs.end() && -key > -(endit->first)) { 
			nextbs.insert(make_pair(key, cursv[i]));
			nextbs.erase(endit); //best sol에서 탈락됨.
		}
	}

	bestsol.clear();
	for(auto it=nextbs.begin(); it != nextbs.end(); it++) {
		bestsol.push_back(it->second);
	}
}


/**
 * Selection
 * 우수한 개체를 다음 세대의 부모를 선택
 * 1. Rank-Based Selection
*/
vector<Solution> selectExcellentParent(vector<Solution>* cursv, int bssize) {
	sort(cursv->begin(), cursv->end(), sortcmp);

	// 1. Rank-Based Selection
	int maxf = (*cursv)[0].Fitness();
	int minf = cursv->back().Fitness();

	int sumf = 0;
	int elitesumf = 0;
	vector<pair<int, int>> RankFitness; //<originIndex, RankFitness>
	int csize = cursv->size();
	for(int i=0; i<cursv->size(); i++) {
		int f = maxf + (i*(minf-maxf) / (csize-1));
		RankFitness.push_back(make_pair(i, f));
		sumf += f;
		if (i < 5)
			elitesumf += f;
	}

	vector<Solution> nextParent;
	ll precision = 1000000;
	/**
	 *  1-1. selection with Probability
	 * 		(using Roulette Wheel)
	 * 		but, the first parent is Elite(top5 in cursv)
	*/
	//		
	for(int i=0; i<PSIZE; i++) {
		ll rwp = genRandom(sumf);
		if (i <= 1) //number of Elite
			rwp = genRandom(elitesumf);

		ll sump = 0;
		int ri=0;
		while(ri < RankFitness.size()) {
			int f = RankFitness[i].second;
			ll fp = (ll)f*precision;
			int p = fp / (ll)sumf;

			sump += p;
			if (sump >= rwp)
				break ;
			ri++;
		}
		cout << "Selection: " << ri << "\n"; //debug
		nextParent.push_back((*cursv)[ri]);
		RankFitness.erase(RankFitness.begin() + ri);
	}
	return (nextParent);
}



/**
	CrossOver

	- 부모로 자식 생성 (돌연변이 포함)
	생성한 자식은 dest에 할당하기
	=> Adjacency Preserving Crossover 비슷하게

	1. 현재 숫자는 주변 8개의 숫자랑 다를 수록 확률이 높아짐.
		0~9까지 숫자에서 상한선을 정해서 랜덤값 생성한 다음에 가장 높은거 선택 ㄱㄱ 
	1-1. 단 자신과 같다면 상한선 조금 높게 
		=> Failed. 200 이상 넘지가 않는다.

	2. 현재 최상의 부모에서 하나만 선택해서 변경
		=> Failed. 2300 점을 넘지 않음.
		=> 그리고 애초에 cross over가 아님.
	
	3. 최상의 2개의 부모에서 One-point crosover
	3-1. (TODO 251011) 지금 child가 parent.size()*20 만큼 생겨나니까
		상한선 genes로 두고 sort하던지 하는거 해야할듯
	3-2. 계속 같은 자식을 생성하기 때문에 Fitness가 동일한거는 제외해야할듯
*/

//dest안의 Solution과 중복인 cur이 존재하는지
int findsameFitness(vector<Solution>* dest, Solution* cur) {
	for(int i=0; i<dest->size(); i++) {
		if ((*dest)[i].Fitness() == cur->Fitness())
			return (1);
	}
	return (0);
} 

void generateChild(vector<Solution>* dest, vector<Solution>* parent, int genes) {
	int mp = 18; //(mp/10)% 확률로 돌연변이

	//pi 부모와 pj 부모의 조합으로 crossover
	for(int pi=0; pi<parent->size()-1; pi++) {
		for(int pj=pi+1; pj<parent->size(); pj++) {
			// 3-1. Row based one-point crossover
			for(int ci=1; ci<R; ci++) { //ci 미만의 행은 parent[0] 에게서, ci 이상의 행은 parent[1] 에게서
				Solution pb;
				for(int i=0; i<R; i++) {
					if (i < ci)
						pb.gene[i] = (*parent)[pi].gene[i];
					else
						pb.gene[i] = (*parent)[pj].gene[i];
					
					// Mutation
					for(int j=0; j<C; j++) {
						int rv = genRandom(1000);
						if (rv <= mp) {
							int sn = genRandom(9);
							pb.gene[i][j] = (char)(sn+48);
						}
					}
				}
				if (!findsameFitness(dest, &pb)) //pb가 중복 아닐 때 자식 선택
					dest->push_back(pb);
			}
			
			// 3-2. Column based one-point crossover
			for(int cj=1; cj<C; cj++) { //cj 미만의 열은 parent[0] 에게서, cj 이상의 열은 parent[1] 에게서
				Solution pb;
				for(int j=0; j<C; j++) {
					//열 복사하려면 for문 한번 더 돌아야됨.
					for(int i=0; i<R; i++) {
						if (j < cj)
							pb.gene[i].push_back((*parent)[pi].gene[i][j]);
						else
							pb.gene[i].push_back((*parent)[pj].gene[i][j]);
						
						//Mutation
						int rv = genRandom(1000);
						if (rv <= mp) {
							int sn = genRandom(9);
							pb.gene[i][j] = (char)(sn+48);
						}
					}
				}
				if (!findsameFitness(dest, &pb)) //pb가 중복 아닐 때 자식 선택
					dest->push_back(pb);
			}
		}
	}
}




/**
 * 수 0 <= a,b < 10 에 해당하는 모든 위치를 바꾸기
 * Fitness계산을 무조건 해줘야합니다...
 * @param{a, b}
 * @param{sol} : 바꿀 대상 Solution
 */
void permutation(int a, int b, Solution& sol) {
	char ca = (char)a + 48;
	char cb = (char)b + 48;

	for(int r=0; r<R; r++) {
		for(int c=0; c<C; c++) {
			if (sol.gene[r][c] == ca)
				sol.gene[r][c] = cb;
			else if (sol.gene[r][c] == cb)
				sol.gene[r][c] = ca;
		}
	}

	//Fitness 재 계산
	sol.fitness = -1;
	sol.Fitness();
}


/**
 * @param{generation} : 세대 수 (Loop 수)
 * @param{genes} : 1세대 당 유전자의 수 
 * @return : 최고 점수 
*/
int GeneticAlgorithm(int ti, int generation, int genes, int bssize) {
	// 1. 초기해 (PSIZE 개)
	vector<Solution> cursv;
	int randomSize = 0;
	for(int i=0; i<PSIZE-randomSize; i++)
		cursv.push_back(bestsol[i]);
	// // 1-1. 초기해 랜덤
	// for(int i=0; i<randomSize; i++) {
	// 	Solution tmp;
	// 	tmp.initSol();
	// 	cursv.push_back(tmp);
	// }


	//같은 해가 몇번동안 반복되는지 <fitness, counting>
	pair<int, int> samebest = make_pair(bestsol[0].Fitness(), 0);
	
	int gi = 0;
	while (gi < generation) {
		// 2. 우수한 개체를 다음 세대의 부모로 선택
		vector<Solution> parent = selectExcellentParent(&cursv, bssize);

		cout << ti << " - GEN" << gi << ": " << parent[0].Fitness() << "(" << parent[0].fitscore << ") /";
		cout << bestsol[0].Fitness() << "(" << bestsol[0].fitscore << ")" << endl; //debug : 현재 세대 Fitness


		// 3. Cross over : 두 부모로 새로운 자식 염색체 생성
		cursv.clear(); //기존 자식 삭제
		generateChild(&cursv, &parent, genes);

		// 3-1. best Solution 판단
		updateBest(bestsol, cursv);

		if (bestsol[0].fitscore >= 8140) {
			return (1);
		}


		// 4. 같은 해 반복 판별
		if (samebest.first == bestsol[0].Fitness()) {
			samebest.second++;
			if (samebest.second >= MAXSAME) { //permutation
				cout << "permutation!\n"; //debug
				for(int i=0; i<bestsol.size(); i++) {
					int a = 0;
					int b = 0;
					while (a == b) {
						a = genRandom(9);
						b = genRandom(9);
					}
					permutation(a, b, bestsol[i]); //a와 b숫자의 모든 위치를 swap
				}
				sort(bestsol.begin(), bestsol.end(), sortcmp);
				samebest = make_pair(bestsol[0].Fitness(), 0);
			}
		}
		else { //0번으로 초기화
			samebest = make_pair(bestsol[0].Fitness(), 0);
		}

		gi++;
	}
	//마지막 한번 더
	updateBest(bestsol, cursv); // best Solution 판단
	
	return (0);
}


int main() {
	int testcase = 200;
	int ti = 0;
	while (ti < testcase) {
		bestsol.clear();
		cout << "========= testcase " << ti << "=========" << endl;

		int generation = 1000; // generation LOOP
		int genes = 100; //number of gene
		int bestsolsize = PSIZE;

		// 0. init bestsol (Prev GA's best)
		ifstream ifs("./best.txt");
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

		int res = GeneticAlgorithm(ti, generation, genes, bestsolsize);

		// write best solution in "best.txt"
		ofstream ofs("./best.txt");
		if (ofs.is_open()) {
			for(int i=0; i<bestsol.size(); i++) {
				bestsol[i].output(ofs);
			}
			ofs.close();
		}
		ti++;
	}
	
	return (0);
}
