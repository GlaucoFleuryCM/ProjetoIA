CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -Iinclude/
FONTES = rede.cpp buscas.cpp metricas.cpp visualizacao.cpp main.cpp
OBJS = $(FONTES:.cpp=.o)
CABECALHOS = rede.h buscas.h metricas.h visualizacao.h
EXECUTAVEL = trabalho

.PHONY: all run clean

all: $(EXECUTAVEL)

$(EXECUTAVEL): $(OBJS)
	$(CXX) -o $(EXECUTAVEL) $^ -Llib -lraylib -lGL -lX11

%.o: %.cpp %.h
	$(CXX) -c -o $@ $(CXXFLAGS) -Iinclude/ $<

run: $(EXECUTAVEL)
	./$(EXECUTAVEL)

clean:
	rm -f $(OBJS) $(EXECUTAVEL)
