#pragma once
#include <cassert>
#include <stdexcept>

#include "io_context.h"
#include "overlapped_awaitable.h"
#include <mswsock.h>

struct send_awaitable;
struct recv_awaitable;
struct accept_awaitable;

struct tcp_socket
{
    explicit tcp_socket(io_context& ctx);

    tcp_socket(tcp_socket const& other) = delete;
    tcp_socket& operator=(tcp_socket const& other) = delete;

    tcp_socket(tcp_socket&& other) noexcept;
    tcp_socket& operator=(tcp_socket&& other) noexcept;

    ~tcp_socket();

    void bind(int port);
    void listen();

    send_awaitable send(void const* buf, size_t buf_size);
    recv_awaitable recv(void* buf, size_t buf_size);
    accept_awaitable accept();

private:
    io_context* ctx;
    SOCKET handle;

    friend struct accept_awaitable;
};

struct send_awaitable : overlapped_awaitable
{
    send_awaitable(SOCKET socket, void const* buf, size_t buf_size);

    send_awaitable(send_awaitable const& other) = delete;
    send_awaitable(send_awaitable&&) = delete;
    send_awaitable& operator=(send_awaitable const& other) = delete;
    send_awaitable& operator=(send_awaitable&& other) = delete;

    bool await_ready() noexcept;
    size_t await_resume() noexcept;

private:
    bool pending = false;
};


struct recv_awaitable : overlapped_awaitable
{
    recv_awaitable(SOCKET socket, void* buf, size_t buf_size);

    recv_awaitable(recv_awaitable const& other) = delete;
    recv_awaitable(recv_awaitable&&) = delete;
    recv_awaitable& operator=(recv_awaitable const& other) = delete;
    recv_awaitable& operator=(recv_awaitable&& other) = delete;

    bool await_ready() noexcept;
    size_t await_resume() noexcept;

private:
    bool pending = false;
};

struct accept_awaitable : overlapped_awaitable
{
    accept_awaitable(SOCKET listened, io_context& ctx);

    accept_awaitable(accept_awaitable const& other) = delete;
    accept_awaitable(accept_awaitable&&) = delete;
    accept_awaitable& operator=(accept_awaitable const& other) = delete;
    accept_awaitable& operator=(accept_awaitable&& other) = delete;

    bool await_ready() noexcept;
    tcp_socket await_resume() noexcept;

private:
    CHAR buffer[2 * (sizeof(SOCKADDR_IN) + 16)];
    bool pending = false;
    tcp_socket result;
};
