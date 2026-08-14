# Week 3

Week 3 starts with the version ID `week3-v1`. The build, tests, and benchmark
now use the files in the `week3` folder.

On macOS, install Readline to use the arrow keys for previous commands:

```sh
brew install readline
```

The program goes straight to the query prompt after a file uploads. After a
result prints, type the next query without entering `s` again. Type `q` or
`quit` to exit.

Build and run from the project root:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/streamline
```

Run `./build/benchmark` to see the five benchmark runs and median time in a
table.
