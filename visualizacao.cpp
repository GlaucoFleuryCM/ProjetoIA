#include <algorithm>
#include <cfloat>
#include <cmath>

#include "visualizacao.h"

#include <rlgl.h>

using namespace std;

static void carrega_shader(Shaders& s){
    /* shader default*/
    s.rl_shader = LoadShader(nullptr, s.filename);

    /* locations glsl */
    s.loc_time = GetShaderLocation(s.rl_shader, "time");
    s.loc_resolution = GetShaderLocation(s.rl_shader, "resolution");
    s.loc_progress = GetShaderLocation(s.rl_shader, "progress");
}

static Shaders novo_shader(const char* filename){
    Shaders s{};
    s.filename = filename;
    carrega_shader(s);
    return s;
}

static RenderTexture2D carrega_render_float(int width, int height){
    RenderTexture2D rt{};

    rt.id = rlLoadFramebuffer();
    rt.texture.id = rlLoadTexture(nullptr, width, height, PIXELFORMAT_UNCOMPRESSED_R16G16B16A16, 1);
    rt.texture.width = width;
    rt.texture.height = height;
    rt.texture.format = PIXELFORMAT_UNCOMPRESSED_R16G16B16A16;
    rt.texture.mipmaps = 1;

    rlFramebufferAttach(rt.id, rt.texture.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);
    
    if(!rlFramebufferComplete(rt.id)){
        TraceLog(LOG_WARNING, "incomplete framebuffer for bloom");
    }
    
    SetTextureFilter(rt.texture, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(rt.texture, TEXTURE_WRAP_CLAMP);

    BeginTextureMode(rt);
    ClearBackground(BLACK);
    EndTextureMode();

    return rt;
}

static void descarrega_render_float(RenderTexture2D& rt){
    rlUnloadTexture(rt.texture.id);
    rlUnloadFramebuffer(rt.id);
}

static Bloom cria_bloom(int scr_w, int scr_h, int scale){
    Bloom b{};

    b.width = scr_w / 2;
    b.height = scr_h / 2;

    b.layer = LoadRenderTexture(scr_w * scale, scr_h * scale);
    SetTextureFilter(b.layer.texture, TEXTURE_FILTER_BILINEAR);
    b.curr = carrega_render_float(b.width, b.height);
    b.history[0] = carrega_render_float(b.width, b.height);
    b.history[1] = carrega_render_float(b.width, b.height);
    b.source[0] = carrega_render_float(b.width, b.height);
    b.source[1] = carrega_render_float(b.width, b.height);
    b.temp = carrega_render_float(b.width, b.height);
    b.thin = carrega_render_float(b.width, b.height);
    b.small_font = carrega_render_float(b.width / 4, b.height / 4);
    b.small_time = carrega_render_float(b.width / 4, b.height / 4);
    b.wide = carrega_render_float(b.width / 4, b.height / 4);

    b.step1 = novo_shader("shaders/bloom_step1.glsl");
    b.step2 = novo_shader("shaders/bloom_step2.glsl");
    b.step3 = novo_shader("shaders/bloom_step3.glsl");
    b.step4 = novo_shader("shaders/bloom_step4.glsl");

    return b;
}

static void destroi_bloom(Bloom& b){
    UnloadRenderTexture(b.layer);

    for(RenderTexture2D* rt : {&b.curr, &b.history[0], &b.history[1], &b.source[0], &b.source[1], &b.temp, &b.thin, &b.small_font, &b.small_time, &b.wide}){
        descarrega_render_float(*rt);
    }

    for(Shaders* s : {&b.step1, &b.step2, &b.step3, &b.step4}){
        UnloadShader(s->rl_shader);
    }
}

template<typename F>
static void prev(const Shaders& s, Texture2D input, RenderTexture2D& output, F uniforms){
    BeginTextureMode(output);
    BeginShaderMode(s.rl_shader);
    uniforms(s.rl_shader);
    DrawTexturePro(input, {0, 0, (float)input.width, -(float)input.height},
                   {0, 0, (float)output.texture.width, (float)output.texture.height}, {0, 0}, 0.0f, WHITE);
    EndShaderMode();
    EndTextureMode();
}

static void uniforme_float(Shader sh, const char* name, float value){
    SetShaderValue(sh, GetShaderLocation(sh, name), &value, SHADER_UNIFORM_FLOAT);
}

static void uniforme_vec2(Shader sh, const char* name, float x, float y){
    float v[2] = {x, y};
    SetShaderValue(sh, GetShaderLocation(sh, name), v, SHADER_UNIFORM_VEC2);
}

static void clamp(Bloom& b, Texture2D input, RenderTexture2D& output){
    prev(b.step1, input, output, [&](Shader sh){
        uniforme_vec2(sh, "texel", 1.0f / input.width, 1.0f / input.height);
    });
}

static void blur(Bloom& b, RenderTexture2D& entrada, RenderTexture2D& time, RenderTexture2D& output, float sigma){
    prev(b.step4, entrada.texture, time, [&](Shader sh){
        uniforme_vec2(sh, "direction", 1.0f / entrada.texture.width, 0.0f);
        uniforme_float(sh, "sigma", sigma);
    });
    prev(b.step4, time.texture, output, [&](Shader sh){
        uniforme_vec2(sh, "direction", 0.0f, 1.0f / time.texture.height);
        uniforme_float(sh, "sigma", sigma);
    });
}

static void atualiza_bloom(Bloom& b, Texture2D input){
    float weight = 1.0f - powf(1.0f - 1.0f / 8.0, GetFrameTime() * 60.0f);
    float decay = powf(0.5f, GetFrameTime() / 0.4);

    RenderTexture2D& prev_r = b.history[b.curr_hist];
    RenderTexture2D& next_r = b.history[1 - b.curr_hist];
    RenderTexture2D& prev_source = b.source[b.curr_hist];
    RenderTexture2D& source = b.source[1 - b.curr_hist];

    clamp(b, input, b.curr);

    prev(b.step2, b.curr.texture, source, [&](Shader sh){
        SetShaderValueTexture(sh, GetShaderLocation(sh, "history"), prev_r.texture);
        SetShaderValueTexture(sh, GetShaderLocation(sh, "glow_history"), prev_source.texture);
        uniforme_float(sh, "decay", decay);
        uniforme_float(sh, "residual", 1.0f);
    });
    prev(b.step3, b.curr.texture, next_r, [&](Shader sh){
        SetShaderValueTexture(sh, GetShaderLocation(sh, "history"), prev_r.texture);
        uniforme_float(sh, "weight", weight);
    });
    b.curr_hist = 1 - b.curr_hist;

    float conv_f = hypotf(b.width, b.height) / hypotf(3840.0f, 2160.0f);
    blur(b, source, b.temp, b.thin, 30.0f * conv_f);

    clamp(b, source.texture, b.small_font);
    blur(b, b.small_font, b.small_time, b.wide, 140.0f * conv_f / 4.0f);
}

static const ResultadoBusca& resultado_atual(const Visualizacao& vis, const vector<Algoritmo>& algoritmos, const vector<ResultadoBusca>& resultados){
    return resultados[vis.search * algoritmos.size() + vis.method];
}

static int nos_revelados(const Visualizacao& vis, const ResultadoBusca& resultado){
    return min((int)vis.step, (int)resultado.nos_considerados.size());
}

struct Translate {
    double min_x, min_y, scale;
    float offset_x, offset_y;

    Vector2 operator()(const Vertice& v) const{
        return {offset_x + (float)((v.x - min_x) * scale), offset_y + (float)((v.y - min_y) * scale)};
    }
};

static Translate calcula_transformacao(const vector<Vertice>& vertices, float largura, float altura, float margem){
    double min_x = DBL_MAX, min_y = DBL_MAX, max_x = -DBL_MAX, max_y = -DBL_MAX;

    for(const Vertice& v : vertices){
        min_x = min(min_x, v.x);
        min_y = min(min_y, v.y);
        max_x = max(max_x, v.x);
        max_y = max(max_y, v.y);
    }

    double escala = min((largura - 2*margem) / (max_x - min_x), (altura - 2*margem) / (max_y - min_y));
    float desloc_x = (largura - (float)((max_x - min_x) * escala)) / 2;
    float desloc_y = (altura - (float)((max_y - min_y) * escala)) / 2;

    return {min_x, min_y, escala, desloc_x, desloc_y};
}

static Color cor_magma(float t){
    /* "emprestei" isso aqui do código do matplotlib */
    static const Color PARADAS[] = {
        {  0,   0,   4, 255}, { 28,  16,  68, 255}, { 79,  18, 123, 255},
        {129,  37, 129, 255}, {181,  54, 122, 255}, {229,  80, 100, 255},
        {251, 135,  97, 255}, {254, 194, 135, 255}, {252, 253, 191, 255},
    };
    const int N = sizeof(PARADAS) / sizeof(PARADAS[0]);

    t = fminf(fmaxf(t, 0.0f), 1.0f) * (N - 1);
    int i = min((int)t, N - 2);
    return ColorLerp(PARADAS[i], PARADAS[i + 1], t - i);
}

static bool caminho_visivel(const Visualizacao& vis, const ResultadoBusca& resultado){
    return resultado.sucesso && nos_revelados(vis, resultado) == (int)resultado.nos_considerados.size();
}

static void desenha_caminho(const Translate& tela, float e, const vector<Vertice>& vertices, const ResultadoBusca& resultado){
    for(size_t k = 1; k < resultado.caminho_escolhido.size(); k++){
        Vector2 a = tela(vertices[resultado.caminho_escolhido[k - 1]]);
        Vector2 b = tela(vertices[resultado.caminho_escolhido[k]]);
        DrawLineEx(a, b, 3.5 * e, {205, 165, 75, 255});
        DrawCircleV(a, 3.5 * e / 2, {205, 165, 75, 255});
    }
}

Visualizacao cria_visualizacao(const char* caminho_shader){
    Visualizacao vis{};
    vis.speed = 20.0;
    vis.scale = 2;
	vis.paused = true;
    vis.scene = LoadRenderTexture(GetScreenWidth() * vis.scale, GetScreenHeight() * vis.scale);
    SetTextureFilter(vis.scene.texture, TEXTURE_FILTER_BILINEAR);

    vis.post.filename = caminho_shader;
    carrega_shader(vis.post);

    vis.bloom = cria_bloom(GetScreenWidth(), GetScreenHeight(), vis.scale);

    return vis;
}

void destroi_visualizacao(Visualizacao& vis){
    destroi_bloom(vis.bloom);
    UnloadShader(vis.post.rl_shader);
    UnloadRenderTexture(vis.scene);
}

void atualiza_visualizacao(Visualizacao& vis, const vector<Algoritmo>& algoritmos, const vector<Busca>& buscas, const vector<ResultadoBusca>& resultados){
    int algoritmos_len = algoritmos.size();
    int buscas_len = buscas.size();
    bool reset = false;

    if(IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)){
        vis.method = (vis.method + 1) % algoritmos_len;
        reset = true;
    }
    if(IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)){
        vis.method = (vis.method + algoritmos_len - 1) % algoritmos_len;
        reset = true;
    }
    if(IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP)){
        vis.search = (vis.search + 1) % buscas_len;
        reset = true;
    }
    if(IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN)){
        vis.search = (vis.search + buscas_len - 1) % buscas_len;
        reset = true;
    }
    if(IsKeyPressed(KEY_R)){
        reset = true;
    }
    if(IsKeyPressed(KEY_SPACE)){
        vis.paused = !vis.paused;
    }
    if(IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD)){
        vis.speed *= 1.5;
    }
    if(IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT)){
        vis.speed /= 1.5;
    }

    const ResultadoBusca& res = resultado_atual(vis, algoritmos, resultados);

    if(reset){
        vis.step = 0.0;
		vis.paused = true;
    }
    else if(IsKeyPressed(KEY_ENTER)){
        vis.step = res.nos_considerados.size();
    }
    else if(!vis.paused){
        vis.step = min(vis.step + vis.speed * GetFrameTime(), (double)res.nos_considerados.size());
    }
}

