from Client.Client import Client


IP = '46.110.48.34'
PORT = 3000

if __name__ == '__main__':
	c = Client(IP,PORT);
	c.mainloop();