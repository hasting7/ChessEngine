import socket
import time
from Client.Board import App
from Client.FEN import FEN_String
BUFFER_SIZE = 1000

MOVE = 0
VIEW = 1
UNDO = 2
GENERATE = 3
GET_COLOR = 4
RESET = 5

class Client():
	def __init__(self, ip, port):
		self.server = socket.socket()
		self.server.connect((ip, port))
		self.ready_quit = False
		self.viewer = False

		status, content = self.take_action(GET_COLOR, None)
		self.color = int(content)

		if self.color == 3:  # viewer
			self.viewer = True
			self.color = 0

		self.chess_app = App((600,600), self.color, self.handle_highlight, self.handle_move, self.quit_game, self.reset_board);

	def quit_game(self):
		self.ready_quit = True
		self.server.close()

	def take_action(self, action_name, args, buffer_size=BUFFER_SIZE):
		action_name = f"{action_name}"
		final_call = (
			action_name + " " + " ".join(str(arg) for arg in args) if args else action_name
		)
		final_call += "\n\n"
		self.server.send(final_call.encode())

	def take_action(self, action_name, args, buffer_size=BUFFER_SIZE):
		action_name = "%s"%action_name
		final_call = action_name + " " + " ".join([str(arg) for arg in args]) if args else action_name
		final_call += "\n\n";
		# print(final_call)
		self.server.send(final_call.encode())
		response = self.server.recv(buffer_size).decode()
		return "OK", response

	def handle_highlight(self, rf_name):
		status, content = self.take_action(GENERATE, [rf_name])

		if content == "-1":
			return []

		tile_indices = []
		for tile in content.split(","):
			file = ord(tile[0]) - ord("a")
			rank = ord(tile[1]) - ord("1")
			index = (7 - rank) * 8 + file
			tile_indices.append(index)

		print(tile_indices)

		return tile_indices

	def handle_move(self, rf_from, rf_to):
		status, content = self.take_action(MOVE, [rf_from, rf_to])
		return content



	def reset_board(self):
		status, content = self.take_action(RESET, None)
		print(status, content)


	def mainloop(self):
		last_update = time.time()
		delta = 0.5
		while (not self.ready_quit):
			self.chess_app.refresh()

			# command = input("Type Ready: ")

			# status, content = self.take_action(command, None)
			# print("%s\n%s"%(status, content))

			curr_time = time.time()
			if curr_time - last_update > delta:
				last_update = delta
				status, content = self.take_action(VIEW, None)
				# print(content)
				self.chess_app.board.render_fen(FEN_String(content))


		self.server.close()

if __name__ == '__main__':
	c = Client();
	c.mainloop();
