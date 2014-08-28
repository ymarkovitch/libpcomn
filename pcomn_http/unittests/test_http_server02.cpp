/*-*- mode: c++; tab-width: 4; c-file-style: "stroustrup"; c-file-offsets:((innamespace . 0) (inline-open . 0) (case-label . +)) -*-*/
/*******************************************************************************
 FILE         :   test_http_server02.cpp

 DESCRIPTION  :   HTTP echo connection.
                  "Serverless" HTTP connection. Responds to a GET request sending response
                  with the text of incoming request as response body.

                  This test/example shows using of standalone HTTPServerConnection. In
                  this test, we serve HTTP GET request, answering  with text of
                  pretty-printed incoming request.
                  There is not any "server" object, there is only a loop in main(), and
                  nonetheless this loop implements a primitive HTTP server, with
                  fundamental limitation that at any moment only single HTTP session can
                  be served.

 CREATION DATE:   17 Mar 2008
*******************************************************************************/
#include <pcomn_http/http_basic_server.h>
#include <pcomn_trace.h>

using namespace pcomn ;

int main(int argc, char *argv[])
{
   DIAG_INITTRACE("httptest.ini") ;

   int port = 50080 ;
   net::sock_address address(argc > 1 ? argv[1] : "localhost", argc > 2 ? (port = atoi(argv[2])) : port) ;
   std::cout << "To stop server, press Ctrl-C or kill -SIGTERM" << std::endl ;

   try {
       // Since there is no "server" top provide us with connections, we need a server socket
       // to accept connections.
       net::server_socket svr_socket (address) ;
       svr_socket.listen() ;

       // Here is our "server".
       while(true)
       {
           std::cout << "Waiting for incoming connections at " << address << std::endl ;
           // Create a communication connection on the stack...
           // Here we accepting incoming connections.
           svr::BlockingConnection comm_connection
               (net::stream_socket_ptr(new net::stream_socket(svr_socket))) ;

           std::cout << "Connected from " << comm_connection.peer() << std::endl ;

           // We've accepted a TCP connection _and_ created a "communication connection"
           // from it.
           // Now we need an HTTP connection working upon this "communication connection".
           http::HTTPServerConnection http_connection (comm_connection) ;
           // Set the default content type.
           http_connection.set_default_content("text/plain") ;


           try {
               // Note this loop. We handle persistent HTTP connection, processing requests in a
               // loop one by one until either client requests close, or an error occurs.
               do {
                   // Parameter "true" tells that we want to ignore any remaining content
                   // of the previous request.
                   const http::HTTPRequest &request = http_connection.receive_request(true) ;

                   // GET is the _only_ method allowed.
                   if (request.method() == http::HTTPRequest::HTTP_GET)
                       // Convert a request into human-readable representation and respond with this text.
                       http_connection.respond(http::HTTP_RSP_OK, request.str(pcomn::eolmode_LF)) ;

                   else
                       // Answer error 405 and set 'Connection: close'. The response
                       // object will be created implicitly. Note that everywhere where both
                       // HTTPResponse constructror and HTTPServerConnection::respond accept the
                       // "response code" parameter they, in fact, can accept a bit-ORed
                       // combination of HTTP response code (as in RFC, e.g. 200, 405, etc.) and
                       // "response flags", describing additional HTTP headers sent for the
                       // response. HTTP_RSPFLAG_CLOSE implies that there will be
                       // "Connection: close" header in the response, letting the client know
                       // that the server will close a connection immediately after sending a
                       // response.
                       // Note also that HTTP connection does _not_ close a communication
                       // connection it works upon, but change its own state: after sending a
                       // response with Connection:close, is_closed() returns true and any
                       // attempt to either send or receive a message through such connection
                       // will throw std::logic_error.
                       http_connection.respond(http::HTTP_RSP_METHOD_NOT_ALLOWED | http::HTTP_RSPFLAG_CLOSE) ;
               }
               // Run until connection is closed
               while(!http_connection.is_closed()) ;

               // All HTTP connections have integral ID, unique (i.e. not resusable) throughout
               // lifetime of a process.
               std::cout << "Connection " << http_connection.id() << " closed." << std::endl ;
           }
           catch(const http::http_connection_closed &e) { std::cout << e.what() << std::endl ; }
           catch(const std::exception &x)
           {
               std::cout << "Exception in connection: " << x.what() << ". Exiting..." << std::endl ;
               exit(1) ;
           }
       }
   }
   catch(const std::exception &x)
   {
       std::cout << "Exception " << PCOMN_TYPENAME(x) << ": " << x.what() << std::endl ;
       return 1 ;
   }
   return 0 ;
}
