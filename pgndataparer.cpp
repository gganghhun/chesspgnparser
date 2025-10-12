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
#include <limits>
#include <zstd.h>
#include <cstdlib>
#include <sstream>
using namespace chess;

class ZstdDecompressingStreamBuf : public std::streambuf {
public:
    // 생성자: 압축된 파일 스트림을 받습니다.
    ZstdDecompressingStreamBuf(std::istream& source)
        : source_(source) {
        
        // ZSTD 스트림 초기화
        dstream_ = ZSTD_createDStream();
        ZSTD_initDStream(dstream_);

        // 입출력 버퍼 할당 (RAII를 위해 vector 사용)
        in_buffer_.resize(ZSTD_DStreamInSize());
        out_buffer_.resize(ZSTD_DStreamOutSize());

        // streambuf의 get 포인터 초기화 (버퍼가 비어있음을 의미)
        setg(nullptr, nullptr, nullptr);
    }

    // 소멸자: ZSTD 리소스 해제
    ~ZstdDecompressingStreamBuf() {
        ZSTD_freeDStream(dstream_);
    }

protected:
    // underflow()는 streambuf의 핵심입니다.
    // istream이 데이터를 다 읽어서 버퍼가 비었을 때 자동으로 호출됩니다.
    int_type underflow() override {
        // 현재 버퍼에 읽을 데이터가 남아있으면 즉시 반환 (일어날 확률 낮음)
        if (gptr() < egptr()) {
            return traits_type::to_int_type(*gptr());
        }

        // 압축 해제된 데이터를 담을 출력 버퍼 설정
        ZSTD_outBuffer out_zstd_buffer = { out_buffer_.data(), out_buffer_.size(), 0 };

        // 출력 버퍼가 채워지거나, 입력 파일이 끝날 때까지 반복
        while (out_zstd_buffer.pos < out_zstd_buffer.size) {
            // ZSTD 입력 버퍼에 남은 데이터가 없으면 파일에서 더 읽어온다.
            if (in_zstd_buffer_.pos >= in_zstd_buffer_.size) {
                source_.read(in_buffer_.data(), in_buffer_.size());
                size_t read_bytes = source_.gcount();
                if (read_bytes == 0) {
                    break; // 파일 끝
                }
                in_zstd_buffer_ = { in_buffer_.data(), read_bytes, 0 };
            }

            // 스트림 압축 해제
            size_t const ret = ZSTD_decompressStream(dstream_, &out_zstd_buffer, &in_zstd_buffer_);
            if (ZSTD_isError(ret)) {
                // 에러 처리
                setg(nullptr, nullptr, nullptr);
                return traits_type::eof();
            }

            // ZSTD가 압축 해제를 완료했고, 더 이상 출력할 데이터가 없으면 종료
            if (ret == 0) {
                break;
            }
        }

        // 압축 해제된 데이터가 조금이라도 있다면
        if (out_zstd_buffer.pos > 0) {
            // streambuf의 "get area"를 새로 채워진 출력 버퍼로 설정
            setg(out_buffer_.data(), out_buffer_.data(), out_buffer_.data() + out_zstd_buffer.pos);
            // 새로 채워진 데이터의 첫 번째 문자를 반환
            return traits_type::to_int_type(*gptr());
        }

        // 파일 끝이고 더 이상 압축 해제할 데이터가 없으면 EOF 반환
        return traits_type::eof();
    }

private:
    std::istream&       source_;
    ZSTD_DStream* dstream_;
    std::vector<char>   in_buffer_;
    std::vector<char>   out_buffer_;
    ZSTD_inBuffer       in_zstd_buffer_ = { nullptr, 0, 0 };
};

const std::unordered_map<std::string_view, int> pgn_resultpoints = {
	{"1-0", 1},
	{"1/2-1/2", 0},
	{"0-1", -1}
};
constexpr uint16_t EMPTY_SLOT = std::numeric_limits<uint16_t>::max();
constexpr int MAX_ACTIVE_FEATURES = 64; // 한 포지션 당 최대 활성 피쳐 수 (넉넉하게 설정)
const size_t BUFFER_SIZE = 16384;//16384;
// 훈련 데이터 샘플 하나를 나타내는 구조체
struct TrainingEntry {
    // 가장 큰 멤버를 맨 위로
    std::uint16_t active_features[MAX_ACTIVE_FEATURES]; // 128 바이트

    std::int8_t result; // 1 바이트
    std::int8_t count = 0;  // 1 바이트

    // 수동 패딩: 총 크기를 132바이트 (4의 배수)로 맞추기 위함
    std::int8_t padding[2]; // 2 바이트
    void add(std::uint16_t feature_id);
    void remove(std::uint16_t feature_id);
};

int fast_convert_view_to_int(std::string_view sv);
void save_buffer_to_binary_file(const std::string& filepath, const std::vector<TrainingEntry>& buffer);
std::uint16_t make_featureindex(const Board& board,Square king_square, Square piece_square);
void make_feacher(Board board, TrainingEntry& entry);
void update_feacher(Board& board, TrainingEntry& entry, Move move);
class MyVisitor : public pgn::Visitor {
public:
    virtual ~MyVisitor() {}
	std::vector<TrainingEntry> feacher_vector;
	
	Board board = Board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	TrainingEntry startentry;
    void startPgn() {
		board = Board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
		memset(startentry.active_features, EMPTY_SLOT, sizeof(startentry.active_features));
		make_feacher(board, startentry);
		std::cout << "start features: ";
		for(std::uint16_t i : startentry.active_features)
		{
			std::cout << i << ",";
		}
		std::cout << "count:" << static_cast<int>(startentry.count)<< std::endl;
		
    }

