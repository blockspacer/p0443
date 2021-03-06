
//          Copyright Andreas Wass 2004 - 2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <p0443_v2/asio/write_all.hpp>
#include <p0443_v2/asio/read_all.hpp>
#include <p0443_v2/then.hpp>

#include <boost/asio/buffer.hpp>

#include <string>
#include <cstdio>

struct chat_message
{
    std::string message_;

    chat_message() = default;
    explicit chat_message(std::string msg) : message_(std::move(msg)) {
    }

    template <class Socket>
    auto deliver(Socket &socket) {
        char header[5] = "";
        std::snprintf(header, 5, "%4d", static_cast<int>(message_.size()));
        message_.insert(0, header);
        return p0443_v2::asio::write_all(socket, boost::asio::buffer(message_));
    }

    template <class Socket>
    auto read(Socket &socket) {
        message_.resize(4);
        auto read_header = p0443_v2::asio::read_all(socket, boost::asio::buffer(message_));
        auto read_body = [this, &socket](std::size_t /*header_read_amount = 4*/) {
            auto body_size = std::atoi(message_.c_str());
            message_.resize(body_size);
            return p0443_v2::asio::read_all(socket, boost::asio::buffer(message_));
        };

        return p0443_v2::then(std::move(read_header), std::move(read_body));
    }
};