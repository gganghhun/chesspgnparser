cmake_minimum_required(VERSION 3.11)
project(chesspgnparser VERSION 0.1.0 LANGUAGES CXX )

set(CMAKE_CXX_STANDARD 17)
# --- 빌드 결과물 출력 위치 설정 ---
# 모든 실행 파일은 ${PROJECT_SOURCE_DIR}/bin 에 생성됩니다.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
# -----------------------------------
add_executable(build/bin/chesspgnparser src/pgndataparer.cpp)