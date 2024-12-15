# IF2230 Computer Networks Project: TCP Over UDP

## Table of Contents
- [General Information](#general-information)
- [Team Members](#team-members)

## Run the Project

### 1. Clone the Repository

```bash
git clone https://github.com/labsister21/project-1-itebeeeee.git

cd project-1-itebeeeee
```

### 2. Compile using makefile

```bash
make clean
make
```

### 3. Run the Server 

```bash
./node <ip> <port>

./node 127.0.0.1 1337

[i] Node started at 127.0.0.1:1337
[?] Please enter the operating mode (sender / receiver):
[?] 1. Sender
[?] 2. Receiver
[?] Enter your choice: 1
[i] Node is now a sender
[?] Please choose the error detection method:
[?] 1. Checksum
[?] 2. CRC
[?] Enter your choice: 2
[?] Please choose the sending mode:
[?] 1. User Input
[?] 2. File Input
[?] Enter your choice: 2
[?] Please enter the file path: <path to file>
```

### 4. Run the Client

```bash
./node <ip> <port>

./node 127.0.0.1 1338

[i] Node started at 127.0.0.1:1338
[?] Please enter the operating mode (sender / receiver):
[?] 1. Sender
[?] 2. Receiver
[?] Enter your choice: 2
[i] Node is now a receiver
[?] Please enter the server port: 1337
```

### 4. Wait for the packets to be sent.

## Team Members

| **NIM**  |           **Nama**            |
| :------: | :---------------------------: |
| 13522018 |      Ibrahim Ihsan Rasyid     |
| 13522020 | Aurelius Justin Philo Fanjaya |
| 13522032 |         Tazkia Nizami         |
| 13522040 |        Dhidit Abdi Aziz       |