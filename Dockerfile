# =================================================================
# 1단계: 빌드 환경 (Builder)
# C++ 코드를 컴파일하기 위한 모든 도구를 갖춘 공간입니다.
# =================================================================
FROM gcc:11 AS builder

# 작업에 필요한 시스템 패키지(git, pkg-config, make, zstd) 설치
# RUN 명령어는 apt-get 같은 셸 명령을 실행합니다.
RUN apt-get update && apt-get install -y \
    git \
    pkg-config \
    libzstd-dev \
    make

# 컨테이너 안의 /app 폴더를 기본 작업 공간으로 지정합니다.
WORKDIR /app

# 내 컴퓨터의 현재 폴더(.)에 있는 모든 것을 컨테이너의 /app 폴더로 복사합니다.
COPY . .

# Git Submodule(chess-library)을 초기화하고 클론합니다. (매우 중요!)
# --init: 서브모듈을 처음으로 초기화합니다.
# --recursive: 서브모듈 안의 또 다른 서브모듈까지 모두 가져옵니다.
RUN git submodule update --init --recursive

# 제공해주신 makefile을 실행하여 프로젝트를 컴파일합니다.
RUN make

# =================================================================
# 2단계: 최종 실행 환경 (Final Image)
# 컴파일된 실행 파일과 실행에 필요한 최소한의 라이브러리만 담습니다.
# =================================================================
FROM debian:11-slim

# zstd 라이브러리의 실행 버전(libzstd1)만 설치합니다. (dev가 아님)
RUN apt-get update && apt-get install -y libzstd1 && \
    # apt 캐시를 정리하여 이미지 크기를 줄입니다.
    rm -rf /var/lib/apt/lists/*

# 작업 디렉토리 설정
WORKDIR /app

# 1단계(builder)에서 컴파일된 최종 실행 파일을 복사해옵니다.
# makefile에 정의된 경로(${BUILD_DIR}/bin/$(EXECUTABLE))를 그대로 사용합니다.
COPY --from=builder /app/build/bin/chesspgnparser .

# 이 컨테이너가 시작될 때 실행할 기본 명령어를 chesspgnparser로 지정합니다.
ENTRYPOINT ["./chesspgnparser"]