# =================================================================
# 1단계: 빌드 환경 (Builder) - "요리를 위한 주방"
# =================================================================
# 베이스 이미지: gcc 11 버전이 설치된 Debian 리눅스를 "주방"으로 사용합니다.
FROM gcc:11 AS builder

# 작업에 필요한 시스템 패키지 설치 (makefile 분석 결과)
RUN apt-get update && apt-get install -y \
    git \
    pkg-config \
    libzstd-dev \
    make

# 컨테이너 안의 /app 폴더를 기본 작업 공간으로 지정합니다.
WORKDIR /app

# 현재 폴더의 모든 것을 컨테이너의 /app 폴더로 복사합니다.
COPY . .

# Git Submodule(chess-library)을 초기화하고 클론합니다. (매우 중요!)
RUN git submodule update --init chess-library

# 제공해주신 makefile을 실행하여 프로젝트를 컴파일합니다.
RUN make

# =================================================================
# 2단계: 최종 실행 환경 (Final Image) - "고객에게 나갈 도시락"
# =================================================================
# 아주 가벼운 Debian 리눅스를 "도시락"으로 사용합니다.
FROM debian:bookworm-slim

# zstd 라이브러리의 '실행용' 버전(libzstd1)만 설치합니다.
RUN apt-get update && apt-get install -y libzstd1 && \
    # apt 캐시를 정리하여 이미지 크기를 줄입니다.
    rm -rf /var/lib/apt/lists/*

# 작업 디렉토리 설정
WORKDIR /app

# 1단계(builder)에서 컴파일된 최종 실행 파일을 복사해옵니다.
# makefile에 정의된 경로(build/bin/chesspgnparser)를 그대로 사용합니다.
COPY --from=builder /app/build/bin/chesspgnparser .

# 이 컨테이너가 시작될 때 실행할 기본 명령어를 chesspgnparser로 지정합니다.
ENTRYPOINT ["./chesspgnparser"]