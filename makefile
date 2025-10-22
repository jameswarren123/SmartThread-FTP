compile:
	gcc client/myftp.c -o client/myftp -lpthread -lm
	gcc server/myftpserver.c -o server/myftpserver -lpthread -lm

clean:
	rm -f client/myftp
	rm -f server/myftpserver
