# Bulletin Board System

This is a simple BBS server written in C++ that allows users to interact with each other by:
1. Creating boards
2. Creating posts and commenting
3. Live chat

## Running the server
Compile the project by typing `make` under `src/`.

Run the server by `./server <port-number>`.

## Connecting to the server
### Option 1: Using `network cat`
Clients may test the server by simply typing `nc <server-address> <port-number>`.
However, the live chat server is not supported here.

### Option 2: Using the provided `client`
Run the provided client by `python3 client.py <port-number>`.

## Test environment
* OS: `ubuntu 20.04`
* C++ version: `C++ 14`