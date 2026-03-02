#pragma once

#include <ncom/errors.h>
#include <ncom/error_info.h>
#include <ncom/string.h>
#include "ncom_ptr.hpp"
#include <string>

namespace ncom {

// (Required if the IDL generator hasn't generated it yet)
template <> struct iid_traits<ncom_ierror_info_t> {
    static const ncom_iid_t* get() { return &NCOM_IID_IERRORINFO; }
};

/**
 * @brief Represents an error state crossing the ABI boundary.
 * * Encapsulates the simple status code (ncom_status_t) and an optional 
 * rich error information object (ncom_ierror_info_t).
 */
class error {
private:
    ncom_status_t code_;
    ncom::ptr<ncom_ierror_info_t> info_;

public:
    /**
     * @brief Constructs an error object.
     * * @param code The failure code (defaults to NCOM_E_FAIL).
     * @param info An optional smart pointer to a rich error info object.
     */
    error(ncom_status_t code = NCOM_E_FAIL, ncom::ptr<ncom_ierror_info_t> info = {})
        : code_(code), info_(std::move(info)) {}

    /**
     * @brief Gets the raw status code.
     */
    ncom_status_t code() const { return code_; }
    
    /**
     * @brief Returns the rich error info object, or null if none was provided.
     */
    ncom::ptr<ncom_ierror_info_t> info() const { return info_; }

    /**
     * @brief Extracts the error message.
     * * Falls back to a generic string format if no rich error info is available
     * or if message extraction fails.
     * * @return std::string The error description.
     */
    std::string message() const {
        if (!info_) {
            return "Error code: " + std::to_string(code_);
        }

        ncom::ptr<ncom_istring_t> str_obj;
        // Attempt to get the string object from the error info
        if (NCOM_SUCCEEDED(info_->vtbl->get_message_string(info_.get(), str_obj.put())) && str_obj) {
            const char* cstr = nullptr;
            if (NCOM_SUCCEEDED(str_obj->vtbl->c_str(str_obj.get(), &cstr)) && cstr) {
                return std::string(cstr);
            }
        }
        
        return "Unknown error (code: " + std::to_string(code_) + ")";
    }
};

} // namespace ncom