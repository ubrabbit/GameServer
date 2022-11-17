#pragma once

class noncopyable
{
protected:
    constexpr noncopyable() = default;
    ~noncopyable() = default;

private:
    noncopyable(const noncopyable &) = delete;
    const noncopyable &operator=(const noncopyable &) = delete;

};
