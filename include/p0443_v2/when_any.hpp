#pragma once

#include <tuple>
#include <boost/mp11/tuple.hpp>
#include <p0443_v2/type_traits.hpp>

#include <p0443_v2/set_value.hpp>
#include <p0443_v2/set_done.hpp>
#include <p0443_v2/set_error.hpp>

#include <memory>
#include <optional>


namespace p0443_v2
{
namespace detail
{
template<class...Senders>
struct when_any_op
{
    using senders_storage = std::tuple<p0443_v2::remove_cvref_t<Senders>...>;

    template<class Receiver>
    struct receiver
    {
        struct shared_state {
            std::optional<p0443_v2::remove_cvref_t<Receiver>> recv_;

            template<class Rx>
            shared_state(Rx &&rx): recv_(std::in_place, std::forward<Rx>(rx)) {}

            template<class...Values>
            void set_value(Values&&...values)
            {
                if(recv_) {
                    try {
                        p0443_v2::set_value(*recv_, std::forward<Values>(values)...);
                        recv_.reset();
                    }
                    catch(...) {
                        p0443_v2::set_error(*recv_, std::current_exception());
                    }
                }
            }
            void set_done() {
                if(recv_) {
                    p0443_v2::set_done(*recv_);
                    recv_.reset();
                }
            }

            template<class E>
            void set_error(E&& err) {
                if(recv_) {
                    p0443_v2::set_error(*recv_, std::current_exception());
                    recv_.reset();
                }
            }
        };

        std::shared_ptr<shared_state> state_;

        receiver(std::shared_ptr<shared_state> state): state_(state) {}

        template<class...Values>
        void set_value(Values&&...values) {
            state_->set_value(std::forward<Values>(values)...);
        }

        void set_done() {
            state_->set_done();
        }

        template<class E>
        void set_error(E&& err) {
            state_->set_error(std::forward<E>(err));
        }
    };

    senders_storage senders_;

    template<class...Tx>
    explicit when_any_op(std::in_place_t, Tx &&...tx): senders_(std::forward<Tx>(tx)...) {}

    template<class Receiver>
    void submit(Receiver&& rx) {
        auto shared_state = std::make_shared<typename receiver<Receiver>::shared_state>(std::forward<Receiver>(rx));
        boost::mp11::tuple_for_each(senders_, [shared_state](auto &&rx) {
            p0443_v2::submit(rx, receiver<Receiver>(shared_state));
        });
    }
};
}

template<class Sender>
auto when_any(Sender&& sender) {
    return std::forward<Sender>(sender);
}

template<class...Senders>
auto when_any(Senders&&...senders) {
    static_assert(sizeof...(Senders) > 0, "when_any must take at least 1 sender");
    return detail::when_any_op<Senders...>(std::in_place, std::forward<Senders>(senders)...);
}
}