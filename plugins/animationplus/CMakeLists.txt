find_package (Compiz REQUIRED)
include (CompizPlugin)

compiz_plugin (animationplus PLUGINDEPS animation animationaddon composite opengl)
