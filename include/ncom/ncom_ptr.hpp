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

/**
 * @file ncom_ptr.hpp
 * @brief C++ RAII smart pointer for ncom interfaces.
 *
 * Nano COM (ncom) is a tiny COM-like, ABI-stable component framework for C11,
 * with optional header-only C++ convenience wrappers.
 *
 */

#ifndef NCOM_NCOM_PTR_HPP
#define NCOM_NCOM_PTR_HPP

#include <ncom/base.h>
#include <type_traits>
#include <utility>

namespace ncom {

/**
 * @brief Trait template for resolving Interface IDs at compile time.
 * * The IDL generator (nidlgen) should emit specializations of this struct 
 * for every generated interface. This allows ncom::ptr to safely and 
 * automatically resolve the correct IID during a query_interface call.
 */
template <typename T>
struct iid_traits;

// Example of what the generator will produce for core interfaces:
template <> struct iid_traits<ncom_iunknown_t> {
    static const ncom_iid_t* get() { return &NCOM_IID_IUNKNOWN; }
};
template <> struct iid_traits<ncom_ifactory_t> {
    static const ncom_iid_t* get() { return &NCOM_IID_IFACTORY; }
};

/**
 * @brief A smart pointer for ncom interfaces managing intrusive reference counting.
 * * Ensures that add_ref() and release() are called automatically (RAII), 
 * preventing memory leaks across ABI boundaries.
 * * @tparam T The C-struct representing the interface (e.g., demo_iclock_t).
 */
template <typename T>
class ptr {
private:
    T* ptr_ = nullptr;

    void internal_add_ref() {
        if (ptr_) {
            // Safe cast: all ncom interfaces strictly start with an ncom_iunknown_vtbl_t pointer.
            auto* unk = reinterpret_cast<ncom_iunknown_t*>(ptr_);
            unk->vtbl->add_ref(unk);
        }
    }

    void internal_release() {
        if (ptr_) {
            auto* unk = reinterpret_cast<ncom_iunknown_t*>(ptr_);
            unk->vtbl->release(unk);
            ptr_ = nullptr;
        }
    }

public:
    // --- Constructors & Destructor ---
    
    ptr() = default;
    ptr(std::nullptr_t) : ptr_(nullptr) {}
    
    ~ptr() { 
        internal_release(); 
    }

    // Copy constructor (increments RefCount)
    ptr(const ptr& other) : ptr_(other.ptr_) {
        internal_add_ref();
    }

    // Move constructor (takes ownership, no RefCount change)
    ptr(ptr&& other) noexcept : ptr_(other.ptr_) {
        other.ptr_ = nullptr;
    }

    // --- Assignments ---

    ptr& operator=(const ptr& other) {
        if (this != &other) {
            internal_release();
            ptr_ = other.ptr_;
            internal_add_ref();
        }
        return *this;
    }

    ptr& operator=(ptr&& other) noexcept {
        if (this != &other) {
            internal_release();
            ptr_ = other.ptr_;
            other.ptr_ = nullptr;
        }
        return *this;
    }

    ptr& operator=(std::nullptr_t) {
        internal_release();
        return *this;
    }

    // --- Accessors ---

    T* get() const { return ptr_; }
    T* operator->() const { return ptr_; }
    T& operator*() const { return *ptr_; }
    explicit operator bool() const { return ptr_ != nullptr; }

    // --- C-API Interoperability ---

    /**
     * @brief Releases the current object and returns the address of the internal pointer.
     * * Strictly used for out-parameters in C APIs (e.g., when passing to create_instance).
     * * @return T** Address of the internal raw pointer.
     */
    T** put() {
        internal_release();
        return &ptr_;
    }

    /**
     * @brief Takes ownership of a raw pointer without calling add_ref().
     * * Used when receiving an already AddRef'ed pointer from a C API boundary.
     * * @param raw The raw interface pointer.
     */
    void attach(T* raw) {
        internal_release();
        ptr_ = raw;
    }

    /**
     * @brief Relinquishes ownership of the pointer without calling release().
     * * @return T* The raw interface pointer.
     */
    T* detach() {
        T* temp = ptr_;
        ptr_ = nullptr;
        return temp;
    }

    // --- Type-safe QueryInterface ---

    /**
     * @brief Queries the underlying object for a different interface.
     * * Uses iid_traits to automatically determine the correct UUID for type U.
     * * @tparam U The requested interface type.
     * @param out The target smart pointer to receive the requested interface.
     * @return ncom_status_t NCOM_OK on success, or an error code otherwise.
     */
    template <typename U>
    ncom_status_t as(ptr<U>& out) const {
        out.attach(nullptr);
        if (!ptr_) return NCOM_E_INVALID_ARG;

        void* raw_out = nullptr;
        auto* unk = reinterpret_cast<ncom_iunknown_t*>(ptr_);
        
        // iid_traits resolves the correct IID pointer at compile time
        ncom_status_t st = unk->vtbl->query_interface(unk, iid_traits<U>::get(), &raw_out);
        
        if (NCOM_SUCCEEDED(st)) {
            // raw_out is already AddRef'ed by the callee, so we just attach it
            out.attach(static_cast<U*>(raw_out));
        }
        return st;
    }

    // --- Comparison Operators ---

    template <typename U>
    bool operator==(const ptr<U>& other) const { return ptr_ == other.get(); }

    template <typename U>
    bool operator!=(const ptr<U>& other) const { return ptr_ != other.get(); }

    bool operator==(std::nullptr_t) const { return ptr_ == nullptr; }
    bool operator!=(std::nullptr_t) const { return ptr_ != nullptr; }

    friend bool operator==(std::nullptr_t, const ptr& p) { return p.get() == nullptr; }
    friend bool operator!=(std::nullptr_t, const ptr& p) { return p.get() != nullptr; }

    bool operator==(const T* other) const { return ptr_ == other; }
    bool operator!=(const T* other) const { return ptr_ != other; }

    friend bool operator==(const T* lhs, const ptr& rhs) { return lhs == rhs.get(); }
    friend bool operator!=(const T* lhs, const ptr& rhs) { return lhs != rhs.get(); }
};

} // namespace ncom

#endif /* NCOM_NCOM_PTR_HPP */
