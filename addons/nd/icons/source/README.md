# ND Icon Source Files

Conical gradients are needed for the rainbow effect. Modern SVG supports this, but older SVG does not. Most apps can't read conical gradients, including both Godot and GIMP. As a work-around, here is my procedure for making these icons:

1. Make the conical gradient in Inkscape, and export it as `GradientND.png` (already done, doesn't need to be repeated).
2. Optimize the PNG using oxipng (already done, doesn't need to be repeated).
3. Have the SVG paths ready, usually copied from Godot and edited in GodSVG.
4. Convert the PNG to base64: `openssl base64 -in GradientND.png | tr -d '\n' | pbcopy` (this copies to the clipboard on macOS).
5. Edit the SVG with VS Code and insert an SVG pattern with the image data: `<pattern id="a" height="100%" patternContentUnits="objectBoundingBox" width="100%"><image height="1" width="1" xlink:href="data:image/png;base64,"/></pattern>` (insert the image data after `base64,`).
6. Reference the pattern in SVG paths: `fill="url(#a)"`
7. This patterned SVG can be read by GIMP, but Godot doesn't support SVG patterns. So open it in GIMP and export a PNG.
8. Increase the saturation. I decided late in the process that a saturated icon looks better.
9. Run oxipng on that PNG (optional).
10. Convert the PNGs to base64: `openssl base64 -in MeshND.png | tr -d '\n' | pbcopy` (this copies to the clipboard on macOS).
11. Edit the SVG with VS Code to have only a PNG image embedded in the SVG: `<svg height="16" width="16" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink"><image height="16" width="16" xlink:href="data:image/png;base64,"/></svg>` (insert the image data after `base64,`).
12. Open the Godot editor to let Godot import the finished SVG with a scale factor of 8.0.
13. Thankfully, Godot supports plain images in SVG files. So you're done!
