ALL: build tidy

run: ALL
		./shfd -d -D -t 1 -T 1

test:
		# PLACEHOLDER

build: Lib.o ServerUtils.o main.o  ShellClient.o ServerResponse.o FileClient.o ThreadMan.o
		@echo -----------------------
		@echo "Building..."
		@echo -----------------------

		g++ -std=c++11 -pthread main.o Lib.o ServerUtils.o ShellClient.o FileClient.o ServerResponse.o ThreadMan.o -o shfd

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

ServerResponse.o: ./src/components/ServerResponse.cpp
		@echo -----------------------
		@echo Building ServerResponse...
		@echo -----------------------

		g++ -c ./src/components/ServerResponse.cpp

ThreadMan.o: ./src/threads/ThreadMan.cpp
		@echo -----------------------
		@echo Building ThreadMan...
		@echo -----------------------

		g++ -c ./src/threads/ThreadMan.cpp

Lib.o: ./src/Lib.cpp		
		@echo -----------------------
		@echo Building Lib...
		@echo -----------------------

		g++ -c ./src/Lib.cpp

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

		rm shfd
		rm *.log
		rm -r *.o *.out 