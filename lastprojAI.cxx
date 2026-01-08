#include <iostream>
#include <vector>
#include <array>
#include <memory>
#include <algorithm>
#include <random>
#include <chrono>
#include <limits>
#include <cmath>
#include <fstream>
#include <unordered_map>
#include <functional>

using namespace std;

// ============================================================================
// ساختارها و ثابت‌ها
// ============================================================================

// انواع مهره‌ها
enum class PieceType {
    EMPTY = 0,
    BLACK_PIECE = 1,
    WHITE_PIECE = 2,
    BLACK_KING = 3,
    WHITE_KING = 4,
    INVALID = -1
};

// ساختار موقعیت
struct Position {
    int row;
    int col;
    
    Position(int r = 0, int c = 0) : row(r), col(c) {}
    
    bool operator==(const Position& other) const {
        return row == other.row && col == other.col;
    }
    
    bool operator!=(const Position& other) const {
        return !(*this == other);
    }
};

// ساختار حرکت
struct Move {
    Position from;
    vector<Position> to;           // مسیر حرکت (برای captureهای زنجیره‌ای)
    vector<Position> captured;     // مهره‌های حذف شده
    bool is_capture;
    int capture_count;
    bool becomes_king;
    string board_before;           // برای یادگیری
    
    Move() : is_capture(false), capture_count(0), becomes_king(false) {}
    
    Move(Position f, Position t, bool cap = false, vector<Position> cap_list = {})
        : from(f), is_capture(cap), capture_count(cap_list.size()), becomes_king(false) {
        to.push_back(t);
        captured = cap_list;
    }
};

// جهت‌های حرکت
const vector<Position> BLACK_DIRECTIONS = {{-1, -1}, {-1, 1}};
const vector<Position> WHITE_DIRECTIONS = {{1, -1}, {1, 1}};
const vector<Position> KING_DIRECTIONS = {{-1, -1}, {-1, 1}, {1, -1}, {1, 1}};

// ============================================================================
// کلاس بازی Checkers
// ============================================================================

class CheckersGame {
private:
    static const int BOARD_SIZE = 6;
    array<array<PieceType, BOARD_SIZE>, BOARD_SIZE> board;
    PieceType current_player;
    bool game_over;
    PieceType winner;
    vector<Move> move_history;
    
    // تولید کننده اعداد تصادفی
    static mt19937 rng;
    
public:
    // سازنده
    CheckersGame() {
        initializeBoard();
        current_player = PieceType::BLACK_PIECE;
        game_over = false;
        winner = PieceType::EMPTY;
    }
    
    // کپی سازنده
    CheckersGame(const CheckersGame& other) {
        board = other.board;
        current_player = other.current_player;
        game_over = other.game_over;
        winner = other.winner;
        move_history = other.move_history;
    }
    
    // مقداردهی اولیه صفحه
    void initializeBoard() {
        // پر کردن با خانه‌های خالی
        for (int i = 0; i < BOARD_SIZE; i++) {
            for (int j = 0; j < BOARD_SIZE; j++) {
                board[i][j] = PieceType::EMPTY;
            }
        }
        
        // قرار دادن مهره‌های سیاه (ردیف‌های 0 و 1)
        for (int row = 0; row < 2; row++) {
            for (int col = 0; col < BOARD_SIZE; col++) {
                if ((row + col) % 2 == 1) {
                    board[row][col] = PieceType::BLACK_PIECE;
                }
            }
        }
        
        // قرار دادن مهره‌های سفید (ردیف‌های 4 و 5)
        for (int row = 4; row < BOARD_SIZE; row++) {
            for (int col = 0; col < BOARD_SIZE; col++) {
                if ((row + col) % 2 == 1) {
                    board[row][col] = PieceType::WHITE_PIECE;
                }
            }
        }
    }
    
    // بررسی معتبر بودن موقعیت
    bool isValidPosition(const Position& pos) const {
        return pos.row >= 0 && pos.row < BOARD_SIZE && 
               pos.col >= 0 && pos.col < BOARD_SIZE &&
               board[pos.row][pos.col] != PieceType::INVALID;
    }
    
    // بررسی خانه سیاه (قابل بازی)
    bool isBlackSquare(const Position& pos) const {
        return (pos.row + pos.col) % 2 == 1;
    }
    
    // دریافت رنگ مهره
    PieceType getPieceColor(PieceType piece) const {
        if (piece == PieceType::BLACK_PIECE || piece == PieceType::BLACK_KING) {
            return PieceType::BLACK_PIECE;
        } else if (piece == PieceType::WHITE_PIECE || piece == PieceType::WHITE_KING) {
            return PieceType::WHITE_PIECE;
        }
        return PieceType::EMPTY;
    }
    
    // بررسی شاه بودن
    bool isKing(PieceType piece) const {
        return piece == PieceType::BLACK_KING || piece == PieceType::WHITE_KING;
    }
    
