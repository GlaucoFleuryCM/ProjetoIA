#ifndef REDE_H
#define REDE_H

#include <random>
#include <vector>

using namespace std;

struct Vertice{
    double x;
    double y;
};

struct Vizinho{
    int vertice;
    double peso;
};

struct Busca{
    int origem;
    int destino;
};

void gera_rede_geografica(int n, double lambda, vector<Vertice>& vertices, vector<vector<Vizinho>>& adjacencias, mt19937& rng);
vector<Busca> gera_buscas(const vector<vector<Vizinho>>& adjacencias, mt19937& rng, int n);

#endif
