#include "inputData_read.h"
using namespace std;

/*
	Max-cut Problem 을 Tabu search로 구현한 코드입니다. 
	1. 이번 inputdata는 간선의 가중치가 따로 주어져 있지 않기에
		두 분할을 잇는 최대 간선을 구하는 것으로 진행 
*/

///// 상수 저장
const int Testcase = 200;
map<int, vector<int>> graphData; //input graph data : <a,b> = a와b를 이음 
/////

class Solution {
public:
	//부분 그래프를 나타냄 (binary vector)
	//pg[i]=='0'이면 i=v1그룹, =='1'이면 i=v2그룹 
	string pg; //partition graph
	set<pair<int, int>> cutl; //v1,v2의 절단선<a,b>의 list
	Solution() {}
	Solution (string pgstr, set<pair<int, int>> l) {
		this->pg = pgstr;
		this->cutl = l;
	}
	bool operator ==(const Solution& s1) const {
		return (this->cutl == s1.cutl); //절단선만 같은지 확인 
	}
	
	/* 자신을 평가하는 함수 (int형으로 평가 점수를 반환)
		평가에 영향을 미치는 항목은 다음과 같다. 
		1. 절단선의 크기가 가장 중요할 거 같고
		2. 그룹의 크기? 도 중요할라나 
	*/
	int evaluation() {
		//1. 절단선의 크기가 클 수록 높은 점수 
		int p1 = this->cutl.size();
		//2. 그룹의 크기가 동일할수록 높은 점수 100 - 차이
		//	절단선 크기가 같을 때 크기를 동일하게 맞추는 것이므로 점수 배분을 작게함
		int dif = 0; //0이면 --, 1이면 ++
		for(int i=1; i<pg.size(); i++)
			pg[i]=='0' ? dif-- : dif++;
		int p2 = max(0, 10 - abs(dif));
		
		//cout << pg << ": " << p1 << ", " << p2 << "\n"; //debug
		
		int totalScore = p1 + p2;
		return (totalScore);
	}
	
	/* 자신과 이웃하는 해 생성
		currentSolution의 binaryVector에서 딱 하나만 바뀐 것이 들어가도록 
	*/
	vector<Solution> generate_neighborSolution() {
		vector<Solution> ns; //neighborSolution 
		for(int i=1; i<pg.size(); i++) {
			//1. v의 이웃해 생성
			if (pg[i] == '0')
				pg[i] = '1';
			else
				pg[i] = '0';
			//2. 분할된 부분 그룹의 절단선 수 구해서 cutl에 집어넣기
			/*2-1. a가 완전히 다른 그룹으로 갔으므로
				기존 cutl 중 a와 연결된 간선을 다 지우고 
				cutl에 없는 graphData 중 a와 연결된 간선을 다 추가하면 됨. 
			*/
			set<pair<int, int>> tmpcutl = cutl;
			for(int j=0; j<graphData[i].size(); j++) {
				int a = i;
				int b = graphData[i][j];
				if (a > b) //무조건 a<=b 이도록
					swap(a, b); 
				pair<int, int> l = make_pair(a,b); //간선 l
				if (tmpcutl.count(l)) //cutl에 존재하므로 지우기
					tmpcutl.erase(l);
				else //cutl에 새로 추가 
					tmpcutl.insert(l);
			}
			ns.push_back(Solution(pg, tmpcutl));
			//3. 다시 원래대로 복구
			if (pg[i] == '0')
				pg[i] = '1';
			else
				pg[i] = '0';
		}
		return (ns);
	}
	
	void output(ofstream& os) {
		os << "\n-------------------------- \n";
		this->outputEval(os);
		os << "부분 그래프1: ";
		for(int i=1; i<pg.size(); i++) {
			if (pg[i] == '0')
				os << i << " ";
		}
		os << "\n부분 그래프2: ";
		for(int i=1; i<pg.size(); i++) {
			if (pg[i] == '1')
				os << i << " ";
		}
		os << "\n절단선: ";
		for(auto it=cutl.begin(); it != cutl.end(); it++) {
			os << "(" << it->first << "," << it->second << ") , ";
		}
		os << "\n-------------------------- \n";
	}
	
