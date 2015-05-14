find_package (Compiz REQUIRED)

include (CompizPlugin)

compiz_plugin(matecompat PLUGINDEPS composite opengl)