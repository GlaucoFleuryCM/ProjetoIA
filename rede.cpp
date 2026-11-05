#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <algorithm>
#include <queue>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>

#include "rede.h"

using namespace std;

/* Retorna vetor de struct 'Vertice', com coordenadas
   x,y na forma de double */
vector<Vertice> gera_vertices(int n, mt19937& rng){
    uniform_real_distribution<double> coordenada(0.0, n);
    vector<Vertice> vertices(n);

    for(Vertice& vertice : vertices){
        vertice.x = coordenada(rng);
        vertice.y = coordenada(rng);
    }

    return vertices;
}

/* Retorna lista de adjacências do Grafo; o critério de 
   existência de aresta entre 2 grafos é rand < p, sendo p
   calculado abaixo, e rand aleaotório e entre 0 e 1;
   O peso entre 2 vértices é sua distância geográfica */
vector<vector<Vizinho>> gera_adjacencias(const vector<Vertice>& vertices, double lambda, mt19937& rng){
    uniform_real_distribution<double> sorteio(0.0, 1.0);
    int n = vertices.size();
    vector<vector<Vizinho>> adjacencias(n);

    for(int i = 0; i < n; i++){
        for(int j = i + 1; j < n; j++){
            double s = hypot(vertices[i].x - vertices[j].x, vertices[i].y - vertices[j].y);
            double p = exp(-lambda * s);

            if(sorteio(rng) < p){
                adjacencias[i].push_back({j, s});
                adjacencias[j].push_back({i, s});
            }
        }
    }

    return adjacencias;
}

/* Gera o Grafo da rede geográfica (vértices e arestas);
   O argumento 'rng' é uma seed que permite a reproducibilidade
   de um grafo específico */
void gera_rede_geografica(int n, double lambda, vector<Vertice>& vertices, vector<vector<Vizinho>>& adjacencias, mt19937& rng){
    vertices = gera_vertices(n, rng);
    adjacencias = gera_adjacencias(vertices, lambda, rng);
}

/* Busca de Maior Componente: BFS em cada vértice ainda não
   marcado como visitado, de modo que cada BFS gera um componente;
   somente o maior será retornado */
vector<int> maior_componente(const vector<vector<Vizinho>>& adjacencias){
    int total_vertices = adjacencias.size();
    vector<bool> visitado(total_vertices, false);
    vector<int> maior;

    for(int inicio = 0; inicio < total_vertices; inicio++){
        if(visitado[inicio]){
            continue;
        }

        vector<int> componente;
        queue<int> fila;
        visitado[inicio] = true;
        fila.push(inicio);

        while(!fila.empty()){
            int atual = fila.front();
            fila.pop();
            componente.push_back(atual);

            for(const Vizinho& vizinho : adjacencias[atual]){
                if(!visitado[vizinho.vertice]){
                    visitado[vizinho.vertice] = true;
                    fila.push(vizinho.vertice);
                }
            }
        }

        if(componente.size() > maior.size()){
            maior = componente;
        }
    }

    return maior;
}

/* Retorna uma lista de tuplas com um vertice de começo e um
   vértice de fim, ambos representados por índices no vetor de 
   vértices do grafo; 'n' é o número de tuplas geradas*/
vector<Busca> gera_buscas(const vector<vector<Vizinho>>& adjacencias, mt19937& rng, int n) {
    vector<int> componente = maior_componente(adjacencias);
    long long tamanho = componente.size();
    long long maximo = tamanho * (tamanho - 1) / 2;

    /* checando se 'n' é natural > 0 e se o tamanho da componente suporta 'n' Buscas */
    if(n < 1 || n > maximo){
        throw invalid_argument("gera_buscas: n deve estar entre 1 e " + to_string(maximo) + ", mas recebeu " + to_string(n));
    }

    vector<Busca> buscas;
    uniform_int_distribution<int> sorteio(0, tamanho - 1);
    set<pair<int, int>> usados;

    /* gerando os pares de Busca, s/repetição */
    while((int)buscas.size() < n){
        int origem = componente[sorteio(rng)];
        int destino = componente[sorteio(rng)];

        if(origem == destino){
            continue;
        }

        pair<int, int> chave = minmax(origem, destino);

        if(usados.insert(chave).second){
            buscas.push_back({origem, destino});
        }
    }

    return buscas;
}