    // دریافت تمام حرکات معتبر برای بازیکن
    vector<Move> getAllValidMoves(PieceType player) const {
        vector<Move> moves;
        vector<Move> capture_moves;
        
        for (int row = 0; row < BOARD_SIZE; row++) {
            for (int col = 0; col < BOARD_SIZE; col++) {
                Position pos(row, col);
                PieceType piece = board[row][col];
                
                if (getPieceColor(piece) == player) {
                    vector<Move> piece_moves = getMovesForPiece(pos, piece);
                    
                    for (const auto& move : piece_moves) {
                        if (move.is_capture) {
                            capture_moves.push_back(move);
                        } else {
                            moves.push_back(move);
                        }
                    }
                }
            }
        }
        
        // اگر حرکت capture وجود دارد، فقط آن‌ها را برگردان
        if (!capture_moves.empty()) {
            // پیدا کردن بیشترین تعداد capture
            int max_captures = 0;
            for (const auto& move : capture_moves) {
                if (move.capture_count > max_captures) {
                    max_captures = move.capture_count;
                }
            }
            
            // فقط حرکات با بیشترین capture
            vector<Move> result;
            for (const auto& move : capture_moves) {
                if (move.capture_count == max_captures) {
                    result.push_back(move);
                }
            }
            return result;
        }
        
        return moves;
    }
    
    // دریافت حرکات برای یک مهره خاص
    vector<Move> getMovesForPiece(const Position& pos, PieceType piece) const {
        vector<Move> moves;
        
        // تعیین جهت‌های حرکت
        vector<Position> directions;
        if (isKing(piece)) {
            directions = KING_DIRECTIONS;
        } else if (piece == PieceType::BLACK_PIECE) {
            directions = BLACK_DIRECTIONS;
        } else {
            directions = WHITE_DIRECTIONS;
        }
        
        // بررسی حرکات capture
        vector<Move> capture_moves = getCaptureMoves(pos, piece, directions);
        if (!capture_moves.empty()) {
            return capture_moves;
        }
        
        // حرکات ساده
        return getSimpleMoves(pos, piece, directions);
    }
    
    // حرکات ساده
    vector<Move> getSimpleMoves(const Position& pos, PieceType piece, 
                               const vector<Position>& directions) const {
        vector<Move> moves;
        
        for (const auto& dir : directions) {
            Position new_pos(pos.row + dir.row, pos.col + dir.col);
            
            if (isValidPosition(new_pos) && board[new_pos.row][new_pos.col] == PieceType::EMPTY) {
                Move move;
                move.from = pos;
                move.to.push_back(new_pos);
                move.is_capture = false;
                move.capture_count = 0;
                move.becomes_king = shouldBecomeKing(new_pos, piece);
                moves.push_back(move);
            }
        }
        
        return moves;
    }
    
    // حرکات capture
    vector<Move> getCaptureMoves(const Position& pos, PieceType piece,
                                const vector<Position>& directions) const {
        vector<Move> capture_moves;
        
        // تعیین مهره‌های حریف
        vector<PieceType> opponent_pieces;
        if (getPieceColor(piece) == PieceType::BLACK_PIECE) {
            opponent_pieces = {PieceType::WHITE_PIECE, PieceType::WHITE_KING};
        } else {
            opponent_pieces = {PieceType::BLACK_PIECE, PieceType::BLACK_KING};
        }
        
        // تابع بازگشتی برای جستجوی زنجیره‌ای
        function<void(Position, vector<Position>, vector<Position>, vector<Move>&)> 
        findCaptureChains = [&](Position current_pos, vector<Position> visited_captures,
                               vector<Position> path, vector<Move>& result) {
            bool found_chain = false;
            
            for (const auto& dir : directions) {
                Position jump_pos(current_pos.row + dir.row, current_pos.col + dir.col);
                Position land_pos(jump_pos.row + dir.row, jump_pos.col + dir.col);
                
                // بررسی امکان capture
                if (isValidPosition(jump_pos) && isValidPosition(land_pos)) {
                    PieceType jumped_piece = board[jump_pos.row][jump_pos.col];
                    
                    // بررسی اینکه مهره حریف باشد و قبلاً capture نشده باشد
                    bool is_opponent = false;
                    for (auto opp_piece : opponent_pieces) {
                        if (jumped_piece == opp_piece) {
                            is_opponent = true;
                            break;
                        }
                    }
                    
                    bool already_captured = false;
                    for (const auto& cap : visited_captures) {
                        if (cap == jump_pos) {
                            already_captured = true;
                            break;
                        }
                    }
                    
                    if (is_opponent && !already_captured && 
                        board[land_pos.row][land_pos.col] == PieceType::EMPTY) {
                        
                        // به‌روزرسانی مسیر
                        vector<Position> new_visited = visited_captures;
                        new_visited.push_back(jump_pos);
                        vector<Position> new_path = path;
                        new_path.push_back(land_pos);
                        vector<Position> new_captured = visited_captures;
                        new_captured.push_back(jump_pos);
                        
                        // جستجوی captureهای بیشتر
                        vector<Move> further_chains;
                        findCaptureChains(land_pos, new_visited, new_path, further_chains);
                        
                        if (!further_chains.empty()) {
                            for (auto& chain : further_chains) {
                                result.push_back(chain);
                            }
                        } else {
                            Move move;
                            move.from = pos;
                            move.to = new_path;
                            move.captured = new_captured;
                            move.is_capture = true;
                            move.capture_count = new_captured.size();
                            move.becomes_king = shouldBecomeKing(land_pos, piece) || isKing(piece);
                            result.push_back(move);
                        }
                        
                        found_chain = true;
                    }
                }
            }
        };
        
        vector<Move> chains;
        findCaptureChains(pos, {}, {}, chains);
        
        return chains;
    }
    
    // بررسی تبدیل به شاه
    bool shouldBecomeKing(const Position& pos, PieceType piece) const {
        if (piece == PieceType::BLACK_PIECE && pos.row == BOARD_SIZE - 1) {
            return true;
        } else if (piece == PieceType::WHITE_PIECE && pos.row == 0) {
            return true;
        }
        return false;
    }
    
