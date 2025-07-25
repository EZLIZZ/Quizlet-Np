# Quizlet-Np
A multi-client quiz game written in C using TCP sockets and pthreads. Multiple players connect to a server from different terminals and answer quiz questions in real-time.


⚙️ How to Compile
Compile the server:

``` bash
gcc server.c -o server -lpthread
Compile the client:

bash
gcc client.c -o client
```
How to Run
Start the Server
Run:

```bash
./server
The server listens on TCP port 5555 and waits for clients.
```
Connect Clients
Open separate terminals for each player. In each terminal, run:

```bash
./client
Follow prompts:

Enter your name

Answer quiz questions (A/B/C/D)
```
🎮 Game Flow
Server waits for the required number of clients.

Clients enter their names.

Server broadcasts quiz questions.

Clients submit answers.

Server updates the scoreboard after each question.

Quiz finishes with final scores.

✅ Requirements
Linux or macOS

GCC compiler


