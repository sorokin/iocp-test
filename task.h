#pragma once

#include <coroutine>

template <typename T>
struct task_promise;

template <typename T>
struct task;

template <typename T>
struct task_promise
{
    task<T> get_return_object();

    std::suspend_never initial_suspend() noexcept;
    std::suspend_never final_suspend() noexcept;

    void return_void() noexcept;
    void unhandled_exception() noexcept;
};

template <typename T>
struct task
{
    using promise_type = task_promise<T>;

    explicit task(std::coroutine_handle<promise_type> handle);

private:
    std::coroutine_handle<promise_type> handle;
};


template <typename T>
task<T> task_promise<T>::get_return_object()
{
    return task<T>(std::coroutine_handle<task_promise>::from_promise(*this));
}

template <typename T>
std::suspend_never task_promise<T>::initial_suspend() noexcept
{
    return {};
}

template <typename T>
std::suspend_never task_promise<T>::final_suspend() noexcept
{
    return {};
}

template <typename T>
void task_promise<T>::return_void() noexcept
{}

template <typename T>
void task_promise<T>::unhandled_exception() noexcept
{}

template <typename T>
task<T>::task(std::coroutine_handle<promise_type> handle)
    : handle(handle)
{
    std::cerr << "task " << handle.address() << "created\n";
}
