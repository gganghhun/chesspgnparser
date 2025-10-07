#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <vector>
#include <typeinfo>
#include "chess.hpp"
#include <cstdint> // int32_t, int8_t 와 같은 명시적 크기 정수 타입 사용
#include <algorithm>
#include <unordered_map>
using namespace chess;
using namespace std;
const std::unordered_map<std::string_view, int> pgn_resultpoints = {
	{"1-0", 1},
	{"1/2-1/2", 0},
	{"0-1", -1}
};

const int MAX_ACTIVE_FEATURES = 64; // 한 포지션 당 최대 활성 피쳐 수 (넉넉하게 설정)
// 훈련 데이터 샘플 하나를 나타내는 구조체
struct TrainingEntry {
    // 게임 결과 (승리: 1, 무: 0, 패: -1)
    std::int8_t result;

    // 패딩(padding) 바이트. 구조체 크기를 특정 배수(e.g., 4바이트)로 맞추기 위함
    std::int8_t padding;

    // 활성화된 피쳐 인덱스들
    // 사용하지 않는 공간은 -1과 같은 특수 값으로 채웁니다(sentinel value).
    std::int32_t active_features[MAX_ACTIVE_FEATURES];
};
class MyVisitor : public pgn::Visitor {
public:
    virtual ~MyVisitor() {}

    void startPgn() {
        // Called at the start of each PGN game
    }

    void header(std::string_view key, std::string_view value) {
		if (key == "Result")
		{
			string a = "0-1";
			cout << value << endl;
		}
        // Called for each header tag (e.g., [Event "F/S Return Match"])
    }

    void startMoves() {
		std::cout << "Aaa";
        // Called before the moves of a game are processed
    }

    void move(std::string_view move, std::string_view comment) {
		std::cout << move<<"  ";
        // Called for each move in the game
    }

    void endPgn() {
        // Called at the end of each PGN game
    }
};
int main() {
	TrainingEntry aa;
	std::ifstream file_stream("chessfeachermaking/pgnsample.pgn");
	MyVisitor myvisitor;
	pgn::StreamParser parser(file_stream);
	auto error = parser.readGames(myvisitor);
	Board board = Board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	cout << board.kingSq(true).index();
	cout << Square(Square::SQ_E8).index();
	Bitboard bitboard = board.them(false);
	while(bitboard)
	{
		int piece_square = bitboard.pop();
		int piece_type = board.at(piece_square).type();
		
		std:cout << piece_type << endl;
		std::cout << bitboard << "a"<< piece_square << ',';

	}
	aa.active_features[12] = 12333;
	cout << aa.active_features[12] << endl;
	auto zxx = find(aa.active_features,aa.active_features + 128,12333);
	int asd = *zxx;
	int zz = distance(aa.active_features, zxx);
	
	cout << zz << endl;
	
	
	
    // std::ifstream file_stream("chessfeachermaking/pgnsample.pgn");
    // if (!file_stream.is_open()) {
    //     // Handle error
    //     return -1;
    // }

    // MyVisitor visitor;
    // pgn::StreamParser parser(file_stream);
	
    // auto error = parser.readGames(visitor);
}