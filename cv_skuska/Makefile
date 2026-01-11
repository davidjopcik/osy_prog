TARGET1=main

all: ${TARGET1} main

${TARGET1}: ${TARGET1}.cpp
	g++ $^ -o $@

scitacka: scitacka.cpp
	g++ main.cpp -o main

clean:
	rm -f main