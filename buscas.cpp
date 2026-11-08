#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <queue>
#include <stack>
#include <utility>
#include <vector>

#include "buscas.h"

using namespace std;

using EntradaFila = pair<double, int>;
using FilaPrioridade = priority_queue<EntradaFila, vector<EntradaFila>, greater<EntradaFila>>;

/* Heurística utilizada = distância euclidiana entre 2 nós */
static double heuristica(const vector<Vertice>& vertices, int atual, int destino){
    return hypot(vertices[atual].x - vertices[destino].x, vertices[atual].y - vertices[destino].y);
}

/* Retorna o resultado da Busca no objeto 'ResultadoBusca'; esse
   objeto está definido em 'buscas.h' */
static ResultadoBusca monta_resultado(const Busca& busca, const vector<int>& pai, const vector<double>& custo, const vector<int>& nos_considerados, bool sucesso){
    ResultadoBusca resultado{{}, 0.0, nos_considerados, {}, {}, sucesso};

    for(int no : nos_considerados){
        resultado.pais_considerados.push_back(pai[no]);
        resultado.custos_considerados.push_back(custo[no]);
    }

    if(!sucesso){
        return resultado;
    }

    for(int vertice = busca.destino; vertice != -1; vertice = pai[vertice]){
        resultado.caminho_escolhido.push_back(vertice);
    }

    reverse(resultado.caminho_escolhido.begin(), resultado.caminho_escolhido.end());
    resultado.peso_final_escolhido = custo[busca.destino];

    return resultado;
}

/* Função genérica, que expande o nó de menor prioridade da fila até
   chegar no destino final; cada algoritmo define como a prioridade é
   calculada através do template 'prioridade' (callback) */
template <typename Prioridade>
static ResultadoBusca busca_por_prioridade(const Busca& busca, const vector<vector<Vizinho>>& adjacencias, Prioridade prioridade){
    int total_vertices = adjacencias.size();
    // nós inicializados com custo infinito
    vector<double> custo(total_vertices, numeric_limits<double>::infinity()); 
    vector<int> pai(total_vertices, -1);
    vector<bool> fechado(total_vertices, false); // nós já expandidos
    vector<int> nos_considerados;
    FilaPrioridade fila;

    // início: origem tem custo 0, e entra na fila
    custo[busca.origem] = 0.0;
    fila.push({prioridade(0.0, busca.origem), busca.origem});

    // ciclo de exploração da fila, em que explora-se sempre o nó
    // de maior prioridade nela a cada ciclo
    while(!fila.empty()){
        int atual = fila.top().second;
        fila.pop();

        if(fechado[atual]){
            continue;
        }

        fechado[atual] = true;
        nos_considerados.push_back(atual);

        if(atual == busca.destino){
            return monta_resultado(busca, pai, custo, nos_considerados, true);
        }

        // relaxamento: atualização de g(n) caso melhor caminho seja descoberto
        for(const Vizinho& vizinho : adjacencias[atual]){
            double novo_custo = custo[atual] + vizinho.peso;

            if(!fechado[vizinho.vertice] && novo_custo < custo[vizinho.vertice]){
                custo[vizinho.vertice] = novo_custo;
                pai[vizinho.vertice] = atual;
                fila.push({prioridade(novo_custo, vizinho.vertice), vizinho.vertice});
            }
        }
    }

    return monta_resultado(busca, pai, custo, nos_considerados, false);
}

/* DFS e BFS são implementados abaixo sem recursão (optou-se por maior velocidade
   de execução dos algoritmos, a custo de um código mais complexo e menos intuitivo) */

ResultadoBusca busca_profundidade(const Busca& busca, const vector<vector<Vizinho>>& adjacencias, const vector<Vertice>&){
    struct EntradaPilha{
        int vertice;
        int pai;
        double custo;
    };

    int total_vertices = adjacencias.size();
    vector<double> custo(total_vertices, 0.0);
    vector<int> pai(total_vertices, -1);
    vector<bool> visitado(total_vertices, false);
    vector<int> nos_considerados;
    stack<EntradaPilha> pilha;

    pilha.push({busca.origem, -1, 0.0});

    while(!pilha.empty()){
        EntradaPilha atual = pilha.top();
        pilha.pop();

        if(visitado[atual.vertice]){
            continue;
        }

        visitado[atual.vertice] = true;
        pai[atual.vertice] = atual.pai;
        custo[atual.vertice] = atual.custo;
        nos_considerados.push_back(atual.vertice);

        if(atual.vertice == busca.destino){
            return monta_resultado(busca, pai, custo, nos_considerados, true);
        }

        const vector<Vizinho>& vizinhos = adjacencias[atual.vertice];

        for(auto vizinho = vizinhos.rbegin(); vizinho != vizinhos.rend(); ++vizinho){
            if(!visitado[vizinho->vertice]){
                pilha.push({vizinho->vertice, atual.vertice, atual.custo + vizinho->peso});
            }
        }
    }

    return monta_resultado(busca, pai, custo, nos_considerados, false);
}