void desenha_rede(const Visualizacao& vis, const vector<Vertice>& vertices, const vector<vector<Vizinho>>& adjacencias, const vector<Algoritmo>& algoritmos, const vector<Busca>& buscas, const vector<ResultadoBusca>& resultados){
    const float e = vis.scale;
    Translate canvas = calcula_transformacao(vertices, vis.scene.texture.width, vis.scene.texture.height, 30*e);
    const ResultadoBusca& res = resultado_atual(vis, algoritmos, resultados);
    const Busca& busca = buscas[vis.search];
    int revealed = nos_revelados(vis, res);

    ClearBackground(Color{10, 12, 20, 255});

    for(int i = 0; i < (int)adjacencias.size(); i++){
        for(const Vizinho& vizinho : adjacencias[i]){
            if(i < vizinho.vertice){
                DrawLineEx(canvas(vertices[i]), canvas(vertices[vizinho.vertice]), e, Color{90, 100, 130, 28});
            }
        }
    }
    for(const Vertice& v : vertices){
        DrawCircleV(canvas(v), 1.8f * e, Color{120, 130, 160, 255});
    }

    for(int k = 0; k < revealed; k++){
        float t = revealed > 1 ? (float)k / (revealed - 1) : 0.0f;
        Color cor = cor_magma(0.2f + 0.45f * t);
        int no = res.nos_considerados[k];
        int pai = res.pais_considerados[k];

        if(pai != -1){
            DrawLineEx(canvas(vertices[pai]), canvas(vertices[no]), 1.5*e, ColorAlpha(cor, 1.0f));
        }
        DrawCircleV(canvas(vertices[no]), 3.0f * e, cor);
    }

    if(caminho_visivel(vis, res)){
        desenha_caminho(canvas, e, vertices, res);
    }
    else if(revealed > 0){
        DrawCircleV(canvas(vertices[res.nos_considerados[revealed - 1]]), 5.4f * e, Color{255, 255, 255, 255});
    }

    DrawCircleV(canvas(vertices[busca.origem]), 7.0f * e, Color{60, 230, 120, 255});
    DrawCircleV(canvas(vertices[busca.destino]), 7.0f * e, Color{255, 70, 70, 255});
}

