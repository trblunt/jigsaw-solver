g++ -ggdb  `pkg-config --cflags opencv` -o `basename solver.cpp .cpp` solver.cpp `pkg-config --libs opencv`