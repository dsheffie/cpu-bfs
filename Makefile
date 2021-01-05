OBJ = main.o bfs.o dfs.o
EXE = bfs
OPT = -O2 -MMD
CXXFLAGS = -g $(OPT)
DEP = $(OBJ:.o=.d)

.PHONY: all clean

all: $(EXE)

$(EXE) : $(OBJ)
	g++ $(CXXFLAGS) $(OBJ) $(LIBS) -o $(EXE) -lpthread

%.o: %.cc
	g++ -MMD $(CXXFLAGS) -c $< 


-include $(DEP)

clean:
	rm -rf $(EXE) $(OBJ) $(DEP)
