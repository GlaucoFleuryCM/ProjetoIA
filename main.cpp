#include <iostream>
#include <fstream>
#include <random>
#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <cstdlib>

#include <raylib.h>

#include "rede.h"
#include "buscas.h"
#include "metricas.h"
#include "visualizacao.h"

using namespace std;

/* notas importantes:
   - 5 redes por lambda (médias mais confiáveis)
   - desvio padrão de tempo e distância
   - coluna de 'Sucessos' (Hill Climbing)
   - 100 repetições por busca pra mesma rede
 */

struct Rede{
    double lambda;
    int seed;
    vector<Vertice> vertices;
    vector<vector<Vizinho>> adjacencias;
    vector<Busca> buscas;
    vector<ResultadoBusca> resultados;
};

static bool botao(Rectangle r, const char* texto, Color cor){
    bool hover = CheckCollisionPointRec(GetMousePosition(), r);
    DrawRectangleRec(r, ColorAlpha(cor, hover ? 0.3f : 0.15f));
    DrawText(texto, r.x + (r.width - MeasureText(texto, 20)) / 2, r.y + (r.height - 20) / 2, 20, cor);
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

int main(int argc, char *argv[]){
    bool visualize = false;
    bool analyze = false;
    int n;

    if(argc==2 || argc==3){
        if(string(argv[1])=="visualize"){
            visualize = true;
            n = 200;
        }
        else if(string(argv[1])=="analyze"){
            analyze = true;
            n = 2500;
        }
        else{
            cout << "usage: " << argv[0] << " <analyze|visualize> [n_vertices]\n";
            return 1;
        }
        if(argc==3){
            n = atoi(argv[2]);
            if(n < 2){
                cout << "n_vertices deve ser um inteiro maior que 1\n";
                return 1;
            }
        }
    }
    else{ // o padrão é fazer visualização com 200 nós
        cout << "usage: " << argv[0] << " <analyze|visualize> [n_vertices]\n";
        return 1;
    }

    const vector<double> lambdas = {0.01, 0.02, 0.03};
    const int redes_por_lambda = 5;
    const int pares_por_rede = 30;
    const int repeticoes = 100;

    vector<Algoritmo> algoritmos = lista_algoritmos();
    vector<Rede> redes(lambdas.size() * redes_por_lambda);
    atomic<int> ready_cnt{0};

    auto analise = [&](){
        ofstream csv;
        if(analyze){
            csv.open("resultados.csv");
            csv << "lambda,algoritmo,buscas,sucessos,distancia_media,distancia_desvio,tempo_medio_us,tempo_desvio_us\n";
        }

        for(double lambda : lambdas){
            vector<vector<AmostraBusca>> amostras;

            for(int seed = 1; seed <= redes_por_lambda; seed++){
                Rede& r = redes[ready_cnt];
                r.lambda = lambda;
                r.seed = seed;
                mt19937 rng(seed);
                gera_rede_geografica(n, lambda, r.vertices, r.adjacencias, rng);

                r.buscas = gera_buscas(r.adjacencias, rng, pares_por_rede);
                r.resultados = mede_buscas(algoritmos, r.buscas, r.adjacencias, r.vertices, repeticoes, amostras);
                ready_cnt++;
            }

            for(size_t i = 0; analyze && i < algoritmos.size(); i++){
                Metricas m = resume_amostras(algoritmos[i].nome, amostras[i]);
                csv << lambda << "," << m.nome << "," << m.total_buscas << "," << m.sucessos << ","
                    << m.distancia_media << "," << m.distancia_desvio << ","
                    << m.tempo_medio_us << "," << m.tempo_desvio_us << "\n";
            }
        }
    };

    thread thread_analise;
    thread_analise = thread(analise);

    if(visualize){
        InitWindow(1150, 600, "visualização");
        SetTargetFPS(60);

        Visualizacao vis = cria_visualizacao("shaders/post.glsl");
        int curr = 0;
        const Rede vazia{};

        while (!WindowShouldClose() && !IsKeyDown(KEY_Q)) {
            bool ready = ready_cnt > 0;
            const Rede& r = ready ? redes[curr] : vazia;
            Color cor = ready ? RAYWHITE : GRAY;

            if(ready){
                atualiza_visualizacao(vis, algoritmos, r.buscas, r.resultados);

                BeginTextureMode(vis.scene);
                desenha_rede(vis, r.vertices, r.adjacencias, algoritmos, r.buscas, r.resultados);
                EndTextureMode();

                processa_bloom(vis, r.vertices, algoritmos, r.resultados);
            }

            BeginDrawing();

            ClearBackground(BLACK);

            if(ready) desenha_cena_com_shader(vis, algoritmos, r.resultados);
            else DrawText("Gerando redes...", (GetScreenWidth() - MeasureText("Gerando redes...", 20)) / 2, GetScreenHeight() / 2 - 10, 20, RAYWHITE);
            desenha_hud(vis, algoritmos, r.buscas, r.resultados);

            int l = curr / redes_por_lambda, s = curr % redes_por_lambda, L = lambdas.size();
            float bx = GetScreenWidth() - 232;
            DrawText(TextFormat("lambda = %.2f", lambdas[l]), bx + 50, 17, 20, cor);
            DrawText(TextFormat("rede %d/%d", s + 1, redes_por_lambda), bx + 50, 53, 20, cor);
            if(botao({bx, 12, 30, 30}, "<", cor)) l = (l + L - 1) % L;
            if(botao({bx + 190, 12, 30, 30}, ">", cor)) l = (l + 1) % L;
            if(botao({bx, 48, 30, 30}, "<", cor)) s = (s + redes_por_lambda - 1) % redes_por_lambda;
            if(botao({bx + 190, 48, 30, 30}, ">", cor)) s = (s + 1) % redes_por_lambda;
            if(ready_cnt < (int)redes.size())
                DrawText(TextFormat("%d/%d redes prontas", ready_cnt.load(), (int)redes.size()), bx + 20, 88, 16, ColorAlpha(RAYWHITE, 0.6f));

            int target = l * redes_por_lambda + s;
            if(target != curr && target < ready_cnt){
                curr = target;
                vis.step = 0.0;
                vis.paused = true;
            }

            EndDrawing();
        }

        destroi_visualizacao(vis);
        CloseWindow();
    }

    if(thread_analise.joinable()) thread_analise.join();

    return 0;
}
