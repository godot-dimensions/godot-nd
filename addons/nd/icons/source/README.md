# ND Icon Source Files

Conical gradients are needed for the rainbow effect. Modern SVG supports this, but older SVG does not. Most apps can't read conical gradients, including both Godot and GIMP. As a work-around, here is my procedure for making these icons:

1. Make the conical gradient in Inkscape, and export it as `GradientND.png` (already done, doesn't need to be repeated).
2. Optimize the PNG using oxipng (already done, doesn't need to be repeated).
3. Have the SVG paths ready, usually copied from Godot and edited in GodSVG.
4. Edit the SVG with VS Code and insert an SVG pattern with the image data: `<pattern id="a" height="100%" patternContentUnits="objectBoundingBox" width="100%"><image height="1" width="1" xlink:href="data:image/png;base64,"/></pattern>`
5. Convert the PNG to base64: `openssl base64 -in GradientND.png | tr -d '\n' | pbcopy` (this copies to the clipboard on macOS).
6. Paste the image data after the `base64,` above.
7. Reference the pattern in SVG paths: `fill="url(#a)"`
8. This patterned SVG can be read by GIMP, but Godot doesn't support SVG patterns. So open it in GIMP and export a PNG.
9. Run oxipng on that PNG (optional).
10. Edit a new SVG with VS Code to have only a PNG image embedded in the SVG: `<svg height="16" width="16" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink"><image height="16" width="16" xlink:href="data:image/png;base64,"/></svg>`
11. Convert the PNGs to base64: `openssl base64 -in MeshND.png | tr -d '\n' | pbcopy` (this copies to the clipboard on macOS).
12. Paste the image data after the `base64,` above.
13. Open the Godot editor to let Godot import the finished SVG with a scale factor of 8.0.
14. Thankfully, Godot supports plain images in SVG files. So you're done!