void processa_bloom(Visualizacao& vis, const vector<Vertice>& vertices, const vector<Algoritmo>& algoritmos, const vector<ResultadoBusca>& resultados){
    const float e = vis.scale;
    RenderTexture2D& layer = vis.bloom.layer;
    Translate canvas = calcula_transformacao(vertices, layer.texture.width, layer.texture.height, 30*e);
    const ResultadoBusca& resultado = resultado_atual(vis, algoritmos, resultados);

    BeginTextureMode(layer);
    ClearBackground(BLACK);
    if(caminho_visivel(vis, resultado)){
        desenha_caminho(canvas, e, vertices, resultado);
    }
    EndTextureMode();

    atualiza_bloom(vis.bloom, layer.texture);
}

void desenha_cena_com_shader(const Visualizacao& vis, const vector<Algoritmo>& algoritmos, const vector<ResultadoBusca>& resultados){
    const Shaders& s = vis.post;
    const ResultadoBusca& res = resultado_atual(vis, algoritmos, resultados);
    float time = GetTime();
    float resolution[2] = {(float)GetScreenWidth(), (float)GetScreenHeight()};
    float progress = res.nos_considerados.empty() ? 1.0f : (float)nos_revelados(vis, res) / res.nos_considerados.size();

    SetShaderValue(s.rl_shader, s.loc_time, &time, SHADER_UNIFORM_FLOAT);
    SetShaderValue(s.rl_shader, s.loc_resolution, resolution, SHADER_UNIFORM_VEC2);
    SetShaderValue(s.rl_shader, s.loc_progress, &progress, SHADER_UNIFORM_FLOAT);

    Rectangle source = {0, 0, (float)vis.scene.texture.width, -(float)vis.scene.texture.height};
    Rectangle target = {0, 0, resolution[0], resolution[1]};

    BeginShaderMode(s.rl_shader);
    SetShaderValueTexture(s.rl_shader, GetShaderLocation(s.rl_shader, "thin_bloom"), vis.bloom.thin.texture);
    SetShaderValueTexture(s.rl_shader, GetShaderLocation(s.rl_shader, "wide_bloom"), vis.bloom.wide.texture);
    DrawTexturePro(vis.scene.texture, source, target, {0, 0}, 0.0f, WHITE);
    EndShaderMode();
}

