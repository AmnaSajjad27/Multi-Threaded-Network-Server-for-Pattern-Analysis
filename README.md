Server Program Setup and Usage Guide

1. Compiling the Server Program

To compile the server program, simply use the provided makefile. Open your terminal and execute the following command:

make

This will generate the executable file.

2. Starting the Server

Initiate the server using the following command;

./assignment3 -l 12345 -p "happy"

Where:
- ./assignment3 is the generated executable file.
- -l 12345 specifies the port number (you can change 12345 to your desired port).
- -p "happy" represents the pattern you are searching for in your books.

The server will now be ready to receive incoming connections.

3. Sending a Book using Netcat

Open a new terminal window which will act as the Netcat client.

4. Connecting to the Server

Use Netcat to connect to the server with the following command, replacing 12345 with your chosen port number:

nc localhost 12345

5. Sending Book Data

Now, send the book data using the following command. Make sure you have saved the book in a .txt format in the same directory beforehand:

nc <server_ip> <port> < <file_name>

6. Repeat for Multiple Books
nc <server_ip> < <file_name>

Repeat for Multiple Books
If you have more books to send, repeat step 5 for each additional book.

Finishing Book Transfer
Once you have sent all the books, press Ctrl + D in the Netcat terminal.

The server program will then start analyzing the input and search for occurrences of your desired string.