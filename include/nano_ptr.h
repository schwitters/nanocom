#pragma once
#include "nano_base.h"
#include <type_traits>
#include <utility>

namespace nano {

/**
 * @brief A smart pointer for nano-COM interfaces managing reference counting.
 * * Similar to Microsoft::WRL::ComPtr, this template class ensures that 
 * add_ref() and release() are called correctly, preventing memory leaks 
 * when crossing ABI boundaries.
 */
template <typename T>
class ptr {
private:
    T* ptr_ = nullptr;

    void internal_add_ref() {
        if (ptr_) {
            // Cast to i_unknown_t is safe because all interface VTables 
            // strictly start with the i_unknown_vtbl_t base struct.
            auto* unk = reinterpret_cast<i_unknown_t*>(ptr_);
            unk->vtbl->add_ref(unk);
        }
    }

    void internal_release() {
        if (ptr_) {
            auto* unk = reinterpret_cast<i_unknown_t*>(ptr_);
            unk->vtbl->release(unk);
            ptr_ = nullptr;
        }
    }

public:
    // --- Constructors & Destructor ---
    ptr() = default;
    
    ~ptr() { internal_release(); }

    // Copy constructor (increments RefCount)
    ptr(const ptr& other) : ptr_(other.ptr_) {
        internal_add_ref();
    }

    // Move constructor (takes ownership without changing RefCount)
    ptr(ptr&& other) noexcept : ptr_(other.ptr_) {
        other.ptr_ = nullptr;
    }

    // Assignment operator
    ptr& operator=(const ptr& other) {
        if (this != &other) {
            internal_release();
            ptr_ = other.ptr_;
            internal_add_ref();
        }
        return *this;
    }

    // --- Pointer Access ---
    T* get() const { return ptr_; }
    T* operator->() const { return ptr_; }
    explicit operator bool() const { return ptr_ != nullptr; }

    // --- C-API Interoperability ---
    
    /**
     * @brief Releases the current object and returns the address of the internal pointer.
     * Useful for passing to out-parameters in C APIs (e.g., query_interface).
     */
    T** put() {
        internal_release();
        return &ptr_;
    }

    /**
     * @brief Takes ownership of a raw pointer without calling add_ref().
     * Typically used when a function returns an already AddRef'ed pointer.
     */
    void attach(T* raw) {
        internal_release();
        ptr_ = raw;
    }

    // --- Type-safe QueryInterface ---

    /**
     * @brief Queries the underlying object for a different interface.
     * * Uses iid_traits to automatically determine the correct UUID for type U.
     * * @param out The target smart pointer to receive the requested interface.
     * @return status_t STATUS_OK on success, or an error code otherwise.
     */
    template <typename U>
    status_t as(ptr<U>& out) const {
        out.attach(nullptr);
        if (!ptr_) return STATUS_E_INVALID_ARG;

        void* raw_out = nullptr;
        auto* unk = reinterpret_cast<i_unknown_t*>(ptr_);
        
        // iid_traits automatically resolves the correct IID at compile time
        status_t st = unk->vtbl->query_interface(unk, iid_traits<U>::get(), &raw_out);
        
        if (STATUS_SUCCEEDED(st)) {
            // raw_out is already AddRef'ed by the callee, so we just attach it
            out.attach(static_cast<U*>(raw_out));
        }
        return st;
    }
};

} // namespace nano