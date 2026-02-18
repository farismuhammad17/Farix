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

#ifndef STRING_H
#define STRING_H

#include <stddef.h>
#include <stdint.h>

class string {
private:
    char*  buffer;
    size_t len;

public:
    string(const char* str = "");
    string(const string& other);
    ~string();

    string& operator=(const string& other);
    void    operator+=(char c);
    bool    operator==(const char* str) const;
    bool    operator==(const string& other) const;
    bool    operator!=(const char* str) const { return !(*this == str); }
    bool    operator!=(decltype(nullptr)) const { return buffer != nullptr; }

    char    operator[](size_t index) const { return buffer[index]; }
    char&   operator[](size_t index) { return buffer[index]; }

    void    pop_back();

    char*   c_str()  const { return buffer; }
    size_t  length() const { return len; }

    static string from_int(int n);
    static string from_hex(uint32_t n);
};

string operator+(const string& lhs, const string& rhs);
string operator+(const string& lhs, char rhs);
string operator+(char lhs, const string& rhs);

#endif
