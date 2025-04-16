from tkinter import *
from Client.FEN import *
from PIL import Image, ImageTk
import os
import chess
import chess.engine


def best_move_for_white(fen, stockfish_path="/opt/homebrew/bin/stockfish", depth=12):

	board = chess.Board(fen)

	engine = chess.engine.SimpleEngine.popen_uci(stockfish_path)

	result = engine.play(board, chess.engine.Limit(depth=depth))
	engine.quit()

	return result.move.from_square, result.move.to_square

IMG_SIZE = 45

BLACK = 1
WHITE = 0
NONE = -1

HIGHLIGHT_COLOR = 'yellow'
OPTION_COLOR = 'blue'
MOVE_COLOR = 'red'


class Tile:
	def __init__(self, drawer, canvas_box, index, state=EMPTY):
		self.index = index
		self.drawer = drawer
		self.state = state
		self.owner = None
		self.box = canvas_box

		coords = self.drawer.coords(self.box)
		self.size = coords[2] - coords[0]
		self.img = None
		self.img_obj = self.drawer.create_image(
			coords[0] + self.size/2,
			coords[1] + self.size/2,
			anchor='center', image=self.img
		)

		self.default_color = self.drawer.itemcget(self.box, "fill")
		self.tile_name = f"{chr(int(index / 8 + 97))}{chr((index % 8) + 49)}"
		# self.tile_name = '%d'%self.index;
		self.title = self.drawer.create_text((coords[0]+coords[2])/2,(coords[1]+coords[3])/2,text=self.tile_name, fill='lime', font=('Arial',24));
		self.was_updated = False
		self.set_piece(state, False)

	def highlight(self, color):
		if self.drawer.player != self.owner or self.drawer.highlighted == self.index:
			return None
		self.drawer.itemconfigure(self.box, fill=color, outline=color)
		return self.index

	def mark_as_option(self):
		self.drawer.itemconfigure(self.box, fill=OPTION_COLOR, outline=OPTION_COLOR)
		return self.index

	def unhighlight(self):
		self.drawer.itemconfigure(self.box, fill=self.default_color, outline=self.default_color)

	def set_piece(self, state, optimize=True):
		if state == self.state and optimize:
			return

		self.state = state
		if state == EMPTY:
			self.img = None
			self.owner = None
			self.drawer.itemconfigure(self.img_obj, image='')
			return

		self.owner = WHITE if state.isupper() else BLACK
		img_path = os.path.join(os.path.abspath("."), 'sprites', STATE_FILES[self.state])
		self.img = ImageTk.PhotoImage(Image.open(img_path).convert('RGBA'))
		self.drawer.itemconfigure(self.img_obj, image=self.img)


