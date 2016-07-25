find_package (Compiz REQUIRED)
include (CompizPlugin)

compiz_plugin (animationjc PLUGINDEPS composite opengl animation)
