# One-Time-Pad
One-Time Pad Client/Server in C

Written for Operating Systems CS344

These programs are designed to run on Unix systems. This program creates encryption and decryption servers, clients to connect to those servers, and a keygen program to create the key needed to encrypt/decrypt plaintext files. These connection are created by TCP sockets. The servers create 5 pthreads each. Every connection is serviced by a single thread.

To run, download all files and run `make` in the root directory.
After the enc_server, enc_client, dec_server, dec_client, and keygen exec files are created, run `./p5testscript.sh` to run the testscript.
This should run all exec files and create text files as the output of the clients/servers. The testscript will grade the project, which should be 150/150.

keygen outputs a string ending w/ a newline of desired length using syntax `./keygen [number] > key_output`.

enc_server and dec_server requires a unique port number to host an encryption/decryption server. 
Syntax of that is `./enc_server [port] &` or `./dec_server [port] &` to run both in the background.

enc_client and dec_client requires plain text or encrypted text, respectively, and the key text generated by keygen.
These client programs also require the ports used to host enc_server and dec_server.
Syntax to use client programs is `./enc_client [plaintext] [key] [enc_server port]` or `./dec_client [ciphertext] [key] [dec_server port]`


