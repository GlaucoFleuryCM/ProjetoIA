#include <chrono>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "metricas.h"

using namespace std;

/* retorna média e desvio padrão de uma lista numérica */
static pair<double, double> media_desvio(const vector<double>& valores){
    if(valores.empty()){
        double indefinido = numeric_limits<double>::quiet_NaN();
        return {indefinido, indefinido}; //erro 
    }

    double soma = 0.0;
    for(double valor : valores){
        soma += valor;
    }

    double media = soma / valores.size();
    if(valores.size() == 1){
        return {media, 0.0};
    }

    double soma_quadrados = 0.0;
    for(double valor : valores){
        soma_quadrados += (valor - media) * (valor - media);
    }

    return {media, sqrt(soma_quadrados/(valores.size()-1))};
}

vector<Algoritmo> lista_algoritmos(){
    return {
        {"DFS", busca_profundidade},
        {"BFS", busca_largura},
        {"Best-First", busca_best_first},
        {"A*", busca_a_estrela},
        {"Hill Climbing", busca_hill_climbing},
        {"Dijkstra", busca_dijkstra}
    };
}

vector<ResultadoBusca> mede_buscas(const vector<Algoritmo>& algoritmos, const vector<Busca>& buscas, const vector<vector<Vizinho>>& adjacencias, const vector<Vertice>& vertices, int repeticoes, vector<vector<AmostraBusca>>& amostras){
    vector<ResultadoBusca> resultados;

    if(repeticoes < 1){
        throw invalid_argument("mede_buscas: repeticoes deve ser pelo menos 1, mas recebeu " + to_string(repeticoes));
    }

    amostras.resize(algoritmos.size());

    for(const Busca& busca : buscas){
        double otimo = busca_dijkstra(busca, adjacencias, vertices).peso_final_escolhido;

        for(size_t i = 0; i < algoritmos.size(); i++){
            ResultadoBusca resultado;

            auto inicio = chrono::steady_clock::now();

            for(int repeticao = 0; repeticao < repeticoes; repeticao++){
                resultado = algoritmos[i].funcao(busca, adjacencias, vertices);
            }

            auto fim = chrono::steady_clock::now();
            double tempo_us = chrono::duration<double, micro>(fim - inicio).count() / repeticoes;

            amostras[i].push_back({
                resultado.sucesso,
                resultado.peso_final_escolhido,
                resultado.peso_final_escolhido / otimo,
                (int)resultado.nos_considerados.size(),
                tempo_us
            });

            resultados.push_back(resultado);
        }
    }

    return resultados;
}

Metricas resume_amostras(const string& nome, const vector<AmostraBusca>& amostras){
    vector<double> distancias;
    vector<double> razoes;
    vector<double> nos;
    vector<double> tempos;

    for(const AmostraBusca& amostra : amostras){
        nos.push_back(amostra.nos_considerados);
        tempos.push_back(amostra.tempo_us);

        if(amostra.sucesso){
            distancias.push_back(amostra.distancia);
            razoes.push_back(amostra.razao_otimo);
        }
    }

    pair<double, double> distancia = media_desvio(distancias);
    pair<double, double> razao = media_desvio(razoes);
    pair<double, double> nos_estatistica = media_desvio(nos);
    pair<double, double> tempo = media_desvio(tempos);
    int total = amostras.size();
    int sucessos = distancias.size();

    return {
        nome,
        total,
        sucessos,
        total > 0 ? (double)sucessos / total : numeric_limits<double>::quiet_NaN(),
        distancia.first,
        distancia.second,
        razao.first,
        nos_estatistica.first,
        nos_estatistica.second,
        tempo.first,
        tempo.second
    };
}
