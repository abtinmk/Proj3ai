import copy

# --- تنظیمات اولیه ---
EMPTY = 0
BLACK = 1
WHITE = 2
BLACK_KING = 3
WHITE_KING = 4
ROWS, COLS = 6, 6

# --- چاپ صفحه ---
def print_board(board):
    symbols = {-1:' ', EMPTY:'.', BLACK:'b', WHITE:'w', BLACK_KING:'B', WHITE_KING:'W'}
    print("  0 1 2 3 4 5")
    for r in range(ROWS):
        print(r, end=' ')
        for c in range(COLS):
            print(symbols[board[r][c]], end=' ')
        print()
    print()

# --- ایجاد صفحه اولیه ---
def init_board():
    board = [[-1 if (r+c)%2==0 else EMPTY for c in range(COLS)] for r in range(ROWS)]
    for r in range(2):
        for c in range(COLS):
            if board[r][c]==EMPTY: board[r][c]=WHITE
    for r in range(ROWS-2, ROWS):
        for c in range(COLS):
            if board[r][c]==EMPTY: board[r][c]=BLACK
    return board

# --- بررسی داخل صفحه ---
def in_board(r,c):
    return 0<=r<ROWS and 0<=c<COLS

# --- حرکت‌های مجاز ---
def get_moves(board,player):
    moves,jump_moves=[],[]
    directions = [(-1,-1),(-1,1)] if player in [BLACK,BLACK_KING] else [(1,-1),(1,1)]
    king_dirs=[(-1,-1),(-1,1),(1,-1),(1,1)]
    for r in range(ROWS):
        for c in range(COLS):
            piece=board[r][c]
            if piece in [EMPTY,-1]: continue
            if (player==BLACK and piece in [BLACK,BLACK_KING]) or (player==WHITE and piece in [WHITE,WHITE_KING]):
                dirs = king_dirs if piece in [BLACK_KING,WHITE_KING] else directions
                for dr,dc in dirs:
                    nr,nc=r+dr,c+dc
                    if in_board(nr,nc) and board[nr][nc]==EMPTY:
                        moves.append(((r,c),(nr,nc)))
                jump_moves+=get_jump_moves(board,r,c,piece)
    return jump_moves if jump_moves else moves

# --- زنجیره خوردن ---
def get_jump_moves(board,r,c,piece,path=None,visited=None):
    if path is None: path=[(r,c)]
    if visited is None: visited=set()
    moves=[]
    directions = [(-1,-1),(-1,1),(1,-1),(1,1)] if piece in [BLACK_KING,WHITE_KING] else [(-1,-1),(-1,1)] if piece==BLACK else [(1,-1),(1,1)]
    for dr,dc in directions:
        nr,nc=r+dr,c+dc
        jr,jc=r+2*dr,c+2*dc
        if in_board(jr,jc) and board[nr][nc] not in [EMPTY,-1] and \
           ((piece in [BLACK,BLACK_KING] and board[nr][nc] in [WHITE,WHITE_KING]) or \
            (piece in [WHITE,WHITE_KING] and board[nr][nc] in [BLACK,BLACK_KING])) and \
           board[jr][jc]==EMPTY and (nr,nc,jr,jc) not in visited:
            visited.add((nr,nc,jr,jc))
            new_board=copy.deepcopy(board)
            new_board[jr][jc]=new_board[r][c]; new_board[r][c]=EMPTY; new_board[nr][nc]=EMPTY
            further=get_jump_moves(new_board,jr,jc,piece,path+[(jr,jc)],visited)
            moves+=further if further else [path+[(jr,jc)]]
    return moves

# --- اعمال حرکت ---
def apply_move(board,move,player):
    new_board=copy.deepcopy(board)
    if isinstance(move[0],tuple) and isinstance(move[1],tuple):
        r1,c1=move[0]; r2,c2=move[1]
        new_board[r2][c2]=new_board[r1][c1]; new_board[r1][c1]=EMPTY
    else:
        path=move
        for i in range(len(path)-1):
            r1,c1=path[i]; r2,c2=path[i+1]
            dr=(r2-r1)//abs(r2-r1) if r2!=r1 else 0
            dc=(c2-c1)//abs(c2-c1) if c2!=c1 else 0
            jumped_r,jumped_c=r1+dr,c1+dc
            new_board[r2][c2]=new_board[r1][c1]; new_board[r1][c1]=EMPTY; new_board[jumped_r][jumped_c]=EMPTY
    # تبدیل به King
    for c in range(COLS):
        if new_board[0][c]==BLACK: new_board[0][c]=BLACK_KING
        if new_board[ROWS-1][c]==WHITE: new_board[ROWS-1][c]=WHITE_KING
    return new_board

