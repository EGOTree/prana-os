/*
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <AK/SinglyLinkedList.h>

namespace AK {

template<typename T>
class SinglyLinkedListWithCount : private SinglyLinkedList<T> {

public:
    SinglyLinkedListWithCount() = default;
    ~SinglyLinkedListWithCount() = default;

    using List = SinglyLinkedList<T>;

    using List::is_empty;
    using List::size_slow;

    inline size_t size() const
    {
        return m_count;
    }

    void clear()
    {
        List::clear();
        m_count = 0;
    }

    T& first()
    {
        return List::first();
    }

    const T& first() const
    {
        return List::first();
    }

    T& last()
    {
        return List::last();
    }

    const T& last() const
    {
        return List::last();
    }

    T take_first()
    {
        m_count--;
        return List::take_first();
    }

    template<typename U = T>
    void append(U&& value)
    {
        m_count++;
        return List::append(forward<T>(value));
    }

    bool contains_slow(const T& value) const
    {
        return List::contains_slow(value);
    }

    using Iterator = typename List::Iterator;
    friend Iterator;
    Iterator begin() { return List::begin(); }
    Iterator end() { return List::end(); }

    using ConstIterator = typename List::ConstIterator;
    friend ConstIterator;
    ConstIterator begin() const { return List::begin(); }
    ConstIterator end() const { return List::end(); }

    template<typename TUnaryPredicate>
    ConstIterator find(TUnaryPredicate&& pred) const
    {
        return List::find_if(forward<TUnaryPredicate>(pred));
    }

    template<typename TUnaryPredicate>
    Iterator find(TUnaryPredicate&& pred)
    {
        return List::find_if(forward<TUnaryPredicate>(pred));
    }

    ConstIterator find(const T& value) const
    {
        return List::find(value);
    }

    Iterator find(const T& value)
    {
        return List::find(value);
    }

    void remove(Iterator iterator)
    {
        m_count--;
        return List::remove(iterator);
    }

    template<typename U = T>
    void insert_before(Iterator iterator, U&& value)
    {
        m_count++;
        List::insert_before(iterator, forward<T>(value));
    }

    template<typename U = T>
    void insert_after(Iterator iterator, U&& value)
    {
        m_count++;
        List::insert_after(iterator, forward<T>(value));
    }

private:
    size_t m_count { 0 };
};

}

using AK::SinglyLinkedListWithCount;
