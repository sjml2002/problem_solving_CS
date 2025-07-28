#ifndef INPUTDATA_READ_H
#define INPUTDATA_READ_H


#include <fstream>
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <stdlib.h>
using namespace std;

int ctoi(char c) {
	return ((int)c - 48);
}


map<int, vector<int>> inputFile_to_graph(string filename) {
	map<int, vector<int>> graphData;
	
	ifstream f(filename.data());
	if (f.is_open()) {
		string line;
		while (getline(f, line)) {
			//<a,b>�� �մ� �����̶�� map[a]�� map[b]�� �Ѵ� �����ϱ�
			// ��> �ٵ� �̰� �̹� ���Ͽ� �� �Ǿ��־ ���� ���ص��ɵ�? 
			int li = 0;
			while (line[li] == ' ')
				li++;
			//1. a
			int a = 0;
			while (line[li] != ' ') {
				a = a*10 + ctoi(line[li]);
				li++;
			}
			while (line[li] == ' ')
				li++;
			//2. ��ǥ�ε� ���� �� �Ѿ�ô�.
			while (line[li] != ' ')
				li++;
			while (line[li] == ' ')
				li++;
			//3. ������ ���� �� �ʿ� �����ϱ� �� �Ѿ
			while (line[li] != ' ')
				li++;
			while (line[li] == ' ')
				li++;
			//4. ������ ����
			vector<int> bv;
			while (li < line.size()) {
				int b = 0;
				while (line[li] != ' ' && li < line.size()) {
					b = b*10 + ctoi(line[li]);
					li++;
				}
				bv.push_back(b);
				
				while (line[li] == ' ' && li < line.size())
					li++;
			}
			
			graphData[a] = bv;
		}
		f.close();
	}
	return (graphData);
}

#endif
