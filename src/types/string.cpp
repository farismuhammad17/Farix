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

#include "memory/heap.h"

#include "types/string.h"

static size_t strlen(const char* str) {
    size_t l = 0;
    while (str[l]) l++;
    return l;
}

string::string(const char* str) {
    len = strlen(str);
    buffer = new char[len + 1];
    for (size_t i = 0; i < len; i++) buffer[i] = str[i];
    buffer[len] = '\0';
}

string::string(const string& other) {
    len = other.len;
    buffer = new char[len + 1];
    for (size_t i = 0; i <= len; i++) buffer[i] = other.buffer[i];
    buffer[len] = '\0';
}

string::~string() {
    if (buffer) {
        delete[] buffer;
    }
}

string& string::operator=(const string& other) {
    if (this == &other) return *this;

    delete[] buffer;
    len = other.len;
    buffer = new char[len + 1];
    for (size_t i = 0; i <= len; i++) buffer[i] = other.buffer[i];

    return *this;
}

bool string::operator==(const char* str) const {
    size_t other_len = strlen(str);
    if (len != other_len) return false;

    for (size_t i = 0; i < len; i++) {
        if (buffer[i] != str[i]) return false;
    }
    return true;
}

bool string::operator==(const string& other) const {
    if (this->len != other.len) return false;

    for (size_t i = 0; i < len; i++) {
        if (this->buffer[i] != other.buffer[i]) return false;
    }

    return true;
}

void string::pop_back() {
    if (len > 0) {
        buffer[len - 1] = '\0';
        len--;
    }
}

string string::from_int(int n) {
    if (n == 0) return string("0");

    char buffer[12]; // Big enough for -2147483648 and \0
    int i = 0;
    bool is_negative = false;

    if (n < 0) {
        is_negative = true;
        n = -n;
    }

    while (n > 0) {
        buffer[i++] = (n % 10) + '0';
        n /= 10;
    }

    if (is_negative) buffer[i++] = '-';

    buffer[i] = '\0';

    for (int j = 0; j < i / 2; j++) {
        char temp = buffer[j];
        buffer[j] = buffer[i - j - 1];
        buffer[i - j - 1] = temp;
    }

    return string(buffer);
}

string string::from_hex(uint32_t n) {
    char hex_chars[] = "0123456789ABCDEF";
    char buffer[9];
    int i = 0;

    if (n == 0) return string("0");

    while (n > 0) {
        buffer[i++] = hex_chars[n % 16];
        n /= 16;
    }
    buffer[i] = '\0';

    // Reverses it
    for (int j = 0; j < i / 2; j++) {
        char temp = buffer[j];
        buffer[j] = buffer[i - j - 1];
        buffer[i - j - 1] = temp;
    }
    return string(buffer);
}

void string::operator+=(char c) {
    char* new_buffer = new char[len + 2];

    for (size_t i = 0; i < len; i++) new_buffer[i] = buffer[i];

    new_buffer[len] = c;
    new_buffer[len + 1] = '\0';

    delete[] buffer;
    buffer = new_buffer;
    len++;
}

string operator+(const string& lhs, const string& rhs) {
    size_t total_len = lhs.length() + rhs.length();

    char* temp = new char[total_len + 1];

    for (size_t i = 0; i < lhs.length(); i++) {
        temp[i] = lhs[i];
    }
    for (size_t i = 0; i < rhs.length(); i++) {
        temp[lhs.length() + i] = rhs[i];
    }

    temp[total_len] = '\0';

    string result(temp);
    delete[] temp;

    return result;
}

string operator+(const string& lhs, char rhs) {
    size_t new_len = lhs.length() + 1;
    char* temp = new char[new_len + 1];

    for (size_t i = 0; i < lhs.length(); i++) {
        temp[i] = lhs[i];
    }

    temp[lhs.length()] = rhs;
    temp[new_len] = '\0';

    string result(temp);
    delete[] temp;

    return result;
}

string operator+(char lhs, const string& rhs) {
    size_t new_len = rhs.length() + 1;
    char* temp = new char[new_len + 1];

    temp[0] = lhs;
    for (size_t i = 0; i < rhs.length(); i++) {
        temp[i + 1] = rhs[i];
    }

    temp[new_len] = '\0';

    string result(temp);
    delete[] temp;

    return result;
}
