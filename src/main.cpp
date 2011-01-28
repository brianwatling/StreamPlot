#ifdef DARWIN
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <boost/program_options.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>
#include <list>
#include <vector>
#include <memory>
#include <iostream>
#include <sstream>
#include <iomanip>
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
    DataPoint(GLdouble x, GLdouble y)
    : x(x), y(x)
    {}

    DataPoint()
    : x(0), y(0)
    {}

    GLdouble x;
    GLdouble y;
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

const GLdouble colours[][3] = {
                                {1, 0, 0},
                                {0, 1, 0},
                                {0, 0, 1},
                                {1, 1, 0},
                                {1, 0, 1},
                           };

const size_t numColours = sizeof(colours) / (3 * sizeof(GLdouble));

enum Mode
{
    Points,
    Lines,
    Impulses,
};

Mode mode = Points;

void renderBitmapString(float x, float y, void* font, const std::string& str)
{  
    glRasterPos3f(x, y, 0);
    BOOST_FOREACH(char c, str)
    {
        glutBitmapCharacter(font, c);
    }
}

std::string doubleToString(double d)
{
    std::ostringstream o;
    o << std::setprecision(13) << d;
    return o.str();
}

void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    const GLdouble width = topRight.x - bottomLeft.x;
    const GLdouble height = topRight.y - bottomLeft.y;

    gluOrtho2D(bottomLeft.x - width * 0.1, topRight.x + width * 0.05, bottomLeft.y - height * 0.1, topRight.y + height * 0.05);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glColor3f(1, 1, 1);
    glBegin(GL_LINES);
        //y axis
        glVertex2d(bottomLeft.x, bottomLeft.y);
        glVertex2d(bottomLeft.x, topRight.y);

        //x axis
        glVertex2d(bottomLeft.x, bottomLeft.y);
        glVertex2d(topRight.x, bottomLeft.y);

        //ticks
        const GLdouble yX = bottomLeft.x - width * 0.02;
        const GLdouble xY = bottomLeft.y - height * 0.02;
        for(GLdouble cur = 0; cur < 1; cur += 0.1)
        {
            //y
            const GLdouble yY = bottomLeft.y + cur * height;
            glVertex2d(bottomLeft.x, yY);
            glVertex2d(yX, yY);

            //x
            const GLdouble xX = bottomLeft.x + cur * width;
            glVertex2d(xX, bottomLeft.y);
            glVertex2d(xX, xY);
        }
    glEnd();

    const double yAxisTextStart = bottomLeft.x - width * 0.09;
    const double xAxisTextStart = bottomLeft.y - height * 0.09;
    for(GLdouble cur = 0; cur <= 10; cur += 1)
    {
        const GLdouble yOffset = cur * height * 0.1;
        renderBitmapString(yAxisTextStart, bottomLeft.y + yOffset, GLUT_BITMAP_8_BY_13, doubleToString(yOffset + bottomLeft.y));

        const GLdouble xOffset = cur * width * 0.1;
        renderBitmapString(bottomLeft.x + xOffset, xAxisTextStart, GLUT_BITMAP_8_BY_13, doubleToString(xOffset + bottomLeft.x));
    }

    size_t colourIndex = 0;
    BOOST_FOREACH(const StreamInfoPtr& cur, streams)
    {
        if(!cur->data.empty())
        {
            glColor3f(colours[colourIndex % numColours][0], colours[colourIndex % numColours][1], colours[colourIndex % numColours][2]);
            switch(mode)
            {
            case Impulses:
                glBegin(GL_LINES);
            case Lines:
                glBegin(GL_LINE_STRIP);
            case Points:
            default:
                glBegin(GL_POINTS);
            };
            BOOST_FOREACH(const DataPoint& data, cur->data)
            {
                if(mode == Impulses)
                    glVertex2d(data.x, bottomLeft.y);
                glVertex2d(data.x, data.y);
            }
            glEnd();
            ++colourIndex;
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
        try
        {
            while(cur->clientSocket.readable() && cur->stream >> newData.x >> newData.y)
            {
                cur->data.push_back(newData);
                //std::cout << "new data " << cur->data.size() << std::endl;

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

                doSleep = false;
            }
        }
        catch(std::exception& e)
        {
            std::cerr << "exception reading stream: " << e.what() << std::endl;
            cur->clientSocket.close();
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
    std::string dispMode;
    glutInit(&argc, argv);
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
        ("width", po::value<int>(&width)->default_value(800), "screen width")
        ("height", po::value<int>(&height)->default_value(600), "screen height")
        ("host", po::value<std::string>(&host)->default_value("localhost"), "host to listen for streams on")
        ("port", po::value<std::string>(&port)->default_value("10000"), "port to listen for streams on")
        ("mode", po::value<std::string>(&dispMode)->default_value("points"), "pick points or lines")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
        std::cout << desc << "\n";
        return 1;
    }

    if(dispMode == "lines")
        mode = Lines;
    if(dispMode == "impulses")
        mode = Impulses;

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

