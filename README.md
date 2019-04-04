# Welcome to EL!

EL is an educational language runtime and programming language! We’re using it to show people how to use [Eclipse OMR] to build their own language runtime with a Just in Time (JIT) Compiler!

[Eclipse OMR]: https://github.com/eclipse/omr

## Getting started

### 1. Requirements

To get started with EL you will need the following:

* `git`
* `build-essential`
* `cmake` **(Minimum version 3.2.0)**

### 2. Clone the repository and get the submodules

```sh
git clone --recursive https://github.com/charliegracie/EL.git
```

### 3. Build EL

```sh
cd EL \
mkdir build \
&& cd build \
&& cmake .. \
&& make
```
