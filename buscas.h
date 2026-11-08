#ifndef BUSCAS_H
#define BUSCAS_H

#include <vector>

#include "rede.h"

using namespace std;

struct ResultadoBusca{
    vector<int> caminho_escolhido; // caminho final para objetivo
    double peso_final_escolhido; // custo do caminho final
    /* nós e pais avaliados durante a execução do algoritmo
    em ordem cronológica (para visualização posterior) */
    vector<int> nos_considerados; 
    vector<int> pais_considerados; 
    vector<double> custos_considerados;
    bool sucesso; // busca falha ou não
};

ResultadoBusca busca_profundidade(const Busca& busca, const vector<vector<Vizinho>>& adjacencias, const vector<Vertice>& vertices);
ResultadoBusca busca_largura(const Busca& busca, const vector<vector<Vizinho>>& adjacencias, const vector<Vertice>& vertices);
ResultadoBusca busca_best_first(const Busca& busca, const vector<vector<Vizinho>>& adjacencias, const vector<Vertice>& vertices);
ResultadoBusca busca_a_estrela(const Busca& busca, const vector<vector<Vizinho>>& adjacencias, const vector<Vertice>& vertices);
ResultadoBusca busca_hill_climbing(const Busca& busca, const vector<vector<Vizinho>>& adjacencias, const vector<Vertice>& vertices);
ResultadoBusca busca_dijkstra(const Busca& busca, const vector<vector<Vizinho>>& adjacencias, const vector<Vertice>& vertices);

#endif
