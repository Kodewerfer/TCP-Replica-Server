ALL: build tidy

run: ALL
		./rsshell

build: lib.o utils.o main.o 
		@echo -----------------------
		@echo "Building sshell..."
		@echo -----------------------

		g++ lib.o utils.o main.o -o rsshell

main.o: ./src/main.cpp
		@echo -----------------------
		@echo Building main...
		@echo -----------------------

		g++ -c ./src/main.cpp

lib.o: ./src/lib.cpp		
		@echo -----------------------
		@echo Building lib...
		@echo -----------------------

		g++ -c ./src/lib.cpp

utils.o: ./src/utils.cpp		
		@echo -----------------------
		@echo Building utils...
		@echo -----------------------

		g++ -c ./src/utils.cpp

tidy:   
	  @echo -----------------------
		@echo Tidying things up...
		@echo -----------------------
		rm -r *.o

clean:
		@echo -----------------------
		@echo "      CLEANING"
		@echo -----------------------

		rm -r *.o *.out sshell