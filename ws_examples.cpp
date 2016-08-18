#include "server_ws.hpp"
#include "client_ws.hpp"
#include <fstream>
#include <iostream>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
//include <GL/glu.h>
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
//include <GL/glext.h>


#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>


#define window_width  640
#define window_height 480

#include <vector>
#include <iostream>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <png.h>

#define GL_GLEXT_PROTOTYPES 1
enum Constants { SCREENSHOT_MAX_FILENAME = 256 };
static const GLenum FORMAT = GL_RGBA;
static const GLuint FORMAT_NBYTES = 4;
static GLubyte *pixels = NULL;
static GLuint fbo;
static GLuint rbo_color;
static GLuint rbo_depth;
static const unsigned int HEIGHT = 480;
static const unsigned int WIDTH = 640;
static int offscreen = 1;
static unsigned int max_nframes = 100;
static unsigned int nframes = 0;
static unsigned int time0;

using namespace std;


static void PngWriteCallback(png_structp  png_ptr, png_bytep data, png_size_t length) {
    std::vector<unsigned char> *p = (std::vector<unsigned char>*)png_get_io_ptr(png_ptr);
    p->insert(p->end(), data, data + length);
}


static png_byte *png_bytes = NULL;
static png_byte **png_rows = NULL;
static void screenshot_png(const char *filename, unsigned int width, unsigned int height,
        GLubyte **pixels, png_byte **png_bytes, png_byte ***png_rows, std::vector<unsigned char> *out) {
    size_t i, nvals;
    const size_t format_nchannels = 4;
    FILE *f = fopen(filename, "wb");
    nvals = format_nchannels * width * height;
    *pixels = (GLubyte*)realloc(*pixels, nvals * sizeof(GLubyte));
    *png_bytes = (png_byte*)realloc(*png_bytes, nvals * sizeof(png_byte));
    *png_rows = (png_byte**)realloc(*png_rows, height * sizeof(png_byte*));
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, *pixels);
    for (i = 0; i < nvals; i++)
        (*png_bytes)[i] = (*pixels)[i];
    for (i = 0; i < height; i++)
        (*png_rows)[height - i - 1] = &(*png_bytes)[i * width * format_nchannels];
    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) abort();
    png_infop info = png_create_info_struct(png);
    if (!info) abort();
    if (setjmp(png_jmpbuf(png))) abort();
    //png_init_io(png, f);
    png_set_IHDR(
        png,
        info,
        width,
        height,
        8,
        PNG_COLOR_TYPE_RGBA,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
    );
    /*png_write_info(png, info);
    png_write_image(png, *png_rows);
    png_write_end(png, NULL);
    fclose(f);*/
    
    png_set_rows(png, info, *png_rows);
    png_set_write_fn(png, out, PngWriteCallback, NULL);
	png_write_png(png, info, PNG_TRANSFORM_IDENTITY, NULL);
}


static void create_ppm(char *prefix, int frame_id, unsigned int width, unsigned int height,
        unsigned int color_max, unsigned int pixel_nbytes, GLubyte *pixels) {
    size_t i, j, k, cur;
    enum Constants { max_filename = 256 };
    char filename[max_filename];
    snprintf(filename, max_filename, "%s%d.ppm", prefix, frame_id);
    FILE *f = fopen(filename, "w");
    fprintf(f, "P3\n%d %d\n%d\n", width, HEIGHT, 255);
    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            cur = pixel_nbytes * ((height - i - 1) * width + j);
            fprintf(f, "%3d %3d %3d ", pixels[cur], pixels[cur + 1], pixels[cur + 2]);
        }
        fprintf(f, "\n");
    }
    fclose(f);
}

// Main loop
void renderea(float angle,int nscreen, std::vector<unsigned char> *rows)
{
   // Z angle
   //static float angle;
   // Clear color (screen) 
   // And depth (used internally to block obstructed objects)
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   // Load identity matrix
   glLoadIdentity();
   // Multiply in translation matrix
   glTranslatef(0,0, -10);
   // Multiply in rotation matrix
   glRotatef(angle, 0, 0, 1);
   // Render colored quad
   glBegin(GL_QUADS);
   glColor3ub(255, 000, 000); glVertex2f(-1,  1);
   glColor3ub(000, 255, 000); glVertex2f( 1,  1);
   glColor3ub(000, 000, 255); glVertex2f( 1, -1);
   glColor3ub(255, 255, 000); glVertex2f(-1, -1);
   glEnd();
   // Swap buffers (color buffers, makes previous render visible)
	//glutSwapBuffers();
	glFlush();
   // Increase angle to rotate
   //angle+=0.25;
   
    /*vector< unsigned char > pixels1( window_width * window_height * 4 );
      glReadPixels
	  ( 
	  0, 0, 
	  window_width, window_height, 
	  GL_RGBA, GL_UNSIGNED_BYTE, &pixels1[0] 
	  );
	 
      for(vector<unsigned char>::iterator it=pixels1.begin();it!=pixels1.end();it++)
	cout << (int)(*it) << endl;
      */
      glReadPixels(0, 0, WIDTH, HEIGHT, FORMAT, GL_UNSIGNED_BYTE, pixels);
      screenshot_png("paQCachis.png",WIDTH, HEIGHT, &pixels, &png_bytes, &png_rows, rows);
      //create_ppm("tmp", nscreen, WIDTH, HEIGHT, 255, FORMAT_NBYTES, pixels);
}

