# This file is for building as a Godot engine module.
def can_build(env, platform):
	return True


def configure(env):
	pass


def get_doc_classes():
	return [
		"ArrayWireMeshND",
		"BoxWireMeshND",
		"MaterialND",
		"MeshInstanceND",
		"MeshND",
		"NodeND",
		"OrthoplexWireMeshND",
		"RectND",
		"TransformND",
		"VectorND",
		"WireMaterialND",
		"WireMeshND",
	]


def get_doc_path():
	return "addons/nd/doc_classes"


def get_icons_path():
	return "addons/nd/icons"
