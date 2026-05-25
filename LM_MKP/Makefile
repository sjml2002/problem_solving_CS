# 컴파일러 및 옵션
CXX      := g++
CXXFLAGS := -std=c++11 -O2 -Wall -Wextra

# 기본 타겟: original FPLS를 사용하는 실행 파일
TARGET_ORIGINAL := fpls_original

# 나중에 new_FPLS.cpp를 쓰는 실행 파일 (미리 자리만 잡아둠)
TARGET_NEW := fpls_new

# 공통 헤더/소스
HDRS := dataIO.h original_FPLS.h
SRCS_COMMON := dataIO.cpp
OBJS_COMMON := $(SRCS_COMMON:.cpp=.o)

# original FPLS용 소스/오브젝트
SRCS_ORIGINAL := original_FPLS.cpp main.cpp
OBJS_ORIGINAL := $(SRCS_ORIGINAL:.cpp=.o)

# new FPLS용 소스/오브젝트 (new_FPLS.cpp, main_new.cpp 를 나중에 만들면 사용)
SRCS_NEW := new_FPLS.cpp main_new.cpp
OBJS_NEW := $(SRCS_NEW:.cpp=.o)

.PHONY: all clean original new

all: original

# ---------- original FPLS 빌드 ----------

original: $(TARGET_ORIGINAL)

$(TARGET_ORIGINAL): $(OBJS_COMMON) $(OBJS_ORIGINAL)
	$(CXX) $(CXXFLAGS) -o $@ $^

# ---------- new FPLS 빌드 (나중에 파일 만들고 사용) ----------

new: $(TARGET_NEW)

$(TARGET_NEW): $(OBJS_COMMON) $(OBJS_NEW)
	$(CXX) $(CXXFLAGS) -o $@ $^

# ---------- 공통 규칙 ----------

%.o: %.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o $(TARGET_ORIGINAL) $(TARGET_NEW)
