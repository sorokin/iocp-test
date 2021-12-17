#include "overlapped_awaitable.h"

#include <iostream>

overlapped_awaitable::overlapped_awaitable()
    : OVERLAPPED()
{}

void overlapped_awaitable::await_suspend(std::coroutine_handle<> c) noexcept
{
    std::cerr << "suspended overlapped " << this << ", task " << c.address() << "\n";
    continuation = c;
}
