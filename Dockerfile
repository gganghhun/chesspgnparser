# =================================================================
# 1단계: 빌드 환경 (Builder)
# =================================================================
FROM gcc:11 AS builder

# 작업에 필요한 시스템 패키지(pkg-config, make, zstd) 설치
# git은 여기서 제거합니다.
RUN apt-get update && apt-get install -y \
    pkg-config \
    libzstd-dev \
    make

WORKDIR /app

# 모든 소스 코드를 복사합니다.
# (이 시점에는 Cloud Build가 이미 submodule을 받아놓은 상태일 것입니다.)
COPY . .

# makefile을 실행하여 프로젝트를 컴파일합니다.
RUN make

# =================================================================
# 2단계: 최종 실행 환경 (Final Image)
# =================================================================
FROM debian:11-slim

RUN apt-get update && apt-get install -y libzstd1 && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /app/build/bin/chesspgnparser .

ENTRYPOINT ["./chesspgnparser"]