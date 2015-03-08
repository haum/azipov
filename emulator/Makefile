SRC=azipov.c
APP=azipov_emu
CXXFLAGS=-std=c++11
LDFLAGS=-l GL -l GLU -lglut

${APP}:${SRC}
	${CXX} -o $@ $^ ${CXXFLAGS} ${LDFLAGS}

clean:
	rm ${APP}
