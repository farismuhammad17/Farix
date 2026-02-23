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

string::string(const char* str) : buffer(nullptr), len(0) {
    if (str) {
        len = strlen(str);
        buffer = new char[len + 1];
        memcpy(buffer, str, len + 1);
    } else {
        buffer = new char[1];
        buffer[0] = '\0';
    }
}

string::string(const string& other) {
    len    = other.len;
    buffer = new char[len + 1];
    memcpy(buffer, other.buffer, len + 1);
}

string::string(const char* str, char c) {
    size_t str_len = str ? strlen(str) : 0;
    len = str_len + 1;
    buffer = new char[len + 1];

    if (str_len > 0) {
        memcpy(buffer, str, str_len);
    }

    buffer[str_len] = c;
    buffer[len] = '\0';
}

string::string(char* allocated_buffer, size_t length) {
    buffer = allocated_buffer;
    len    = length;
}

string::~string() {
    if (buffer) {
        delete[] buffer;
    }
}

string& string::operator=(const string& other) {
    if (this == &other) return *this;
    if (buffer) delete[] buffer;

    len = other.len;

    if (other.buffer) {
        buffer = new char[len + 1];
        memcpy(buffer, other.buffer, len + 1);
    } else {
        buffer = new char[1];
        buffer[0] = '\0';
        len = 0;
    }

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

string* string::split(char delim, int lim, size_t& out_count) const {
    size_t occurrences = count(delim);
    size_t max_parts   = (lim > 0 && lim <= occurrences) ? lim + 1 : occurrences + 1;

    string* parts = new string[max_parts];
    out_count = 0;
    string current = "";

    for (size_t i = 0; i < len; i++) {
        if (this->buffer[i] == delim && (lim == 0 || out_count < lim)) {
            parts[out_count++] = current;
            current = "";
        } else {
            current += this->buffer[i];
        }
    }

    parts[out_count++] = current;

    return parts;
}

string* string::upper() const {
    string* result = new string(*this);

    for (size_t i = 0; i < len; ++i) {
        if (result->buffer[i] >= 'a' && result->buffer[i] <= 'z') {
            result->buffer[i] -= 32;
        }
    }

    return result;
}

string string::substr(int start) const {
    if (start >= (int)len) return string("");

    size_t new_len = len - start;
    char* temp = new char[new_len + 1];

    for (size_t i = 0; i < new_len; i++) {
        temp[i] = buffer[start + i];
    }
    temp[new_len] = '\0';

    string result(temp);
    delete[] temp;
    return result;
}

size_t string::count(char c) const {
    size_t out = 0;
    for (size_t i = 0; i < len; i++) {
        if (this->buffer[i] == c) {
            out++;
        }
    }
    return out;
}

size_t string::find(char c, size_t start) const {
    for (size_t i = start; i < len; i++) {
        if (buffer[i] == c) return i;
    }
    return (size_t)-1;
}

bool string::starts_with(const string& other) const {
    if (other.len > len) return false;
    for (size_t i = 0; i < other.len; i++) {
        if (buffer[i] != other.buffer[i]) return false;
    }
    return true;
}

bool string::ends_with(const string& other) const {
    if (other.len > len) return false;
    for (size_t i = other.len - 1; i > 0; i--) {
        if (buffer[i] != other.buffer[i]) return false;
    }
    return true;
}

bool string::contains(char c) const {
    for (int i = 0; i < this->length(); i++) {
        if ((*this)[i] == c) return true;
    }
    return false;
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
    if (buffer) {
        memcpy(new_buffer, buffer, len);
        delete[] buffer;
    }
    new_buffer[len] = c;
    new_buffer[len + 1] = '\0';
    buffer = new_buffer;
    len++;
}

string operator+(const string& lhs, const string& rhs) {
    size_t total_len = lhs.length() + rhs.length();

    char* b = new char[total_len + 1];

    memcpy(b, lhs.c_str(), lhs.length());
    memcpy(b + lhs.length(), rhs.c_str(), rhs.length());
    b[total_len] = '\0';

    return string(b, total_len);
}

string operator+(const string& lhs, char rhs) {
    size_t old_len = lhs.length();
    size_t new_len = old_len + 1;
    char* temp    = new char[new_len + 1];

    memcpy(temp, lhs.c_str(), old_len);
    temp[old_len] = rhs;
    temp[new_len] = '\0';

    return string(temp, new_len);
}

string operator+(char lhs, const string& rhs) {
    size_t old_len = rhs.length();
    size_t new_len = old_len + 1;
    char* temp    = new char[new_len + 1];

    temp[0] = lhs;
    memcpy(temp + 1, rhs.c_str(), old_len);
    temp[new_len] = '\0';

    return string(temp, new_len);
}