	void outputEval(ofstream& os) {
		//os << "점수: " << this->evaluation() << "\n";
		os << "절단선 크기: " << this->cutl.size() << "\n";
	}
};
vector<Solution> TabuList;
Solution maxcut; //역대 solution 중 가장 좋은(최적) 해 

// Solution이 Tabu에 존재하는가? (존재하면 return 1) 
int isInTabuList(Solution s) {
	for(int i=0; i<TabuList.size(); i++) {
		if (TabuList[i] == s) {
			//허용 기준 추가
			//1. 역대 최적해보다도 더 좋은가? 
			if (maxcut.evaluation() < s.evaluation())
				continue;
			else
				return (1); //Tabulist에 걸림 
		}
	}
	return (0);
}

void TabuSearch_main(ofstream& os) {
	auto endit = graphData.end(); 
	endit--;
	int maxn = endit->first; //정점의 최대 
	
	// 1. Generate first solution
	Solution currentSolution;
	// 가장 처음 나오는 정점 중에 1개 이상 연결되어있는 정점으로 ㄱㄱ 
	int a = 0;
	for(auto it=graphData.begin(); it != graphData.end(); it++) {
		if (it->second.empty())
			continue; 
		a = it->first;
		break ;
	}
	string tmpv;
	for(int i=0; i<=maxn; i++) {
		if (a == i) {
			tmpv.append("1");
		}
		else
			tmpv.append("0");
	}
	currentSolution.pg = tmpv;
	//a와 연결된 모든 간선을 절단선으로 놓기 
	for(int i=0; i<graphData[a].size(); i++)
		currentSolution.cutl.insert(make_pair(a, graphData[a][i]));
	
	//2. (Start) TabuSearch generate neighborSolution Start
	int t = 1;
	while (t <= Testcase) { //t <= Testcase
		cout << t << "번째 실행\n"; //debug
		
		os << t << "번째 실행: ";
		currentSolution.outputEval(os); //debug
		
		//2-1. neighbor solution 생성
		vector<Solution> neighborSolution = currentSolution.generate_neighborSolution();
		//2-2. neighbor solution 중에서 Tabu에 있는거 제외 후
		// 	가장 좋은거 다음 current Solution으로 고르기 
		for(int i=0; i<neighborSolution.size(); i++) {
			if (!isInTabuList(neighborSolution[i])) {
				if (currentSolution.evaluation() < neighborSolution[i].evaluation())
					currentSolution = neighborSolution[i];
				//여기서 TabuList에 추가해도 될듯 
			}
		}
		//2-3. Tabulist에 추가
		TabuList.push_back(currentSolution);
		//2-4. maxcut update
		if (maxcut.evaluation() < currentSolution.evaluation())
			maxcut = currentSolution;
		t++;
	}
}

vector<string> fileList = {
	"testcase1.txt",
//	"G500.2.5",
//	"G500.10",
//	"G500.20",
//	"G1000.2.5",
//	"G1000.05",
//	"G1000.10",
//	"G1000.20",
//	"pcart.352",
//	"pcart.702",
//	"pcart.1052",
//	"pcgrid100.20",
//	"pcgrid500.42",
//	"prcart.134",
//	"prcart.554",
//	"U500.05",
//	"U500.10",
//	"U1000.05",
//	"U1000.10",
//	"pgrid100.10",
//	"pgrid500.21",
//	"pgrid1000.20",
};

int main() {
	for(int fli=0; fli<fileList.size(); fli++) {
		//초기 설정
		string filename = fileList[fli];
		cout << filename << "시작\n";
		string inputfilename = "InputData/" + filename;
		graphData = inputFile_to_graph(inputfilename);
		string outputfilename = "TabuSearch_" + filename + "_output.txt";
		cout << "출력파일: " << outputfilename << "\n";
		ofstream os(outputfilename.data());
		if (!os.is_open()) {
			cout << "출력파일 열리지 않음";
			return (0);
		}
		
		TabuSearch_main(os);
		
		//최종 maxcut 출력
		maxcut.output(os);
		os.close();
		
		//초기화
		TabuList.clear();
		maxcut = Solution();
	}
	
	return (0);
}