    void header(std::string_view key, std::string_view value) {
		if (key == "WhiteElo"){
			startentry.count = 0;
			skipPgn(fast_convert_view_to_int(value) < 2300);
		}
		else if (key == "Termination"){
			startentry.count = 0;
			skipPgn(value != "Normal");
		}
		else if (key == "Result")
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
		if(feacher_vector.size() >= BUFFER_SIZE){
			
			save_buffer_to_binary_file("bin/training_data.bin", feacher_vector);
			feacher_vector.clear();
		}
		// std::cout << board << std::endl;
		for(std::uint16_t i : startentry.active_features)
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
		if(feacher_vector.empty()){
			return;
		}
		std::cout << "assdw";
		for(std::uint16_t i : feacher_vector[1].active_features)
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
    // std::ifstream file_stream("pgnsample.pgn");
	std::string input_path = "pgnsample.pgn.zst";
	std::ifstream inputFile(input_path, std::ios::binary);
    if (!inputFile) {
        std::cerr << "Error: Cannot open input file " << input_path << std::endl;
        return -1;
    }
	ZstdDecompressingStreamBuf zstdBuf(inputFile);
	std::istream file_stream2(&zstdBuf);



	MyVisitor myvisitor;
	myvisitor.feacher_vector.reserve(BUFFER_SIZE);
	pgn::StreamParser parser(file_stream2);
	auto error = parser.readGames(myvisitor);
	if (myvisitor.feacher_vector.size() > 0) {
		save_buffer_to_binary_file("bin/training_data.bin", myvisitor.feacher_vector);
	}
    if (error) {
        std::cerr << "Error parsing PGN: " << error.message() << std::endl;
    }

	std::cout << "vectorsize: "<< myvisitor.feacher_vector.size() << std::endl;
	for (TrainingEntry i :  myvisitor.feacher_vector)
	{
	
		for(std::uint16_t j : i.active_features)
		{
			std::cout << j << ",";
		}
		std::cout << "count:" << static_cast<int>(i.count)<< std::endl;
	}	
	// std::vector<std::uint16_t> feacher_index_v = make_feacher(board);
	// std::cout << feacher_index_v.size() << std::endl;
	// for(std::uint16_t i : feacher_index_v)
	// {
	// 	std::cout << i << std::endl;
	// }
}

std::uint16_t make_featureindex(const Board& board,Square king_square, Square piece_square)
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
		memset(entry.active_features, EMPTY_SLOT, sizeof(entry.active_features));
		entry.count = 0;
		make_feacher(board, entry);
		std::cout << "kingmove:";
		for(std::uint16_t i : entry.active_features)
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
			entry.add(40960 + Wking_square.index());
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
			entry.add(40960 + Bking_square.index());
		}
	}
    // std::cout << std::endl;
}


void TrainingEntry::add(std::uint16_t feature_id)
{
    // 배열이 꽉 찼는지 확인 (안전장치)
    if (count < MAX_ACTIVE_FEATURES) {
        active_features[count] = feature_id;
        count++;
    }
}
void TrainingEntry::remove(std::uint16_t feature_id) {
    // 1. 배열에서 제거할 피쳐를 찾습니다.
	std::cout << " want remove " << feature_id << std::endl;
    auto it = std::find(active_features, active_features + count, feature_id);
    // 2. 피쳐를 찾았다면
    if (it != active_features + count) {
		std::cout << "remove " << feature_id << std::endl;
        // 3. (핵심) 찾은 위치를 "맨 뒤에 있던 활성 피쳐" 값으로 덮어씁니다.
        *it = active_features[count - 1];
        active_features[count -1] = EMPTY_SLOT;
        // 4. 전체 피쳐 개수를 하나 줄입니다.
        count--;
    }
}
void save_buffer_to_binary_file(const std::string& filepath, const std::vector<TrainingEntry>& buffer) {
    // 1. std::ofstream을 사용하여 파일을 엽니다.
    //    - std::ios::binary: 텍스트 모드가 아닌 바이너리 모드로 엽니다. (매우 중요!)
    //    - std::ios::app: 파일의 끝에 데이터를 이어서 씁니다. (append)
    std::ofstream file(filepath, std::ios::binary | std::ios::app);

    // 파일이 정상적으로 열렸는지 확인
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filepath << " for writing." << std::endl;
        return;
    }

    // 2. file.write() 함수로 버퍼의 모든 데이터를 한 번에 씁니다.
    file.write(
        // 2a. reinterpret_cast로 TrainingEntry* 포인터를 const char* 포인터로 변환
        reinterpret_cast<const char*>(buffer.data()),

        // 2b. 쓸 데이터의 총 바이트(byte) 크기를 계산
        buffer.size() * sizeof(TrainingEntry)
    );

    file.close();
}
int fast_convert_view_to_int(std::string_view sv) {
    int value;
    
    // ⭐️ sv가 참조하는 데이터의 시작 주소와 끝 주소를 전달
    auto [ptr, ec] = std::from_chars(sv.data(), 
                                      sv.data() + sv.size(), 
                                      value);
	std::cout << value << std::endl; 
	return value;
    // ec(error code)를 확인하여 변환 성공 여부 판단
    // if (ec == std::errc{}) {
    //     std::cout << "빠르게 변환된 값: " << value << std::endl;
    // } else {
    //     std::cerr << "변환 실패 (std::from_chars)" << std::endl;
    // }
}
