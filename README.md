# ProxyLab Project

This repository contains the code and documentation for the ProxyLab project, which is part of the course on Computer Networks. The project involves developing a multi-threaded web proxy that is capable of handling HTTP requests efficiently. This proxy offers features like caching and dynamic content filtering to optimize web traffic.

## Features
- **Multi-threaded Design:** Enables concurrent processing of multiple client requests.
- **Caching:** Implements an in-memory cache to store frequently requested web objects, reducing bandwidth usage and improving response times.
- **Dynamic Content Filtering:** Blocks inappropriate content based on predefined filtering criteria, ensuring secure browsing.
- **Logging and Monitoring:** Keeps detailed logs of client requests and server responses for debugging and monitoring purposes.

## Installation and Usage
1. Clone this repository to your local machine.
    ```bash
    git clone https://github.com/your-username/proxylab.git
    ```
2. Navigate to the project directory and compile the source code.
    ```bash
    cd proxylab
    make
    ```
3. Start the proxy server by executing the following command:
    ```bash
    ./proxy <port>
    ```
   Replace `<port>` with the desired port number.

4. Configure your web browser to use the proxy server:
   - Set the proxy address to `localhost:<port>`.
   - Browse as usual, and the proxy server will intercept and handle the HTTP traffic.

## Architecture
- **Client Request Handler:** Accepts incoming connections and processes client requests in separate threads to ensure high throughput.
- **Cache Manager:** Handles the caching mechanism, keeping frequently accessed web objects in memory for faster retrieval.
- **Content Filter:** Analyzes web content to block certain URLs or inappropriate data according to specified rules.
- **Logger:** Records logs of HTTP transactions for analysis.

## Requirements
- **C Compiler:** A GCC or Clang compiler is recommended for building the source code.
- **POSIX Threads:** The project uses POSIX threads for concurrent processing.

## Contribution Guidelines
- Fork this repository and create your branch.
- Submit a pull request with a clear description of your changes.
- Ensure your code passes the existing unit tests and follows coding style guidelines.

## License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
