
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/tuple.hpp>
#include <p0443_v2/type_traits.hpp>
#include <tuple>

#include <p0443_v2/set_done.hpp>
#include <p0443_v2/set_error.hpp>
#include <p0443_v2/set_value.hpp>
#include <p0443_v2/sender_traits.hpp>

#include <memory>
#include <utility>
#include <optional>

namespace p0443_v2
{
namespace detail
{
template <class... Senders>
struct when_all_op
{
    using senders_storage = std::tuple<Senders...>;

    template<template<class...> class Tuple, template<class...> class Variant>
    using value_types = boost::mp11::mp_unique<
        boost::mp11::mp_append<
            typename p0443_v2::sender_traits<Senders>::template value_types<Tuple, Variant>...
        >
    >;

    template<template<class...> class Variant>
    using error_types = boost::mp11::mp_unique<
        boost::mp11::mp_append<
            typename p0443_v2::sender_traits<Senders>::template error_types<Variant>...
        >
    >;

    static constexpr bool sends_done = (p0443_v2::sender_traits<Senders>::sends_done || ...);

    template <class Receiver>
    struct receiver
    {
        struct shared_state
        {
            int waiting_for_ = 0;
            std::optional<p0443_v2::remove_cvref_t<Receiver>> recv_;

            template <class Rx>
            shared_state(int waiting_for, Rx &&rx)
                : waiting_for_(waiting_for), recv_(std::in_place, std::forward<Rx>(rx)) {
            }

            template <class... Values>
            void set_value(Values &&... values) {
                if(recv_) {
                    waiting_for_--;
                    if (waiting_for_ == 0) {
                        try {
                            p0443_v2::set_value(*recv_, std::forward<Values>(values)...);
                            recv_.reset();
                        }
                        catch (...) {
                            p0443_v2::set_error(*recv_, std::current_exception());
                        }
                    }
                }
            }
            void set_done() {
                if (recv_) {
                    p0443_v2::set_done(*recv_);
                    recv_.reset();
                }
            }

            template <class E>
            void set_error(E &&err) {
                if (recv_) {
                    p0443_v2::set_error(*recv_, std::current_exception());
                    recv_.reset();
                }
            }
        };

        std::shared_ptr<shared_state> state_;

        receiver(std::shared_ptr<shared_state> state) : state_(state) {
        }

        template <class... Values>
        void set_value(Values &&... values) {
            state_->set_value(std::forward<Values>(values)...);
        }

        void set_done() {
            state_->set_done();
        }

        template <class E>
        void set_error(E &&err) {
            state_->set_error(std::forward<E>(err));
        }
    };

    senders_storage senders_;

    template <class... Tx>
    when_all_op(Tx &&... tx) : senders_(std::forward<Tx>(tx)...) {
    }

    template <class Receiver>
    void submit(Receiver &&rx) {
        auto shared_state =
            std::make_shared<typename receiver<Receiver>::shared_state>(static_cast<int>(std::tuple_size_v<senders_storage>), std::forward<Receiver>(rx));
        boost::mp11::tuple_for_each(senders_, [shared_state](auto &&rx) {
            p0443_v2::submit(rx, receiver<Receiver>(shared_state));
        });
    }
};
} // namespace detail

template<class S>
auto when_all(S &&sender) {
    return std::forward<p0443_v2::remove_cvref_t<S>>(sender);
}

template <class... Senders>
auto when_all(Senders &&... senders) {
    static_assert(sizeof...(Senders) > 0, "when_all must take at least 1 sender");
    return detail::when_all_op<p0443_v2::remove_cvref_t<Senders>...>(std::forward<Senders>(senders)...);
}
} // namespace p0443_v2