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
			//<a,b>를 잇는 간선이라면 map[a]와 map[b]에 둘다 저장하기
			// ㄴ> 근데 이거 이미 파일에 잘 되어있어서 굳이 안해도될듯? 
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
			//2. 좌표인데 여기 걍 넘어갑시다.
			while (line[li] != ' ')
				li++;
			while (line[li] == ' ')
				li++;
			//3. 간선의 수도 셀 필요 없으니까 걍 넘어가
			while (line[li] != ' ')
				li++;
			while (line[li] == ' ')
				li++;
			//4. 간선들 세기
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
