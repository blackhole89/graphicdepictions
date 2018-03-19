graphic depictions
==================

This program is a graphical workbench for creating and manipulating graphs and running JavaScript on them to automate a variety of calculations.

Possible applications include typesetting graphs for academic papers and presentations (it exports to [TikZ](https://en.wikipedia.org/wiki/PGF/TikZ)), looking for counterexamples to conjectures and testing graph algorithms.

You are looking at an early release, and a lot of features and polish are still missing. I am grateful for any bug reports, pull requests, feature suggestions and other feedback.

<img src="https://raw.githubusercontent.com/blackhole89/graphicdepictions/master/screenshots/sandpile.gif" />
<a href="https://raw.githubusercontent.com/blackhole89/graphicdepictions/master/screenshots/gd-erdoes-renyi.png"><img src="https://raw.githubusercontent.com/blackhole89/graphicdepictions/master/screenshots/gd-erdoes-renyi.png" width="300" /></a> <a href="https://raw.githubusercontent.com/blackhole89/graphicdepictions/master/screenshots/gd-discrete-cube.png"><img src="https://raw.githubusercontent.com/blackhole89/graphicdepictions/master/screenshots/gd-discrete-cube.png" width="300" /></a> <a href="https://raw.githubusercontent.com/blackhole89/graphicdepictions/master/screenshots/gd-0.4-features.png"><img src="https://raw.githubusercontent.com/blackhole89/graphicdepictions/master/screenshots/gd-0.4-features.png" width="300" /></a>
 

How to install
--------------

The easiest way to use this program is to pull one of the binary builds (version 0.4 of 2018-03-19):

* [Linux x86_64](http://twilightro.kafuka.org/%7Eblackhole89/files/gdepictions-0.4.tar.gz)
* [Windows x86_64](http://twilightro.kafuka.org/%7Eblackhole89/files/gdepictions-0.4-win64.zip)

Once you have unpacked the relevant package, switch into the directory contained and run the binary (`./gdepictions` in a Linux shell, or `gdepictions.exe` on Windows).

On Linux, you additionally require
* a version of libv8 (>= 3.14.5 or thereabouts, as found in the Debian and Ubuntu repositories as `libv8-3.14.5`)
* `zenity` for save/load dialogs.
The Windows build includes the required DLLs.

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
* `X` extrudes the currently selected vertices, that is, it duplicates the subgraph induced by them and connects each new node to the respective old node that gave rise to it. Pressing `X` automatically initiates `G`o mode on the new vertices.

### Moving the camera

The camera may be manipulated in a variety of ways.

* To pan (slide) the graph, hold and drag the middle mouse button or mouse wheel, or alternatively use the arrow keys.
* To zoom in and out, roll the mouse wheel up or down respectively.
* When operating in 3D mode (Layout > Unlock 3D), hold and drag the right mouse button to rotate the view in space.
* When a nonzero number of vertices is selected, press `M` to center the view on the selected vertices.

### Writing and running scripts

A key feature of graphic depictions is the ability to run JavaScript programs on the graph. To create a script, press the `Add` button in the **Scripts** window, and click on the `New script` entry that is created in the list. A script can either be applied to each selected node (default) or the whole graph (disable the `on nodes` option in the script editor). A few example scripts are included with the distribution, and if you are already familiar with JavaScript, these may be the fastest way to familiarise yourself with the environment. The important basics are the following:

* When running on nodes, the current node is made available locally as an object named `N`. This object can store arbitrary data, so for instance the script `N.v=3;` creates an attribute named `v` in every node that is selected when it is run and assigns the integer `3` to it. If the script `N.v = N.v * N.v;` is then run on a subset of those nodes, the nodes it will run on will have their attribute `v` set to `9`.
* Nodes also have a special boolean attribute `selected` which is true iff the node in question is currently selected.
* If `N` is a node object, then `gamma(N)` is the array of nodes in the node's neighbourhood, that is, all nodes connected to it, not including itself. This set can be iterated over as follows:
```javascript
gamma(N).forEach(function (M) {
    M.selected = true;
});
```
This script adds all nodes that are adjacent to `N` to the current selection.
* If `N` is a node object, then `delta(N)` is an array of edges on the node's boundary, that is, all edges incident to it. This set can be iterated over as follows:
```javascript
delta(N).forEach(function (E) {
    // Get other end.
    // + forces comparison of underlying nodes rather than references.
    var M;
    if(E.n1 ==+ N)
        M = E.n2;
    else
        M = E.n1;

    // label edges incident to N with difference in v going out of N 
    E.label = M.v - N.v;
});
```
* The set of all nodes is always made available as an array in global scope as `nodes()`. So the action of a script "on nodes" can be emulated with a script "on graphs" as follows:
```javascript
nodes().forEach(function (N) {
    if(N.selected) {
        // script here
    }
});
```
Likewise, the set of all edges is made available as `edges()`.
* New nodes and edges can be added by scripts using `addNode` and `addEdge`. When adding nodes, the position in 2D or 3D space needs to be specified. For instance, the following script creates a new dangling vertex that is connected to each vertex in the selection next to the respective vertex:
```javascript
M = addNode(N.pos.x+0.1, N.pos.y); // or addNode(N.pos.x+0.1, N.pos.y, N.pos.z)
E = addEdge(M,N);
```
* `getEdge(M,N)` returns a handle to the edge between `N` and `M` if there is one, and `undefined` otherwise.
* Existing nodes and edges can be deleted with `delNode` and `delEdge` respectively. All references to a deleted node or edge in global scope are rendered undefined by this. However, it is the user's responsibility to ensure that local references and those stored in closures are disposed of properly.
```javascript
addNode(0,0);
addNode(0.1,0.1);
E = addEdge(nodes()[0], nodes()[1]);
var Elocal = E;
delEdge(nodes()[0], nodes()[1]); // Also ok: delEdge(E);
// E is now undefined
// Elocal is now toxic sludge and should not be read
delNode(nodes()[0]);
```
* `print` and `println` can be used to print any single value to the console (opened with `Tab`), potentially creating a new line.
* The display colour of a node `N` can be adjusted by calling `N.setColo(u)r`. By default, nodes are drawn white.
```javascript
N = addNode(0,0);   // add node at origin
N.setColor(1.0,0.0,0.0);    // make it red
```
* In addition, all builtins of the ECMAScript 5 standard, e.g. `Math.sin`, are provided via V8.

By default, scripts are saved with the graph you are editing. (File > Save) You can instead make a script global by right-clicking it in the script list and unchecking `Stored with graph`. You can also quickly enter scripts that will not be saved by
* opening the **JavaScript console** with `Tab` (quit with `Esc`) or 
* right-clicking the graph view and selecting `Execute on selection...`.

The **Data** subwindow allows you to inspect all global JavaScript variables and objects. Create them by assigning bare variables in scripts, e.g. `a=5;` (as opposed to `var a=5;`, or in the console.

The **Node Appearance** subwindow contains a 3x3 matrix of input boxes which can be used to display the current values of vertex attributes next to the vertex in the graph view. For instance, if the previously-mentioned script `N.v=3;` was run on all nodes and `v` is entered into the second input box in the top line of the window, then a `3` will be rendered above every node. By right-clicking a field in the Node Appearance window, you can also adjust its display colour.

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

Known issues
------------

* Drawing lots of node labels can be slow on some platforms. Generally, the "separate texture for each glyph" approach my font implementation (`fonts.cpp`) uses seems to be much slower than imgui's font atlas + texture coordinates one; I will eventually switch over to it.
* In the Windows binary build, TikZ export fails to correctly compute node label colours. This is a complete and utter mystery to me at the moment.

Changelog
---------

### 0.4

* Nodes and edges can now store arbitrary JavaScript data.
* Likewise, all global variables created by JS get stored with the graph.
* Global variables can now be inspected in the user interface.
* Adding a javascript console, accessible with `Tab`.
* Adding builtin functions `print` and `println`.
* Bump file format version.

### 0.3

* Added support for manipulating the colour of nodes (N.setColour(float,float,float)) from scripts.

### 0.2.1

* Made X11 keyboard support more portable.

### 0.2

* Added basic support for edge data. At the moment, the "edge look" is hardcoded to display the field named "label" only.
* Switched the node label font to a more legible one, and replaced shadows with outlines.
* Fixed "Execute on selected..." doing nothing.
* Added rudimentary editor for JS node properties in context menu.
* Bump file format version.

### 0.1

* Initial release.
