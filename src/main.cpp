#include <GLUT/glut.h>
#include <boost/program_options.hpp>
#include <iostream>

namespace po = boost::program_options;

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
    glutInit(&argc, argv);
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
        ("width", po::value<int>(&width)->default_value(1024), "screen width")
        ("height", po::value<int>(&height)->default_value(768), "screen height")
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

    glutMainLoop();

    return 0;
}

