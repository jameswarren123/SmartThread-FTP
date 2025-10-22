# SmartThread-FTP
A C based application built in a Unix environment which mimics common bash operations. This app enables the sending and retrieval of files across a TCP connection managed with appropriate file locks to allow for multiple simultaneous connections. All bash commands actually manipulate the file system of the server(not file transfer speed has been slowed to exagerate the difference in file sizes to show case terminate function).
## Features
- Working CLI to show case commands and Server outputs to show activity
- get [remote filename]
- put [local filename]
- delete [remote filename]
- ls
- cd [remote directory or ..]
- mkdir [remote directory]
- pwd
- terminate [command id]
- append any command with & to make it a background operation ie put file &
- quit
## Technologies Used
- Language: Java, Navigation is Bash
- Developed in: Emacs
- Environment: Unix server
- UI: CLI
## Setup
1. Clone this repository
```bash
git clone https://github.com/jameswarren123/SmartThread-FTP.git
```
2. Open in a compatible C environment where multiple connections are possible
3. Use ./client/myftp [server ip] [command port] [termination port] and ./client/myftp [command port] [termination port] for 1 server and as many clients as wanted
