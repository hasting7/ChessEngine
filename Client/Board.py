from tkinter import *
from Client.FEN import *
from PIL import Image, ImageTk
import os
import chess
import chess.engine


def best_move_for_white(fen, stockfish_path="/opt/homebrew/bin/stockfish", elo=1320):
    board = chess.Board(fen)
    engine = chess.engine.SimpleEngine.popen_uci(stockfish_path)
    
    # Set engine to desired Elo rating
    engine.configure({"UCI_LimitStrength": True, "UCI_Elo": elo})
    
    # You can still set a move-time limit or depth for quicker response
    result = engine.play(board, chess.engine.Limit(time=0.1))
    
    engine.quit()
    
    return result.move.from_square, result.move.to_square

BLACK = 1
WHITE = 0
NONE = -1


COLORS = [
    'yellow',
    'blue',
    'red',
    'orange'
]

SELECTED = 0
OPTION = 1
HINT = 2
LAST = 3


class Tile:
    def __init__(self, drawer, canvas_box, index, board_pos, state=EMPTY):
        self.index = index
        self.board_pos = board_pos
        self.drawer = drawer
        self.state = state
        self.owner = None
        self.box = canvas_box

        self.default = self.drawer.itemcget(self.box, "fill")

        coords = self.drawer.coords(self.box)
        self.size = coords[2] - coords[0]
        self.img = None
        self.img_obj = self.drawer.create_image(
            coords[0] + self.size/2,
            coords[1] + self.size/2,
            anchor='center', image=self.img
        )

        # derive tile name from board position so coordinates match server
        file = chr((self.board_pos % 8) + ord('a'))
        rank = str(8 - (self.board_pos // 8))
        self.tile_name = f"{file}{rank}"
        # self.title = self.drawer.create_text((coords[0]+coords[2])/2,(coords[1]+coords[3])/2,text=index, fill='lime', font=('Arial',24));
        self.set_piece(state, False)

    def unhighlight(self):
        self.drawer.itemconfigure(self.box, fill=self.default,outline=self.default )

    def highlight(self, color):
        self.drawer.itemconfigure(self.box, fill=color,outline=color)


    def set_piece(self, state, optimize=True):
        if state == self.state and optimize:
            return None

        self.state = state
        if state == EMPTY:
            self.img = None
            self.owner = None
            self.drawer.itemconfigure(self.img_obj, image='')
            return self.index

        self.owner = WHITE if state.isupper() else BLACK
        img_path = os.path.join(os.path.abspath("."), 'sprites', STATE_FILES[self.state])
        image = Image.open(img_path).convert('RGBA')
        if image.size[0] != self.size or image.size[1] != self.size:
            image = image.resize((int(self.size), int(self.size)), Image.LANCZOS)
        self.img = ImageTk.PhotoImage(image)
        self.drawer.itemconfigure(self.img_obj, image=self.img)

        return self.index


class Board(Canvas):
    def __init__(self, player_color, tile_size, boarder_size, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.tile_size = tile_size
        self.player = player_color
        self.boarder_thickness = boarder_size

        self.create_mappings()
        self.tiles = self.create_board(tile_size)
        self.draw_labels()
        self.bind('<Button-1>', self.click)

        self.last_fen = None
        self.last_states = None

        self.handle_highlight = None
        self.handle_move = None

        self.moves = [set(), set(), set(), set()]

    def resize(self, tile_size):
        """Resize the board and all sprites to a new tile size."""
        self.tile_size = tile_size
        board_size = tile_size * 8
        self.config(width=board_size, height=board_size)

        for idx, tile in enumerate(self.tiles):
            j, i = divmod(idx, 8)
            x1 = i * tile_size
            x2 = (i + 1) * tile_size
            y1 = board_size - j * tile_size
            y2 = board_size - (j + 1) * tile_size
            self.coords(tile.box, x1, y1, x2, y2)
            self.coords(tile.img_obj, x1 + tile_size/2, y2 + tile_size/2)
            tile.size = tile_size
            tile.set_piece(tile.state, optimize=False)

        self.draw_labels()

    def create_mappings(self):
        self.board_to_fen = []
        self.fen_to_board = []
        for idx in range(64):
            j, i = divmod(idx, 8)
            if self.player == WHITE:
                fen = (7 - j) * 8 + i
            else:
                fen = j * 8 + (7 - i)
            self.board_to_fen.append(fen)

        for fen in range(64):
            r, c = divmod(fen, 8)
            if self.player == WHITE:
                board_idx = (7 - r) * 8 + c
            else:
                board_idx = r * 8 + (7 - c)
            self.fen_to_board.append(board_idx)

    def highlight_best_move(self, fen):
        from_idx, to_idx = best_move_for_white(fen)
        if (from_idx == None): return;
        # print(63 -from_idx,63- to_idx);

        self.unhighlight(HINT)
        self.highlight(self.fen_to_board[from_idx], HINT)
        self.highlight(self.fen_to_board[to_idx], HINT)

    def unhighlight(self, type):
        for index in self.moves[type]:
            keep = False
            for check_type in [SELECTED, OPTION, HINT, LAST]:
                # self.tiles[index].unhighlight();
                if check_type != type and index in self.moves[check_type]:
                    keep = True
                    self.highlight(index, check_type)
            if not keep:
                self.tiles[index].unhighlight();
            
        self.moves[type] = set();

    def highlight(self, tile_index, type):
        if type == SELECTED and self.tiles[tile_index].owner != self.player: return False
        self.moves[type].add(tile_index);
        self.tiles[tile_index].highlight(COLORS[type]);
        return True

        # MAKE THIS RETURN TRUE ON SUCESS   


    def click(self, event):
        x, y = int((event.x - self.boarder_thickness) / self.tile_size), int((event.y - self.boarder_thickness) / self.tile_size)
        print(x,y);

        index = (7 - y) * 8 + x
        tile = self.tiles[index]
        print("clicked on:", index, self.tiles[index].tile_name);

        if index in self.moves[OPTION]:
            from_index = self.moves[SELECTED].pop()
            from_rf = self.tiles[from_index].tile_name;
            self.moves[SELECTED].add(from_index)
            to_rf = self.tiles[index].tile_name;
            self.handle_move(from_rf,to_rf);

            self.unhighlight(LAST);
            self.highlight(from_index,LAST);
            self.highlight(index,LAST);
            self.unhighlight(SELECTED);
            self.unhighlight(OPTION);
            return;

        new_selection = False

        if index in self.moves[SELECTED]:
            self.unhighlight(SELECTED);
            self.unhighlight(OPTION);
        else:
            self.unhighlight(SELECTED);
            if len(self.moves[SELECTED]) == 0 and self.highlight(index, SELECTED):
                self.highlight(index, SELECTED)
                new_selection = True

        if new_selection:
            self.unhighlight(OPTION);
            tile_resp = self.handle_highlight(self.tiles[index].tile_name)
            for pos in tile_resp:
                board_idx = self.fen_to_board[pos]
                print(pos)
                self.highlight(board_idx, OPTION)


    def create_board(self, tile_size, colors=('#eeeed2', '#769656')):
        offset = (0, self.tile_size * 8)
        board_tiles = []
        k = 0
        for j in range(8):
            for i in range(8):
                board_pos = self.board_to_fen[k]
                r, c = divmod(board_pos, 8)
                color = colors[(r + c) % 2]
                obj = self.create_rectangle(
                    offset[0] + i * tile_size + self.boarder_thickness,
                    offset[1] - j * tile_size + self.boarder_thickness,
                    offset[0] + (i + 1) * tile_size + self.boarder_thickness,
                    offset[1] - (j + 1) * tile_size + self.boarder_thickness,
                    fill=color,
                    outline=color
                )
                board_tiles.append(Tile(self, obj, k, board_pos))
                k += 1
        return board_tiles

    def draw_labels(self):
        """Draw file (a-h) and rank (1-8) labels around the board."""
        board_size = self.tile_size * 8
        files = 'ABCDEFGH' 
        ranks = '12345678'

        if self.player == BLACK: files = files[::-1]
        if self.player == BLACK: ranks = ranks[::-1]

        label_font = ('Arial', int(self.tile_size * 0.3))

        margin = 0.17
        self.delete('label')

        # file labels along the bottom
        for i, f in enumerate(files):
            x = (i + 0.5) * self.tile_size + self.boarder_thickness
            y = board_size - self.tile_size * -margin + self.boarder_thickness
            self.create_text(x, y, text=f, font=label_font, tags='label')

        # rank labels along the left side
        for i, r in enumerate(ranks):
            x = self.tile_size * -margin + self.boarder_thickness
            y = board_size - (i + 0.5) * self.tile_size + self.boarder_thickness
            self.create_text(x, y, text=r, font=label_font, tags='label')


    def render_fen(self, fen_string):
        if fen_string.string == self.last_fen:
            return

        self.unhighlight(LAST)

        for fen_idx, state in enumerate(fen_string.states):
            board_idx = self.fen_to_board[fen_idx]
            tile = self.tiles[board_idx]
            changed = tile.set_piece(state)
            if self.last_fen is not None and changed is not None:
                if self.last_states[fen_idx] != state:
                    self.highlight(board_idx, LAST)

        self.last_states = fen_string.states
        self.last_fen = fen_string.string


        # self.highlight_best_move(self.last_fen)



class App(Tk):
    def __init__(self, size, color, highlight_handler, move_handler, on_quit, reset_handler):
        super().__init__()
        screen_width = self.winfo_screenwidth()
        screen_height = self.winfo_screenheight()
        tile_size = min(size) // 8
        board_size = tile_size * 8
        self.size = (board_size, board_size)
        self.color = color
        boarder_thickness = 40

#         self.resizable(0, 0)
        self.geometry(f"{board_size + boarder_thickness * 2}x{board_size + 25 + boarder_thickness * 2}+{(screen_width // 2) - (board_size // 2)}+{(screen_height // 2) - ((board_size + 120) // 2)}")
        self.title("Chess Engine")
        self.attributes('-topmost', True)

        self.board = Board(
            self.color,
            tile_size,
            boarder_thickness,
            self,
            width=board_size + 2 * boarder_thickness,
            height=board_size + 2 * boarder_thickness,
            highlightthickness=0,
            bd=0,
            bg='#4f301f'
        )
        self.board.pack(expand=True, fill=BOTH)

        # color_text = "White" if self.color == WHITE else "Black"
        # self.color_label = Label(self, text=f"You are {color_text}", font=('Arial', 12))
        # self.color_label.pack()

        self.reset_btn = Button(self, text="Reset Board", command=self.reset)
        self.reset_btn.pack(side=BOTTOM);

        self.board.handle_highlight = highlight_handler
        self.board.handle_move = move_handler
        self.on_quit = on_quit
        self.reset_handler = reset_handler

        self.bind('q', self.leave)
        self.board.bind('<Configure>', self.on_board_resize)

    def leave(self, e):
        print("QUITTING")
        self.on_quit()
        self.destroy()

    def refresh(self):
        self.update_idletasks()
        self.update()

    def on_board_resize(self, event):
        new_tile = min(event.width, event.height) / 8
        if abs(new_tile - self.board.tile_size) < 1:
            return
        self.board.resize(new_tile)

    def reset(self):
        print('reseting')
        self.reset_handler()
        self.board.render_fen(FEN_String())


if __name__ == '__main__':
    def dummy_highlight(tile):
        return []

    def dummy_move(frm, to):
        return "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

    app = App((500, 500), WHITE, dummy_highlight, dummy_move, lambda: None)
    app.board.render_fen(FEN_String())
    app.mainloop()
