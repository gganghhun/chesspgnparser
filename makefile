
CXX = g++
CXXFLAGS = -std=c++17 -Wall -I./chess-library/include
PKGS = libzstd
CXXCFLAGS += $(shell pkg-config --cflags $(PKGS))
LDFLAGS = $(shell pkg-config --libs $(PKGS))
SOURCES = src/pgndataparer.cpp 
EXECUTABLE = chesspgnparser
TARGET = $(BIN_DIR)/$(EXECUTABLE)
BUILD_DIR = build
BIN_DIR = $(BUILD_DIR)/bin
OBJ_DIR = $(BUILD_DIR)/obj
OBJS = $(patsubst src/%.cpp,build/obj/%.o,$(SOURCES))

all: $(TARGET)
$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR) # 실행 파일 디렉토리 생성
	$(CXX) -o $(TARGET) $(OBJS) $(LDFLAGS)

$(OBJ_DIR)/%.o: src/%.cpp
	@mkdir -p $(OBJ_DIR) # 오브젝트 파일 디렉토리 생성
	$(CXX) $(CXXFLAGS) -c $< -o $@


clean:
	rm -rf $(BUILD_DIR)/*
.PHONY: all clean