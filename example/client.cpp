#include <nodepp/nodepp.h>
#include <apify/apify.h>
#include <nodepp/ws.h>

using namespace nodepp;

void onMain(){
    auto srv = ws::client( "ws://localhost:8000" );
    auto app = apify::add<ws_t>();

    app.on("DONE",nullptr,[=]( apify_t<ws_t> cli ){
        console::done( cli.message );
    });

    app.on("FAIL",nullptr,[=]( apify_t<ws_t> cli ){
        console::error( cli.message );
    });

    srv.onConnect([=]( ws_t cli ){

        cli.onData([=]( string_t data ){
            app.next( cli, data );
        });

        srv.onClose([=](){
            console::log("Disconnected");
        }); console::log("Connected");

        apify::add(cli).emit("SUBS"  ,nullptr    ,"hello world!");
        apify::add(cli).emit("VALUE" ,"/100/2000","hello world!");
        apify::add(cli).emit("METHOD","/sub/path","hello world!");
        apify::add(cli).emit("METHOD","/PATH"    ,"hello world!");
        apify::add(cli).emit(nullptr ,"/PATH"    ,"hello world!");

    });

}