    // اعمال حرکت
    bool applyMove(const Move& move) {
        if (!isValidPosition(move.from) || move.to.empty()) {
            return false;
        }
        
        // ذخیره مهره
        PieceType piece = board[move.from.row][move.from.col];
        
        // حذف مهره از مبدأ
        board[move.from.row][move.from.col] = PieceType::EMPTY;
        
        // حذف مهره‌های capture شده
        for (const auto& cap_pos : move.captured) {
            board[cap_pos.row][cap_pos.col] = PieceType::EMPTY;
        }
        
        // قرار دادن مهره در مقصد نهایی
        Position final_pos = move.to.back();
        
        // تبدیل به شاه اگر لازم باشد
        if (move.becomes_king) {
            if (getPieceColor(piece) == PieceType::BLACK_PIECE) {
                board[final_pos.row][final_pos.col] = PieceType::BLACK_KING;
            } else {
                board[final_pos.row][final_pos.col] = PieceType::WHITE_KING;
            }
        } else {
            board[final_pos.row][final_pos.col] = piece;
        }
        
        // ذخیره در تاریخچه
        move_history.push_back(move);
        
        // تغییر نوبت
        current_player = (current_player == PieceType::BLACK_PIECE) ? 
                        PieceType::WHITE_PIECE : PieceType::BLACK_PIECE;
        
        // بررسی پایان بازی
        checkGameOver();
        
        return true;
    }
    
    // بررسی پایان بازی
    void checkGameOver() {
        int black_count = 0;
        int white_count = 0;
        
        // شمارش مهره‌ها
        for (int i = 0; i < BOARD_SIZE; i++) {
            for (int j = 0; j < BOARD_SIZE; j++) {
                PieceType piece = board[i][j];
                if (piece == PieceType::BLACK_PIECE || piece == PieceType::BLACK_KING) {
                    black_count++;
                } else if (piece == PieceType::WHITE_PIECE || piece == PieceType::WHITE_KING) {
                    white_count++;
                }
            }
        }
        
        // اگر یکی از بازیکنان مهره‌ای نداشته باشد
        if (black_count == 0) {
            game_over = true;
            winner = PieceType::WHITE_PIECE;
            return;
        }
        
        if (white_count == 0) {
            game_over = true;
            winner = PieceType::BLACK_PIECE;
            return;
        }
        
        // بررسی حرکت معتبر برای بازیکن فعلی
        vector<Move> valid_moves = getAllValidMoves(current_player);
        if (valid_moves.empty()) {
            game_over = true;
            winner = (current_player == PieceType::BLACK_PIECE) ? 
                    PieceType::WHITE_PIECE : PieceType::BLACK_PIECE;
            return;
        }
        
        // جلوگیری از حلقه بی‌نهایت
        if (move_history.size() >= 30) {
            int recent_captures = 0;
            int start_idx = move_history.size() > 30 ? move_history.size() - 30 : 0;
            
            for (size_t i = start_idx; i < move_history.size(); i++) {
                if (move_history[i].is_capture) {
                    recent_captures++;
                }
            }
            
            if (recent_captures == 0) {
                game_over = true;
                winner = PieceType::EMPTY; // مساوی
                return;
            }
        }
        
        game_over = false;
    }
    
    // نمایش صفحه
    void printBoard() const {
        cout << "  0 1 2 3 4 5" << endl;
        
        for (int row = BOARD_SIZE - 1; row >= 0; row--) {
            cout << row << " ";
            for (int col = 0; col < BOARD_SIZE; col++) {
                Position pos(row, col);
                if (!isBlackSquare(pos)) {
                    cout << "██";
                } else {
                    switch (board[row][col]) {
                        case PieceType::EMPTY:
                            cout << "  ";
                            break;
                        case PieceType::BLACK_PIECE:
                            cout << "● ";
                            break;
                        case PieceType::WHITE_PIECE:
                            cout << "○ ";
                            break;
                        case PieceType::BLACK_KING:
                            cout << "♔ ";
                            break;
                        case PieceType::WHITE_KING:
                            cout << "♕ ";
                            break;
                        default:
                            cout << "  ";
                    }
                }
            }
            cout << " " << row << endl;
        }
        cout << "  0 1 2 3 4 5" << endl;
    }
    
    // دریافت کلید صفحه برای یادگیری
    string getBoardKey() const {
        string key;
        for (int i = 0; i < BOARD_SIZE; i++) {
            for (int j = 0; j < BOARD_SIZE; j++) {
                key += to_string(static_cast<int>(board[i][j]));
            }
        }
        return key;
    }
    
    // getterها
    bool isGameOver() const { return game_over; }
    PieceType getWinner() const { return winner; }
    PieceType getCurrentPlayer() const { return current_player; }
    const array<array<PieceType, BOARD_SIZE>, BOARD_SIZE>& getBoard() const { return board; }
    const vector<Move>& getMoveHistory() const { return move_history; }
    
    // ایجاد کپی
    CheckersGame copy() const {
        return CheckersGame(*this);
    }
};

// تعریف static member
mt19937 CheckersGame::rng(chrono::steady_clock::now().time_since_epoch().count());

// ============================================================================
// کلاس پایه عامل
// ============================================================================

