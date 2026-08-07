/*
 * Copyright 2026 nano_com authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <ncom/ncom.h>
#include <ncom/string.h>
#include <ncom/ncom_ptr.hpp>
#include <ncom/core_impl.h>
#include <iostream>
#include <cassert>

int main()
{
    std::cout << "=== ncom C++ Unit Tests ===\n";

    // 1. Create string objects
    ncom::ptr<ncom_istring_t> str1;
    ncom::ptr<ncom_istring_t> str2;
    ncom::ptr<ncom_istring_t> str3;

    ncom_istring_t* raw_str1 = nullptr;
    ncom_istring_t* raw_str2 = nullptr;

    assert(NCOM_SUCCEEDED(ncom_create_string("hello", &raw_str1)));
    assert(NCOM_SUCCEEDED(ncom_create_string("world", &raw_str2)));

    str1.attach(raw_str1);
    str2.attach(raw_str2);
    str3 = str1; // copies reference, increments refcount

    // 2. Test operator== and operator!= between ptr instances
    assert(str1 == str3);
    assert(str1 != str2);
    assert(!(str1 == str2));
    assert(!(str1 != str3));

    // 3. Test operator== and operator!= with nullptr (both directions)
    ncom::ptr<ncom_istring_t> empty_ptr;
    assert(empty_ptr == nullptr);
    assert(nullptr == empty_ptr);
    assert(str1 != nullptr);
    assert(nullptr != str1);

    // 4. Test operator== and operator!= with raw pointers (both directions)
    assert(str1 == raw_str1);
    assert(raw_str1 == str1);
    assert(str1 != raw_str2);
    assert(raw_str2 != str1);

    std::cout << "C++ tests passed successfully!\n";
    return 0;
}
