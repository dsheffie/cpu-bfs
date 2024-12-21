OBJ = main.o bfs.o dfs.o
EXE = bfs
OPT = -O2 -MMD -mavx512f -mavx512vl
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
