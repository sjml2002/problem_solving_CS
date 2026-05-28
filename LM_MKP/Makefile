# 컴파일러 및 옵션
CXX      := g++
CXXFLAGS := -std=c++11 -O2 -Wall -Wextra

# 타겟
TARGET_ORIGINAL := fpls_original
TARGET_DP       := fpls_dp

# 공통 소스 (입출력)
OBJS_COMMON := dataIO.o

# original FPLS
OBJS_ORIGINAL := original_FPLS.o main.o

# DP+FPLS
OBJS_DP := original_FPLS.o dp_FPLS.o main_dp.o

.PHONY: all clean original dp

all: original dp

# ---------- original FPLS 빌드 ----------
original: $(TARGET_ORIGINAL)

$(TARGET_ORIGINAL): $(OBJS_COMMON) $(OBJS_ORIGINAL)
	$(CXX) $(CXXFLAGS) -o $@ $^

# ---------- DP+FPLS 빌드 ----------
dp: $(TARGET_DP)

$(TARGET_DP): $(OBJS_COMMON) $(OBJS_DP)
	$(CXX) $(CXXFLAGS) -o $@ $^

# ---------- 공통 컴파일 규칙 ----------
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ---------- 헤더 의존성 ----------
dataIO.o:        dataIO.cpp        dataIO.h
original_FPLS.o: original_FPLS.cpp original_FPLS.h dataIO.h
dp_FPLS.o:       dp_FPLS.cpp       dp_FPLS.h       original_FPLS.h dataIO.h
main.o:          main.cpp          original_FPLS.h  dataIO.h
main_dp.o:       main_dp.cpp       dp_FPLS.h        original_FPLS.h dataIO.h

clean:
	rm -f *.o $(TARGET_ORIGINAL) $(TARGET_DP)