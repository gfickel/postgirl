# Postgirl
Postgirl is supposed to be a bloat-free simple, small and somewhat limited program to do HTTP requests. The idea is to be a simple yet reliable tool to do some quick tests, but being very good at it.

This idea came from my insatisfaction with Postman lately. The native app sometimes takes several seconds to boot, sometimes crashes and if you let it open for a long time it may consume an insane amount of RAM (it topped at around 1GB on my Desktop once). Then it came the idea of a simple alternative called Postgirl.


## Main Development Ideas
This is definitely a toy project of mine. I do indeed pretend to use it on a daily basis, therefore it must be somewhat functional and easy to use however it is being a playground for tests. Those are some guidelines that I'm trying to follow:

* Minimal external dependencies. Having to install libcurl appart is really bugging me...
* Minimal STL usage: STL is great but it also comes with some costs that are usually ignored such as (sometimes greatly) increased compilation time and horrible stack traces to follow on debug. For example, I added both thread and vector and the main.cpp compilation went from 600ms to 1000ms :( . Looking to remove those on the near future.
* NEVER loose GUI responsiveness
* Cross plataform code: everything here should work on Mac, Windows and Linux.
* Simple code without excessive abstractions


## Used libs
I'm using the excelent [Dear ImGui](https://github.com/ocornut/imgui) for GUI, created using the [Immediate Mode Gui](https://www.youtube.com/watch?v=Z1qyvQsjK5Y) architecture. It is a very easy intuitive way to create GUIs, so I suggest that you take a look if you don't know about it.

For the HTTP requests I'm using [libcurl](https://curl.haxx.se/libcurl/), but it was not so easy to incorporate it's code in here so yourself should install it on your system (check Dependencies).


## Dependencies
For Linux (Ubuntu):
```sh
apt-get install libglfw3 libglfw3-dev libcurl4-openssl-dev
```

All of those dependencies are available both on Windows and Mac but I don't have the time to check how to install them, unfortunately :(.