class CheckersAgent {
protected:
    PieceType player;
    string name;
    
public:
    CheckersAgent(PieceType p, string n = "Base Agent") : player(p), name(n) {}
    virtual ~CheckersAgent() = default;
    
    virtual Move getMove(const CheckersGame& game) = 0;
    
    string getName() const { return name; }
    PieceType getPlayer() const { return player; }
};

// ============================================================================
// عامل تصادفی
// ============================================================================

class RandomAgent : public CheckersAgent {
private:
    mt19937 rng;
    
public:
    RandomAgent(PieceType p) : CheckersAgent(p, "Random Agent") {
        rng.seed(chrono::steady_clock::now().time_since_epoch().count());
    }
    
    Move getMove(const CheckersGame& game) override {
        vector<Move> moves = game.getAllValidMoves(player);
        
        if (moves.empty()) {
            return Move();
        }
        
        uniform_int_distribution<int> dist(0, moves.size() - 1);
        return moves[dist(rng)];
    }
};

// ============================================================================
// عامل حریصانه
// ============================================================================

class GreedyAgent : public CheckersAgent {
public:
    GreedyAgent(PieceType p) : CheckersAgent(p, "Greedy Agent") {}
    
    Move getMove(const CheckersGame& game) override {
        vector<Move> moves = game.getAllValidMoves(player);
        
        if (moves.empty()) {
            return Move();
        }
        
        // پیدا کردن حرکت با بیشترین capture
        Move best_move = moves[0];
        for (const auto& move : moves) {
            if (move.capture_count > best_move.capture_count) {
                best_move = move;
            }
        }
        
        return best_move;
    }
};

// ============================================================================
// عامل Minimax
// ============================================================================

class MinimaxAgent : public CheckersAgent {
protected:
    int depth;
    bool use_alpha_beta;
    string eval_func;
    int nodes_expanded;
    
    // تعریف تابع ارزیابی
    using EvalFunc = function<double(const CheckersGame&, PieceType)>;
    unordered_map<string, EvalFunc> eval_functions;
    
public:
    MinimaxAgent(PieceType p, int d =3, bool ab = true, string ef = "basic") 
        : CheckersAgent(p, "Minimax Agent"), depth(d), use_alpha_beta(ab), 
          eval_func(ef), nodes_expanded(0) {
        
        name = "Minimax (d=" + to_string(depth) + ", AB=" + (use_alpha_beta ? "Y" : "N") + ")";
        
        // ثبت توابع ارزیابی
        eval_functions["basic"] = [this](const CheckersGame& game, PieceType player) {
            return evaluateBasic(game, player);
        };
        eval_functions["advanced"] = [this](const CheckersGame& game, PieceType player) {
            return evaluateAdvanced(game, player);
        };
        
        eval_functions["positional"] = [this](const CheckersGame& game, PieceType player) {
            return evaluatePositional(game, player);
        };
    }
    
    virtual Move getMove(const CheckersGame& game) override {
        nodes_expanded = 0;
        vector<Move> moves = game.getAllValidMoves(player);
        if (moves.empty()) {
            return Move();
        }
        
        Move best_move;
        double best_value = -numeric_limits<double>::infinity();
        
        // ارزیابی هر حرکت
        for (const auto& move : moves) {
            CheckersGame game_copy = game.copy();
            game_copy.applyMove(move);
            
            double value;
            if (use_alpha_beta) {
                value = alphaBeta(game_copy, depth - 1, 
                                 -numeric_limits<double>::infinity(),
                                 numeric_limits<double>::infinity(), false);
            } else {
                value = minimax(game_copy, depth - 1, false);
            }
            
            if (value > best_value) {
                best_value = value;
                best_move = move;
            }
        }
        
        return best_move;
    }
    // الگوریتم Minimax استاندارد
    double minimax(CheckersGame& game, int depth, bool maximizing_player) {
        nodes_expanded++;
        
        if (depth == 0 || game.isGameOver()) {
            return evaluate(game);
        }
        
        PieceType current_player = game.getCurrentPlayer();
        vector<Move> moves = game.getAllValidMoves(current_player);
        
        if (maximizing_player) {
            double max_eval = -numeric_limits<double>::infinity();
            for (const auto& move : moves) {CheckersGame game_copy = game.copy();
                game_copy.applyMove(move);
                double eval = minimax(game_copy, depth - 1, false);
                max_eval = max(max_eval, eval);
            }
            return max_eval;
        } else {
            double min_eval = numeric_limits<double>::infinity();
            for (const auto& move : moves) {
                CheckersGame game_copy = game.copy();
                game_copy.applyMove(move);
                double eval = minimax(game_copy, depth - 1, true);
                min_eval = min(min_eval, eval);
            }
            return min_eval;
        }
    }
    
    // الگوریتم Alpha-Beta Pruning
    double alphaBeta(CheckersGame& game, int depth, double alpha, double beta, 
                    bool maximizing_player) {
        nodes_expanded++;
        
        if (depth == 0 || game.isGameOver()) {
            return evaluate(game);
        }
        
        PieceType current_player = game.getCurrentPlayer();
        vector<Move> moves = game.getAllValidMoves(current_player);
        
        if (maximizing_player) {
            double max_eval = -numeric_limits<double>::infinity();
            for (const auto& move : moves) {
                CheckersGame game_copy = game.copy();
                game_copy.applyMove(move);
                double eval = alphaBeta(game_copy, depth - 1, alpha, beta, false);
                max_eval = max(max_eval, eval);
                alpha = max(alpha, eval);
                if (beta <= alpha) {
                    break; // Beta cutoff
                }
            }
            return max_eval;
        } else {
            double min_eval = numeric_limits<double>::infinity();
            for (const auto& move : moves) {
                CheckersGame game_copy = game.copy();
                game_copy.applyMove(move);
                double eval = alphaBeta(game_copy, depth - 1, alpha, beta, true);
                min_eval = min(min_eval, eval);
                beta = min(beta, eval);
                if (beta <= alpha) {
                    break; // Alpha cutoff
                }
            }
            return min_eval;
        }
    }
    
