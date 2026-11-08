#include <fstream>
#include <random>
#include <vector>

#include "rede.h"
#include "buscas.h"
#include "metricas.h"

using namespace std;

/* notas importantes:
   - 5 redes por lambda (médias mais confiáveis)
   - desvio padrão de tempo e distância
   - coluna de 'Sucessos' (Hill Climbing)
   - 100 repetições por busca pra mesma rede 
 */

int main(){
    const int n = 2500;
    const vector<double> lambdas = {0.01, 0.02, 0.03};
    const int redes_por_lambda = 5;
    const int pares_por_rede = 30;
    const int repeticoes = 100;

    vector<Algoritmo> algoritmos = lista_algoritmos();
    ofstream csv("resultados.csv");
    csv << "lambda,algoritmo,buscas,sucessos,distancia_media,distancia_desvio,tempo_medio_us,tempo_desvio_us\n";

    for(double lambda : lambdas){
        vector<vector<AmostraBusca>> amostras;

        for(int seed = 1; seed <= redes_por_lambda; seed++){
            mt19937 rng(seed);
            vector<Vertice> vertices;
            vector<vector<Vizinho>> adjacencias;
            gera_rede_geografica(n, lambda, vertices, adjacencias, rng);

            vector<Busca> buscas = gera_buscas(adjacencias, rng, pares_por_rede);
            mede_buscas(algoritmos, buscas, adjacencias, vertices, repeticoes, amostras);
        }

        for(size_t i = 0; i < algoritmos.size(); i++){
            Metricas m = resume_amostras(algoritmos[i].nome, amostras[i]);
            csv << lambda << "," << m.nome << "," << m.total_buscas << "," << m.sucessos << ","
                << m.distancia_media << "," << m.distancia_desvio << ","
                << m.tempo_medio_us << "," << m.tempo_desvio_us << "\n";
        }
    }

    return 0;
}
