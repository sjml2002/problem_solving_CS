#include "inputData_read.h"

map<int, vector<int>> graphData; //input graph data : [a]�� ����� ���� vector�� 

int main() {
	string filename = "testcase1.txt";
	string inputfilename = "InputData/" + filename;
	graphData = inputFile_to_graph(inputfilename);
	
	auto endit = graphData.end(); 
	endit--;
	int maxn = endit->first; //������ �ִ�
	
	for(int i=1; i<=maxn; i++)
		cout << i << "\n";
	
	for(auto it=graphData.begin(); it != graphData.end(); it++) {
		for(int i=0; i<it->second.size(); i++)
			cout << it->first << " " << it->second[i] << "\n";
		
	}
	
	return (0);
}
