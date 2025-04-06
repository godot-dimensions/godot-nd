# Godot ND (N-dimensional)

NodeND and other N-dimensional types for making 5D, 6D, 7D, etc games in Godot.

![N-dimensional Editor Viewport](.github/nd_editor_viewport.png)

## Math

- `GeometryND`: Singleton for working with ND geometry.
- `PlaneND`: Class for working with ND planes.
- `TransformND`: Class for working with ND transformations.
- `VectorND`: Singleton with math functions for VectorN (PackedFloat64Array).

## Folder Structure

- `editor/`: All editor-related classes including the ND viewport main screen tab.
- `math/`: All math-related classes including linear algebra.
- `mesh/`: All mesh-related classes including nodes and resources.
- `nodes/`: Any nodes that do not fit into other categories: NodeND and CameraND.
- `render/`: All rendering-related classes including server and engines.
- `addons/nd/`: Contains documentation, icons, and files for GDExtension.

## License

This repo is free and open source software licensed under The Unlicense.
Credit and attribution is appreciated but not required.

Some parts of some files in this repository are derivative from Godot Engine
and therefore [its MIT license](https://godotengine.org/license) applies.
You must provide credit to Godot Engine by including its LICENSE.
Considering this repo is only usable in conjunction with Godot anyway,
this will not be a problem because you should already be crediting Godot.
