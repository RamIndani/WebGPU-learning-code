LearnWebGPU   
Step: 11
===========

Render a triangle and in the background copy data from CPU to GPU, GPU to GPU and back to CPU

This is a repository of me learning webgpu using  [Learn WebGPU](https://eliemichel.github.io/LearnWebGPU) web book.

You will need webgpu, glfw and glfw3webgpu from the root directory, then correct the path in the CMakeList to point to the correct subDirectory for these dependencies.

Building
--------

Build the downloaded webgpu dependency first, from the webgpu directory execute below command.
```
cmake webgpu
```

```
cmake . -B build
cmake --build build 
```

Run on Windows  `build\Debug\App.exe`   
Run on (linux/macOS/WinGW) `./build/App`


Output on the terminal: 
```
Buffer reading status 0
bufferData = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15]
```