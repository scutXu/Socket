#include "misc_error.h"
#include <string>

namespace std
{
    error_code make_error_code(misc_errc e) noexcept
    {
        return error_code(static_cast<int>(e), misc_category());
    }
}

const char * misc_errc_category::name() const noexcept
{
    return "misc";
}

std::string misc_errc_category::message(int ev) const
{
    switch(static_cast<misc_errc>(ev)) {
        case misc_errc::eof:
            return "end of file";
        default:
            return "Unknown misc error";
    }
}

const misc_errc_category & misc_category() noexcept
{
    static misc_errc_category instance;
    return instance;
}
