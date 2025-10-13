# 1단계: 빌드 환경 (Builder)
FROM gcc:11 AS builder
WORKDIR /app
COPY . .
RUN g++ -static -O2 -o pgnparcer pgndataparer.cpp

# 2단계: 최종 실행 환경 (Final Image)
FROM debian:11-slim
WORKDIR /app
COPY --from=builder /app/pgn_converter .
ENTRYPOINT ["./pgnparcer"]
