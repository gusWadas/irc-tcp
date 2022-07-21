# ------------------- # --- VARIÁVEIS DE AMBIENTE --- # -------------------- #

CPP = g++
RM = rm -f
CFLAGS = -Wall -Wextra
ZIP = irc_tcp.zip
UTILS_OBJ = obj/socket.o obj/commandhandler.o obj/changemode.o obj/readline.o obj/channel.o obj/channel_controller.o obj/user.o

# ------------------- # --- DIRETIVAS PRINCIPAIS --- # -------------------- #

# Global
all: compile-server compile-client

# Produção dos executáveis
compile-server: obj/server.o $(UTILS_OBJ)
	$(CPP) -I lib/ obj/server.o $(UTILS_OBJ) -o server
compile-client: obj/client.o $(UTILS_OBJ)
	$(CPP) -I lib/ obj/client.o $(UTILS_OBJ) -o client

# Execução convencional do programa
run-server:
	stty -icanon
	./server
run-client:
	stty -icanon
	./client

# Execução do programa com Valgrind
valgrind-server:
	valgrind -s --tool=memcheck --leak-check=full --track-origins=yes --show-leak-kinds=all ./server
valgrind-client:
	valgrind -s --tool=memcheck --leak-check=full --track-origins=yes --show-leak-kinds=all ./client

# Compressão dos arquivos
zip: clean
	zip -r $(ZIP) Makefile lib/* src/* obj/

# Limpeza de objetos e de executável
clean:
	$(RM) server client $(ZIP) obj/*.o

# ----------------------- # --- OBJETIFICAÇÃO --- # ------------------------ #

obj/server.o: src/server.cpp lib/utils.hpp lib/readline.hpp
	$(CPP) -c src/server.cpp -o obj/server.o $(CFLAGS)

obj/client.o: src/client.cpp lib/utils.hpp lib/socket.hpp lib/readline.hpp
	$(CPP) -c src/client.cpp -o obj/client.o $(CFLAGS)

obj/socket.o: src/socket.cpp lib/socket.hpp
	$(CPP) -c src/socket.cpp -o obj/socket.o $(CFLAGS)

obj/commandhandler.o: src/commandhandler.cpp lib/utils.hpp
	$(CPP) -c src/commandhandler.cpp -o obj/commandhandler.o $(CFLAGS)

obj/changemode.o: src/changemode.cpp lib/utils.hpp
	$(CPP) -c src/changemode.cpp -o obj/changemode.o $(CFLAGS)

obj/readline.o: src/readline.cpp lib/readline.hpp
	$(CPP) -c src/readline.cpp -o obj/readline.o $(CFLAGS)

obj/channel.o: src/channel.cpp lib/channel.hpp lib/user.hpp
	$(CPP) -c src/channel.cpp -o obj/channel.o $(CFLAGS)

obj/channel_controller.o: src/channel_controller.cpp lib/channel_controller.hpp lib/channel.hpp
	$(CPP) -c src/channel_controller.cpp -o obj/channel_controller.o $(CFLAGS)

obj/user.o: src/user.cpp lib/user.hpp
	$(CPP) -c src/user.cpp -o obj/user.o $(CFLAGS)