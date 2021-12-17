#include <cassert>
#include <coroutine>
#include <iostream>
#include <span>
#include <stdexcept>
#include <winsock2.h>
#include <mswsock.h>
#include <optional>
#include <windows.h>

#include "overlapped_awaitable.h"
#include "task.h"
#include "tcp_socket.h"


task<void> foo(tcp_socket s)
{
    co_await s.send(nullptr, 0);
    co_await s.recv(nullptr, 0);
    co_await s.accept();
}

task<void> recv_send_loop(tcp_socket connection)
{
    char buf[1500];
    for (;;)
    {
        size_t received = co_await connection.recv(std::data(buf), std::size(buf));
        if (received == 0)
            break;

        co_await connection.send(std::data(buf), received);
    }
}

task<void> accept_loop(tcp_socket& listener)
{
    for (size_t i = 0; i != 5; ++i)
    {
        tcp_socket connected = co_await listener.accept();

        recv_send_loop(std::move(connected));
    }
}

int main()
{
    WSAData wsadata;
    WSAStartup(MAKEWORD(2, 2), &wsadata);

    io_context ctx;
    tcp_socket listener(ctx);

    listener.bind(10000);
    listener.listen();

    accept_loop(listener);

    for (;;)
        ctx.wait();
}

/*struct tcp_socket
{
    tcp_socket(io_context& ctx)
        : handle(WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED))
    {
        if (handle == INVALID_SOCKET)
            throw std::runtime_error("WSASocket failed");

        CreateIoCompletionPort(reinterpret_cast<HANDLE>(handle), ctx.iocp_handle, 0, 0);
    }

    tcp_socket(tcp_socket const& other) = delete;
    tcp_socket& operator=(tcp_socket const& other) = delete;

    ~tcp_socket()
    {
        closesocket(handle);
    }

    void bind()
    {
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = 0;
        addr.sin_port = htons(12345);
        int res = ::bind(handle, reinterpret_cast<sockaddr*>(&addr), sizeof addr);
        if (res != 0)
            throw std::runtime_error("bind failed");
    }

    void listen()
    {
        int res = ::listen(handle, SOMAXCONN);
        if (res != 0)
            throw std::runtime_error("listen failed");
    }

    bool accept(tcp_socket& connection, OVERLAPPED* overlapped)
    {
        CHAR buffer[2 * (sizeof(SOCKADDR_IN) + 16)];
        DWORD received;
        BOOL result = AcceptEx(handle, connection.get_handle(), buffer, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &received, overlapped);
        if (result)
            return true;
        int err = WSAGetLastError();
        if (err == ERROR_IO_PENDING)
            return false;

        throw std::runtime_error("AcceptEx failed");
    }

    void connect(wchar_t const* host_name, wchar_t const* service_name)
    {
        BOOL res = WSAConnectByName(handle, const_cast<wchar_t*>(host_name), const_cast<wchar_t*>(service_name), nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
        if (!res)
            throw std::runtime_error("WSAConnectByName failed");
    }

    bool recv(void* buf, size_t buf_size, OVERLAPPED* overlapped)
    {
        WSABUF wsabuf = {buf_size, reinterpret_cast<char*>(buf)};
        DWORD flags = 0;
        int res = WSARecv(handle, &wsabuf, 1, nullptr, &flags, overlapped, nullptr);
        if (res == 0)
            return true;

        assert(res == SOCKET_ERROR);
        int last_error = WSAGetLastError();
        if (last_error == WSA_IO_PENDING)
            return false;

        throw std::runtime_error("WSARecv failed");
    }

    bool send(void const* buf, size_t buf_size,  OVERLAPPED* overlapped)
    {
        WSABUF wsabuf = {buf_size, reinterpret_cast<char*>(const_cast<void*>(buf))};
        int res = WSASend(handle, &wsabuf, 1, nullptr, 0, overlapped, nullptr);
        if (res == 0)
            return true;

        assert(res == SOCKET_ERROR);
        int last_error = WSAGetLastError();
        if (last_error == WSA_IO_PENDING)
            return false;

        throw std::runtime_error("WSARecv failed");
    }

    SOCKET get_handle() const
    {
        return handle;
    }

private:
    SOCKET handle;
};

task<void> run_server(tcp_socket& sock)
{
    co_await send_awaitable{sock.get_handle(), nullptr, 0};
}


int main()
{
    WSADATA data;
    WSAStartup(MAKEWORD(2, 2), &data);

    io_context ctx;
    tcp_socket sock(ctx);
    sock.connect(L"lwn.net", L"80");

    OVERLAPPED overlapped = {};
    char const sendbuf[] = "GET / HTTP/1.1\r\nHost: en.wikipedia.org\r\nConnection: close\r\n\r\n";
    bool r = sock.send(sendbuf, (sizeof sendbuf) - 1, &overlapped);
    if (!r)
    {
        auto res = ctx.wait();
        assert(res.overlapped == &overlapped);
        assert(res.bytes_transferred == (sizeof sendbuf) - 1);
    }

    shutdown(sock.get_handle(), SD_SEND);

    for (;;)
    {
        char recvbuf[1500];
        bool completed_immediately = sock.recv(recvbuf, sizeof recvbuf, &overlapped);
        if (!completed_immediately)
        {
            auto res = ctx.wait();
            assert(res.overlapped == &overlapped);

            std::cout << "received later " << res.bytes_transferred << '\n';
        }
        else
        {
            DWORD bytes_transferred;
            DWORD flags;
            BOOL res = WSAGetOverlappedResult(sock.get_handle(), &overlapped, &bytes_transferred, FALSE, &flags);
            if (!res)
                throw std::runtime_error("WSAGetOverlappedResult failed");

            std::cout << "received immediately " << bytes_transferred << '\n';
        }
    }

    WSACleanup();
}
*/