class Board(Canvas):
	def __init__(self, player_color, tile_size, *args, **kwargs):
		super().__init__(*args, **kwargs)
		self.tile_size = tile_size
		self.player = player_color

		self.tiles = self.create_board(tile_size)
		self.bind('<Button-1>', self.click)

		self.highlighted = None
		self.option_tiles = []
		self.last_move = None

		self.handle_highlight = None
		self.handle_move = None

		self.last_fen = None

	def click(self, event):
		x, y = int(event.x / self.tile_size), int(event.y / self.tile_size)

		# print(x,y)
		# print(x,7 - y)
		# if self.player == BLACK:
		# 	y = 7 - y
		# 	# x = 7 - x
		# # else:
		# 	# x = 7 - x
		index = (7 - y) * 8 + x

		print("clicked on:", self.tiles[index].tile_name);

		# Clear previous move highlights
		if self.last_move:
			self.tiles[self.last_move[0]].unhighlight()
			self.tiles[self.last_move[1]].unhighlight()
			self.last_move = None

		# Move handling
		if index in self.option_tiles:
			from_tile = self.highlighted
			to_tile = index;
			print("gonna move")
			fen = self.handle_move(self.tiles[from_tile].tile_name, self.tiles[to_tile].tile_name)
			self.render_fen(FEN_String(fen))

			for tile in self.option_tiles:
				self.tiles[tile].unhighlight()
			self.tiles[self.highlighted].unhighlight()

			# self.tiles[from_tile].highlight(MOVE_COLOR)
			# self.tiles[to_tile].highlight(MOVE_COLOR)
			self.last_move = (from_tile, to_tile)

			self.option_tiles = []
			self.highlighted = None
			return

		# Clear highlights if new selection
		if self.highlighted is not None:
			self.tiles[self.highlighted].unhighlight()
			for tile in self.option_tiles:
				self.tiles[tile].unhighlight()
			self.option_tiles = []

		print('--')
		print(index);
		self.highlighted = self.tiles[index].highlight(HIGHLIGHT_COLOR);
		print(self.highlighted)
		print('--')

		if self.highlighted is not None:
			print(self.highlighted)
			print("higlighed tile:",self.tiles[self.highlighted].tile_name);
			available = self.handle_highlight(self.tiles[self.highlighted].tile_name)
			if available != -1:
				for tile_index in available:
					self.tiles[tile_index].mark_as_option()
					self.option_tiles.append(tile_index)

	def create_board(self, tile_size, colors=('#eeeed2','#769656')):
		offset = (0, 0)
		board_tiles = []
		for j in range(7,-1,-1):
			for i in range(8):
				obj = self.create_rectangle(
					offset[0] + i * tile_size,
					offset[1] + (7-j) * tile_size,
					offset[0] + (i + 1) * tile_size,
					offset[1] + ((7-j) + 1) * tile_size,
					fill=colors[(i + j) % 2],
					outline=colors[(i + j) % 2]
				)
				board_tiles.append(Tile(self, obj, j * 8 + i))
		return board_tiles

	def highlight_best_move(self, fen):
		from_idx, to_idx = best_move_for_white(fen)
		if (from_idx == None): return;
		print(63 -from_idx,63- to_idx);

		# Unhighlight previous move if needed
		if self.last_move:
			self.tiles[self.last_move[0]].unhighlight()
			self.tiles[self.last_move[1]].unhighlight()

		# Highlight new best move
		self.tiles[from_idx].highlight('red')
		self.tiles[to_idx].highlight('red')

		self.last_move = (from_idx, to_idx)

	def render_fen(self, fen_string):
		if (fen_string.string == self.last_fen): return;
		self.last_fen = fen_string.string
		print(fen_string.string)
		for tile, state in zip(self.tiles, fen_string.states):
			tile.set_piece(state)

		self.highlight_best_move(fen_string.string);


class App(Tk):
	def __init__(self, size, color, highlight_handler, move_handler, on_quit):
		super().__init__()
		screen_width = self.winfo_screenwidth()
		screen_height = self.winfo_screenheight()
		self.size = size
		self.color = color

		self.resizable(0, 0)
		self.geometry(f"{size[0]}x{size[1]}+{(screen_width // 2) - (size[0] // 2)}+{(screen_height // 2) - (size[1] // 2)}")
		self.title("Chess Engine")
		self.attributes('-topmost', True)

		self.board = Board(self.color, size[0] / 8, self, width=size[0], height=size[1], highlightthickness=0, bd=0, bg='white')
		self.board.pack()

		self.board.handle_highlight = highlight_handler
		self.board.handle_move = move_handler
		self.on_quit = on_quit

		self.bind('q', self.leave)

	def leave(self, e):
		print("QUITTING")
		self.on_quit()
		self.destroy()

	def refresh(self):
		self.update_idletasks()
		self.update()


if __name__ == '__main__':
	def dummy_highlight(tile):
		return []

	def dummy_move(frm, to):
		return "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

	app = App((500, 500), WHITE, dummy_highlight, dummy_move, lambda: None)
	app.board.render_fen(FEN_String())
	app.mainloop()