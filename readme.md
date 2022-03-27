# ZAP
(ZAP Array programming)

This is what I cam calling an array based programming language. The idea is that all data in the program is contained in arrays. This allows for performing some operation much easier. It is inspired by th [APL](https://en.wikipedia.org/wiki/APL_(programming_language)) programming language and uses mostly symbols for operations. It is currently working but does not implement all features yet.

to run a program download everything and then navigate to it. you need to have gcc installed and then can run make to get to a repl.

    make

if you want to complile without going into the repl type 

    make release

After doing either you will have a release executable in the ZAP2/builds directory you can then run the examples by passing in the path to them as a parameter

    ZAP2/builds/release.exe examples/stringSearch.zap