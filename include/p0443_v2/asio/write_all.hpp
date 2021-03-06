
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <boost/type_traits/is_detected.hpp>

#include <boost/asio/buffer.hpp>

#include <p0443_v2/set_value.hpp>
#include <p0443_v2/set_done.hpp>

namespace p0443_v2::asio
{
namespace detail
{
void async_write();
struct write_all_cpo
{
    template<class Stream, class...Args>
    using member_detector = decltype(std::declval<Stream>().async_write(std::declval<Args>()...));

    template<class Stream, class...Args>
    using use_member = boost::is_detected<member_detector, Stream, Args...>;

    template<class Stream, class...Args>
    auto operator()(Stream& stream, Args&&...args) const
    {
        if constexpr(use_member<Stream, Args...>::value)
        {
            stream.async_write(std::forward<Args>(args)...);
        }
        else {
            async_write(stream, std::forward<Args>(args)...);
        }
    }
};

static constexpr write_all_cpo write_all_impl;
}
template <class Stream>
struct write_all
{
    template <template <class...> class Tuple, template <class...> class Variant>
    using value_types = Variant<Tuple<std::size_t>>;

    template <template <class...> class Variant>
    using error_types = Variant<std::exception_ptr>;

    static constexpr bool sends_done = true;

    Stream *stream_;
    boost::asio::const_buffer buffer_;

    template <class Buffer>
    write_all(Stream &stream, Buffer &&buffer) : stream_(&stream), buffer_(buffer) {
    }

    template <class Receiver>
    struct operation_state
    {
        Receiver receiver_;
        Stream *stream_;
        boost::asio::const_buffer buffer_;

        void start() {
            detail::write_all_impl(
                *stream_, buffer_,
                [recv = std::move(receiver_)](const auto &ec, std::size_t bytes_written) mutable {
                    if (!ec) {
                        p0443_v2::set_value(std::move(recv), bytes_written);
                    }
                    else {
                        p0443_v2::set_done(std::move(recv));
                    }
                });
        }
    };

    template <class Receiver>
    auto connect(Receiver &&receiver) {
        return operation_state<p0443_v2::remove_cvref_t<Receiver>>{std::forward<Receiver>(receiver),
                                                                   stream_, buffer_};
    }
};
} // namespace p0443_v2::asio