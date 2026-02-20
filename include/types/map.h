/*
-----------------------------------------------------------------------
Copyright (C) 2026 Faris Muhammad

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
-----------------------------------------------------------------------
*/

#ifndef MAP_H
#define MAP_H

#include "memory/heap.h"

template <typename K, typename V>
struct Pair {
    K key;
    V value;
    bool active = false; // To track if the slot is used
};

template <typename K, typename V>
class map {
private:
    Pair<K, V>* entries;
    size_t _capacity;
    size_t _size;

    void expand() {
        size_t new_capacity = _capacity * 2;
        Pair<K, V>* new_entries = new Pair<K, V>[new_capacity];

        for (size_t i = 0; i < _capacity; i++) {
            new_entries[i] = entries[i];
        }

        delete[] entries;
        entries = new_entries;
        _capacity = new_capacity;
    }

public:
    map(size_t initial_capacity = 10) {
        _capacity = initial_capacity;
        _size = 0;
        entries = new Pair<K, V>[_capacity];
    }

    ~map() {
        if (entries) {
            delete[] entries;
        }
    }

    void put(K key, V value) {
        // Check if key already exists to update it
        for (size_t i = 0; i < _capacity; i++) {
            if (entries[i].active && entries[i].key == key) {
                entries[i].value = value;
                return;
            }
        }

        if (_size >= _capacity) expand();

        // Find first empty slot
        for (size_t i = 0; i < _capacity; i++) {
            if (!entries[i].active) {
                entries[i].key = key;
                entries[i].value = value;
                entries[i].active = true;
                _size++;
                return;
            }
        }
    }

    void remove(K key) {
        for (size_t i = 0; i < _capacity; i++) {
            if (entries[i].active && entries[i].key == key) {
                entries[i].active = false; // "Delete" it
                _size--;
                return;
            }
        }
    }

    V get(K key) {
        for (size_t i = 0; i < _capacity; i++) {
            if (entries[i].active && entries[i].key == key) {
                return entries[i].value;
            }
        }
        return V(); // Return null/default if not found
    }

    bool contains(K key) {
        for (size_t i = 0; i < _capacity; i++) {
            if (entries[i].active && entries[i].key == key) {
                return true;
            }
        }
        return false;
    }

    K getKeyAt(size_t index) {
        size_t count = 0;
        for (size_t i = 0; i < _capacity; i++) {
            if (entries[i].active) {
                if (count == index) return entries[i].key;
                count++;
            }
        }
        return K();
    }

    size_t size() { return _size; }
};

#endif
