# dependecies

This project depends of another two project: TCPServer and ThreadPool. You will need to inform the root folders of these projects to the compiler.

In the GCC, you just need to use the command line option '-I':
    g++ -o myprogram -I"path/to/TCPServer" -I"path/to/ThreadPool"