#ifndef misc_error_h
#define misc_error_h

#include <system_error>

enum class misc_errc
{
    eof = 1,
    read_cause_close,
    write_cause_close,
};

namespace std
{
    template <> struct is_error_code_enum<misc_errc>
    : true_type { };
    
    error_code make_error_code(misc_errc e) noexcept;
}

class misc_errc_category : public std::error_category
{
public:
    virtual const char * name() const noexcept override;
    virtual std::string message(int ev) const override;
};

const misc_errc_category & misc_category() noexcept;

#endif /* misc_error_h */
