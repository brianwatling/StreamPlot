# A program to plot a stream

- Plots live data from a socket as it is generated
- Can plot points, lines, or impulses
- Potentially useful for analyzing log files on the fly or monitoring an application 

## Command line options

Allowed options:

    --help                  produce help message
    --width arg (=800)      screen width
    --height arg (=600)     screen height
    --host arg (=localhost) host to listen for streams on
    --port arg (=10000)     port to listen for streams on
    --mode arg (=points)    pick points or lines or impulses

## Streams

- You can make StreamPlot plot data by connecting to the specified port (10000 by default) and sending in x-y tuples
- The plot will scale automatically based on the input ranges

## Example

- This example should run on linux, some Makefile adjustments may be needed to work on Mac or Windows

Terminal 1:

    make
    ./streamplot --mode=impulses

Terminal 2:

    cur=1; while [ "$cur" -lt "100" ]; do echo "$cur $cur"; cur=`expr $cur + 1`; sleep 1; done | telnet localhost 10000

After starting the while loop in the second terminal, watch as the plot is updated dynamically

## TODO

- 3D plotting?
- multiple plots from the same stream? (ie. in [x, y1, y2, y3, ...] format)
- labels?
- better/more colours?
- support a maximum number of samples (possibly implemented by a circular buffer)
- vertex arrays or vertex buffers if performance becomes a problem
- let me know what you need!

## License

Public domain. Credit is appreciated and I would like to hear about how you use it.