ResultadoBusca busca_largura(const Busca& busca, const vector<vector<Vizinho>>& adjacencias, const vector<Vertice>&){
    int total_vertices = adjacencias.size();
    vector<double> custo(total_vertices, 0.0);
    vector<int> pai(total_vertices, -1);
    vector<bool> visitado(total_vertices, false);
    vector<int> nos_considerados;
    queue<int> fila;

    visitado[busca.origem] = true;
    fila.push(busca.origem);

    while(!fila.empty()){
        int atual = fila.front();
        fila.pop();
        nos_considerados.push_back(atual);

        if(atual == busca.destino){
            return monta_resultado(busca, pai, custo, nos_considerados, true);
        }

        for(const Vizinho& vizinho : adjacencias[atual]){
            if(!visitado[vizinho.vertice]){
                visitado[vizinho.vertice] = true;
                pai[vizinho.vertice] = atual;
                custo[vizinho.vertice] = custo[atual] + vizinho.peso;
                fila.push(vizinho.vertice);
            }
        }
    }

    return monta_resultado(busca, pai, custo, nos_considerados, false);
}

/* Abaixo é possível notar que as 3 buscas (Dijkstra, A* e Best-First) são 
   variações do mesmo algoritmo, que involve explorar o vértice de menor custo
   f(n); apenas varia o cálculo desse f(n) de acordo com o custo para alcançar
   o nó 'n', g(n), e a estimativa para alcançar a meta via 'n', a heurística h(n):
   - f(h) = g(n) + h(n) [A*]
   - f(h) = g(n) [Dijkstra]
   - f(h) = h(n) [Best-First]
*/

ResultadoBusca busca_best_first(const Busca& busca, const vector<vector<Vizinho>>& adjacencias, const vector<Vertice>& vertices){
    return busca_por_prioridade(busca, adjacencias, [&](double, int vertice){
        return heuristica(vertices, vertice, busca.destino);
    });
}

ResultadoBusca busca_a_estrela(const Busca& busca, const vector<vector<Vizinho>>& adjacencias, const vector<Vertice>& vertices){
    return busca_por_prioridade(busca, adjacencias, [&](double custo, int vertice){
        return custo + heuristica(vertices, vertice, busca.destino);
    });
}

ResultadoBusca busca_dijkstra(const Busca& busca, const vector<vector<Vizinho>>& adjacencias, const vector<Vertice>&){
    return busca_por_prioridade(busca, adjacencias, [](double custo, int){
        return custo;
    });
}

/* expande filhos do nó atual, escolhe o melhor, e seguem em frente;
   diferente do Best-First, o Hill-Climbing jamais retorna para ramos 
   anteriores no Grafo */
ResultadoBusca busca_hill_climbing(const Busca& busca, const vector<vector<Vizinho>>& adjacencias, const vector<Vertice>& vertices){
    ResultadoBusca resultado{{busca.origem}, 0.0, {busca.origem}, {-1}, {0.0}, false};
    int atual = busca.origem;

    while(atual != busca.destino){
        double melhor_h = heuristica(vertices, atual, busca.destino);
        double peso_melhor = 0.0;
        int melhor = -1;

        // avalia todos os nós expandidos, e busca o melhor com base na heurística
        for(const Vizinho& vizinho : adjacencias[atual]){
            double h = heuristica(vertices, vizinho.vertice, busca.destino);

            if(h < melhor_h){
                melhor_h = h;
                melhor = vizinho.vertice;
                peso_melhor = vizinho.peso;
            }
        }

        if(melhor == -1){
            return resultado;
        }

        // avança para o melhor nó expandido, e atualiza vetor de predecessores e pais
        resultado.pais_considerados.push_back(atual);
        atual = melhor;
        resultado.peso_final_escolhido += peso_melhor;
        resultado.caminho_escolhido.push_back(atual);
        resultado.nos_considerados.push_back(atual);
        resultado.custos_considerados.push_back(resultado.peso_final_escolhido);
    }

    resultado.sucesso = true;
    return resultado;
}