    // تابع ارزیابی اصلی
    double evaluate(const CheckersGame& game) {
        if (game.isGameOver()) {
            PieceType winner = game.getWinner();
            if (winner == player) {
                return 1000.0;
            } else if (winner == PieceType::EMPTY) {
                return 0.0;} else {
                return -1000.0;
            }
        }
        
        return eval_functions[eval_func](game, player);
    }
    
    // تابع ارزیابی پایه
    double evaluateBasic(const CheckersGame& game, PieceType player) {
        double score = 0.0;
        auto board = game.getBoard();
        
        for (int i = 0; i < 6; i++) {
            for (int j = 0; j < 6; j++) {PieceType piece = board[i][j];
                
                if (piece == PieceType::EMPTY || !game.isBlackSquare({i, j})) {
                    continue;
                }
                
                double piece_value = 0.0;
                
                if (piece == player) {
                    piece_value = 1.0; // مهره معمولی خودی
                } else if ((player == PieceType::BLACK_PIECE && piece == PieceType::BLACK_KING) ||(player == PieceType::WHITE_PIECE && piece == PieceType::WHITE_KING)) {
                    piece_value = 3.0; // شاه خودی
                } else if (piece == PieceType::BLACK_PIECE || piece == PieceType::WHITE_PIECE) {
                    piece_value = -1.0; // مهره معمولی حریف
                } else {
                    piece_value = -3.0; // شاه حریف
                }
                
                score += piece_value;
            }
        }
        
        return score;
    }
    // تابع ارزیابی پیشرفته
    double evaluateAdvanced(const CheckersGame& game, PieceType player) {
        double score = evaluateBasic(game, player);
        auto board = game.getBoard();
        
        // ماتریس ارزش موقعیت
        vector<vector<double>> positional_value = {
            {0, 0, 0, 0, 0, 0},
            {0, 0.1, 0, 0.1, 0, 0.1},
            {0.1, 0, 0.2, 0, 0.2, 0},
            {0, 0.2, 0, 0.2, 0, 0.1},
            {0.1, 0, 0.2, 0, 0.1, 0},
            {0, 0.1, 0, 0.1, 0, 0}
        };
        for (int i = 0; i < 6; i++) {
            for (int j = 0; j < 6; j++) {
                if (!game.isBlackSquare({i, j})) continue;
                
                PieceType piece = board[i][j];
                PieceType piece_color = game.getPieceColor(piece);
                
                if (piece_color == player) {
                    // امتیاز موقعیت برای مهره‌های خودی
                    score += positional_value[i][j];
                       // امتیاز برای نزدیکی به تبدیل شدن به شاه
                    if (!game.isKing(piece)) {
                        if (player == PieceType::BLACK_PIECE) {
                            score += i * 0.05; // سیاه: هر چه پایین‌تر بهتر
                        } else {
                            score += (5 - i) * 0.05; // سفید: هر چه بالاتر بهتر
                        }
                    }
                } else if (piece_color != PieceType::EMPTY) {
                    // کسر امتیاز برای مهره‌های حریف
                    score -= positional_value[i][j];
                }
            }
        }
        
        return score;
    }
    
    // تابع ارزیابی موقعیتی
    double evaluatePositional(const CheckersGame& game, PieceType player) {
        double score = evaluateBasic(game, player);
        
        // اهمیت مهره‌های دفاعی و تهاجمی
        auto board = game.getBoard();
        
        for (int i = 0; i < 6; i++) {
            for (int j = 0; j < 6; j++) {
                if (!game.isBlackSquare({i, j})) continue;
                
                PieceType piece = board[i][j];
                PieceType piece_color = game.getPieceColor(piece);
                
                if (piece_color == player) {
                    // مهره‌های در خانه‌های امن (مرکزی) ارزش بیشتری دارند
                    if (j >= 1 && j <= 4) {
                        score += 0.1;
                    }
                    
                    // مهره‌های نزدیک به دیوار آسیب‌پذیر هستند
                    if (j == 0 || j == 5) {
                        score -= 0.05;
                    }
                }
            }
        }
        
        return score;
    }
    
    int getNodesExpanded() const { return nodes_expanded; }
};

// ============================================================================
// عامل یادگیرنده
// ============================================================================

class LearningAgent : public MinimaxAgent {
private:
    double learning_rate;
    unordered_map<string, pair<double, Move>> experience;
    string experience_file = "checkers_experience.dat";
    
public:
    LearningAgent(PieceType p, int d = 3, bool ab = true, 
                 string ef = "basic", double lr = 0.1)
        : MinimaxAgent(p, d, ab, ef), learning_rate(lr) {
        name = "Learning Agent (d=" + to_string(depth) + ")";
        loadExperience();
    }
    
    ~LearningAgent() {
        saveExperience();
    }
    
