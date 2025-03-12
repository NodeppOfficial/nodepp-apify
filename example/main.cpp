#include <nodepp/nodepp.h>
#include <apify/apify.h>
#include <nodepp/ws.h>

using namespace nodepp;

void server() {
    auto app = apify::add<ws_t>();
    auto srv = ws::server();

    app.ADD("/:id",[=]( apify_t<ws_t> cli ){
        console::log( cli.params["id"] );
        cli.done();
    });

    app.ADD([=]( apify_t<ws_t> cli ){
        console::log( cli.message );
        cli.done();
    });

    srv.onConnect([=]( ws_t cli ){

        cli.onData([=]( string_t data ){
            app.next( cli, data );
        });

        cli.onClose([=](){
            console::log("Disconnected");
        }); console::log("Connected");

    });

    srv.listen("localhost",8000,[=](...){
        console::log( "<>", "ws:/localhost:8000" );
    }); console::log("Started");

}

void client(){
    auto srv = ws::client( "ws://localhost:8000" );

    srv.onConnect([=]( ws_t cli ){

        apify_t<ws_t> app( cli );

        srv.onClose([=](){
            console::log("Disconnected");
        }); console::log("Connected");

        app.respond( "Hello World" );
        app.respond( "/done", "AAA" );

    });

}

void onMain(){

    if( process::env::get("mode")=="client" )
      { client(); } else { server(); }

}
