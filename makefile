ALL: build tidy

run: ALL
		./shfd -d -D

test:
		# PLACEHOLDER

build: lib.o ServerUtils.o main.o  ShellClient.o STDResponse.o FileClient.o ThreadMan.o
		@echo -----------------------
		@echo "Building..."
		@echo -----------------------

		g++ -std=c++11 -pthread main.o lib.o ServerUtils.o ShellClient.o FileClient.o STDResponse.o ThreadMan.o -o shfd

main.o: ./src/main.cpp
		@echo -----------------------
		@echo Building main...
		@echo -----------------------

		g++ -c ./src/main.cpp

ShellClient.o: ./src/components/ShellClient.cpp
		@echo -----------------------
		@echo Building ShellClient...
		@echo -----------------------

		g++ -c ./src/components/ShellClient.cpp

FileClient.o: ./src/components/FileClient.cpp
		@echo -----------------------
		@echo Building FileClient...
		@echo -----------------------

		g++ -c ./src/components/FileClient.cpp

STDResponse.o: ./src/components/STDResponse.cpp
		@echo -----------------------
		@echo Building STDResponse...
		@echo -----------------------

		g++ -c ./src/components/STDResponse.cpp

ThreadMan.o: ./src/threads/ThreadMan.cpp
		@echo -----------------------
		@echo Building ThreadMan...
		@echo -----------------------

		g++ -c ./src/threads/ThreadMan.cpp

lib.o: ./src/lib.cpp		
		@echo -----------------------
		@echo Building lib...
		@echo -----------------------

		g++ -c ./src/lib.cpp

ServerUtils.o: ./src/ServerUtils.cpp		
		@echo -----------------------
		@echo Building ServerUtils...
		@echo -----------------------

		g++ -c ./src/ServerUtils.cpp

tidy:   
	  @echo -----------------------
		@echo Tidying things up...
		@echo -----------------------
		rm -r *.o

clean:
		@echo -----------------------
		@echo "      CLEANING"
		@echo -----------------------

		rm -r *.o *.out shfd *.log