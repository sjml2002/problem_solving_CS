#include "libct.h"
# define R 8
# define C 14
using namespace std;

/*
    하나의 solution을 가지고 brute-force로 숫자 하나씩 바꿔보기
*/


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

/**
 * sort cmp
 */
int sortcmp(Solution& s1, Solution& s2) {
	return (s1.Fitness() > s2.Fitness());
}

Solution bestsol;



/**
 * 바로 옆에 있는 숫자와 같은 숫자가 되도록 유도
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
 * 현재 최상의 부모에서 하나만 선택해서 변경
 */
void generateChild(vector<Solution>* dest, Solution* parent) {
    Solution origin = Solution(parent->gene);

    for(int num=0; num<10; num++) {
        for(int i=0; i<R; i++) {
            for(int j=0; j<C; j++) {
                char originnum = origin.gene[i][j];
                char newnum = (int)(num + 48);
                origin.gene[i][j] = newnum;
                dest->push_back(origin);
                origin.gene[i][j] = originnum;
            }
        }
    }
}


/**
 * 메인
 */
/**
 * @param{generation} : 세대 수 (Loop 수)
 * @param{genes} : 1세대 당 유전자의 수 
 * @return : 최고 점수 
*/
int bruteForceMain(int generation, int genes) {
	vector<Solution> cursv;
    cursv.push_back(bestsol);

	int gi = 0;
	while (gi < generation) {
		// 1. 우수한 개체를 다음 세대의 부모로 선택
        sort(cursv.begin(), cursv.end(), sortcmp);
        Solution parent = cursv[0];
        // 1-1. best Solution 판단
		if (parent.Fitness() > bestsol.Fitness())
            bestsol = parent;
		
		cout << " - GEN" << gi << ": " << parent.Fitness() << " / " << bestsol.Fitness() << endl; //debug : 현재 세대 Fitness

		// 2. Cross over : 두 부모로 새로운 자식 생성
		cursv.clear(); //기존 자식 삭제
		generateChild(&cursv, &parent);

		gi++;
	}
	//마지막 한번 더
    sort(cursv.begin(), cursv.end(), sortcmp);
    if (cursv[0].Fitness() > bestsol.Fitness())
            bestsol = cursv[0];
	
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
	int testcase = 5;
	int ti = 0;
	while (ti < testcase) {
		cout << "========= testcase " << ti << "=========" << endl;

		int generation = 3000; // generation LOOP
		int genes = 100; //number of gene

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
        getline(ifs, line); //first line is Fitness
        int ri = 0;
        string prevbest[R];
        while (getline(ifs, line)) {
            if (line.size() <= 1)
                break ;
            prevbest[ri] = line;
            ri++;
        }
        bestsol = Solution(prevbest);
        bestsol.output(cout); //debug
		ifs.close();
		
		int res = bruteForceMain(generation, genes);

		// write best solution in "best.txt"
		ofstream ofs("./best_bruteforce.txt");
		if (ofs.is_open()) {
			/** 만약 best.txt에 기록되는 해를 늘리고 싶다면 bestsol.size()로 바꾸기 */
			// for(int i=0; i<bestsolsize; i++) {
			// 	bestsol[i].output(ofs);
			// }
			bestsol.output(ofs);
			ofs.close();
		}
		ti++;
	}
	
	return (0);
}
