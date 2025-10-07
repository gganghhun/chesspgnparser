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
#include <array>
using namespace chess;

const std::unordered_map<std::string_view, int> pgn_resultpoints = {
	{"1-0", 1},
	{"1/2-1/2", 0},
	{"0-1", -1}
};

constexpr int MAX_ACTIVE_FEATURES = 64; // 한 포지션 당 최대 활성 피쳐 수 (넉넉하게 설정)
// 훈련 데이터 샘플 하나를 나타내는 구조체
struct TrainingEntry {
    // 게임 결과 (승리: 1, 무: 0, 패: -1)
    std::int8_t result;

    // 패딩(padding) 바이트. 구조체 크기를 특정 배수(e.g., 4바이트)로 맞추기 위함
    // std::int8_t padding;

    // 활성화된 피쳐 인덱스들
    // 사용하지 않는 공간은 -1과 같은 특수 값으로 채웁니다(sentinel value).
    std::int32_t active_features[MAX_ACTIVE_FEATURES];
    // 현재 활성화된 피쳐의 개수
	std::int8_t count = 0;
	//생성자
    // 기능 함수들
    void add(std::int32_t feature_id);
    void remove(std::int32_t feature_id);
};
std::int32_t make_featureindex(const Board& board,Square king_square, Square piece_square);
void make_feacher(Board board, TrainingEntry& entry);
void update_feacher(Board& board, TrainingEntry& entry, Move move);
class MyVisitor : public pgn::Visitor {
public:
    virtual ~MyVisitor() {}
	std::vector<TrainingEntry> feacher_vector;
	Board board = Board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	TrainingEntry startentry;
    void startPgn() {
		memset(startentry.active_features, -1, sizeof(startentry.active_features));
		make_feacher(board, startentry);
		std::cout << "start features: ";
		for(std::int32_t i : startentry.active_features)
		{
			std::cout << i << ",";
		}
		std::cout << "count:" << static_cast<int>(startentry.count)<< std::endl;
		
    }

    void header(std::string_view key, std::string_view value) {
		if (key == "Result")
		{
			std::cout << key << " " << value << std::endl;
			startentry.result = pgn_resultpoints.at(value);
			std::cout << "result: " << static_cast<int>(startentry.result) << std::endl;
		}
		std::cout << key << " " << value << std::endl;
        // Called for each header tag (e.g., [Event "F/S Return Match"])
    }

    void startMoves() {	
		std::cout << "ssㄴㄴ";
        // Called before the moves of a game are processed
    }

    void move(std::string_view move, std::string_view comment) {
		std::cout << "ss";
		Move current_move = uci::parseSan(board,move);
		update_feacher(board, startentry, current_move);
		feacher_vector.push_back(startentry);
		std::cout << board << std::endl;
		for(std::int32_t i : startentry.active_features)
		{
			std::cout << i << ",";
		}
		std::cout << "count:" << static_cast<int>(startentry.count)<< std::endl;
		// TrainingEntry aaa = feacher_vector[0];
		// std::cout << "asdasd" << aaa.active_features[0] << std::endl;
        // Called for each move in the game
    }

    void endPgn() {
		std::cout << "end" << std::endl;
		for(std::int32_t i : feacher_vector[1].active_features)
		{
			std::cout << i << ",";
		}
		std::cout << std::endl;
        // Called at the end of each PGN game
    }
};

// std::string cutting_header(std::string filename);
int main()
{
	// std::string pgn_cutted_header = cutting_header("chessfeachermaking/pgnsample.pgn");
    std::ifstream file_stream("chessfeachermaking/pgnsample.pgn");
	MyVisitor myvisitor;
	pgn::StreamParser parser(file_stream);
	std::cout << "sssx";
	auto error = parser.readGames(myvisitor);
    if (error) {
        std::cerr << "Error parsing PGN: " << error.message() << std::endl;
    }
	std::cout << "vectorsize: "<< myvisitor.feacher_vector.size() << std::endl;
	// for (TrainingEntry i :  myvisitor.feacher_vector)
	// {
	
	// 	for(std::int32_t j : i.active_features)
	// 	{
	// 		std::cout << j << ",";
	// 	}
	// 	std::cout << "count:" << static_cast<int>(i.count)<< std::endl;
	// }	
	// std::vector<std::uint16_t> feacher_index_v = make_feacher(board);
	// std::cout << feacher_index_v.size() << std::endl;
	// for(std::uint16_t i : feacher_index_v)
	// {
	// 	std::cout << i << std::endl;
	// }
}

