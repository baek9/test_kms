all:
	g++ main.cpp drm.cpp gbm.cpp egl.cpp glCube.cpp esTransform.cpp -ldrm -lgbm -lEGL -lGLESv2 -I/usr/include/libdrm -g -fpermissive

