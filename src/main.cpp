#include <GLUT/glut.h>
#include <boost/program_options.hpp>
#include <memory>
#include <iostream>
#include <string>
#include "ServerSocket.h"

namespace po = boost::program_options;

std::auto_ptr<streamsocket::ServerSocket> serverSocket;


void display(void)
{ 
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(1.0,1.0,1.0);
    glLoadIdentity();

    glutSwapBuffers();
}

int main(int argc, char* argv[])
{
    int width;
    int height;
    std::string host;
    std::string port;
    glutInit(&argc, argv);
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
        ("width", po::value<int>(&width)->default_value(1024), "screen width")
        ("height", po::value<int>(&height)->default_value(768), "screen height")
        ("host", po::value<std::string>(&host)->default_value("localhost"), "host to listen for streams on")
        ("port", po::value<std::string>(&port)->default_value("10000"), "port to listen for streams on")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
        std::cout << desc << "\n";
        return 1;
    }

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(width, height);
    glutInitWindowPosition (100, 100);
    glutCreateWindow ("StreamPlot");

    glutDisplayFunc(display);

    serverSocket.reset(new streamsocket::ServerSocket(host, port));

    glutMainLoop();

    return 0;
}