std::int32_t make_featureindex(const Board& board,Square king_square, Square piece_square)
{
	std::cout << "kingindex:" << king_square.index() << std::endl;
	int piece_type = board.at(piece_square).type();	
	
	return (king_square.index() * 5 * 64) + (piece_type * 64) + piece_square.index();
}

void update_feacher(Board& board, TrainingEntry& entry, Move move)
{
	Square Wking_square = board.kingSq(false);
	Square Bking_square = board.kingSq(true);
	std::string ucimove = uci::moveToUci(move);
	Square piece_square_before = Square(ucimove.substr(0,2));
	Square piece_square_after = Square(ucimove.substr(2,2));
	int woffset = 0, boffset = 20480;
	if(piece_square_before == Wking_square || piece_square_before == Bking_square){
		board.makeMove(move);
		memset(entry.active_features, -1, sizeof(entry.active_features));
		entry.count = 0;
		make_feacher(board, entry);
		std::cout << "kingmove:";
		for(std::int32_t i : entry.active_features)
		{
			std::cout << i << ",";
		}
		std::cout << std::endl;
		return;
	}
	if(board.at(piece_square_before).color()){
		entry.remove(woffset + make_featureindex(board, Wking_square, piece_square_before));
		entry.remove(boffset + make_featureindex(board, Bking_square, piece_square_after));
		board.makeMove(move);
		entry.add(woffset + make_featureindex(board, Wking_square, piece_square_after));
	}
	else {
		entry.remove(boffset + make_featureindex(board, Bking_square, piece_square_before));
		entry.remove(woffset + make_featureindex(board, Wking_square, piece_square_after));
		board.makeMove(move);
		entry.add(boffset + make_featureindex(board, Bking_square, piece_square_after));
	}
}

void make_feacher(Board board, TrainingEntry& entry)
{
	
	Bitboard  bitboard = board.them(false);
	Square Wking_square = board.kingSq(false);
	int offset = 0;
	while(bitboard)
	{
		int piece_square = bitboard.pop();
		int piece_type = board.at(piece_square).type();
		// std::cout << "a"<< offset + (Wking_square.index() * 5 * 64) + (piece_type * 64) + piece_square << ',';
		if(piece_type != 5){
			entry.add(offset + (Wking_square.index() * 5 * 64) + (piece_type * 64) + piece_square);
		}
	}
	Square Bking_square = board.kingSq(true);
	bitboard = board.them(true);
	offset = 20480;
	while(bitboard)
	{
		int piece_square = bitboard.pop();
		int piece_type = board.at(piece_square).type();
		// std::cout << offset + (Bking_square.index() * 5 * 64) + (piece_type * 64) + piece_square << ',';
		if(piece_type != 5){
			entry.add(offset + (Bking_square.index() * 5 * 64) + (piece_type * 64) + piece_square);
		}
	}
    // std::cout << std::endl;
}


void TrainingEntry::add(std::int32_t feature_id)
{
    // 배열이 꽉 찼는지 확인 (안전장치)
    if (count < MAX_ACTIVE_FEATURES) {
        active_features[count] = feature_id;
        count++;
    }
}
void TrainingEntry::remove(std::int32_t feature_id) {
    // 1. 배열에서 제거할 피쳐를 찾습니다.
	std::cout << " want remove " << feature_id << std::endl;
    auto it = std::find(active_features, active_features + count, feature_id);
    // 2. 피쳐를 찾았다면
    if (it != active_features + count) {
		std::cout << "remove " << feature_id << std::endl;
        // 3. (핵심) 찾은 위치를 "맨 뒤에 있던 활성 피쳐" 값으로 덮어씁니다.
        *it = active_features[count - 1];
        active_features[count -1] = -1;
        // 4. 전체 피쳐 개수를 하나 줄입니다.
        count--;
    }
}