# --- بررسی پایان بازی ---
def is_terminal(board):
    black_moves=get_moves(board,BLACK); white_moves=get_moves(board,WHITE)
    black_pieces=any(board[r][c] in [BLACK,BLACK_KING] for r in range(ROWS) for c in range(COLS))
    white_pieces=any(board[r][c] in [WHITE,WHITE_KING] for r in range(ROWS) for c in range(COLS))
    if not black_pieces or not black_moves: return True,WHITE
    if not white_pieces or not white_moves: return True,BLACK
    return False,None

# --- ارزیابی ---
def evaluate1(board):
    score=0
    for r in range(ROWS):
        for c in range(COLS):
            piece=board[r][c]
            if piece==BLACK: score+=1
            elif piece==BLACK_KING: score+=3
            elif piece==WHITE: score-=1
            elif piece==WHITE_KING: score-=3
    return score

# --- Minimax Alpha-Beta ---
def minimax_ab(board,depth,maximizing,eval_func,alpha=float('-inf'),beta=float('inf'),node_count=None):
    if node_count: node_count[0]+=1
    term,w=is_terminal(board)
    if depth==0 or term:
        return float('inf') if w==BLACK else float('-inf') if w==WHITE else eval_func(board)
    if maximizing:
        m=float('-inf')
        for move in get_moves(board,BLACK):
            m=max(m,minimax_ab(apply_move(board,move,BLACK),depth-1,False,eval_func,alpha,beta,node_count))
            alpha=max(alpha,m)
            if beta<=alpha: break
        return m
    else:
        m=float('inf')
        for move in get_moves(board,WHITE):
            m=min(m,minimax_ab(apply_move(board,move,WHITE),depth-1,True,eval_func,alpha,beta,node_count))
            beta=min(beta,m)
            if beta<=alpha: break
        return m

# --- انتخاب حرکت AI ---
def choose_move(board,player,depth=3,eval_func=evaluate1):
    moves=get_moves(board,player)
    if not moves: return None,0
    best_score,best_move,nodes=float('-inf'),None,[0]
    if player==BLACK: best_score=-float('inf')
    else: best_score=float('inf')
    for move in moves:
        new_board=apply_move(board,move,player)
        if player==BLACK:
            score=minimax_ab(new_board,depth-1,False,eval_func,node_count=nodes)
            if score>best_score: best_score,best_move=score,move
        else:
            score=minimax_ab(new_board,depth-1,True,eval_func,node_count=nodes)
            if score<best_score: best_score,best_move=score,move
    return best_move,nodes[0]

# --- بازی ---
def play_game():
    board=init_board()
    history=set()
    print_board(board)
    mode=input("Enter mode: 1=Human vs AI, 2=AI vs Human, 3=AI vs AI: ")
    human_player=None
    if mode=='1' or mode=='2':
        color=input("Choose your color (b=Black, w=White): ").lower()
        human_player=BLACK if color=='b' else WHITE
    player_turn=BLACK
    while True:
        # تشخیص تساوی با تکرار صفحه
        board_tuple=tuple(tuple(row) for row in board)
        if board_tuple in history:
            print("Draw! Board repeated.")
            break
        history.add(board_tuple)

        term,w=is_terminal(board)
        if term:
            print("Game Over! Winner:", "Black" if w==BLACK else "White")
            break
        print_board(board)
        if human_player==player_turn:
            moves=get_moves(board,player_turn)
            if not moves:
                print("No moves available for Human. Game over!")
                print("Winner:", "Black" if player_turn==WHITE else "White")
                break
            for i,m in enumerate(moves): print(f"{i}: {m}")
            idx=int(input("Select move: "))
            move=moves[idx]
        else:
            move,nodes=choose_move(board,player_turn)
            if move is None:
                print(f"No moves available for AI ({'Black' if player_turn==BLACK else 'White'}). Game over!")
                print("Winner:", "Black" if player_turn==WHITE else "White")
                break
            print(f"AI ({'Black' if player_turn==BLACK else 'White'}) move:",move,"Nodes:",nodes)
        board=apply_move(board,move,player_turn)
        player_turn=WHITE if player_turn==BLACK else BLACK

# --- اجرا ---
if __name__=="__main__":
    print("شروع بازی 6x6 Checkers")
    play_game()