    Move getMove(const CheckersGame& game) override {
        nodes_expanded = 0;
        vector<Move> moves = game.getAllValidMoves(player);
        
        if (moves.empty()) {
            return Move();
        }
        
        // بررسی تجربیات گذشته
        string board_key = game.getBoardKey();
        auto it = experience.find(board_key);
        if (it != experience.end()) {
            const Move& stored_move = it->second.second;
            
            // بررسی معتبر بودن حرکت ذخیره شده
            for (const auto& move : moves) {
                if (movesEqual(move, stored_move)) {
                    return move;
                }
            }
        }
        
        // استفاده از Minimax
        return MinimaxAgent::getMove(game);
    }
    
    // یادگیری از بازی
    void learnFromGame(const vector<Move>& game_history, double result) {
        // result: 1.0 برای برد، 0.0 برای مساوی، -1.0 برای باخت
        
        for (size_t i = 0; i < game_history.size(); i++) {
            double move_value = result * (1.0 - static_cast<double>(i) / game_history.size());
            
            const Move& move = game_history[i];
            if (!move.board_before.empty()) {
                string key = move.board_before;
                auto it = experience.find(key);
                
                if (it != experience.end()) {
                    // به‌روزرسانی تجربه موجود
                    double old_value = it->second.first;
                    double new_value = old_value + learning_rate * (move_value - old_value);
                    it->second = make_pair(new_value, move);
                } else {
                    // افزودن تجربه جدید
                    experience[key] = make_pair(move_value, move);
                }
            }
        }
        
        saveExperience();
    }
    
    // مقایسه دو حرکت
    bool movesEqual(const Move& m1, const Move& m2) const {
        if (m1.from != m2.from) return false;
        if (m1.to.empty() || m2.to.empty()) return false;
        return m1.to.back() == m2.to.back();
    }
    
    // ذخیره تجربیات
    void saveExperience() {
        ofstream file(experience_file, ios::binary);
        if (file.is_open()) {
            size_t size = experience.size();
            file.write(reinterpret_cast<const char*>(&size), sizeof(size));
            
            for (const auto& entry : experience) {
                // ذخیره کلید
                size_t key_size = entry.first.size();
                file.write(reinterpret_cast<const char*>(&key_size), sizeof(key_size));
                file.write(entry.first.c_str(), key_size);
                
                // ذخیره مقدار
                file.write(reinterpret_cast<const char*>(&entry.second.first), sizeof(double));
                
                // ذخیره حرکت (ساده شده)
                const Move& m = entry.second.second;
                file.write(reinterpret_cast<const char*>(&m.from.row), sizeof(int));
                file.write(reinterpret_cast<const char*>(&m.from.col), sizeof(int));
                Position to_pos = m.to.empty() ? Position() : m.to.back();
                file.write(reinterpret_cast<const char*>(&to_pos.row), sizeof(int));
                file.write(reinterpret_cast<const char*>(&to_pos.col), sizeof(int));
            }
            file.close();
        }
    }
    
    // بارگذاری تجربیات
    void loadExperience() {
        ifstream file(experience_file, ios::binary);
        if (file.is_open()) {
            size_t size;
            file.read(reinterpret_cast<char*>(&size), sizeof(size));
            
            for (size_t i = 0; i < size; i++) {
                // خواندن کلید
                size_t key_size;
                file.read(reinterpret_cast<char*>(&key_size), sizeof(key_size));
                string key(key_size, ' ');
                file.read(&key[0], key_size);
                
                // خواندن مقدار
                double value;
                file.read(reinterpret_cast<char*>(&value), sizeof(double));
                
                // خواندن حرکت
                Move m;
                file.read(reinterpret_cast<char*>(&m.from.row), sizeof(int));
                file.read(reinterpret_cast<char*>(&m.from.col), sizeof(int));
                Position to_pos;
                file.read(reinterpret_cast<char*>(&to_pos.row), sizeof(int));
                file.read(reinterpret_cast<char*>(&to_pos.col), sizeof(int));
                m.to.push_back(to_pos);
                
                experience[key] = make_pair(value, m);
            }
            file.close();
        }
    }
};

// ============================================================================
// کلاس مدیریت بازی
// ============================================================================

class GameManager {
private:
    CheckersGame game;
    unique_ptr<CheckersAgent> agent1; // سیاه
    unique_ptr<CheckersAgent> agent2; // سفید
    string game_mode;
    
public:
    GameManager() : game() {}
    
    // تنظیم بازی
    void setupGame(const string& mode = "human_vs_agent", 
                   const string& agent1_type = "minimax",
                   const string& agent2_type = "random",
                   int depth = 3, bool use_alpha_beta = true) {
        game_mode = mode;
        
        if (mode == "human_vs_agent") {
            agent1 = nullptr; // انسان
            agent2 = createAgent(agent2_type, PieceType::WHITE_PIECE, depth, use_alpha_beta);
        } else if (mode == "human_vs_human") {
            agent1 = nullptr;
            agent2 = nullptr;
        }
    }
    
    // ایجاد عامل
    unique_ptr<CheckersAgent> createAgent(const string& type, PieceType player, 
                                         int depth, bool use_alpha_beta) {
        if (type == "random") {
            return make_unique<RandomAgent>(player);
        } else if (type == "greedy") {
            return make_unique<GreedyAgent>(player);
        } else if (type == "minimax") 
        {
            return make_unique<MinimaxAgent>(player, depth, use_alpha_beta, "advanced");
        } else if (type == "learning") {
            return make_unique<LearningAgent>(player, depth, use_alpha_beta, "advanced", 0.1);
        }
        return make_unique<RandomAgent>(player); // پیش‌فرض
    }
    
