#ifndef METRICAS_H
#define METRICAS_H

#include <string>
#include <vector>

#include "buscas.h"

using namespace std;

using FuncaoBusca = ResultadoBusca (*)(const Busca&, const vector<vector<Vizinho>>&, const vector<Vertice>&);

struct Algoritmo{
    string nome;
    FuncaoBusca funcao;
};

struct AmostraBusca{
    bool sucesso;
    double distancia;
    double razao_otimo;
    int nos_considerados;
    double tempo_us;
};

struct Metricas{
    string nome;
    int total_buscas;
    int sucessos;
    double taxa_sucesso;
    double distancia_media;
    double distancia_desvio;
    double razao_otimo_media;
    double nos_media;
    double nos_desvio;
    double tempo_medio_us;
    double tempo_desvio_us;
};

vector<Algoritmo> lista_algoritmos();
void mede_buscas(const vector<Algoritmo>& algoritmos, const vector<Busca>& buscas, const vector<vector<Vizinho>>& adjacencias, const vector<Vertice>& vertices, int repeticoes, vector<vector<AmostraBusca>>& amostras);
Metricas resume_amostras(const string& nome, const vector<AmostraBusca>& amostras);

#endif
