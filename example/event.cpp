#include <nodepp/nodepp.h>
#include <apify/apify.h>
#include <nodepp/ws.h>

using namespace nodepp;

void onMain() {
    auto app = apify::add<ws_t>();
    auto srv = ws::server();

    app.on("EVENT_1",nullptr,[=]( apify_t<ws_t> cli ){
        cli.emit( "DONE", nullptr, "Event_1" );
    });

    app.on("EVENT_2",nullptr,[=]( apify_t<ws_t> cli ){
        cli.emit( "DONE", nullptr, "Event_2" );
    });

    app.on("EVENT_3",nullptr,[=]( apify_t<ws_t> cli ){
        cli.emit( "DONE", nullptr, "Event_3" );
    });

    app.on(nullptr,"/PATH",[=]( apify_t<ws_t> cli ){
        console::log( "->", cli.message );
        cli.emit( "DONE", nullptr, "done" );
        app.emit( cli.get_fd(), "EVENT_3", nullptr, nullptr );
        app.emit( cli.get_fd(), "EVENT_2", nullptr, nullptr );
        app.emit( cli.get_fd(), "EVENT_1", nullptr, nullptr );
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
        console::log( "ws:/localhost:8000" );
    }); console::log( "Started" );

}