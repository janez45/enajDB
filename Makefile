CXX=g++
CXXFLAGS=-std=c++17 -Wall -g -MMD
EXEC=enajDB
CCFILES=$(wildcard *.cc)
OBJECTS=${CCFILES:.cc=.o}
DEPENDS=${CCFILES:.cc=.d}

${EXEC}: ${OBJECTS}
	${CXX} ${OBJECTS} -o ${EXEC}

-include ${DEPENDS}

# Extra recipe that allows us to delete temporary files by saying
# make clean

.PHONY: clean

clean:
	rm ${OBJECTS} ${DEPENDS} ${EXEC}