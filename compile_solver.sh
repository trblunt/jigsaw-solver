g++ -ggdb `pkg-config --cflags opencv4` -o `basename solver.cpp .cpp` ./src/solver.cpp ./src/MST_solver.cpp ./src/GA_solver.cpp `pkg-config --libs opencv4`
