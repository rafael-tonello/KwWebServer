# About this library (resume)


# How to use

## instanciating

## handling requests

## handling websockets

# Others resources

## auto sending files

# Helpers

## WebServerObserver

## KWWebServerRouter

# HttpCodes

# workers



##

# dependecies

This project depends of another two project: TCPServer and ThreadPool. You will need to inform the root folders of these projects to the compiler.

In the GCC, you just need to use the command line option '-I':
    g++ -o myprogram -I"path/to/TCPServer" -I"path/to/ThreadPool"

# Task lists

## main tasklist

    [ ] create the ContentStream Internface and basic classes to read content
        [ ] Analyse if the cpp native streams can be used instead

    [ ] Rename all setContent* methods to just to 'setContet' and uses function overload.
    [ ] Separate url vars and resource.