// Initialze OpenGL perspective matrix
void GL_Setup(int width, int height)
{

	glViewport( 0, 0, width, height );
	glMatrixMode( GL_PROJECTION );
	glEnable( GL_DEPTH_TEST );
	gluPerspective( 45, (float)width/height, .1, 100 );
	glMatrixMode( GL_MODELVIEW );
	
	pixels = (GLubyte*)malloc(FORMAT_NBYTES * WIDTH * HEIGHT);
}








typedef SimpleWeb::SocketServer<SimpleWeb::WS> WsServer;
typedef SimpleWeb::SocketClient<SimpleWeb::WS> WsClient;

int main() {
    //WebSocket (WS)-server at port 8080 using 4 threads
    WsServer server(8080, 4);
    
    //Example 1: echo WebSocket endpoint
    //  Added debug messages for example use of the callbacks
    //  Test with the following JavaScript:
    //    var ws=new WebSocket("ws://localhost:8080/echo");
    //    ws.onmessage=function(evt){console.log(evt.data);};
    //    ws.send("test");
    auto& echo=server.endpoint["^/echo/?$"];
    
    echo.onmessage=[&server](shared_ptr<WsServer::Connection> connection, shared_ptr<WsServer::Message> message) {
        //WsServer::Message::string() is a convenience function for:
        //stringstream data_ss;
        //data_ss << message->rdbuf();
        //auto message_str = data_ss.str();
        auto message_str=message->string();
        
        cout << "Server: Message received: \"" << message_str << "\" from " << (size_t)connection.get() << endl;
                
        cout << "Server: Sending message \"" << message_str <<  "\" to " << (size_t)connection.get() << endl;
        
        auto send_stream=make_shared<WsServer::SendStream>();
        *send_stream << message_str;
        //server.send is an asynchronous function
        server.send(connection, send_stream, [](const boost::system::error_code& ec){
            if(ec) {
                cout << "Server: Error sending message. " <<
                //See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
                        "Error: " << ec << ", error message: " << ec.message() << endl;
            }
        });
    };
    
    echo.onopen=[](shared_ptr<WsServer::Connection> connection) {
        cout << "Server: Opened connection " << (size_t)connection.get() << endl;
    };
    
    //See RFC 6455 7.4.1. for status codes
    echo.onclose=[](shared_ptr<WsServer::Connection> connection, int status, const string& /*reason*/) {
        cout << "Server: Closed connection " << (size_t)connection.get() << " with status code " << status << endl;
    };
    
    //See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
    echo.onerror=[](shared_ptr<WsServer::Connection> connection, const boost::system::error_code& ec) {
        cout << "Server: Error in connection " << (size_t)connection.get() << ". " << 
                "Error: " << ec << ", error message: " << ec.message() << endl;
    };
    
    //Example 2: Echo thrice
    //  Send a received message three times back to the client
    //  Test with the following JavaScript:
    //    var ws=new WebSocket("ws://localhost:8080/echo_thrice");
    //    ws.onmessage=function(evt){console.log(evt.data);};
    //    ws.send("test");
    auto& echo_thrice=server.endpoint["^/echo_thrice/?$"];
    echo_thrice.onmessage=[&server](shared_ptr<WsServer::Connection> connection, shared_ptr<WsServer::Message> message) {
        auto message_str=message->string();
        
        auto send_stream1=make_shared<WsServer::SendStream>();
        *send_stream1 << message_str;
        //server.send is an asynchronous function
        server.send(connection, send_stream1, [&server, connection, message_str](const boost::system::error_code& ec) {
            if(!ec) {
                auto send_stream3=make_shared<WsServer::SendStream>();
                *send_stream3 << message_str;
                server.send(connection, send_stream3); //Sent after send_stream1 is sent, and most likely after send_stream2
            }
        });
        //Do not reuse send_stream1 here as it most likely is not sent yet
        auto send_stream2=make_shared<WsServer::SendStream>();
        *send_stream2 << message_str;
        server.send(connection, send_stream2); //Most likely queued, and sent after send_stream1
    };

    //Example 3: Echo to all WebSocket endpoints
    //  Sending received messages to all connected clients
    //  Test with the following JavaScript on more than one browser windows:
    //    var ws=new WebSocket("ws://localhost:8080/echo_all");
    //    ws.onmessage=function(evt){console.log(evt.data);};
    //    ws.send("test");
    auto& echo_all=server.endpoint["^/echo_all/?$"];
    echo_all.onmessage=[&server](shared_ptr<WsServer::Connection> /*connection*/, shared_ptr<WsServer::Message> message) {
        auto message_str=message->string();
        
        //echo_all.get_connections() can also be used to solely receive connections on this endpoint
        for(auto a_connection: server.get_connections()) {
            auto send_stream=make_shared<WsServer::SendStream>();
            *send_stream << message_str;
            
            //server.send is an asynchronous function
            server.send(a_connection, send_stream);
        }
    };
    
    
    auto& img=server.endpoint["^/img/?$"];
    
    img.onmessage=[&server](shared_ptr<WsServer::Connection> connection, shared_ptr<WsServer::Message> message) {
        //WsServer::Message::string() is a convenience function for:
        //stringstream data_ss;
        //data_ss << message->rdbuf();
        //auto message_str = data_ss.str();
        auto message_str=message->string();
        
        cout << "Server: Message received: \"" << message_str << "\" from " << (size_t)connection.get() << endl;
                
        cout << "Server: Sending image " << endl;
        
        auto send_stream=make_shared<WsServer::SendStream>();

  		//ifstream f1 ("Foto0337.jpg",fstream::binary);       
        //*send_stream << f1.rdbuf();



	glutInitWindowSize(window_width, window_height);

	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);

	glutCreateWindow("GLUT Example!!!");

	GL_Setup(window_width, window_height);
   
	//glutMainLoop();
	float angulo = atof(message_str.c_str());
	std::vector<unsigned char> rows;
	renderea(angulo,1,&rows);

    boost::iostreams::array_source my_vec_source(reinterpret_cast<char*>(&rows[0]), rows.size());
    boost::iostreams::stream<boost::iostreams::array_source> is(my_vec_source);
    
    	*send_stream << is.rdbuf();

cout << "ANGULO: " << angulo << " - " << message_str << endl;
        cout << "Server: SIZE: " << send_stream->size() << endl;
        
        //server.send is an asynchronous function
        server.send(connection, send_stream, [](const boost::system::error_code& ec){
            if(ec) {
                cout << "Server: Error sending message. " <<
                //See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
                        "Error: " << ec << ", error message: " << ec.message() << endl;
            }
        }, 130);
    };
    
    img.onopen=[](shared_ptr<WsServer::Connection> connection) {
        cout << "Server: Opened connection " << (size_t)connection.get() << endl;
    };
    
    //See RFC 6455 7.4.1. for status codes
    img.onclose=[](shared_ptr<WsServer::Connection> connection, int status, const string& /*reason*/) {
        cout << "Server: Closed connection " << (size_t)connection.get() << " with status code " << status << endl;
    };
    
    //See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
    img.onerror=[](shared_ptr<WsServer::Connection> connection, const boost::system::error_code& ec) {
        cout << "Server: Error in connection " << (size_t)connection.get() << ". " << 
                "Error: " << ec << ", error message: " << ec.message() << endl;
    };    
    
    thread server_thread([&server](){
        //Start WS-server
        server.start();
    });
    
    //Wait for server to start so that the client can connect
    this_thread::sleep_for(chrono::seconds(1));
    
    //Example 4: Client communication with server
    //Possible output:
    //Server: Opened connection 140184920260656
    //Client: Opened connection
    //Client: Sending message: "Hello"
    //Server: Message received: "Hello" from 140184920260656
    //Server: Sending message "Hello" to 140184920260656
    //Client: Message received: "Hello"
    //Client: Sending close connection
    //Server: Closed connection 140184920260656 with status code 1000
    //Client: Closed connection with status code 1000
    WsClient client("localhost:8080/echo");
    client.onmessage=[&client](shared_ptr<WsClient::Message> message) {
        auto message_str=message->string();
        
        cout << "Client: Message received: \"" << message_str << "\"" << endl;
        
        cout << "Client: Sending close connection" << endl;
        client.send_close(1000);
    };
    
    client.onopen=[&client]() {
        cout << "Client: Opened connection" << endl;
        
        string message="Hello";
        cout << "Client: Sending message: \"" << message << "\"" << endl;

        auto send_stream=make_shared<WsClient::SendStream>();
        *send_stream << message;
        client.send(send_stream);
    };
    
    client.onclose=[](int status, const string& /*reason*/) {
        cout << "Client: Closed connection with status code " << status << endl;
    };
    
    //See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
    client.onerror=[](const boost::system::error_code& ec) {
        cout << "Client: Error: " << ec << ", error message: " << ec.message() << endl;
    };
    
    client.start();
    
    server_thread.join();
    
    return 0;
}
