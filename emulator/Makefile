SRC=azipov.cpp
APP=azipov_emu
CXXFLAGS=-std=c++11 -g
LDFLAGS=-l GL -l GLU -lglut -g

${APP}:${SRC}
	${CXX} -o $@ $^ ${CXXFLAGS} ${LDFLAGS}

clean:
	rm ${APP}
