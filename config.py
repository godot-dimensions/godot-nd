# This file is for building as a Godot engine module.
def can_build(env, platform):
	return True


def configure(env):
	pass


def get_doc_classes():
	return [
		# Math (in dependency order).
		"VectorND",
		"BasisND",
		"MathND",
		"PlaneND",
		"RectND",
		"TransformND",
		"EulerND",
		"GeometryND",
		# General.
		"CameraND",
		"RenderingEngineND",
		"RenderingServerND",
		"NodeND",
		# Mesh.
		"ArrayCellMeshND",
		"ArrayWireMeshND",
		"BoxCellMeshND",
		"BoxWireMeshND",
		"CellMaterialND",
		"CellMeshND",
		"MaterialND",
		"MeshInstanceND",
		"MeshND",
		"OFFDocumentND",
		"OrthoplexCellMeshND",
		"OrthoplexWireMeshND",
		"WireMaterialND",
		"WireMeshND",
		# Depends on mesh.
		"MarkerND",
	]


def get_doc_path():
	return "addons/nd/doc_classes"


def get_icons_path():
	return "addons/nd/icons"
