#pragma once

namespace entis
{
template<typename T, size_t Capacity>
requires std::equality_comparable<T> && std::copy_constructible<T>
class StaticVector
{
public:
    struct Iterator;

    void add(const T& val) { m_data[m_size++] = val; }
    T& at(size_t index) { return m_data[index]; }

    [[nodiscard]] size_t size() const { return m_size; }
    constexpr size_t capacity() { return Capacity; }

    Iterator begin() { return Iterator(m_data); }
    Iterator end()   { return Iterator(m_data + m_size); }

    T& operator[] (size_t index) { return m_data[index]; }

    StaticVector() = default;
    StaticVector(const StaticVector& other) : m_size(other.m_size)
    {
        for (size_t i = 0; i < m_size; ++i)
            m_data[i] = other.m_data[i];
    }
private:
    size_t m_size{};
    T m_data[Capacity];

public:
    struct Iterator
    {
        using IteratorCategory = std::forward_iterator_tag;
        using DifferenceType   = std::ptrdiff_t;
        using ValueType        = T;
        using PointerType      = T*;
        using ReferenceType    = T&;

        explicit Iterator(T* ptr) : m_ptr(ptr) { }

        ReferenceType operator*() const { return *m_ptr; }
        PointerType operator->() { return m_ptr; }

        Iterator& operator++() { ++m_ptr; return *this; }

        Iterator operator++(int) { Iterator tmp = *this; ++(*this); return tmp; }

        friend bool operator== (const Iterator& a, const Iterator& b) { return a.m_ptr == b.m_ptr; };
        friend bool operator!= (const Iterator& a, const Iterator& b) { return a.m_ptr != b.m_ptr; };
    private:
        T* m_ptr;
    };
};
}
