from includes.StatusCodes import *
from ChessBackend import Chess
import socket, threading, datetime, json, sys

class AtomicData():
	def __init__(self, init_value=None):
		self.value = init_value

		self.mutex = threading.Lock()

	def set(self, value):
		with self.mutex:
			self.value = value

	def get(self):
		with self.mutex:
			return self.value

	def bump(self, change):
		with self.mutex:
			self.value += change
			return self.value

class ClientInstance:
	def __init__(self, socket, address, thread):
		self.name = AtomicData()
		self.address = address
		self.socket = socket
		self.thread = thread

		self.claimed = False


class ChessServer:
	def __init__(self, port):
		self.socket = socket.socket()
		self.socket.bind(('0.0.0.0', port))
		self.socket.listen(MAX_THREADS)

		self.threads = {}
		self.lock = threading.Lock()

		self.table = Chess()
		self.table_lock = threading.Lock()


	def start(self):
		try:
			while True:
				c, addr = self.socket.accept()
				print(f"Connection accepted from {addr}")
				
				new_thread = threading.Thread(target=self.thread_mainloop, args=(c, addr))
				client_instances = ClientInstance(c, addr, new_thread)
				
				with self.lock:  # Ensure thread list is thread-safe
					self.threads[addr] = client_instances
				
				new_thread.start()
		except KeyboardInterrupt:
			self.shutdown()

	def construct_response(self,content, status):
		time = self.last_updated_time.get().isoformat()

		data = {
			'status' : status,
			'content': content
		}
		# print(json.dumps(data))
		return json.dumps(data)

	def parse_request(self, request, address):

		args = request.split(' ')
		command = args[0]
		args = args[1:]

		status = None
		content = None

		if command == JOIN:
			with self.lock:
				client_data = self.threads[address]

				if not client_data.claimed and self.table.has_room():
					status = SUCCESS
					content = '%s added as a player'%args[0]

					# add to table
					with self.table_lock:
						self.table.join_table(address, client_data.money.bump, lambda : print("No leave function yet"))
				else:
					status = ILLEGAL
					content = 'aleady claimed'

		elif command == LEAVE:
			status = INVALID


		elif command == VIEW:
			status = SUCCESS

			content={
				'string' : 'rnbqkbnr/ppp2ppp/8/3pp3/8/4P3/PPPP1PPP/RNBQKBNR w KQkq - 0 3',
				'player' : 0
			}

		return content, status

	def thread_mainloop(self, client, addr):
		try:
			# Placeholder for actual client handling logic
			while True:
				data = client.recv(BUFFER_SIZE).decode()
				if not data:
					break

				content, status = self.parse_request(data, addr)

				response_message = self.construct_response(content, status)

				# print(f"Received from {addr}: {data.decode()}")
				client.send(response_message.encode())  # Echo back the data

				if status == CLOSED:
					with self.lock:
						client_data = self.threads[addr]
						client_data.socket.close()

						del self.threads[addr]
						# client_data.thread.join()
					break
					# close connection 


		except Exception as e:
			raise e
			print(f"Error with client {addr}: {e}")
			with self.table_lock:
				self.table.leave_table(addr)
		finally:
			client.close()
			print(f"Connection with {addr} closed")

	def shutdown(self):
		print("Shutting down...")
		with self.lock:
			for address, client_data in self.threads.items():
				client_data.socket.close()
				client_data.thread.join()
		self.socket.close()
		print("Server closed")

# game thread
# create a request queue and a action queue


if __name__ == '__main__':
	port = PORT
	if len(sys.argv) == 2:
		try:
			port = int(sys.argv[1])
		except:
			print("invalid port")

	print("server starting on port: %d"%port)

	server = BlackJackServer(port)
	server.start()
