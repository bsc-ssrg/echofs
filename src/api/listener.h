/*************************************************************************
 * (C) Copyright 2016-2017 Barcelona Supercomputing Center               *
 *                         Centro Nacional de Supercomputacion           *
 *                                                                       *
 * This file is part of the Echo Filesystem NG.                          *
 *                                                                       *
 * See AUTHORS file in the top level directory for information           *
 * regarding developers and contributors.                                *
 *                                                                       *
 * This library is free software; you can redistribute it and/or         *
 * modify it under the terms of the GNU Lesser General Public            *
 * License as published by the Free Software Foundation; either          *
 * version 3 of the License, or (at your option) any later version.      *
 *                                                                       *
 * The Echo Filesystem NG is distributed in the hope that it will        *
 * be useful, but WITHOUT ANY WARRANTY; without even the implied         *
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR               *
 * PURPOSE.  See the GNU Lesser General Public License for more          *
 * details.                                                              *
 *                                                                       *
 * You should have received a copy of the GNU Lesser General Public      *
 * License along with Echo Filesystem NG; if not, write to the Free      *
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.    *
 *                                                                       *
 *************************************************************************/

#ifndef __API_LISTENER_H__
#define __API_LISTENER_H__

#include <boost/asio.hpp>
//#include <norns.h>
//#include <norns-rpc.h>

namespace efsng {
namespace api {

namespace ba = boost::asio;
namespace bfs = boost::filesystem;

template <typename Input, typename Output>
using callback_fun = std::function<std::shared_ptr<Output>(const std::shared_ptr<Input>)>;


/* helper class for managing communication sessions with a client */
// template <typename Message, typename Input, typename Output>
// class session : public std::enable_shared_from_this<session<Message, Input, Output>> {

template <typename Message>
class session : public std::enable_shared_from_this<session<Message>> {

    using Input = typename Message::request_type;
    using Output = typename Message::response_type;

public:
    session(ba::local::stream_protocol::socket socket, callback_fun<Input, Output> callback)
        : m_socket(std::move(socket)),
          m_callback(callback) {}

    ~session() {
//        std::cerr << "session dying\n";
    }

    void start(){
        do_read_request();
    }

private:
    void do_read_request(){

        auto self(std::enable_shared_from_this<session<Message>>::shared_from_this());

        std::size_t header_length = m_message.expected_length(Message::header);

        // read the request header and use the information provided in it
        // to read the request body
        ba::async_read(m_socket,
                ba::buffer(m_message.buffer(Message::header), header_length),
                [this, self](boost::system::error_code ec, std::size_t length) {

                    if(!ec && m_message.decode_header(length)) {
                        //FIXME: check what happens if the caller never
                        //sends a body... are we leaking?
                        do_read_request_body();
                    }
                });
    }

    void do_read_request_body() {

        auto self(std::enable_shared_from_this<session<Message>>::shared_from_this());

        std::size_t body_length = m_message.expected_length(Message::body);

        if(body_length != 0) {
            ba::async_read(m_socket,
                    ba::buffer(m_message.buffer(Message::body), body_length),
                    [this, self](boost::system::error_code ec, std::size_t length) {

                        if(!ec) {
                            std::shared_ptr<Input> req = m_message.decode_body(length);
                            std::shared_ptr<Output> resp = m_callback(req);

                            assert(resp != nullptr);

                            m_message.clear();

                            if(m_message.encode_response(resp)) {
                                do_write_response();
                            }
                        }
                    });
        }
    }

    void do_write_response() {

        std::vector<ba::const_buffer> buffers;
        buffers.push_back(ba::buffer(m_message.buffer(Message::header)));
        buffers.push_back(ba::buffer(m_message.buffer(Message::body)));

        //Message::print_hex(m_message.buffer(Message::header));
        //Message::print_hex(m_message.buffer(Message::body));

        auto self(std::enable_shared_from_this<session<Message>>::shared_from_this());

        ba::async_write(m_socket, buffers,
            [this, self](boost::system::error_code ec, std::size_t /*length*/){

//                std::cerr << "Writing done!\n";

                if(!ec){
                    //do_read_request();
                }
            });
    }

    ba::local::stream_protocol::socket  m_socket;
    callback_fun<Input, Output>         m_callback;
    Message                             m_message;
};


/* simple lister for an AF_UNIX socket that accepts requests asynchronously and
 * invokes a callback with a fixed-length payload */
template <typename Message>
class listener {

    using Input = typename Message::request_type;
    using Output = typename Message::response_type;

//    using DataPtr = std::shared_ptr<Input>;

public:
    listener(const bfs::path& socket_file, callback_fun<Input, Output> callback) 
        : m_acceptor(m_ios, ba::local::stream_protocol::endpoint(socket_file.string())),
          m_socket(m_ios),
          m_callback(callback) {
        do_accept();
    }

    void run() {

        if(m_thread != nullptr) {
            return;
        }

        m_thread.reset(new std::thread(
                    [&]() {
                        m_ios.run();
                    }
                ));

        //m_ios.run();
    }

    void stop() {

        if(m_thread == nullptr) {
            return;
        }

        m_ios.stop();
        m_thread->join();
        m_ios.reset();
        m_thread.reset();

        //m_ios.stop();
    }

private:
    void do_accept(){
        /* start an asynchronous accept: the call to async_accept returns immediately, 
         * and we use a lambda function as the handler */
        m_acceptor.async_accept(m_socket,
            [this](const boost::system::error_code& ec){
                if(!ec){
                    std::make_shared<session<Message>>(std::move(m_socket), m_callback)->start();
                }

                do_accept();
            });
    }

    boost::asio::io_service                m_ios;
    ba::local::stream_protocol::acceptor   m_acceptor;
    ba::local::stream_protocol::socket     m_socket;
    callback_fun<Input, Output>            m_callback;

    std::unique_ptr<std::thread>           m_thread;
};

} // namespace api
} // namespace efsng

#endif /* __API_LISTENER_H__ */
