# ND Mesh Type Comparison

Godot ND provides 3 main mesh types for representing visible ND geometry:

- **Poly Meshes**: Composed of a hierarchy of geometric structures assembling together to form higher-dimensional structures. Edges are made of vertices, faces are made of edges, and cells are made of faces.
- **Cell Meshes**: Composed entirely of simplex cells (ND analog of triangles) referencing vertices.
- **Wire Meshes**: Composed entirely of edges referencing vertices.

Here is table comparing their features (referring to the `Array`\* implementations of each mesh type):

| Feature                   | Poly Meshes | Cell Meshes | Wire Meshes |
| ------------------------- | ----------- | ------------ | ----------- |
| Fast & Efficient          | ❌          | ✅           | ✅          |
| Good for Authoring        | ✅          | ❌           | ✅          |
| Explicit Edges            | ✅          | ❌           | ✅          |
| Explicit Simplex Cells    | ⚠️          | ✅           | ❌          |
| Simplex Cell Renderable   | ✅          | ✅           | ❌          |
| Convertible to Poly Mesh  | 🟢          | ⚠️           | ❌          |
| Convertible to Cell Mesh  | ✅          | 🟢           | ❌          |
| Convertible to Wire Mesh  | ✅          | ⚠️           | 🟢          |

Legend: ✅ = yes, ❌ = no, ⚠️ = partial or lossy, 🟢 = same format.

Poly meshes are the ideal format for authoring complex ND geometry, as they can represent detailed hierarchical geometry, and be converted to other formats as needed. Wire meshes can also be good for authoring geometry, if the desired end result geometry is wireframe, without faces or cells. Cell meshes are poorly suited for authoring, since working with individual simplex cells can be tedious and unintuitive for complex shapes.

Poly meshes can be converted to any other format losslessly. Cell meshes can be converted to poly meshes if cells sharing a start vertex function as a pivot that combines multiple cells into a polytope cell, but the conversion may not be perfect. Cell meshes can also be converted to wire meshes, but extra edges may be created within a face or cell. Wire meshes are not convertible to other formats, as they lack the necessary information to reconstruct faces or cells.

Poly meshes are simplex cell renderable, meaning they can be rendered as cross-sections just like cell meshes. In fact, PolyMeshND inherits CellMeshND, allowing for PolyMeshND to be used as-is by functions that accept a CellMeshND or MeshND. Poly meshes can represent simplex cells directly, but they are usually used for more complex shapes, therefore the intended use case of poly meshes does not involve explicit simplex cells. Cell meshes directly represent simplex cells. Wire meshes cannot be converted to simplex cells, but they can still be used in the cross-section renderer via its support for rendering wireframes.
