import mmap
import os
import struct
import torch
from torch.utils.data import Dataset, DataLoader
import torch.nn as nn
import torch.nn.functional as F
import torch.optim as optim
import argparse




class ChessDataset(Dataset):
    def __init__(self, binary_path, num_total_indices=64): # 킹당 개수가 아닌 '총' 개수
        self.binary_path = binary_path
        # C++에서 저장한 총 피처 인덱스의 개수 (예: 64)
        self.num_total_indices = num_total_indices

        # '킹 한쪽당' 피처의 최대 인덱스 (오프셋 기준)
        self.OFFSET = 41024

        # 각 샘플의 바이트 크기
        self.sample_size = (self.num_total_indices * 4) + 1 + 1 + 2

        # struct 포맷 문자열
        self.format_string = f'<{self.num_total_indices}ibbh' # 예: '64iff'

        self.file_handle = None 
        self.mm = None
        self.num_samples = os.path.getsize(self.binary_path)// self.sample_size

    def __len__(self):
        return self.num_samples

    def __getitem__(self, idx):
          if self.file_handle is None:
            self.file_handle = open(self.binary_path, 'r+b')
            self.mm = mmap.mmap(self.file_handle.fileno(), 0, access=mmap.ACCESS_READ)
          self.file_handle.seek(idx * self.sample_size)
          data = self.file_handle.read(self.sample_size)
          unpacked_data = struct.unpack(self.format_string, data)

            # 1. 섞여있는 모든 피처 인덱스를 텐서로 읽어옵니다.
          all_indices = torch.tensor(
              unpacked_data[:self.num_total_indices], dtype=torch.long
          )

            # 2. 🚨 핵심: 백킹/흑킹 피처 분리 (Modulo 연산 사용)

            # 2-1. 백킹 피처 (w_features)
            # 인덱스가 OFFSET(41024)보다 '작으면' -> 원래 값 유지
            # 인덱스가 OFFSET(41024)보다 '크거나 같으면' -> 0으로 변경 (패딩)
          w_features = torch.where(
               (all_indices < self.OFFSET) & (all_indices !=-1),  # 조건
               all_indices,                # 참일 때
               0                           # 거짓일 때
          )

            # 2-2. 흑킹 피처 (b_features)
            # 인덱스가 OFFSET(41024)보다 '크거나 같으면' -> (인덱스 % OFFSET) 값 사용
            # 인덱스가 OFFSET(41024)보다 '작으면' -> 0으로 변경 (패딩)
          b_features = torch.where(
                all_indices >= self.OFFSET, # 조건
                all_indices % self.OFFSET,  # 참일 때 (예: 46024 -> 4900)
                0                           # 거짓일 때
            )

            # 3. 점수와 결과는 동일하게 읽어옵니다.
          result = torch.tensor([unpacked_data[self.num_total_indices]], dtype=torch.float32)
          count = torch.tensor([unpacked_data[self.num_total_indices + 1]], dtype=torch.float32)

          return w_features, b_features, result, count

class ChessNet(nn.Module):
    def __init__(self):
        super(ChessNet, self).__init__()
        self.fc1 = nn.EmbeddingBag(num_embeddings=41024, embedding_dim=256, mode= "sum",padding_idx=0)
        self.fc1_bias = nn.Parameter(torch.zeros(256, requires_grad=True))
        self.fc2 = nn.Linear(512, 32)
        self.fc3 = nn.Linear(32, 32)
        self.fc4 = nn.Linear(32, 1)

    def forward(self, w_features, b_features):
        w_features = torch.relu(self.fc1(w_features, offsets=None) + self.fc1_bias)
        b_features = torch.relu(self.fc1(b_features, offsets=None)+ self.fc1_bias)
        hiddenlayer_1 = torch.cat((w_features, b_features), dim=1)
        hiddenlayer_2 = torch.relu(self.fc2(hiddenlayer_1))
        hiddenlayer_3 = torch.relu(self.fc3(hiddenlayer_2))
        output = self.fc4(hiddenlayer_3)

        return output


if __name__ == "__main__":
    print("v1.6")
    # 인자 파서 설정
    parser = argparse.ArgumentParser()
    parser.add_argument('--data', type=str, required=True, help='학습 데이터(.bin) 경로')
    parser.add_argument('--output-dir', type=str, required=True, help='모델 가중치 저장 경로 (폴더)')
    parser.add_argument('--epochs', type=int, default=100, help='학습 에포크 수')
    args = parser.parse_args()

    # 입력받은 경로 사용
    input_path = args.data
    print(f"학습 데이터 경로: {input_path}")

    if not os.path.exists(input_path):
        print(f"에러: 파일을 찾을 수 없습니다 -> {input_path}")
        exit(1)
    # --- 데이터로더 생성 ---
    # C++ 코드에 맞춰 num_total_indices 값 설정 (예: 64)
    dataset = ChessDataset(input_path, num_total_indices=64)
    dataloader = DataLoader(dataset, batch_size=1024, shuffle=True, num_workers=2, persistent_workers= True, pin_memory=True)
    print(f"데이터셋 크기: {dataset.__len__()}")
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    print(f"사용 장치: {device}")
    embedding_dim = 256
    module = ChessNet().to(device)
    epochs = args.epochs
    optimizer = optim.AdamW(module.parameters(), lr=0.001)
    for j in range(epochs):
        for i, (w_features, b_features, raw_result, count) in enumerate(dataloader):
            # print(i, ("w_features:", w_features, "b_features", b_features, result, count))
            # print(module.parameters())
            w_features = w_features.to(device)
            b_features = b_features.to(device)
            raw_result = raw_result.to(device)
            result = (raw_result + 1.0) / 2.0
            output = module(w_features, b_features)
            # print("output:", output)
            # print("result:", result)
            # print("cost tensor",((torch.sigmoid(output) - result) ** 2))
            cost  = torch.mean((torch.sigmoid(output) - result) ** 2)
            # print(cost)
            optimizer.zero_grad()
            cost.backward()
            optimizer.step()
        print("Epoch {:4d}/{} Cost: {:.6f}".format(j, epochs, cost.item()))
        if (epoch + 1) % 10 == 0:
            checkpoint = {
                'epoch': epoch,
                'model_state_dict': module.state_dict(),
                'optimizer_state_dict': optimizer.state_dict(), # 옵티마이저의 관성 정보도 저장
                'loss': cost.item(),
            }
            # 파일명에 에포크 번호를 붙여서 저장
            torch.save(checkpoint, f"checkpoint_epoch_{epoch+1}.pth")
            print(f"체크포인트 저장됨: checkpoint_epoch_{epoch+1}.pth")
    final_save_path = os.path.join(args.output_dir, "model_final.pth")
    torch.save(module.state_dict(), final_save_path)
    print(f"최종 모델 저장 완료: {final_save_path}")