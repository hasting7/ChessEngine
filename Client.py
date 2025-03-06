import socket, datetime, sys, json, os, time
from Board import App
from FEN import FEN_String

PORT = 6000
BUFFER_SIZE = 1000;

MOVE = 0
VIEW = 1
UNDO = 2
GENERATE = 3
GET_COLOR = 4

class Client():
	def __init__(self):
		self.server = socket.socket()
		self.server.connect(('127.0.0.1', PORT))


		status, content = self.take_action(GET_COLOR, None)

		self.color = int(content);

		self.chess_app = App((500,500), self.color, self.handle_highlight, self.handle_move);

		status, content = self.take_action(VIEW, None)
		self.chess_app.board.render_fen(FEN_String(content))
		

	def take_action(self, action_name, args, buffer_size=BUFFER_SIZE):
		action_name = "%s"%action_name
		final_call = action_name + " " + " ".join([str(arg) for arg in args]) if args else action_name
		final_call += "\n\n";
		print("FINAL COMMAND",final_call)
		self.server.send(final_call.encode())

		response = self.server.recv(buffer_size).decode()

		print(response)

		return "OK", response;
		# json_resp = json.loads(response)

		# return json_resp['status'], json_resp['content']

	def handle_highlight(self, rf_name):
		print(rf_name);
		status, content = self.take_action(GENERATE, [rf_name])

		tile_indicies = []

		if content == '-1': return []

		for tile in content.split(','):
			index = (ord(tile[0]) - 97) * 8 + (ord(tile[1]) - 49);
			print(index, tile);

			tile_indicies.append(index)

		return tile_indicies

	def handle_move(self, rf_from, rf_to):
		print("moving: %s to %s"%(rf_from, rf_to))

		status, content = self.take_action(MOVE, [rf_from,rf_to])

		return content


	def mainloop(self):
		last_update = time.time()
		delta = 0.5
		while (1):
			self.chess_app.refresh()

			# command = input("Type Ready: ")

			# status, content = self.take_action(command, None)
			# print("%s\n%s"%(status, content))

			curr_time = time.time()
			if curr_time - last_update > delta:
				last_update = delta
				status, content = self.take_action(VIEW, None)
				self.chess_app.board.render_fen(FEN_String(content))


		self.server.close()

if __name__ == '__main__':
	c = Client();
	c.mainloop();