#ifndef VISUALIZACAO_H
#define VISUALIZACAO_H

#include <vector>

#include <raylib.h>

#include "rede.h"
#include "buscas.h"
#include "metricas.h"

using namespace std;

struct Shaders {
    const char* filename;
    Shader rl_shader;
    int loc_time;
    int loc_resolution;
    int loc_progress;
};

struct Bloom {
    int width, height;
    RenderTexture2D layer;
    RenderTexture2D curr;
    RenderTexture2D history[2];
    int curr_hist;
    RenderTexture2D source[2];
    RenderTexture2D temp, thin;
    RenderTexture2D small_font, small_time, wide;
    Shaders step1, step2, step3, step4;
};

struct Visualizacao {
    int search;
    int method;
    double step;
    double speed;
    bool paused;
    int scale;
    RenderTexture2D scene;
    Shaders post;
    Bloom bloom;
};

Visualizacao cria_visualizacao(const char* filename);
void destroi_visualizacao(Visualizacao& vis);

void atualiza_visualizacao(Visualizacao& vis, const vector<Algoritmo>& algoritmos, const vector<Busca>& buscas, const vector<ResultadoBusca>& resultados);

void desenha_rede(const Visualizacao& vis, const vector<Vertice>& vertices, const vector<vector<Vizinho>>& adjacencias, const vector<Algoritmo>& algoritmos, const vector<Busca>& buscas, const vector<ResultadoBusca>& resultados);
void processa_bloom(Visualizacao& vis, const vector<Vertice>& vertices, const vector<Algoritmo>& algoritmos, const vector<ResultadoBusca>& resultados);
void desenha_cena_com_shader(const Visualizacao& vis, const vector<Algoritmo>& algoritmos, const vector<ResultadoBusca>& resultados);
void desenha_hud(const Visualizacao& vis, const vector<Algoritmo>& algoritmos, [[maybe_unused]] const vector<Busca>& buscas, const vector<ResultadoBusca>& resultados);

#endif
