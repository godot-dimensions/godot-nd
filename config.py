# This file is for building as a Godot engine module.
def can_build(env, platform):
	return True


def configure(env):
	pass


def get_doc_classes():
	return [
		# General.
		"CameraND",
		"GeometryND",
		"RenderingEngineND",
		"RenderingServerND",
		"NodeND",
		"PlaneND",
		"RectND",
		"TransformND",
		"VectorND",
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
