#include <nodepp/nodepp.h>
#include <nodepp/tcp.h>
#include <nodepp/fs.h>

#include <apify/apify.h>

using namespace nodepp;

void server(){

    auto srv = tcp::server();
    auto app = apify::add<socket_t>();

    app.on( "GET", "/test/:id", [=]( apify_t<socket_t> cli ){
        console::log( "->", cli.message );
        cli.emit( "DONE", nullptr, regex::format( "AAA ${0}", cli.params["id"] ) );
        cli.emit( "DONE", nullptr, regex::format( "AAA ${0}", cli.params["id"] ) );
        cli.emit( "DONE", nullptr, regex::format( "AAA ${0}", cli.params["id"] ) );
    });

    app.on( [=]( apify_t<socket_t> cli ){
        cli.emit( "FAIL", nullptr, "something went wrong" );
    });

    srv.onConnect([=]( socket_t cli ){

        console::log("connected" );

        cli.onData([=]( string_t data ){
            app.next( cli, data );
        });

        cli.onClose.once([=](){
            console::log("closed");
        });

        stream::pipe( cli );

    });

    srv.listen( "localhost", 8000, []( socket_t srv ){
        console::log("-> tcp://localhost:8000");
    });

}

void client(){

    auto cli = tcp::client();
    auto app = apify::add<socket_t>();

    app.on( "DONE", nullptr, [=]( apify_t<socket_t> cli ){
        console::done( "->", cli.message );
        cli.done();
    });

    app.on( "FAIL", nullptr, [=]( apify_t<socket_t> cli ){
        console::error( "->", cli.message );
        cli.done();
    });

    app.on( [=]( apify_t<socket_t> cli ){
        console::warning( "->", "something went wrong" );
        cli.done();
    });

    cli.onOpen([=]( socket_t cli ){

        console::log("connected" );
    
        cli.onData([=]( string_t data ){
            app.next( cli, data );
        });

        cli.onClose.once([=](){
            console::log("closed");
        });

        apify::add(cli).send( "GET", "/test/id1", "hello world!" );
        apify::add(cli).send( "GET", "/test/id2", "hello world!" );
        apify::add(cli).send( "GET", "/test/id3", "hello world!" );
        stream::pipe( cli );

    });

    cli.connect( "localhost", 8000, []( socket_t cli ){
        console::log("-> tcp://localhost:8000");
    });

}

void onMain() {

    if( process::env::get("mode")=="client" ) 
      { client(); } else { server(); }

}

// g++ -o main main.cpp -I./include ; ./main ?mode=client