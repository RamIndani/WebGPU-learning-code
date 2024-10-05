LearnWebGPU   
Step: just playing
===========

Render a Rectangle with Index Buffer and Color Buffer, along with a uniform to animate the scean.

Uniform mapping   
![demo](screenshot/demo.mov)
![demo1](screenshot/demo1.mp4)

![demo2](screenshot/demo2.mp4)

![demo3](screenshot/demo3.mp4)

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
