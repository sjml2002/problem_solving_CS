#include "libct.h"
# define R 8
# define C 14
# define PSIZE 3 //부모의 총 개수
# define MAXNUM 8140
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
	int fitscore = -1;
	vector<vector<pair<int, int>>> possibleRepresent[MAXNUM+1]; //[score][path][coord]
	int uniquePathCnt = 0;
	double upc = 0.01;

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
		while (1) {
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
			
			if (possibleRepresent[score].size() == 1)
				uniquePathCnt++;
			
			if (!flag) //score을 찾지 못했음 
				break ;
			score++;
		}

		this->fitscore = score-1;
		this->fitness = score-1 - (upc * uniquePathCnt);
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
/**
 * best sol은 업데이트 함
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
 * Selection
 * 우수한 개체를 다음 세대의 부모를 선택
*/
vector<Solution> selectExcellentParent(vector<Solution>* cursv, int bssize) {
	sort(cursv->begin(), cursv->end(), sortcmp);

	vector<Solution> nextParent;
	for(int i=0; i<PSIZE; i++) {
		nextParent.push_back((*cursv)[i]);
	}
	return (nextParent);
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
		int flag = 0;
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
			rv = genRandom(1000/flag); //flag=2 : 500 , flag=3 : 333
		
		if (rv > maxv) {
			maxv = rv;
			maxnum = num;
		}
	}
	return (maxnum);
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

// //dest안의 Solution과 중복인 cur이 존재하는지
// int findsameFitness(vector<Solution>* dest, Solution* cur) {
// 	for(int i=0; i<dest->size(); i++) {
// 		if ((*dest)[i].Fitness() == cur->Fitness())
// 			return (1);
// 	}
// 	return (0);
// } 

void generateChild(vector<Solution>* dest, vector<Solution>* parent, int genes) {
	int mp = 1; //mp% 확률로 돌연변이

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
						int rv = genRandom(100);
						if (rv <= mp) {
							int sn = selectNum(pb.gene, i, j);
							pb.gene[i][j] = (char)(sn+48);
						}
					}
				}
				//if (!findsameFitness(dest, &pb)) //pb가 중복인지 확인
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
						int rv = genRandom(100);
						if (rv <= mp) {
							int sn = selectNum(pb.gene, i, j);
							pb.gene[i][j] = (char)(sn+48);
						}
					}
				}
				//if (!findsameFitness(dest, &pb)) //pb가 중복인지 확인
					dest->push_back(pb);
			}
		}
	}

	// //dest의 개수를 상위 genes로 줄이기
	// if (dest->size() > genes) {
	// 	sort(dest->begin(), dest->end(), sortcmp);
	// 	dest->resize(genes);
	// }
	// else if (dest->size() < genes) { //기존의 부모들로 dest의 개수를 genes로 채우기
	// 	int pi = 0;
	// 	for(int gi=dest->size(); gi < genes; gi++) {
	// 		dest->push_back((*parent)[pi++]);
	// 	}
	// }
}

/**
 * @param{generation} : 세대 수 (Loop 수)
 * @param{genes} : 1세대 당 유전자의 수 
 * @return : 최고 점수 
*/
int GeneticAlgorithm(int ti, int generation, int genes, int bssize) {
	// 1. 초기해 (bestsol, bestsol2)
	vector<Solution> cursv;
	int randomSize = 1;
	for(int i=0; i<PSIZE-randomSize; i++)
		cursv.push_back(bestsol[i]);
	// 1-1. 초기해 랜덤
	for(int i=0; i<randomSize; i++) {
		Solution tmp;
		tmp.initSol();
		cursv.push_back(tmp);
	}


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
		gi++;
	}
	//마지막 한번 더
	updateBest(bestsol, cursv); // best Solution 판단
	/** 
		만약, best.txt에 기록되는 해를 더 늘리고 싶다면
		현재 bestsol.size() 인덱스의 cursv를 넣으면 됨. 	
	**/ 
	//bestsol.push_back(cursv[0]);
	
	//이번 프로그램에서 가장 좋았던 5개 출력
	// (TODO 251011) 해의 동일성 및 Fitness 수렴도
	sort(cursv.begin(), cursv.end(), sortcmp);
	cout << endl << "RESULT" << endl; //debug
	for(int i=0; i<5; i++) {
		cout << i << ": ";
		cursv[i].output(cout);
	}
	
	return (0);
}


int main() {
	int testcase = 25;
	int ti = 0;
	while (ti < testcase) {
		cout << "========= testcase " << ti << "=========" << endl;

		int generation = 4000; // generation LOOP
		int genes = 100; //number of gene
		int bestsolsize = 2;

		//임시 검증
	//	string tmp[R];
	//	for(int i=0; i<R; i++)
	//		cin >> tmp[i];
	//	Solution tmpsol = Solution(tmp);
	//	tmpsol.output(cout);

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
