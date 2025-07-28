#include "inputData_read.h"
using namespace std;

/*
	Max-cut Problem �� Tabu search�� ������ �ڵ��Դϴ�. 
	1. �̹� inputdata�� ������ ����ġ�� ���� �־��� ���� �ʱ⿡
		�� ������ �մ� �ִ� ������ ���ϴ� ������ ���� 
*/

///// ��� ����
const int Testcase = 200;
map<int, vector<int>> graphData; //input graph data : <a,b> = a��b�� ���� 
/////

class Solution {
public:
	//�κ� �׷����� ��Ÿ�� (binary vector)
	//pg[i]=='0'�̸� i=v1�׷�, =='1'�̸� i=v2�׷� 
	string pg; //partition graph
	set<pair<int, int>> cutl; //v1,v2�� ���ܼ�<a,b>�� list
	Solution() {}
	Solution (string pgstr, set<pair<int, int>> l) {
		this->pg = pgstr;
		this->cutl = l;
	}
	bool operator ==(const Solution& s1) const {
		return (this->cutl == s1.cutl); //���ܼ��� ������ Ȯ�� 
	}
	
	/* �ڽ��� ���ϴ� �Լ� (int������ �� ������ ��ȯ)
		�򰡿� ������ ��ġ�� �׸��� ������ ����. 
		1. ���ܼ��� ũ�Ⱑ ���� �߿��� �� ����
		2. �׷��� ũ��? �� �߿��Ҷ� 
	*/
	int evaluation() {
		//1. ���ܼ��� ũ�Ⱑ Ŭ ���� ���� ���� 
		int p1 = this->cutl.size();
		//2. �׷��� ũ�Ⱑ �����Ҽ��� ���� ���� 100 - ����
		//	���ܼ� ũ�Ⱑ ���� �� ũ�⸦ �����ϰ� ���ߴ� ���̹Ƿ� ���� ����� �۰���
		int dif = 0; //0�̸� --, 1�̸� ++
		for(int i=1; i<pg.size(); i++)
			pg[i]=='0' ? dif-- : dif++;
		int p2 = max(0, 10 - abs(dif));
		
		//cout << pg << ": " << p1 << ", " << p2 << "\n"; //debug
		
		int totalScore = p1 + p2;
		return (totalScore);
	}
	
	/* �ڽŰ� �̿��ϴ� �� ����
		currentSolution�� binaryVector���� �� �ϳ��� �ٲ� ���� ������ 
	*/
	vector<Solution> generate_neighborSolution() {
		vector<Solution> ns; //neighborSolution 
		for(int i=1; i<pg.size(); i++) {
			//1. v�� �̿��� ����
			if (pg[i] == '0')
				pg[i] = '1';
			else
				pg[i] = '0';
			//2. ���ҵ� �κ� �׷��� ���ܼ� �� ���ؼ� cutl�� ����ֱ�
			/*2-1. a�� ������ �ٸ� �׷����� �����Ƿ�
				���� cutl �� a�� ����� ������ �� ����� 
				cutl�� ���� graphData �� a�� ����� ������ �� �߰��ϸ� ��. 
			*/
			set<pair<int, int>> tmpcutl = cutl;
			for(int j=0; j<graphData[i].size(); j++) {
				int a = i;
				int b = graphData[i][j];
				if (a > b) //������ a<=b �̵���
					swap(a, b); 
				pair<int, int> l = make_pair(a,b); //���� l
				if (tmpcutl.count(l)) //cutl�� �����ϹǷ� �����
					tmpcutl.erase(l);
				else //cutl�� ���� �߰� 
					tmpcutl.insert(l);
			}
			ns.push_back(Solution(pg, tmpcutl));
			//3. �ٽ� ������� ����
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
		os << "�κ� �׷���1: ";
		for(int i=1; i<pg.size(); i++) {
			if (pg[i] == '0')
				os << i << " ";
		}
		os << "\n�κ� �׷���2: ";
		for(int i=1; i<pg.size(); i++) {
			if (pg[i] == '1')
				os << i << " ";
		}
		os << "\n���ܼ�: ";
		for(auto it=cutl.begin(); it != cutl.end(); it++) {
			os << "(" << it->first << "," << it->second << ") , ";
		}
		os << "\n-------------------------- \n";
	}
	
	void outputEval(ofstream& os) {
		//os << "����: " << this->evaluation() << "\n";
		os << "���ܼ� ũ��: " << this->cutl.size() << "\n";
	}
};
vector<Solution> TabuList;
Solution maxcut; //���� solution �� ���� ����(����) �� 

// Solution�� Tabu�� �����ϴ°�? (�����ϸ� return 1) 
int isInTabuList(Solution s) {
	for(int i=0; i<TabuList.size(); i++) {
		if (TabuList[i] == s) {
			//��� ���� �߰�
			//1. ���� �����غ��ٵ� �� ������? 
			if (maxcut.evaluation() < s.evaluation())
				continue;
			else
				return (1); //Tabulist�� �ɸ� 
		}
	}
	return (0);
}

void TabuSearch_main(ofstream& os) {
	auto endit = graphData.end(); 
	endit--;
	int maxn = endit->first; //������ �ִ� 
	
	// 1. Generate first solution
	Solution currentSolution;
	// ���� ó�� ������ ���� �߿� 1�� �̻� ����Ǿ��ִ� �������� ���� 
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
	//a�� ����� ��� ������ ���ܼ����� ���� 
	for(int i=0; i<graphData[a].size(); i++)
		currentSolution.cutl.insert(make_pair(a, graphData[a][i]));
	
	//2. (Start) TabuSearch generate neighborSolution Start
	int t = 1;
	while (t <= Testcase) { //t <= Testcase
		cout << t << "��° ����\n"; //debug
		
		os << t << "��° ����: ";
		currentSolution.outputEval(os); //debug
		
		//2-1. neighbor solution ����
		vector<Solution> neighborSolution = currentSolution.generate_neighborSolution();
		//2-2. neighbor solution �߿��� Tabu�� �ִ°� ���� ��
		// 	���� ������ ���� current Solution���� ���� 
		for(int i=0; i<neighborSolution.size(); i++) {
			if (!isInTabuList(neighborSolution[i])) {
				if (currentSolution.evaluation() < neighborSolution[i].evaluation())
					currentSolution = neighborSolution[i];
				//���⼭ TabuList�� �߰��ص� �ɵ� 
			}
		}
		//2-3. Tabulist�� �߰�
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
		//�ʱ� ����
		string filename = fileList[fli];
		cout << filename << "����\n";
		string inputfilename = "InputData/" + filename;
		graphData = inputFile_to_graph(inputfilename);
		string outputfilename = "TabuSearch_" + filename + "_output.txt";
		cout << "�������: " << outputfilename << "\n";
		ofstream os(outputfilename.data());
		if (!os.is_open()) {
			cout << "������� ������ ����";
			return (0);
		}
		
		TabuSearch_main(os);
		
		//���� maxcut ���
		maxcut.output(os);
		os.close();
		
		//�ʱ�ȭ
		TabuList.clear();
		maxcut = Solution();
	}
	
	return (0);
}
