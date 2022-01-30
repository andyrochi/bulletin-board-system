OUT = server
FILE_NAME = server
FLAGS = -Wall -std=c++14

all: ${FILE_NAME}.*
	@if [ "$(suffix $<)" = ".cpp" ]; then\
		g++ ${FLAGS} $< -o ${OUT};\
	else\
		echo "There is no server.cpp file in the src/ folder.";\
    fi

clean:
	rm server
