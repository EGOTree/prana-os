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
#include <AK/Find.h>
#include <AK/StdLibExtras.h>
#include <AK/Traits.h>
#include <AK/Types.h>

namespace AK {

template<typename ListType, typename ElementType>
class SinglyLinkedListIterator {
public:
    SinglyLinkedListIterator() = default;
    bool operator!=(const SinglyLinkedListIterator& other) const { return m_node != other.m_node; }
    SinglyLinkedListIterator& operator++()
    {
        m_prev = m_node;
        m_node = m_node->next;
        return *this;
    }
    ElementType& operator*() { return m_node->value; }
    ElementType* operator->() { return &m_node->value; }
    bool is_end() const { return !m_node; }
    bool is_begin() const { return !m_prev; }

private:
    friend ListType;
    explicit SinglyLinkedListIterator(typename ListType::Node* node, typename ListType::Node* prev = nullptr)
        : m_node(node)
        , m_prev(prev)
    {
    }
    typename ListType::Node* m_node { nullptr };
    typename ListType::Node* m_prev { nullptr };
};

template<typename T>
class SinglyLinkedList {
private:
    struct Node {
        explicit Node(T&& v)
            : value(move(v))
        {
        }
        explicit Node(const T& v)
            : value(v)
        {
        }
        T value;
        Node* next { nullptr };
    };

public:
    SinglyLinkedList() = default;
    ~SinglyLinkedList() { clear(); }

    bool is_empty() const { return !head(); }

    inline size_t size_slow() const
    {
        size_t size = 0;
        for (auto* node = m_head; node; node = node->next)
            ++size;
        return size;
    }

    void clear()
    {
        for (auto* node = m_head; node;) {
            auto* next = node->next;
            delete node;
            node = next;
        }
        m_head = nullptr;
        m_tail = nullptr;
    }

    T& first()
    {
        VERIFY(head());
        return head()->value;
    }
    const T& first() const
    {
        VERIFY(head());
        return head()->value;
    }
    T& last()
    {
        VERIFY(head());
        return tail()->value;
    }
    const T& last() const
    {
        VERIFY(head());
        return tail()->value;
    }

    T take_first()
    {
        VERIFY(m_head);
        auto* prev_head = m_head;
        T value = move(first());
        if (m_tail == m_head)
            m_tail = nullptr;
        m_head = m_head->next;
        delete prev_head;
        return value;
    }

    template<typename U = T>
    void append(U&& value)
    {
        auto* node = new Node(forward<U>(value));
        if (!m_head) {
            m_head = node;
            m_tail = node;
            return;
        }
        m_tail->next = node;
        m_tail = node;
    }

    bool contains_slow(const T& value) const
    {
        return find(value) != end();
    }

    using Iterator = SinglyLinkedListIterator<SinglyLinkedList, T>;
    friend Iterator;
    Iterator begin() { return Iterator(m_head); }
    Iterator end() { return {}; }

    using ConstIterator = SinglyLinkedListIterator<const SinglyLinkedList, const T>;
    friend ConstIterator;
    ConstIterator begin() const { return ConstIterator(m_head); }
    ConstIterator end() const { return {}; }

    template<typename TUnaryPredicate>
    ConstIterator find_if(TUnaryPredicate&& pred) const
    {
        return AK::find_if(begin(), end(), forward<TUnaryPredicate>(pred));
    }

    template<typename TUnaryPredicate>
    Iterator find_if(TUnaryPredicate&& pred)
    {
        return AK::find_if(begin(), end(), forward<TUnaryPredicate>(pred));
    }

    ConstIterator find(const T& value) const
    {
        return find_if([&](auto& other) { return Traits<T>::equals(value, other); });
    }

    Iterator find(const T& value)
    {
        return find_if([&](auto& other) { return Traits<T>::equals(value, other); });
    }

    void remove(Iterator iterator)
    {
        VERIFY(!iterator.is_end());
        if (m_head == iterator.m_node)
            m_head = iterator.m_node->next;
        if (m_tail == iterator.m_node)
            m_tail = iterator.m_prev;
        if (iterator.m_prev)
            iterator.m_prev->next = iterator.m_node->next;
        delete iterator.m_node;
    }

    template<typename U = T>
    void insert_before(Iterator iterator, U&& value)
    {
        auto* node = new Node(forward<U>(value));
        node->next = iterator.m_node;
        if (m_head == iterator.m_node)
            m_head = node;
        if (iterator.m_prev)
            iterator.m_prev->next = node;
    }

    template<typename U = T>
    void insert_after(Iterator iterator, U&& value)
    {
        if (iterator.is_end()) {
            append(value);
            return;
        }

        auto* node = new Node(forward<U>(value));
        node->next = iterator.m_node->next;

        iterator.m_node->next = node;

        if (m_tail == iterator.m_node)
            m_tail = node;
    }

private:
    Node* head() { return m_head; }
    const Node* head() const { return m_head; }

    Node* tail() { return m_tail; }
    const Node* tail() const { return m_tail; }

    Node* m_head { nullptr };
    Node* m_tail { nullptr };
};

}

using AK::SinglyLinkedList;
