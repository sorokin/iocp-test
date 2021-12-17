#include "tcp_socket.h"

#include <iostream>
#include <stdexcept>
#include <winsock2.h>

tcp_socket::tcp_socket(io_context& ctx)
    : ctx(&ctx)
    , handle(WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED))
{
    if (handle == INVALID_SOCKET)
        throw std::runtime_error("WSASocket failed");

    HANDLE res = CreateIoCompletionPort(reinterpret_cast<HANDLE>(handle), ctx.iocp_handle, 0, 0);
    if (res == nullptr)
    {
        closesocket(handle);
        throw std::runtime_error("association of socket and completion port failed");
    }
}

tcp_socket::tcp_socket(tcp_socket&& other) noexcept
    : ctx(other.ctx)
    , handle(other.handle)
{
    other.ctx = nullptr;
    other.handle = 0;
}

tcp_socket& tcp_socket::operator=(tcp_socket&& other) noexcept
{
    if (this == &other)
        return *this;

    closesocket(handle);

    ctx = other.ctx;
    handle = other.handle;

    other.ctx = nullptr;
    other.handle = 0;

    return *this;
}

tcp_socket::~tcp_socket()
{
    if (handle)
        closesocket(handle);
}

void tcp_socket::bind(int port)
{
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = 0;
    addr.sin_port = htons(12345);
    int res = ::bind(handle, reinterpret_cast<sockaddr*>(&addr), sizeof addr);
    if (res != 0)
        throw std::runtime_error("bind failed");
}

void tcp_socket::listen()
{
    int res = ::listen(handle, SOMAXCONN);
    if (res != 0)
        throw std::runtime_error("listen failed");
}

send_awaitable tcp_socket::send(void const* buf, size_t buf_size)
{
    return send_awaitable(handle, buf, buf_size);
}

recv_awaitable tcp_socket::recv(void* buf, size_t buf_size)
{
    return recv_awaitable(handle, buf, buf_size);
}

accept_awaitable tcp_socket::accept()
{
    return accept_awaitable(handle, *ctx);
}

send_awaitable::send_awaitable(SOCKET socket, void const* buf, size_t buf_size)
{
    DWORD bytes_transferred;
    WSABUF wsabuf = { buf_size, reinterpret_cast<char*>(const_cast<void*>(buf)) };
    std::cerr << "socket #" << socket << " starting send of " << buf_size << " bytes, overlapped " << this << "...\n";
    int res = WSASend(socket, &wsabuf, 1, &bytes_transferred, 0, this, nullptr);
    if (res == 0)
    {
        std::cerr << "    ... completed immediately for " << bytes_transferred << " bytes\n";
        assert(bytes_transferred == buf_size);
        this->bytes_transferred = bytes_transferred;
        return;
    }

    assert(res == SOCKET_ERROR);
    int last_error = WSAGetLastError();
    if (last_error == WSA_IO_PENDING)
    {
        std::cerr << "    ... pending\n";
        pending = true;
        return;
    }

    throw std::runtime_error("WSASend failed");
}

bool send_awaitable::await_ready() noexcept
{
    return !pending;
}

size_t send_awaitable::await_resume() noexcept
{
    return bytes_transferred;
}

recv_awaitable::recv_awaitable(SOCKET socket, void* buf, size_t buf_size)
{
    DWORD bytes_transferred;
    WSABUF wsabuf = { buf_size, reinterpret_cast<char*>(buf) };
    DWORD flags = 0;
    std::cerr << "socket #" << socket << " starting recv of " << buf_size << " bytes, overlapped " << this << "...\n";
    int res = WSARecv(socket, &wsabuf, 1, &bytes_transferred, &flags, this, nullptr);
    if (res == 0)
    {
        std::cerr << "    ... completed immediately for " << bytes_transferred << " bytes\n";
        this->bytes_transferred = bytes_transferred;
        return;
    }

    assert(res == SOCKET_ERROR);
    int last_error = WSAGetLastError();
    if (last_error == WSA_IO_PENDING)
    {
        std::cerr << "    ... pending\n";
        pending = true;
        return;
    }

    throw std::runtime_error("WSASend failed");
}

bool recv_awaitable::await_ready() noexcept
{
    return !pending;
}

size_t recv_awaitable::await_resume() noexcept
{
    return bytes_transferred;
}

accept_awaitable::accept_awaitable(SOCKET listened, io_context& ctx)
    : result(ctx)
{
    DWORD received;
    std::cerr << "socket #" << socket << " starting accept for socket #" << this->result.handle << ", overlapped " << this << "...\n";
    BOOL result = AcceptEx(listened, this->result.handle, buffer, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &received, this);
    if (result)
    {
        std::cerr << "    ... completed immediately\n";
        return;
    }

    int err = WSAGetLastError();
    if (err == ERROR_IO_PENDING)
    {
        std::cerr << "    ... pending\n";
        pending = true;
        return;
    }

    throw std::runtime_error("AcceptEx failed");
}

bool accept_awaitable::await_ready() noexcept
{
    return !pending;
}

tcp_socket accept_awaitable::await_resume() noexcept
{
    return std::move(result);
}
