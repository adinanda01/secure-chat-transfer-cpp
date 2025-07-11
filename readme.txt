g++ client/main.cpp -o client/client -pthread $CPPFLAGS $LDFLAGS -lssl -lcrypto


g++ server/main.cpp -o server/server -pthread


g++ server/main.cpp server/server_utils.cpp -o server/server -pthread \
-I$(brew --prefix openssl)/include \
-L$(brew --prefix openssl)/lib \
-lssl -lcrypto


now we can chat to two personal people using our chatting app (this is my personal app to chat)
u just need to register and login then you can caryy on the talking till u dead :)

> to clear the database
 cat server/users.db


> File Transfer Protocol
Here’s how it works:

Client sends to server:
/sendfile bob filename.ext

<file-size> (as string, e.g., 128000\n)

<file-bytes> (raw content)

<checksum>\n (SHA-256 of file bytes)

Server forwards to bob:
Message: FILE_INCOMING alice filename.ext size

Then sends raw bytes

Then sends checksum

Receiver client:
Automatically accepts the files

Recomputes checksum to verify integrity




Goal:
When the user types:
/sendfile bob path/to/file.txt

The client should:
Open the file
Read its contents
Compute file size and checksum

Send:
/sendfile bob filename.ext
File size (in bytes)
Raw file bytes
SHA-256 checksum :




When the server sends this to the receiving client:
FILE_INCOMING alice filename.ext size

The client should:

Parse sender, filename, and size

Receive raw file bytes

Receive checksum

Save file

Compute and verify checksum

Show success/failure

