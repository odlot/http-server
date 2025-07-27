#include <csignal>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <format>
#include <sys/select.h>

#define handle_error(msg)   \
    do                      \
    {                       \
        perror(msg);        \
        exit(EXIT_FAILURE); \
    } while (0)

volatile std::sig_atomic_t is_running = 1;

void signal_handler(int signal)
{
    is_running = 0;
}

int main(int argc, char *argv[])
{
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    struct sockaddr_in address;
    int port = 8080;
    char buffer[1024] = {0};

    int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_socket == -1)
        handle_error(NULL);
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    if (bind(tcp_socket, (struct sockaddr *)&address, sizeof(address)) == -1)
        handle_error(NULL);
    if (listen(tcp_socket, 1))
        handle_error(NULL);
    std::cout << std::format("Server is listening on port {}.", port) << std::endl;
    struct timeval timeout;
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    while (is_running)
    {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(tcp_socket, &readfds);
        int ready = select(tcp_socket + 1, &readfds, NULL, NULL, &timeout);
        if (ready == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }
            handle_error(NULL);
        }
        if (FD_ISSET(tcp_socket, &readfds))
        {
            int tcp_client_socket = accept(tcp_socket, NULL, NULL);
            if (tcp_client_socket == -1)
                handle_error(NULL);
            ssize_t bytes_read = read(tcp_client_socket, buffer, 1024);
            if (bytes_read == -1)
                handle_error(NULL);
            if (bytes_read > 0)
            {
                std::cout << std::format("Read {} byte(s).", bytes_read);

                const char *http_response = "HTTP/1.1 200 OK\r\n"
                                            "Content-Type: text/plain\r\n"
                                            "Content-Length: 13\r\n"
                                            "\r\n"
                                            "Hello, world!";
                ssize_t bytes_written = write(tcp_client_socket, http_response, strlen(http_response));
                if (bytes_written == -1)
                    handle_error(NULL);
                if (bytes_written > 0)
                {
                    std::cout << std::format("Wrote {} byte(s).", bytes_written);
                }
            }
            std::cout << "Attempting to close the client's TCP socket..." << std::endl;
            if (close(tcp_client_socket) == -1)
                handle_error(NULL);
            std::cout << "The client's TCP socket has been shut down." << std::endl;
        }
    }
    std::cout << "Attempting to close the server's TCP socket..." << std::endl;
    if (close(tcp_socket) == -1)
        handle_error(NULL);
    std::cout << "The server's TCP socket has been shut down." << std::endl;
    return 0;
}