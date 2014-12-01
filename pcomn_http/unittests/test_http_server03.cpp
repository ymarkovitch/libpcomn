/*-*- mode: c++; tab-width: 4; c-file-style: "stroustrup"; c-file-offsets:((innamespace . 0) (inline-open . 0) (case-label . +)) -*-*/
/*******************************************************************************
 FILE         :   test_http_server03.cpp
 COPYRIGHT    :   Yakov Markovitch, 2008-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   HTTP 'ls' server.

 CREATION DATE:   10 Mar 2008
*******************************************************************************/
#include <pcomn_http/http_basic_server.h>

#include <pcomn_cmdline/cmdext.h>

#include <pcomn_syncobj.h>
#include <pcomn_trace.h>
#include <pcomn_handle.h>

using namespace pcomn ;
using namespace pcomn::http ;
using namespace pcomn::svr ;

using pcomn::StreamLock ;

// Global variable that says whether to print messages when someone connects to the server,
// when a request is being handled, and when connection is closed.

CMDL_GLOBAL_OPT(port, uint16_t, 50080, 'p', "port","number", "Bind port");
CMDL_GLOBAL_OPT(threads, unsigned, 5, 't', "threads", "number", "Thread pool size");
CMDL_GLOBAL_OPT(capacity, unsigned, 50, 'c', "capacity", "number", "New connection queue size");
CMDL_GLOBAL_OPT(backlog, unsigned, 30, 'b', "backlog", "number", "Listen socket pending connections queue size");
CMDL_GLOBAL_BOOL(quiet, false, 'q', "quiet", "Be quiet about accepting new connection");
CMDL_GLOBAL_OPT(host, std::string, "localhost", 'h', "host", "ipaddr", "Bind ip");

#define PRINTOUT(msg) (!quiet && (::pcomn::StreamLock(std::cout).stream() << msg << std::endl))

class HTTPLSServerSession : public HTTPServerSession {
    typedef HTTPServerSession ancestor ;
public:
    explicit HTTPLSServerSession(connection_ptr connection) :
        ancestor(connection)
    {}
private:
    // Implement abstract virtual function run()
    int run() ;

    void handle_request() ;

    void answer_help(bool good_request) ;

    void answer_chunked() ;
    void answer_apiece() ;

    const std::string ls_command() const ;
    FILE *open_command_pipe() ;
} ;

int HTTPLSServerSession::run()
{
    TRACEP("Running HTTPLSServerSession on connection " << connection()) ;
    PRINTOUT("HTTPLSServerSession was connected to " << connection().peer()) ;

    try {
        // Note this loop. The server processes requests in a loop one by one until either
        // client requests close, or an error occurs.
        do {
            // We needn't any request content anyway, so skip it if exists.
            const HTTPRequest &request = http().receive_request(true) ;

            // Allow GET methods
            switch (request.method())
            {
                case HTTPRequest::HTTP_GET: handle_request() ; break ;

                default:
                    // Answer error 405 and indicate this connection is closed
                    HTTPResponse response (HTTP_RSP_METHOD_NOT_ALLOWED | HTTP_RSPFLAG_CLOSE) ;
                    // Be polite, say what _is_ allowed.
                    response.set_header("allow", "GET") ;
                    http().respond(response) ;
            }
        }
        // Run until connection is closed
        while(!http().is_closed()) ;

        PRINTOUT("Closing connection " << http().id() << " in normal fashion.") ;
    }
    catch(const std::exception &x)
    {
        StreamLock(std::cout).stream() << "Exception in connection: " << x.what() << std::endl ;
        throw ;
    }

    return 1 ;
}

void HTTPLSServerSession::handle_request()
{
    const HTTPRequest &req = http().last_request() ;
    const pcomn::uri::query_dictionary &query = req.query_fields() ;
    const std::string *command = query.find("command") ;

    // HTTP request already has dictionary of parsed urldecoded query fields
    if (!command)
    {
        answer_help(false) ;
        return ;
    }
    if (*command == "help")
    {
        answer_help(true) ;
        return ;
    }
    if (*command != "ls")
    {
        answer_help(false) ;
        return ;
    }
    // HTTP 1.0 doesn't support chunked transfer. But we _want_ to demonstrate chunked
    // transfer, that's why we should check for HTTP version...
    if (req.version().second == 0)
        answer_apiece() ;
    else
        answer_chunked() ;
}

static const char help_text[] = (
    "This server can issue 'ls' command with parameters and return result to a client.\n\n\n"
    "To get help:                      http://localhost:50080/?command=help\n\n"
    "To list '/home' directory:        http://localhost:50080/home?command=ls\n\n"
    "To list '/usr/local' recursively: http://localhost:50080/usr/local?command=ls&options=-l -R\n\n"
    ) ;

