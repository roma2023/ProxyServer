# Proxy Lab: Caching Web Proxy

## Introduction
This repository contains my solution for the Proxy Lab project assigned as part of the 15-213/15-513 course during the Summer 2023 semester. The goal of this project is to implement a caching web proxy with support for handling HTTP/1.0 GET requests, concurrency, and caching of web objects.

## Project Overview
A proxy server acts as an intermediary between clients and web servers, forwarding requests and responses. This project consists of three parts:

### Part I: Implementing a Sequential Web Proxy
- Handling HTTP/1.0 GET requests
- Parsing request headers
- Forwarding requests to web servers
- Reading server responses and forwarding them to clients

### Part II: Dealing with Multiple Concurrent Requests
- Using POSIX Threads for concurrency
- Implementing a basic thread-safe proxy

### Part III: Caching Web Objects
- Adding a cache to the proxy
- Implementing a least-recently-used (LRU) eviction policy
- Ensuring thread safety for cache operations

## Getting Started
Follow these instructions to build and run the proxy on your local machine.

### Prerequisites
- [GCC](https://gcc.gnu.org/) (GNU Compiler Collection)
- Unix-based operating system (Linux or macOS)

### Build
Compile the proxy by running the following command:

```bash
$ make
