NeuroTessMesh
=====================================================

## Introduction
NeuroTessMesh provides a visual environment for the generation of 3D polygonal
meshes that approximate the membrane of neuronal cells, from the morphological
tracings that describe the morphology of the neurons. The 3D models can be
tessellated at different levels of detail, providing either homogeneous or
adaptive resolution along the model. The soma shape is recovered from the
incomplete information of the tracings, applying a physical deformation model
that can be interactively adjusted.

See [NeuroTessMesh web page](http://gmrv.es/neurotessmesh/) and
[NeuroTessMesh  manual](http://gmrv.es/neurotessmesh/NeuroTessMeshUserManual.pdf)
for a complete description and sample data tests.

## Dependencies

* Required dependencies:
    * neurolots

* Optional dependencies:
    * Qt5.4: enables building NeuroTessMesh viewer (*)
    * ZeroEQ: enables syncing selections and syncing camera
    * Lexis: provides the base vocabulary for ZeroEQ for sync operations
    * gmrvLex: enables sending focus messages

Note: nsol library is automatically downloaded and built if not found by cmake.

Note: Qt version 5.4 or above is mandatory to build NeuroTessMesh.

## Building

NeuroTessMesh has been successfully built and used on Ubuntu 16.04 LTS, Mac OSX
High Sierra and Windows 10 64-bits with Visual Studio 2015. Please ensure that
you build the Release version in order to get the best performance possible.

```bash
git clone https://gitlab.gmrv.es/nsviz/NeuroTessMesh.git
mkdir neurotessmesh/build && cd neurotessmesh/build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

## Running

If building process was OK, you can try NeuroTessMesh with the command:

```bash
./bin/NeuroTessMesh -swc nsol/tests/ExampleNeuron.swc
```

All the features can be accessed using the NeuroTessMesh GUI but some of them
are also available through CLI. Running the following can provide a list of
the CLI arguments NeuroTessMesh accepts:

```bash
./bin/NeuroTessMesh --help
```

In order to check which optional dependencies have been used, the following
command can be used:

```bash
./bin/NeuroTessMesh --version
```
