#include <iostream>
#include <string>
#include <vector>
# define R 8
# define C 14
using namespace std;

/*
	유전 알고리즘으로 풀어보자. 


*/

class Solution {
public:
	string gene[R]; //실제 해 
	
	Solution(string tmp[]) {
		for(int i=0; i<R; i++)
			gene[i] = tmp[i];
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
		return (score);
	}
	
	int Fitness_BFS(string& scorestr, int i, int j, int idx) {
		if (idx >= scorestr.size())
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
		cout << "점수: " << Fitness() << "\n";
		for(int i=0; i<R; i++)
			cout << gene[i] << "\n";
	}
}; 


/**
	- 우수한 부모를 선택
	
*/ 
vector<Solution> selectExcellentParent() {
	
}


/**
	- 부모로 자식 생성 (돌연변이 포함) 
*/
vector<Solution> generateChild(vector<Solution>* parent) {
	
	
	
	
	//자식에서 돌연변이 
}

/**
	@param{generation} : 세대 수 (Loop 수)
	@param{genes} : 1세대 당 유전자의 수 
	@return : 최고 점수 
*/
int GeneticAlgorithm(int generation, int genes) {
	// 1. genes 만큼 초기해 생성 
	
	
	
	// Crossover - 두 부모의 염색체 교환
	
	//인접한 부분이 영향을 끼치기 때문에 PMX 방식이나,
	//Adjacency Preserving Crossover를 해봅시다. 
	
	return (0);
} 

int main() {
	int generation = 100; //100 세대 진행
	int genes = 100; //100개의 유전자들
	
	string tmp[R] = {
		"10203344536473",
		"01020102010201",
		"00000000008390",
		"00000000000400",
		"00000000000000",
		"55600000000089",
		"78900066000089",
		"00000789000077"
	};
	
	Solution tmpsol = Solution(tmp);
	tmpsol.output();
	
	int res = GeneticAlgorithm(generation, genes); 
	
	return (0);
}