static bool eh_otimo(const Visualizacao& vis, const vector<Algoritmo>& algoritmos, const vector<ResultadoBusca>& resultados){
    const ResultadoBusca& atual = resultado_atual(vis, algoritmos, resultados);

    if(!atual.sucesso) return false;
    
    for(size_t a = 0; a < algoritmos.size(); a++){
        const ResultadoBusca& outro = resultados[vis.search * algoritmos.size() + a];
        if(outro.sucesso && outro.peso_final_escolhido < atual.peso_final_escolhido - 1e-9){
            return false;
        }
    }
    
    return true;
}

void desenha_hud(const Visualizacao& vis, const vector<Algoritmo>& algoritmos, [[maybe_unused]] const vector<Busca>& buscas, const vector<ResultadoBusca>& resultados){
    const ResultadoBusca& resultado = resultado_atual(vis, algoritmos, resultados);
    int x = 12, y = 12, tamanho = 20, linha = 24;

    Color cor_nome = eh_otimo(vis, algoritmos, resultados) ? Color{255, 232, 170, 255} : RAYWHITE;
    DrawText(algoritmos[vis.method].nome.c_str(), x, y, tamanho * 3 / 2, cor_nome);
    y += linha * 3 / 2;
    DrawText(TextFormat("Nós considerados: %d/%d", nos_revelados(vis, resultado), (int)resultado.nos_considerados.size()), x, y, tamanho, RAYWHITE);
    y += linha;
    if(resultado.sucesso){
        DrawText(TextFormat("Custo do caminho: %.1f", resultado.peso_final_escolhido), x, y, tamanho, RAYWHITE);
    }
    else{
        DrawText("Falhou (não chegou ao destino)", x, y, tamanho, RAYWHITE);
    }
    y += linha;
    DrawText(TextFormat("Velocidade: %.0f nós/s%s", vis.speed, vis.paused ? "  [PAUSADO]" : ""), x, y, tamanho, RAYWHITE);

    DrawText("esq/dir: mudar algoritmo | cima/baixo: mudar nó inicial e final | espaço: pausar | R: reiniciar | enter: resultado | +/-: mudar velocidade | Q: sair",
             x, GetScreenHeight() - linha, 16, ColorAlpha(RAYWHITE, 0.6f));
}
