#include <nodepp/nodepp.h>
#include <apify/apify.h>
#include <nodepp/ws.h>

using namespace nodepp;

void onMain(){
    auto srv = ws::client( "ws://localhost:8000" );
    auto app = apify::add<ws_t>();

    app.on("DONE","/PATH",[=]( apify_t<ws_t> cli ){
        console::log( cli.message );
    });

    srv.onConnect([=]( ws_t cli ){

        cli.onData([=]( string_t data ){
            app.next( cli, data );
        });

        srv.onClose([=](){
            console::log("Disconnected");
        }); console::log("Connected");

        apify::add(cli).emit("METHOD","/PATH","hello world!");

    });

}