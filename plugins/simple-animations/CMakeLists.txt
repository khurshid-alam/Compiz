find_package (Compiz REQUIRED)
include (CompizPlugin)

compiz_plugin (animationsim PLUGINDEPS composite opengl animation)
