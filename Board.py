from tkinter import *
from FEN import *
from PIL import Image, ImageTk
import os

IMG_SIZE = 45

BLACK = 1
WHITE = 0
NONE = -1

HIGHLIGHT = 'yellow'


class Tile():
	def __init__(self,drawer, canvas_box, index, state=EMPTY):
		self.index = index
		self.drawer = drawer
		self.state = state
		self.owner = None
		self.box = canvas_box
		coords = self.drawer.coords(self.box)
		self.size = coords[2] - coords[0]
		self.img = None
		self.img_obj = self.drawer.create_image(coords[0] + self.size/2,coords[1] + self.size/2, anchor='center', image=self.img)

		self.default_color = self.drawer.itemcget(self.box, "fill")

		self.tile_name = "%s%s"%(chr(int(index / 8 + 97)),chr((index % 8) + 49))

		self.was_updated = False

		#remove later
		text = '%d'%(index)


		self.name = self.drawer.create_text(coords[0] + self.size/2,coords[1] + self.size/2,anchor='center',font=('Arial',18),fill='#d90166',text=text)

		self.set_piece(state,False)
		

	def highlight(self, color):
		if self.drawer.player != self.owner or self.drawer.highlighted == self.index: return None
		self.drawer.itemconfigure(self.box, fill=color, outline=color)
		return self.index

	def mark_as_option(self):
		self.drawer.itemconfigure(self.box, fill='blue', outline='blue')
		return self.index


	def unhighlight(self):
		self.drawer.itemconfigure(self.box, fill=self.default_color, outline=self.default_color)
		

	def set_piece(self, state, optimize=True):
		if state == self.state and optimize: return # same state

		self.state = state

		if state == EMPTY:
			self.img = None
			self.owner = None
			return

		self.owner = WHITE if state.isupper() else BLACK
		self.img = ImageTk.PhotoImage(Image.open(os.path.join(os.path.abspath("."),'sprites',STATE_FILES[self.state])).convert('RGBA'))
		self.drawer.itemconfigure(self.img_obj, image = self.img)



class Board(Canvas):
	def __init__(self, player_color, tile_size, *args,**kwargs):
		super().__init__(*args,**kwargs)
		self.tile_size = tile_size
		self.player = player_color

		self.tiles = self.create_board(tile_size)

		self.bind('<Button-1>', self.click)

		self.highlighted = None
		self.option_tiles = []

		self.handle_highlight = None
		self.handle_move = None


	def click(self, event):
		x,y =  int(event.x/self.tile_size), int(event.y/self.tile_size)
		if self.player == WHITE: 
			y = 7 - y
			x = 7 - x
			
		index = y * 8 + x


		if index in self.option_tiles:
			print("MOVING", self.tiles[index].tile_name);
			fen = self.handle_move(self.tiles[self.highlighted].tile_name,self.tiles[index].tile_name)
			self.render_fen(FEN_String(fen))
			for tile in self.option_tiles:
				self.tiles[tile].unhighlight()
			self.option_tiles = []
			self.tiles[self.highlighted].unhighlight()
			self.highlighted = None

			return

		if self.highlighted != None:
			self.tiles[self.highlighted].unhighlight()
			for tile in self.option_tiles:
				self.tiles[tile].unhighlight()
			self.option_tiles = []

		self.highlighted = self.tiles[index].highlight(HIGHLIGHT)

		if self.highlighted != None:
			avaliable = self.handle_highlight(self.tiles[self.highlighted].tile_name);
			print(avaliable)
			if avaliable != -1:
				for tile_index in avaliable:
					self.tiles[tile_index].mark_as_option()
					self.option_tiles.append(tile_index);

		# move_generator()


	def create_board(self, tile_size, colors=('#769656','#eeeed2')):
		# right to left
		# down to up
		offset = (0,0)
		board_tiles = []
		for j in range(8):
			for i in range(8):
				location_i = i
				location_j = j
				if self.player == WHITE:
					location_j = 7 - j
					location_i = 7 - i
				obj = self.create_rectangle(
					offset[0] + location_i * tile_size,
					offset[0] + location_j * tile_size,
					offset[0] + (location_i + 1) * tile_size,
					offset[0] + (location_j + 1) * tile_size,
					fill=colors[(i+j)%2],
					outline=colors[(i+j)%2]
				)
				board_tiles.append(Tile(self,obj, j * 8 + i))

		return board_tiles

	def render_fen(self, fen_string):
		# white_bit = fen_string.white_bitmap
		for tile, state in zip(self.tiles, fen_string.states):
			tile.set_piece(state)



class App(Tk):
	def __init__(self, size, color, highlight_handler, move_handler, on_quit):
		super().__init__()
		screen_width = self.winfo_screenwidth()
		screen_height = self.winfo_screenheight()
		self.size = size
		self.color = color

		self.resizable(0,0)
		self.geometry("%sx%s+%s+%s"%(size[0],size[1],(screen_width // 2) - (size[0] // 2),(screen_height // 2) - ((size[1]) // 2)))
		self.title("Chess Engine")
		self.attributes('-topmost', True) 

		self.board = Board(self.color, size[0]/8, self , width=size[0], height=size[1], highlightthickness=0, bd=0, bg='white')
		self.board.pack()

		self.board.handle_highlight = highlight_handler
		self.board.handle_move = move_handler
		self.on_quit = on_quit
		self.board.player = color


		self.bind('q', self.leave)

	def leave(self,e):
		print("QUITTING")
		self.on_quit()
		self.destroy()
		

	def refresh(self):
		self.update_idletasks()
		self.update()



if __name__ == '__main__':
	a = App((500,500))

	a.board.render_fen(FEN_String())
	# de = FEN_String()
	# a.render_fen(FEN_String)

	a.mainloop()