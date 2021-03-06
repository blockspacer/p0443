
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/utility/string_view.hpp>
#include <string>

#include <p0443_v2/set_done.hpp>
#include <p0443_v2/set_value.hpp>
#include <p0443_v2/type_traits.hpp>

#include "executor.hpp"

namespace p0443_v2::asio
{
template <class ExecutionContext>
struct resolve
{
    // using value_type = boost::asio::ip::tcp::resolver::results_type;

    template <template <class...> class Tuple, template <class...> class Variant>
    using value_types = Variant<Tuple<boost::asio::ip::tcp::resolver::results_type>>;

    template <template <class...> class Variant>
    using error_types = Variant<std::exception_ptr>;

    static constexpr bool sends_done = true;

    template <class Receiver>
    struct operation_state
    {
        Receiver receiver_;
        boost::asio::ip::tcp::resolver resolv_;
        std::string host_;
        std::string service_;

        void start() {
            if (host_.empty() || service_.empty()) {
                p0443_v2::set_done(std::move(receiver_));
            }
            else {
                resolv_.async_resolve(
                    host_, service_,
                    [receiver = std::move(receiver_)](const auto &ec, auto results) mutable {
                        if (!ec) {
                            p0443_v2::set_value(std::move(receiver), results);
                        }
                        else {
                            p0443_v2::set_done(std::move(receiver));
                        }
                    });
            }
        }
    };

    template <class Receiver>
    auto connect(Receiver &&receiver) {
        return operation_state<p0443_v2::remove_cvref_t<Receiver>>{
            std::forward<Receiver>(receiver), boost::asio::ip::tcp::resolver{*context_}, host_,
            service_};
    }

    resolve() = default;

    resolve(ExecutionContext &context, boost::string_view host, boost::string_view service)
        : context_(&context), host_(host), service_(service) {
    }

    ExecutionContext *context_;
    std::string host_, service_;
};
} // namespace p0443_v2::asio
