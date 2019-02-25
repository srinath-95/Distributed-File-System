README for Distributed File system

Client side:

Implemented a menu of commands -
1. get - Gets all the 4 file parts and assigns on the client side to get the entire file.
2. put - PUT command uploads file onto 4 DFS using md5sum%4 value of file. File parts are uploaded according to the table provided.
3. list - LIST command gets all the file pieces from all teh servers and checks if the pieces can be formed to file a complete file or not, which is mentioned if files are complete or not
4. mkdir - Implemented an extra command “mkdir” with in DFC, so that you can make subfolder on DFS under user's folder.

All possible error conditions are handled gracefully.
Each command is sent with the username and password info for User authentication. Request would be served only if user validation happens.
Encryption is also implemented.. For encryption I have used XOR technique.

Server side:

1. I have create 4 servers to handle the requests from the client and authentication is done.
2. The above requests are served gracefully for each request coming from the client and all the error conditions are handled.

How to run:
Client:

>make
>./client <conf file> 

In each server folder
>make
>./server <IP_ADDRr> <port number>


