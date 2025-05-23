DEFAULT_FEN = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1'

STATE_FILES = {
	'p': 'black-pawn.png',
	'P': 'white-pawn.png',
	'n': 'black-knight.png',
	'N': 'white-knight.png',
	'b': 'black-bishop.png',
	'B': 'white-bishop.png',
	'r': 'black-rook.png',
	'R': 'white-rook.png',
	'q': 'black-queen.png',
	'Q': 'white-queen.png',
	'k': 'black-king.png',
	'K': 'white-king.png',
	'' : None
}

EMPTY = ''

class FEN_String():
	def __init__(self, fen_string=DEFAULT_FEN):
		sections = fen_string.split(' ')
		self.half_moves = int(sections[4])
		self.turn = int(sections[5])

		ranks = sections[0].split('/')
		self.string = fen_string;
		self.states = []

		for i, rank in enumerate(ranks):
			for char in rank:
				if char.isdigit():
					for i in range(int(char)):
						self.states.append('')
				else:
					self.states.append(char)
