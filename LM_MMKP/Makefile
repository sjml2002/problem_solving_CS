CXX := g++
CXXFLAGS := -std=c++17 -Wall -O2

OBJ_DIR := obj
COMMON_OBJS := $(OBJ_DIR)/main.o $(OBJ_DIR)/dataIO.o

.PHONY: all original dp clean

all: original

# make original -> solution.cpp 사용
original: mmkp_original

mmkp_original: $(COMMON_OBJS) $(OBJ_DIR)/solution.o
	$(CXX) $(CXXFLAGS) $^ -o $@

# make dp -> solution_dp.cpp 사용
dp: mmkp_dp

mmkp_dp: $(COMMON_OBJS) $(OBJ_DIR)/solution_dp.o
	$(CXX) $(CXXFLAGS) $^ -o $@

$(OBJ_DIR)/%.o: %.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR) mmkp_original mmkp_dp