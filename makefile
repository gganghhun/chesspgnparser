all: $(BIN_DIR)/$(TARGET)
CXX = g++
CXXFLAGS = -std=c++17 -Wall -I./chess-library/include
PKGS = libzstd
CXXCFLAGS += $(shell pkg-config --cflags $(PKGS))
LDFLAGS = $(shell pkg-config --libs $(PKGS))
SOURCES = src/pgndataparer.cpp
EXECUTABLE = chesspgnparser: 
TARGET = $(EXECUTABLE)
BUILD_DIR = build
BIN_DIR = $(BUILD_DIR)/bin
OBJ_DIR = $(BUILD_DIR)/obj
${OBJ_DIR}%.o: %.cpp
	@mkdir -p $(OBJ_DIR) # 오브젝트 파일 디렉토리 생성
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BIN_DIR)/$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR) # 실행 파일 디렉토리 생성
	$(CXX) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)/*
.PHONY: all