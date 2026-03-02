#pragma once

#include "ncom_error.hpp"
#include <optional>
#include <cassert>

namespace ncom {

/**
 * @brief A Rust-like Result type for handling success values or errors.
 * * Forces the caller to check is_ok() or is_err() before accessing the value.
 * Designed to replace C++ exceptions for robust, predictable control flow.
 * * @tparam T The type of the value returned on success.
 */
template <typename T>
class result {
private:
    std::optional<T> value_;
    ncom::error error_;
    bool is_ok_;

public:
    /**
     * @brief Construct a successful result.
     * @param value The success payload.
     */
    result(T value) 
        : value_(std::move(value)), error_(NCOM_OK), is_ok_(true) {}

    /**
     * @brief Construct an error result.
     * @param err The encapsulated error.
     */
    result(ncom::error err) 
        : value_(std::nullopt), error_(std::move(err)), is_ok_(false) {}

    // --- State Queries ---
    
    bool is_ok() const { return is_ok_; }
    bool is_err() const { return !is_ok_; }

    /**
     * @brief Evaluates to true if the result is successful.
     */
    explicit operator bool() const { return is_ok(); }

    // --- Accessors ---
    
    /**
     * @brief Access the success value. 
     * @warning Asserts that the result is actually OK. Check is_ok() first!
     */
    T& unwrap() {
        assert(is_ok_ && "Called unwrap() on an error result!");
        return *value_;
    }

    const T& unwrap() const {
        assert(is_ok_ && "Called unwrap() on an error result!");
        return *value_;
    }

    /**
     * @brief Access the error object.
     * @warning Asserts that the result is actually an error. Check is_err() first!
     */
    const ncom::error& unwrap_err() const {
        assert(!is_ok_ && "Called unwrap_err() on a success result!");
        return error_;
    }
};

/**
 * @brief Specialization for functions that don't return a payload (void equivalent).
 */
template <>
class result<void> {
private:
    ncom::error error_;
    bool is_ok_;

public:
    result() : error_(NCOM_OK), is_ok_(true) {}
    result(ncom::error err) : error_(std::move(err)), is_ok_(false) {}

    bool is_ok() const { return is_ok_; }
    bool is_err() const { return !is_ok_; }
    explicit operator bool() const { return is_ok(); }

    const ncom::error& unwrap_err() const {
        assert(!is_ok_ && "Called unwrap_err() on a success result!");
        return error_;
    }
};

} // namespace ncom