#!/bin/bash
set -eo pipefail

# 작업(Job) 실행 시 전달되는 인자 확인 (입력 GCS 경로, 출력 GCS 경로)
if [ "$#" -ne 2 ]; then
    echo "오류: 입력 GCS 경로와 출력 GCS 경로, 총 2개의 인자가 필요합니다."
    exit 1
fi

INPUT_GCS_PATH="$1"  # 예: gs://input-bucket/pgnsample.pgn.zst
OUTPUT_GCS_PATH="$2" # 예: gs://output-bucket/features/pgnsample.pgn.zst.bin

echo "입력 경로 (GCS): $INPUT_GCS_PATH"
echo "출력 경로 (GCS): $OUTPUT_GCS_PATH"

# 임시 로컬 파일 경로 설정 (컨테이너 내부 경로)
# basename 명령어로 GCS 경로에서 파일 이름만 추출
INPUT_FILENAME=$(basename "$INPUT_GCS_PATH")
OUTPUT_FILENAME=$(basename "$OUTPUT_GCS_PATH")
LOCAL_INPUT_PATH="/tmp/$INPUT_FILENAME"
LOCAL_OUTPUT_PATH="/tmp/$OUTPUT_FILENAME"

# 1. 입력 파일을 GCS에서 컨테이너 내부(/tmp)로 다운로드
echo "입력 파일 다운로드 중..."
gcloud storage cp "$INPUT_GCS_PATH" "$LOCAL_INPUT_PATH"
if [ $? -ne 0 ]; then
    echo "오류: 입력 파일 다운로드 실패"
    exit 1
fi
echo "다운로드 완료: $LOCAL_INPUT_PATH"

# 2. C++ 프로그램 실행 (로컬 경로 사용)
echo "C++ 프로그램 실행 시작..."
./pgnparser "$LOCAL_INPUT_PATH" "$LOCAL_OUTPUT_PATH"
EXIT_CODE=$? # C++ 프로그램의 종료 코드 저장
if [ $EXIT_CODE -ne 0 ]; then
    echo "오류: C++ 프로그램 실행 실패 (종료 코드: $EXIT_CODE)"
else
    echo "C++ 프로그램 실행 완료."
fi

# 3. 결과 파일을 컨테이너 내부(/tmp)에서 GCS로 업로드 (C++이 성공했을 때만)
if [ $EXIT_CODE -eq 0 ] && [ -f "$LOCAL_OUTPUT_PATH" ]; then
    echo "결과 파일 업로드 중..."
    gcloud storage cp "$LOCAL_OUTPUT_PATH" "$OUTPUT_GCS_PATH"
    if [ $? -ne 0 ]; then
        echo "오류: 결과 파일 업로드 실패"
        EXIT_CODE=1 # 업로드 실패 시 에러 코드로 설정
    else
        echo "업로드 완료: $OUTPUT_GCS_PATH"
    fi
fi

# 4. 임시 로컬 파일 삭제 (정리)
echo "임시 파일 삭제 중..."
rm -f "$LOCAL_INPUT_PATH" "$LOCAL_OUTPUT_PATH"

# 최종 종료 코드로 스크립트 종료
exit $EXIT_CODE