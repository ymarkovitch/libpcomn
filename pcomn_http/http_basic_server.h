/*-*- mode:c++;tab-width:4;indent-tabs-mode:nil;c-file-style:"stroustrup";c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +)) -*-*/
#ifndef __HTTP_BASIC_SERVER_H
#define __HTTP_BASIC_SERVER_H
/*******************************************************************************
 FILE         :   http_basic_server.h
 COPYRIGHT    :   Yakov Markovitch, 2008-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Basic HTTP server.

 CREATION DATE:   9 Mar 2008
*******************************************************************************/
/** @file
    A @em very basic HTTP server.
*******************************************************************************/
#include <pcomn_http/http_connection.h>
#include <pcomn_csvr/commsvr_threaded_server.h>

namespace pcomn {
namespace http {

/******************************************************************************/
/** HTTP server session.
 @note This class is abstract, derived classes should implement method run.
*******************************************************************************/
class HTTPServerSession : public svr::ServerSession {
    typedef ServerSession ancestor ;
public:
    HTTPServerSession(const svr::connection_ptr &connection) :
        ancestor(connection),
        _http(*connection)
    {}

    HTTPServerConnection &http() { return _http ; }
    const HTTPServerConnection &http() const { return _http ; }

private:
    HTTPServerConnection _http ;
} ;

/******************************************************************************/
/** A very basic HTTP server.
*******************************************************************************/
template<class HTTPSession, class Server=svr::ThreadedServer>
class HTTPBasicServer : public Server {
    typedef Server ancestor ;
public:

    /// Constructor.
    /// @param server_name      The name of server, which appears as a value of 'Server'
    /// @param worker_threads   The maximal number of worker threads serving HTTP connections.
    /// header in this server's responses.
    HTTPBasicServer(const std::string &server_name,
                    unsigned worker_threads, unsigned threadpool_capacity,
                    size_t stack_size = 0) :
        ancestor(worker_threads, threadpool_capacity, stack_size),
        _name(server_name),
        _content("application/octet-stream")
    {}

    /// @overload
    /// @param content_type  The default content type for this server. This type appears
    /// as a value of 'Content-Type' header if the response has content but the type of
    /// content isn't explicily specified.
    HTTPBasicServer(const std::string &server_name, const std::string &content_type,
                    unsigned worker_threads, unsigned threadpool_capacity,
                    size_t stack_size = 0) :
        ancestor(worker_threads, threadpool_capacity, stack_size),
        _name(server_name),
        _content(content_type)
    {}

    const std::string &name() const { return _name ; }
    const std::string &default_content() const { return _content ; }

protected:
    svr::server_session_ptr new_session(svr::connection_ptr connection)
    {
        PTSafePtr<HTTPServerSession> http_session (new_http_session(connection)) ;
        NOXCHECK(http_session) ;
        http_session->http().set_server_name(name()) ;
        http_session->http().set_default_content(default_content()) ;

        return svr::server_session_ptr(http_session.release()) ;
    }

private:
    const std::string _name ;
    const std::string _content ;

    virtual HTTPServerSession *new_http_session(svr::connection_ptr) ;
} ;

template<class HTTPSession, class Server>
HTTPServerSession *HTTPBasicServer<HTTPSession, Server>::new_http_session(svr::connection_ptr connection)
{
    return new HTTPSession(connection) ;
}

} // end of namespace pcomn::http
} // end of namespace pcomn

#endif /* __HTTP_BASIC_SERVER_H */
