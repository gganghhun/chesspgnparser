# =================================================================
# 1단계: 빌드 환경 (Builder) - 이전과 동일
# =================================================================
FROM gcc:11 AS builder
RUN apt-get update && apt-get install -y \
    git \
    pkg-config \
    libzstd-dev \
    make
WORKDIR /app
ARG GIT_REPO_URL=https://github.com/gganghhun/chesspgnparser.git
RUN git clone --recursive ${GIT_REPO_URL} .
RUN make
# =================================================================
# 2단계: 최종 실행 환경 (Final Image) - Google Cloud SDK 설치 추가
# =================================================================
# google/cloud-sdk:slim 이미지를 사용하여 SDK를 포함합니다.
# 이 이미지에는 Debian과 기본적인 SDK가 이미 설치되어 있습니다.
FROM google/cloud-sdk:slim

# zstd 라이브러리의 '실행용' 버전(libzstd1) 설치 (여전히 필요)
RUN apt-get update && apt-get install -y libzstd1 && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app

# 빌드된 C++ 실행 파일 복사
COPY --from=builder /app/build/bin/chesspgnparser .

# ========================= 핵심 변경점 =========================
# 컨테이너 시작 시 실행될 스크립트 복사 및 실행 권한 부여
COPY entrypoint_gcloud_cp.sh .
RUN chmod +x entrypoint_gcloud_cp.sh

# 컨테이너 시작 명령어를 새 스크립트로 변경
ENTRYPOINT ["./entrypoint_gcloud_cp.sh"]
# ===============================================================