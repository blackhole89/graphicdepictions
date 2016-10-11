graphic depictions
==================

This program is a graphical workbench for creating and manipulating graphs and running JavaScript on them to automate a variety of calculations.

Possible applications include typesetting graphs for academic papers and presentations, looking for counterexamples to conjectures and testing graph algorithms.

You are looking at an early release, and a lot of features and polish are still missing. I am grateful for any bug reports, pull requests, feature suggestions and other feedback.

<a href="https://raw.githubusercontent.com/blackhole89/graphicdepictions/master/screenshots/gd-erdoes-renyi.png"><img src="https://raw.githubusercontent.com/blackhole89/graphicdepictions/master/screenshots/gd-erdoes-renyi.png" width="300" /></a> <a href="https://raw.githubusercontent.com/blackhole89/graphicdepictions/master/screenshots/gd-discrete-cube.png"><img src="https://raw.githubusercontent.com/blackhole89/graphicdepictions/master/screenshots/gd-discrete-cube.png" width="300" /></a> 
 

How to install
--------------

The easiest way to use this program is to pull one of the binary builds:

* [Linux x86_64](http://twilightro.kafuka.org/%7Eblackhole89/files/gdepictions-0.1.tar.gz)
* [Windows x86_64](http://twilightro.kafuka.org/%7Eblackhole89/files/gdepictions-0.1-win64.zip)

On Linux, you additionally require a version of libv8 (>= 3.14.5 or thereabouts, as found in the Debian and Ubuntu repositories as `libv8-3.14.5`). The Windows build includes the required DLLs.

You may also try to compile it yourself, which may involve some effort. In fact, natively creating a Windows binary of the old version of V8 that graphic depictions is based on turned out to be so painful that I gave up and cross-compiled everything using mingw-w64. If you are not discouraged, consult the "How to compile from source" section further below.

How to use
----------

The program uses a [Blender](https://www.blender.org/)-inspired modal keyboard-and-mouse interface. Most actions are performed by pressing some key or key combination to _initiate_ an action at the mouse position. When more input is necessary (e.g. a second endpoint for a new edge, or the other corner of a box selection), the action can be completed by moving the mouse and either committing the results by clicking or cancelling by pressing Escape. The state in which no complex action is awaiting more input shall be referred to as _neutral mode_.

### Editing the graph

The following list of examples doubles as an outline of basic workflow:

* `V` to create a selected vertex at the current cursor position.
* `C` to connect all selected vertices with edges.
* `E` to initiate an edge at the vertex closest to the mouse cursor. A dangling edge will be drawn from the vertex to the mouse cursor to indicate that an edge is currently being added. Once another vertex is clicked, the interface returns to neutral mode and an edge is added between the two vertices. Alternatively, pressing `E` again adds the edge and initiates a new edge at the target vertex.
* `B` initiates a selection box at the current location of the mouse pointer. When the left mouse button is clicked, all vertices in a square region between where `B` was pressed and the mouse was clicked are selected.
* `A` unselects all vertices when vertices are selected, and selects all vertices when none are selected.
* A left mouse button click in neutral mode selects the vertex closest to the mouse cursor. If `Shift` is held, the vertex is instead added to the current selection.
* A right mouse button click in neutral mode opens the context menu.
* `D` deletes all edges between selected vertices.
* `G` initiates moving of the currently selected vertices by moving the mouse pointer. Click the graph view with the left mouse button to place the vertices at their current location, or press `Esc` to cancel.

### Moving the camera

The camera may be manipulated in a variety of ways.

* To pan (slide) the graph, hold and drag the middle mouse button or mouse wheel.
* To zoom in and out, roll the mouse wheel up or down respectively.
* When operating in 3D mode (Layout > Unlock 3D), hold and drag the right mouse button to rotate the view in space.
* When a nonzero number of vertices is selected, press `M` to center the view on the selected vertices.

### Writing and running scripts

A key feature of graphic depictions is the ability to run JavaScript programs on the graph. To create a script, press the `Add` button in the `Scripts` window, and click on the `New script` entry that is created in the list. A script can either be applied to each selected node (default) or the whole graph (disable the `on nodes` option in the script editor). A few example scripts are included with the distribution, and if you are already familiar with JavaScript, these may be the fastest way to familiarise yourself with the environment. The important basics are the following:

* When running on nodes, the current node is made available in the global scope as a special object named `N`. This object can store arbitrary integer and floating-point attributes, so for instance the script `N.v=3;` creates an attribute named `v` in every node that is selected when it is run and assigns the integer `3` to it. If the script `N.v = N.v * N.v;` is then run on a subset of those nodes, the nodes it will run on will have their attribute `v` set to `9`.
* Nodes also have a special boolean attribute `selected` which is true iff the node in question is currently selected.
* If `N` is a node object, then `gamma(N)` is an object that denotes the node's neighbourhood, that is, the set of all nodes connected to it, not including itself. This set can be iterated over as follows:
```javascript
gamma(N).forEach(function (M) {
    M.selected = true;
});
```
This script adds all nodes that are adjacent to `N` to the current selection.
* The set of all nodes is always made available in global scope as `nodes()`. So the action of a script "on nodes" can be emulated with a script "on graphs" as follows:
```javascript
nodes().forEach(function (N) {
    if(N.selected) {
        // script here
    }
});
```
* New nodes and edges can be added by scripts using `addNode` and `addEdge`. When adding nodes, the position in 2D or 3D space needs to be specified. For instance, the following script creates a new dangling vertex that is connected to each vertex in the selection next to the respective vertex:
```javascript
M = addNode(N.pos.x+0.1, N.pos.y); // or addNode(N.pos.x+0.1, N.pos.y, N.pos.z)
addEdge(M,N);
```
By default, scripts are saved with the graph you are editing. (File > Save) You can instead make a script global by right-clicking it in the script list and unchecking `Stored with graph`. You can also quickly enter scripts that will not be saved by right-clicking the graph view and selecting `Execute on selection...`.

The subwindow `Node Apperance` contains a 3x3 matrix of input boxes which can be used to display the current values of vertex attributes next to the vertex in the graph view. For instance, if the previously-mentioned script `N.v=3;` was run on all nodes and `v` is entered into the second input box in the top line of the window, then a `3` will be rendered above every node.

Licensing
---------

graphic depictions is made available under the terms of the GNU General Public License, Version 3. Parts of the program are also covered by different licenses; see the LICENSE file for details.

How to compile from source
--------------------------

On Linux, you will require the development headers for:

* V8 (`libv8-dev` >=3.14.5 or thereabouts, 4.x onwards will probably not work)
* freetype2 (`libfreetype6-dev`)
* OpenGL (`libgl1-mesa-dev` and `libglu1-mesa-dev`)

If you have these and a sufficiently recent version of G++ installed, simply running the included Makefile should work.

For Windows builds, I strongly recommend compiling with MinGW-w64 (g++-mingw-w64-x86-64 and its dependencies on Debianesque systems). If you are happy with reusing the included binary DLLs, this may be as simple as running `make -f Makefile.win64` in the program root. You can procure binaries and build scripts for the fairly standard dependencies such as zlib and freetype from Fedora's repositories, but compiling libv8.dll is a bit of an adventure and would probably require more than the length of this document to explain. I am grateful that its license frees me from the obligation to do so, but may expand upon this section later.

I expect the only significant obstacle to compiling from MSVC++ to be V8, too; while the 3.x libv8 sources include some provisions for a Microsoft build environment, they seem to be based on several assumptions about standard compliancy and state of the platform headers that no longer hold true for more recent versions.

