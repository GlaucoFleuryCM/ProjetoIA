CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra
FONTES = main.cpp rede.cpp buscas.cpp metricas.cpp
CABECALHOS = rede.h buscas.h metricas.h
EXECUTAVEL = trabalho

.PHONY: all run clean

all: $(EXECUTAVEL)

$(EXECUTAVEL): $(FONTES) $(CABECALHOS)
	$(CXX) $(CXXFLAGS) $(FONTES) -o $(EXECUTAVEL)

run: $(EXECUTAVEL)
	./$(EXECUTAVEL)

clean:
	rm -f $(EXECUTAVEL)
