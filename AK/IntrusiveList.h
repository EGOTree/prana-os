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

#include <AK/Assertions.h>

namespace AK {

class IntrusiveListNode;
class IntrusiveListStorage {
private:
    friend class IntrusiveListNode;
    template<class T, IntrusiveListNode T::*member>
    friend class IntrusiveList;
    IntrusiveListNode* m_first { nullptr };
    IntrusiveListNode* m_last { nullptr };
};

template<class T, IntrusiveListNode T::*member>
class IntrusiveList {
public:
    IntrusiveList();
    ~IntrusiveList();

    void clear();
    bool is_empty() const;
    void append(T& n);
    void prepend(T& n);
    void remove(T& n);
    bool contains(const T&) const;
    T* first() const;
    T* last() const;

    T* take_first();
    T* take_last();

    class Iterator {
    public:
        Iterator() = default;
        Iterator(T* value)
            : m_value(value)
        {
        }

        T& operator*() const { return *m_value; }
        T* operator->() const { return m_value; }
        bool operator==(const Iterator& other) const { return other.m_value == m_value; }
        bool operator!=(const Iterator& other) const { return !(*this == other); }
        Iterator& operator++()
        {
            m_value = IntrusiveList<T, member>::next(m_value);
            return *this;
        }
        Iterator& erase();

    private:
        T* m_value { nullptr };
    };

    Iterator begin();
    Iterator end() { return Iterator {}; }

    class ConstIterator {
    public:
        ConstIterator() = default;
        ConstIterator(const T* value)
            : m_value(value)
        {
        }

        const T& operator*() const { return *m_value; }
        const T* operator->() const { return m_value; }
        bool operator==(const ConstIterator& other) const { return other.m_value == m_value; }
        bool operator!=(const ConstIterator& other) const { return !(*this == other); }
        ConstIterator& operator++()
        {
            m_value = IntrusiveList<T, member>::next(const_cast<T*>(m_value));
            return *this;
        }

    private:
        const T* m_value { nullptr };
    };

    ConstIterator begin() const;
    ConstIterator end() const { return ConstIterator {}; }

private:
    static T* next(T* current);
    static T* node_to_value(IntrusiveListNode& node);
    IntrusiveListStorage m_storage;
};

class IntrusiveListNode {
public:
    ~IntrusiveListNode();
    void remove();
    bool is_in_list() const;

private:
    template<class T, IntrusiveListNode T::*member>
    friend class IntrusiveList;
    IntrusiveListStorage* m_storage = nullptr;
    IntrusiveListNode* m_next = nullptr;
    IntrusiveListNode* m_prev = nullptr;
};

template<class T, IntrusiveListNode T::*member>
inline typename IntrusiveList<T, member>::Iterator& IntrusiveList<T, member>::Iterator::erase()
{
    T* old = m_value;
    m_value = IntrusiveList<T, member>::next(m_value);
    (old->*member).remove();
    return *this;
}

template<class T, IntrusiveListNode T::*member>
inline IntrusiveList<T, member>::IntrusiveList()

{
}

template<class T, IntrusiveListNode T::*member>
inline IntrusiveList<T, member>::~IntrusiveList()
{
    clear();
}

template<class T, IntrusiveListNode T::*member>
inline void IntrusiveList<T, member>::clear()
{
    while (m_storage.m_first)
        m_storage.m_first->remove();
}

template<class T, IntrusiveListNode T::*member>
inline bool IntrusiveList<T, member>::is_empty() const
{
    return m_storage.m_first == nullptr;
}

template<class T, IntrusiveListNode T::*member>
inline void IntrusiveList<T, member>::append(T& n)
{
    auto& nnode = n.*member;
    if (nnode.m_storage)
        nnode.remove();

    nnode.m_storage = &m_storage;
    nnode.m_prev = m_storage.m_last;
    nnode.m_next = nullptr;

    if (m_storage.m_last)
        m_storage.m_last->m_next = &nnode;
    m_storage.m_last = &nnode;
    if (!m_storage.m_first)
        m_storage.m_first = &nnode;
}

template<class T, IntrusiveListNode T::*member>
inline void IntrusiveList<T, member>::prepend(T& n)
{
    auto& nnode = n.*member;
    if (nnode.m_storage)
        nnode.remove();

    nnode.m_storage = &m_storage;
    nnode.m_prev = nullptr;
    nnode.m_next = m_storage.m_first;

    if (m_storage.m_first)
        m_storage.m_first->m_prev = &nnode;
    m_storage.m_first = &nnode;
    if (!m_storage.m_last)
        m_storage.m_last = &nnode;
}

template<class T, IntrusiveListNode T::*member>
inline void IntrusiveList<T, member>::remove(T& n)
{
    auto& nnode = n.*member;
    if (nnode.m_storage)
        nnode.remove();
}

template<class T, IntrusiveListNode T::*member>
inline bool IntrusiveList<T, member>::contains(const T& n) const
{
    auto& nnode = n.*member;
    return nnode.m_storage == &m_storage;
}

template<class T, IntrusiveListNode T::*member>
inline T* IntrusiveList<T, member>::first() const
{
    return m_storage.m_first ? node_to_value(*m_storage.m_first) : nullptr;
}

template<class T, IntrusiveListNode T::*member>
inline T* IntrusiveList<T, member>::take_first()
{
    if (auto* ptr = first()) {
        remove(*ptr);
        return ptr;
    }
    return nullptr;
}

template<class T, IntrusiveListNode T::*member>
inline T* IntrusiveList<T, member>::take_last()
{
    if (auto* ptr = last()) {
        remove(*ptr);
        return ptr;
    }
    return nullptr;
}

template<class T, IntrusiveListNode T::*member>
inline T* IntrusiveList<T, member>::last() const
{
    return m_storage.m_last ? node_to_value(*m_storage.m_last) : nullptr;
}

template<class T, IntrusiveListNode T::*member>
inline T* IntrusiveList<T, member>::next(T* current)
{
    auto& nextnode = (current->*member).m_next;
    T* nextstruct = nextnode ? node_to_value(*nextnode) : nullptr;
    return nextstruct;
}

template<class T, IntrusiveListNode T::*member>
inline typename IntrusiveList<T, member>::Iterator IntrusiveList<T, member>::begin()
{
    return m_storage.m_first ? Iterator(node_to_value(*m_storage.m_first)) : Iterator();
}

template<class T, IntrusiveListNode T::*member>
inline typename IntrusiveList<T, member>::ConstIterator IntrusiveList<T, member>::begin() const
{
    return m_storage.m_first ? ConstIterator(node_to_value(*m_storage.m_first)) : ConstIterator();
}

template<class T, IntrusiveListNode T::*member>
inline T* IntrusiveList<T, member>::node_to_value(IntrusiveListNode& node)
{
    return (T*)((char*)&node - ((char*)&(((T*)nullptr)->*member) - (char*)nullptr));
}

inline IntrusiveListNode::~IntrusiveListNode()
{
    if (m_storage)
        remove();
}

inline void IntrusiveListNode::remove()
{
    VERIFY(m_storage);
    if (m_storage->m_first == this)
        m_storage->m_first = m_next;
    if (m_storage->m_last == this)
        m_storage->m_last = m_prev;
    if (m_prev)
        m_prev->m_next = m_next;
    if (m_next)
        m_next->m_prev = m_prev;
    m_prev = nullptr;
    m_next = nullptr;
    m_storage = nullptr;
}

inline bool IntrusiveListNode::is_in_list() const
{
    return m_storage != nullptr;
}

}

using AK::IntrusiveList;
using AK::IntrusiveListNode;