void HTTPLSServerSession::answer_help(bool good_request)
{
    PRINTOUT((good_request ? "Help requested " : "Invalid query ") << "from " << http().peer()) ;

    const HTTPRequest &req = http().last_request() ;
    if (good_request)
    {
        req.uri().user().empty()
            ? http().respond(HTTP_RSP_OK, help_text)
            : http().respond(HTTP_RSP_OK, pcomn::strprintf("Hello, %s!\n%s\n",
                                                           req.uri().user().stdstring().c_str(), help_text)) ;
        return ;
    }

    pcomn::uri::query_dictionary sample_query ;
    sample_query["command"] = "help" ;

    http().respond(HTTP_RSP_BAD_REQUEST,
                   pcomn::strprintf("%s. Please specify a valid command.\n"
                                    "For help, send query with a 'help' command, e.g.:\n%s",

                                    req.query_fields().empty()
                                    ? "There is no query in URL"
                                    : "There is no 'command' field in URL query",

                                    URI("http", "localhost", 50080, req.uri().path(), sample_query).str().c_str())) ;
}

const std::string HTTPLSServerSession::ls_command() const
{
    const HTTPRequest &req = http().last_request() ;
    // Put together an 'ls' command string
    // Note the usage of default string in call to query_dictionary::get()
    // Note also that the last ls argument is  a path from the request URL
    return pcomn::strprintf("ls %s %s 2>&1",
                            req.query_fields().get("options", "-l").c_str(),
                            req.uri().path().stdstring().c_str()) ;
}

FILE *HTTPLSServerSession::open_command_pipe()
{
    const std::string command (ls_command()) ;
    StreamLock(std::cout).stream() << command << std::endl ;

    if (FILE *p = popen(command.c_str(), "r"))
        return p ;
    // Answer error
    http().respond(HTTP_RSP_INTERNAL_SERVER_ERROR | HTTP_RSPFLAG_CLOSE) ;
    return NULL ;
}

void HTTPLSServerSession::answer_chunked()
{
    pcomn::safe_handle<pcomn::FILE_handle_tag> p (open_command_pipe()) ;
    HTTPResponse response (HTTP_RSP_OK) ;
    // By the chunked trunsfer, one needn't know the size of data to transfer in
    // advance. This is extremely convenient when data being sent is, e.g. output from
    // some script, etc.

    // 1. Set response to chunked transfer mode; in order to do that, pass -1
    // (or HTTP_CHUNKED_CONTENT - this is the same) as content_length value.
    response.content_length(HTTP_CHUNKED_CONTENT) ;
    // 2. Send response _before_ starting trunsfer data
    http().respond(response) ;

    // 3. Send data chunk-by-chunk. Every call to transmit() sends a chunk. Chunks can
    // be of arbitrary _nonzero_ length.
    char buf[1024] ;
    size_t last_sz ;
    do {
        last_sz = fread(buf, 1, sizeof buf, p) ;
        if (last_sz)
            http().transmit(buf, last_sz) ;
    }
    while(last_sz == sizeof buf) ;

    // 4. In order to finalize transfer, transmit zero-length data
    http().transmit("", 0) ;
}

void HTTPLSServerSession::answer_apiece()
{
    pcomn::safe_handle<pcomn::FILE_handle_tag> p (open_command_pipe()) ;
    char buf[8192] ;
    size_t last_sz ;
    std::string result ;
    // Prepare the _whole_ buffer first
    do {
        last_sz = fread(buf, 1, sizeof buf, p) ;
        result.append(buf + 0, buf + last_sz) ;
    }
    while(last_sz == sizeof buf) ;

    // Send the whole buffer
    http().respond(HTTP_RSP_OK, result) ;
}

typedef HTTPBasicServer<HTTPLSServerSession> HTTPLSServer ;

int main(int argc, char *argv[])
{
    DIAG_INITTRACE("httptest.ini") ;

    cmdl::global::parse_cmdline(argc, argv) ;

    int pending_q = 30 ;

    try
    {
        HTTPLSServer server("HTTP List Files Test Server/0.1", "text/plain", threads, capacity) ;
        const net::sock_address address (net::inet_address(host.value()), port) ;

        StreamLock(std::cout).stream()
            << "Starting server(" << address << ", " << backlog << ", " << pending_q << ')' << std::endl
            << "worker_threads=" << threads << " capacity=" << capacity << std::endl ;

        if (!server.start(address, backlog, pending_q))
        {
            StreamLock(std::cout).stream() << "Server hasn't started" << std::endl ;
            return 1 ;
        }

        StreamLock(std::cout).stream() << "Server has started\nPlease hit <AnyKey><ENTER> to exit." << std::endl ;
        char c ; std::cin >> c ;
        StreamLock(std::cout).stream() << "Crash-stopping the server..." << std::endl ;
        server.stop(0) ;
        StreamLock(std::cout).stream() << "The server has stopped." << std::endl ;
    }
    catch(const std::exception &x)
    {
        StreamLock(std::cout).stream() << "Exception: " << x.what() << std::endl ;
        return 1 ;
    }
    return 0 ;
}
