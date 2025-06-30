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

IMG_SIZE = 45

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
	def __init__(self, drawer, canvas_box, index, state=EMPTY):
		self.index = index
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

		# self.default_color = self.drawer.itemcget(self.box, "fill")
		self.tile_name = f"{chr((index % 8 + 97))}{chr(int(index / 8) + 49)}"
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
		self.img = ImageTk.PhotoImage(Image.open(img_path).convert('RGBA'))
		self.drawer.itemconfigure(self.img_obj, image=self.img)

		return self.index


class Board(Canvas):
	def __init__(self, player_color, tile_size, *args, **kwargs):
		super().__init__(*args, **kwargs)
		self.tile_size = tile_size
		self.player = player_color

		self.tiles = self.create_board(tile_size)
		self.bind('<Button-1>', self.click)

		self.last_fen = None;

		self.handle_highlight = None;
		self.handle_move = None;

		self.moves = [set(),set(),set(),set()]

	def highlight_best_move(self, fen):
		from_idx, to_idx = best_move_for_white(fen)
		if (from_idx == None): return;
		# print(63 -from_idx,63- to_idx);

		self.unhighlight(HINT)
		self.highlight(from_idx, HINT);
		self.highlight(to_idx, HINT);

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
		x, y = int(event.x / self.tile_size), int(event.y / self.tile_size)
		print(x,y);

		index = (7-y) * 8 + x;
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
			for tile in tile_resp:
				print(tile)
				self.highlight(tile, OPTION);


	def create_board(self, tile_size, colors=('#eeeed2','#769656')):
		offset = (0, self.tile_size*8)
		board_tiles = []
		k = 0
		for j in range(8):
			for i in range(8):
				obj = self.create_rectangle(
					offset[0] + i * tile_size,
					offset[1] - j * tile_size,
					offset[0] + (i + 1) * tile_size,
					offset[1] - (j + 1) * tile_size,
					fill=colors[(i + j) % 2],
					outline=colors[(i + j) % 2]
				)
				board_tiles.append(Tile(self, obj, k))
				k += 1;
		return board_tiles

	def render_fen(self, fen_string):
		if (fen_string.string == self.last_fen): return;
		
		print(fen_string.string)

		self.unhighlight(LAST);

		# tiles = self.tiles;
		for i in range(7,-1,-1):
			i_rev = (7 - i) * 8
			i *= 8;
			print(i,i+8, i_rev, i_rev+8)
			for tile, state in zip(self.tiles[i:i+8], fen_string.states[i_rev:i_rev+8]):
				updated_index = tile.set_piece(state)
				if self.last_fen != None and updated_index != None:
					self.highlight(updated_index,LAST);

		self.last_fen = fen_string.string


		# self.highlight_best_move(self.last_fen)



class App(Tk):
	def __init__(self, size, color, highlight_handler, move_handler, on_quit, reset_handler):
		super().__init__()
		screen_width = self.winfo_screenwidth()
		screen_height = self.winfo_screenheight()
		self.size = size
		self.color = color

		self.resizable(0, 0)
		self.geometry(f"{size[0]}x{size[1]+100}+{(screen_width // 2) - (size[0] // 2)}+{(screen_height // 2) - (size[1] // 2)}")
		self.title("Chess Engine")
		self.attributes('-topmost', True)

		self.board = Board(self.color, size[0] / 8, self, width=size[0], height=size[1], highlightthickness=0, bd=0, bg='white')
		self.board.pack()

		self.reset_btn = Button(self, text="Reset Board", command=self.reset)
		self.reset_btn.pack(side=BOTTOM);

		self.board.handle_highlight = highlight_handler
		self.board.handle_move = move_handler
		self.on_quit = on_quit
		self.reset_handler = reset_handler

		self.bind('q', self.leave)

	def leave(self, e):
		print("QUITTING")
		self.on_quit()
		self.destroy()

	def refresh(self):
		self.update_idletasks()
		self.update()

	def reset(self):
		print('reseting')
		self.reset_handler()


if __name__ == '__main__':
	def dummy_highlight(tile):
		return []

	def dummy_move(frm, to):
		return "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

	app = App((900, 900), WHITE, dummy_highlight, dummy_move, lambda: None)
	app.board.render_fen(FEN_String())
	app.mainloop()
