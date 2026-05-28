# 컴파일러 및 옵션
CXX      := g++
CXXFLAGS := -std=c++11 -O2 -Wall -Wextra

# 타겟
TARGET_ORIGINAL := fpls_original
TARGET_DP_FPLS  := fpls_dp

# 공통 소스
SRCS_COMMON  := dataIO.cpp main.cpp
OBJS_COMMON  := $(SRCS_COMMON:.cpp=.o)

# original FPLS 전용 소스
SRCS_ORIGINAL := original_FPLS.cpp
OBJS_ORIGINAL := $(SRCS_ORIGINAL:.cpp=.o)

# DP+FPLS 전용 소스
SRCS_DP  := dp_FPLS.cpp
OBJS_DP  := $(SRCS_DP:.cpp=.o)

.PHONY: all clean original dp

all: original dp

# ---------- original FPLS 빌드 ----------
original: $(TARGET_ORIGINAL)

$(TARGET_ORIGINAL): $(OBJS_COMMON) $(OBJS_ORIGINAL)
	$(CXX) $(CXXFLAGS) -o $@ $^

# ---------- DP+FPLS 빌드 ----------
dp: $(TARGET_DP_FPLS)

$(TARGET_DP_FPLS): $(OBJS_COMMON) $(OBJS_DP)
	$(CXX) $(CXXFLAGS) -o $@ $^

# ---------- 공통 규칙 ----------
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o $(TARGET_ORIGINAL) $(TARGET_DP_FPLS)