    // اجرای یک بازی کامل
    void playGame(bool display = true) {
        game = CheckersGame();
        vector<Move> game_history;
        
        if (display) {
            cout << "start checker << endl;
            game.printBoard();
        }
        
        while (!game.isGameOver()) {
            string player_name = (game.getCurrentPlayer() == PieceType::BLACK_PIECE) ? 
                               "black" :"white";
            
            if (display) {
                cout << "\n player " << player_name << endl;
            }
            
            // دریافت حرکت
            Move move;
            if (game.getCurrentPlayer() == PieceType::BLACK_PIECE) {move = getAgentMove(agent1.get(), PieceType::BLACK_PIECE, display);
            } else {
                move = getAgentMove(agent2.get(), PieceType::WHITE_PIECE, display);
            }
            
            if (move.to.empty()) {
                cout << "هیچ حرکت معتبری وجود ندارد!" << endl;
                break;
            }
            
            // ذخیره وضعیت قبل از حرکت برای یادگیری
            move.board_before = game.getBoardKey();
            game_history.push_back(move);
            // اعمال حرکت
            game.applyMove(move);
            
            if (display) {
                cout << "move from" << move.from.row << "," << move.from.col 
                     << "to" << move.to.back().row << "," << move.to.back().col << ")" << endl;
                
                if (!move.captured.empty()) {
                    cout << "delete checkers ";
                    for (const auto& cap : move.captured) {
                        cout << "(" << cap.row << "," << cap.col << ") ";
                    }cout << endl;
                }
                
                game.printBoard();
            }
        }
        
        // نمایش نتیجه
        if (display) {
            PieceType winner = game.getWinner();
            if (winner == PieceType::EMPTY) {
                cout << "\n draw" << endl;
            } else {
                string winner_name = (winner == PieceType::BLACK_PIECE) ? "black" : "white";
                cout << "\n player " << winner_name <<"win" << endl;}
        }
        double black_result = 0.0;
        if (game.getWinner() == PieceType::BLACK_PIECE) black_result = 1.0;
        else if (game.getWinner() == PieceType::WHITE_PIECE) black_result = -1.0;
        
        if (auto learning_agent = dynamic_cast<LearningAgent*>(agent1.get())) {
            learning_agent->learnFromGame(game_history, black_result);
        }
        
        if (auto learning_agent = dynamic_cast<LearningAgent*>(agent2.get())) {
            learning_agent->learnFromGame(game_history, -black_result);
        }
    }
    // دریافت حرکت از عامل یا انسان
    Move getAgentMove(CheckersAgent* agent, PieceType player, bool display) {
        vector<Move> moves = game.getAllValidMoves(player);
        
        if (moves.empty()) {
            return Move();
        }
        
        if (agent == nullptr) { // حرکت انسان
            if (display) {
                cout << "moves:"<< endl;
                for (size_t i = 0; i < moves.size(); i++) {
                    const auto& move = moves[i];
                    cout << i << ": from" << move.from.row << "," << move.from.col 
                         << "to" << move.to.back().row << "," << move.to.back().col << ") ";
                    cout << (move.is_capture ? "(capture)" : "(basic)") << endl;
                }
            }
            
            int choice;
            while (true) {
                cout << "choose number";
                cin >> choice;
                
                if (choice >= 0 && choice < static_cast<int>(moves.size())) {return moves[choice];
                } else {
                    cout << "شماره حرکت نامعتبر است." << endl;
                }
            }
        } else { // حرکت عامل
            if (display) {
                cout << agent->getName() << " in calculation" << endl;
            }
            
            auto start = chrono::high_resolution_clock::now();
            Move move = agent->getMove(game);
            auto end = chrono::high_resolution_clock::now();
            auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
            
            if (display) {
                cout << "time of calculation:" << duration.count() << " mlsecend" << endl;
                
                if (auto minimax_agent = dynamic_cast<MinimaxAgent*>(agent)) {
                    cout << "number of nudes " 
                         << minimax_agent->getNodesExpanded() << endl;
                }
            }
            
            return move;}
    }
    // اجرای آزمایش‌های تجربی
    void runExperiments(int num_games = 10, const string& agent1_type = "minimax",
                       const string& agent2_type = "random", int depth = 3,
                       bool use_alpha_beta = true) {
        struct Results {
            int black_wins = 0;
            int white_wins = 0;
            int draws = 0;
            double avg_moves = 0;
            long long total_nodes = 0;
        } results;
        cout  << num_games << " game between " 
             << agent1_type << " black and" << agent2_type << " (white)" << endl;
        cout << "depth search: " << depth << ", Alpha-Beta: " 
             << (use_alpha_beta ? "active" : "inactive") << endl;
        
        for (int game_num = 0; game_num < num_games; game_num++) {
            cout << "\n number game" << game_num + 1 << ":" << endl;
            
            // تنظیم بازی
            setupGame("agent_vs_agent", agent1_type, agent2_type, depth, use_alpha_beta);
            // اجرای بازی
            playGame(false);
            
            // جمع‌آوری نتایج
            PieceType winner = game.getWinner();
            if (winner == PieceType::BLACK_PIECE) {
                results.black_wins++;
                cout << "  نتیجه: " << agent1_type << " (سیاه) برنده شد";
            } else if (winner == PieceType::WHITE_PIECE) {
                results.white_wins++;
                cout << "  نتیجه: " << agent2_type << " (سفید) برنده شد";
            } else {
                results.draws++;
                cout << "  draw";
            }
            int moves_count = game.getMoveHistory().size();
            results.avg_moves += moves_count;
            cout << " (" << moves_count << " move" << endl;
            
            // جمع‌آوری تعداد گره‌ها
            if (auto minimax_agent = dynamic_cast<MinimaxAgent*>(agent1.get())) {
                results.total_nodes += minimax_agent->getNodesExpanded();
            }
        }
        
        // محاسبه میانگین‌ها
        results.avg_moves /= num_games;
        // نمایش نتایج کلی
        cout << "\n results:" << endl;
        cout << "black wins" << agent1_type << "): " << results.black_wins << endl;
        cout << "white wins:" << agent2_type << "): " << results.white_wins << endl;
        cout << "numbers of draws" << results.draws << endl;
        cout << "mean of moves:" << results.avg_moves << endl;
        
        if (results.total_nodes > 0) {
            cout << "mean" 
                 << results.total_nodes / num_games << endl;
        }
    }
};
int main() {
    GameManager manager;
    
    // تنظیم زبان فارسی در کنسول (ویندوز)
    system("chcp 65001 > nul");
    
    cout << "=============================================" << endl;
    cout << "    بازی Checkers 6x6 با عامل هوشمند" << endl;
    cout << "=============================================" << endl;
    
    int choice;
    do {
        cout << "\n main menu:" << endl;
        cout << "1.agent vs human" << endl;
        cout << "2. بازی عامل در مقابل عامل" << endl;
        cout << "3. آزمایش‌های تجربی" << endl;
        cout << "4. خروج" << endl;
        cout << "انتخاب کنید: ";
        cin >> choice;
        
        switch (choice) {
            case 1: {
                // انسان در مقابل عامل
                cout << "\nانتخاب نوع عامل:" << endl;
                cout << "1. Random Agent" << endl;
                cout << "2. Greedy Agent" << endl;
                cout << "3. Minimax Agent" << endl;
                cout << "4. Learning Agent" << endl;
                cout << "انتخاب کنید: ";
                int agent_choice;
                cin >> agent_choice;
                
                string agent_type;
                switch (agent_choice) {
                    case 1: agent_type = "random"; break;
                    case 2: agent_type = "greedy"; break;
                    case 3: agent_type = "minimax"; break;
                    case 4: agent_type = "learning"; break;
                    default: agent_type = "minimax";
                }
                manager.setupGame("human_vs_agent", "minimax", agent_type, 3, true);
                manager.playGame(true);
                break;
            }
            
            case 2: {
                // عامل در مقابل عامل
                cout << "\n type of black agent" << endl;
                cout << "1. Random Agent" << endl;
                cout << "2. Greedy Agent" << endl;
                cout << "3. Minimax Agent" << endl;
                cout << "4. Learning Agent" << endl;
                int agent1_choice;
                cin >> agent1_choice;
                
                string agent1_type;
                switch (agent1_choice) {
                    case 1: agent1_type = "random"; break;
                    case 2: agent1_type = "greedy"; break;
                    case 3: agent1_type = "minimax"; break;
                    case 4: agent1_type = "learning"; break;
                    default: agent1_type = "minimax";
                }
                cout << "\n type of white agent:" << endl;
                cout << "1. Random Agent" << endl;
                cout << "2. Greedy Agent" << endl;
                cout << "3. Minimax Agent" << endl;
                cout << "4. Learning Agent" << endl;
                cout << "انتخاب کنید: ";
                int agent2_choice;
                cin >> agent2_choice;
                
                string agent2_type;
                switch (agent2_choice) {
                    case 1: agent2_type = "random"; break;
                    case 2: agent2_type = "greedy"; break;
                    int agent2_choice;
                cin >> agent2_choice;
                
                string agent2_type;
                switch (agent2_choice) {
                    case 1: agent2_type = "random"; break;
                    case 2: agent2_type = "greedy"; break;
                    case 3: agent2_type = "minimax"; break;
                    case 4: agent2_type = "learning"; break;
                    default: agent2_type = "minimax";
                }
                
                manager.setupGame("agent_vs_agent", agent1_type, agent2_type, 3, true);
                manager.playGame(true);
                break;
            }
            
            case 3: {
                // آزمایش‌های تجربی
                cout << "1.moghayese baa minmax bedoon alphbeta<< endl;
                cout << "2. moghaayese agent haaye mokhtalef" << endl;
                cout << "3. tasir omgh" << endl;
                int exp_choice;
                cin >> exp_choice;
                
                switch (exp_choice) {
                    case 1:
                    cout << "\n Minimax با Alpha-Beta vs Alpha-Bet" << endl;
                        manager.runExperiments(5, "minimax", "minimax", 3, true);
                        break;
                    case 2:
                        cout << "\n moghayese agent haaye mokhtalef" << endl;
                        manager.runExperiments(5, "minimax", "random", 3, true);
                        break;
                    case 3:
                        cout << "\n tasir omgh search" << endl;
                        for (int depth = 2; depth <= 5; depth++) {cout << "\n depth search: " << depth << endl;
                            manager.runExperiments(3, "minimax", "greedy", depth, true);
                        }
                        break;
                }
                break;
            }
            
            case 4:
                cout <<" exit" << endl;
                break;
                
            default:
                cout << "bad!" << endl;
        }
          } while (choice != 4);
    
    return 0;
}