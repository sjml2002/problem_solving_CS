CXX := g++
CXXFLAGS := -std=c++17 -Wall -O2

OBJ_DIR := obj
COMMON_OBJS := $(OBJ_DIR)/dataIO.o

.PHONY: all original fpls dp clean

all: original fpls

# make original -> solution.cpp (DROP/ADD repair heuristic)
original: mmkp_original

mmkp_original: $(COMMON_OBJS) $(OBJ_DIR)/main.o $(OBJ_DIR)/solution.o
	$(CXX) $(CXXFLAGS) $^ -o $@

# make fpls -> fpls.cpp (Feasibility-Pursuing Lagrangian Search)
fpls: mmkp_fpls

mmkp_fpls: $(COMMON_OBJS) $(OBJ_DIR)/main_fpls.o $(OBJ_DIR)/fpls.o
	$(CXX) $(CXXFLAGS) $^ -o $@

# make dp -> solution_dp.cpp (kept for compatibility with existing workflow)
dp: mmkp_dp

mmkp_dp: $(COMMON_OBJS) $(OBJ_DIR)/main.o $(OBJ_DIR)/solution_dp.o
	$(CXX) $(CXXFLAGS) $^ -o $@

$(OBJ_DIR)/%.o: %.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR) mmkp_original mmkp_fpls mmkp_dp