find_package (Compiz REQUIRED)
include (CompizPlugin)

compiz_plugin (animationaddon PLUGINDEPS composite opengl animation)
