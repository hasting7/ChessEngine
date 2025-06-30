from Client.Client import Client


IP = '127.0.0.1'
PORT = 6000

if __name__ == '__main__':
	c = Client(IP,PORT);
	c.mainloop();