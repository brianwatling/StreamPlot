#include <GLUT/glut.h>
#include <boost/program_options.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>
#include <list>
#include <vector>
#include <memory>
#include <iostream>
#include <string>
#include "ServerSocket.h"

#ifdef _WIN32
static void mssleep(int ms)
{
    Sleep(ms);
}
#else
static void mssleep(int ms)
{
    usleep(ms * 1000);
}
#endif

namespace po = boost::program_options;

struct DataPoint
{
    DataPoint(GLfloat x, GLfloat y)
    : x(x), y(x)
    {}

    DataPoint()
    : x(0), y(0)
    {}

    GLfloat x;
    GLfloat y;
};

struct StreamInfo
{
    StreamInfo(streamsocket::SOCKET sockFd)
    : clientSocket(sockFd), stream(clientSocket.getStreamBuf())
    {}

    streamsocket::ClientSocket clientSocket;
    std::iostream stream;
    std::vector<DataPoint> data;
};

typedef boost::shared_ptr<StreamInfo> StreamInfoPtr;

std::auto_ptr<streamsocket::ServerSocket> serverSocket;
std::vector<StreamInfoPtr> streams;
DataPoint bottomLeft(0, 0);
bool gotFirst = 0;
DataPoint topRight(0,0);

void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(bottomLeft.x, topRight.x, bottomLeft.y, topRight.y);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glColor3f(1.0,1.0,1.0);

    /*glBegin(GL_LINES);
        glVertex3f(-1,-1,0.0);
        glVertex3f(1,1,0.0);
    glEnd();*/

    BOOST_FOREACH(const StreamInfoPtr& cur, streams)
    {
        if(!cur->data.empty())
        {
            glBegin(GL_POINTS);
            BOOST_FOREACH(const DataPoint& data, cur->data)
            {
                glVertex2f(data.x, data.y);
            }
            glEnd();
        }
    }

    glFlush();
    glutSwapBuffers();
}


void update(void)
{
    bool doSleep = true;
    if(serverSocket->ready())
    {
        try
        {
            StreamInfoPtr newStream(new StreamInfo(serverSocket->accept()));
            streams.push_back(newStream);
            std::cout << "added new stream" << std::endl;
        }
        catch(std::exception& e)
        {
            std::cerr << "exception accepting new stream: " << e.what() << std::endl;
        }
        doSleep = false;
    }

    DataPoint newData;
    BOOST_FOREACH(StreamInfoPtr& cur, streams)
    {
        while(cur->clientSocket.readable() && cur->stream >> newData.x >> newData.y)
        {
            try
            {
                cur->data.push_back(newData);
                std::cout << "new data " << cur->data.size() << std::endl;

                if(!gotFirst)
                {
                    topRight = newData;
                    bottomLeft = newData;
                    gotFirst = true;
                }
                else
                {
                    if(newData.x < bottomLeft.x)
                        bottomLeft.x = newData.x;
                    if(newData.y < bottomLeft.y)
                        bottomLeft.y = newData.y;
                    if(newData.x > topRight.x)
                        topRight.x = newData.x;
                    if(newData.y > topRight.y)
                        topRight.y = newData.y;
                }
            }
            catch(std::exception& e)
            {
                std::cerr << "exception reading stream: " << e.what() << std::endl;
                cur->clientSocket.close();
            }
            doSleep = false;
        }
    }

    if(doSleep)
    {
        mssleep(100);
    }

    glutPostRedisplay();
}

void changeSize(int w, int h)
{
    if(h == 0)
        h = 1;
    glViewport(0, 0, w, h);
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
        ("width", po::value<int>(&width)->default_value(800), "screen width")
        ("height", po::value<int>(&height)->default_value(600), "screen height")
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

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(width, height);
    glutInitWindowPosition (-1, -1);
    glutCreateWindow ("StreamPlot");

    glutDisplayFunc(&display);
    glutIdleFunc(&update);
    glutReshapeFunc(&changeSize);

    serverSocket.reset(new streamsocket::ServerSocket(host, port));

    glutMainLoop();

    return 0;
}

