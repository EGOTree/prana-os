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

#include <AK/HashTable.h>
#include <AK/Optional.h>
#include <AK/Vector.h>

// NOTE: We can't include <initializer_list> during the toolchain bootstrap,
//       since it's part of libstdc++, and libstdc++ depends on LibC.
//       For this reason, we don't support HashMap(initializer_list) in LibC.
#ifndef SERENITY_LIBC_BUILD
#    include <initializer_list>
#endif

namespace AK {

template<typename K, typename V, typename KeyTraits>
class HashMap {
private:
    struct Entry {
        K key;
        V value;
    };

    struct EntryTraits {
        static unsigned hash(const Entry& entry) { return KeyTraits::hash(entry.key); }
        static bool equals(const Entry& a, const Entry& b) { return KeyTraits::equals(a.key, b.key); }
    };

public:
    HashMap() = default;

#ifndef SERENITY_LIBC_BUILD
    HashMap(std::initializer_list<Entry> list)
    {
        ensure_capacity(list.size());
        for (auto& item : list)
            set(item.key, item.value);
    }
#endif

    bool is_empty() const
    {
        return m_table.is_empty();
    }
    size_t size() const { return m_table.size(); }
    size_t capacity() const { return m_table.capacity(); }
    void clear() { m_table.clear(); }

    HashSetResult set(const K& key, const V& value) { return m_table.set({ key, value }); }
    HashSetResult set(const K& key, V&& value) { return m_table.set({ key, move(value) }); }
    bool remove(const K& key)
    {
        auto it = find(key);
        if (it != end()) {
            m_table.remove(it);
            return true;
        }
        return false;
    }
    void remove_one_randomly() { m_table.remove(m_table.begin()); }

    using HashTableType = HashTable<Entry, EntryTraits>;
    using IteratorType = typename HashTableType::Iterator;
    using ConstIteratorType = typename HashTableType::ConstIterator;

    IteratorType begin() { return m_table.begin(); }
    IteratorType end() { return m_table.end(); }
    IteratorType find(const K& key)
    {
        return m_table.find(KeyTraits::hash(key), [&](auto& entry) { return KeyTraits::equals(key, entry.key); });
    }
    template<typename Finder>
    IteratorType find(unsigned hash, Finder finder)
    {
        return m_table.find(hash, finder);
    }

    ConstIteratorType begin() const { return m_table.begin(); }
    ConstIteratorType end() const { return m_table.end(); }
    ConstIteratorType find(const K& key) const
    {
        return m_table.find(KeyTraits::hash(key), [&](auto& entry) { return KeyTraits::equals(key, entry.key); });
    }
    template<typename Finder>
    ConstIteratorType find(unsigned hash, Finder finder) const
    {
        return m_table.find(hash, finder);
    }

    void ensure_capacity(size_t capacity) { m_table.ensure_capacity(capacity); }

    Optional<typename Traits<V>::PeekType> get(const K& key) const
    {
        auto it = find(key);
        if (it == end())
            return {};
        return (*it).value;
    }

    bool contains(const K& key) const
    {
        return find(key) != end();
    }

    void remove(IteratorType it)
    {
        m_table.remove(it);
    }

    V& ensure(const K& key)
    {
        auto it = find(key);
        if (it == end())
            set(key, V());
        return find(key)->value;
    }

    Vector<K> keys() const
    {
        Vector<K> list;
        list.ensure_capacity(size());
        for (auto& it : *this)
            list.unchecked_append(it.key);
        return list;
    }

private:
    HashTableType m_table;
};

}

using AK::HashMap;
