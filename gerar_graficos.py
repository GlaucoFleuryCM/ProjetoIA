"""
Gera 1 figura por valor de lambda a partir de um CSV de benchmark de algoritmos de busca.

- Painel de distância: baseline = distância média do Dijkstra/A* (ótimos). Barras abaixo
  da baseline não são plotadas (resultado inválido: só contam os casos em que houve sucesso).
- Eixos Y de distância e de tempo são iguais em todas as figuras.

Uso:
    python gerar_graficos.py dados.csv
    python gerar_graficos.py dados.csv --saida graficos --mostrar
"""
import argparse
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

ALGORITMOS_OTIMOS = ["Dijkstra", "A*"]
TOLERANCIA = 1e-6  # tolerância relativa para comparar com a baseline


def calcular_baseline(df_lambda):
    """Distância média ótima (Dijkstra/A*) do lambda."""
    otimos = df_lambda[df_lambda["algoritmo"].isin(ALGORITMOS_OTIMOS)]
    if otimos.empty:
        return None
    return otimos["distancia_media"].min()


def mascara_validas(df_lambda, baseline):
    """True para as barras que ficam na baseline ou acima dela."""
    if baseline is None:
        return pd.Series(True, index=df_lambda.index)
    return df_lambda["distancia_media"] >= baseline * (1 - TOLERANCIA)


def limites_log(medias, desvios):
    """Limites (log) considerando as barras de erro."""
    medias = np.asarray(medias, dtype=float)
    desvios = np.asarray(desvios, dtype=float)
    minimo = (medias - np.minimum(desvios, medias * 0.999)).min()
    maximo = (medias + desvios).max()
    return minimo * 0.7, maximo * 1.5


def limites_globais(df):
    """Limites Y comuns a todos os lambdas (distância só com barras válidas)."""
    dist_m, dist_d = [], []
    for _, g in df.groupby("lambda"):
        validas = g[mascara_validas(g, calcular_baseline(g))]
        dist_m += validas["distancia_media"].tolist()
        dist_d += validas["distancia_desvio"].tolist()
    lim_dist = limites_log(dist_m, dist_d)
    lim_tempo = limites_log(df["tempo_medio_us"], df["tempo_desvio_us"])
    return lim_dist, lim_tempo


def barras_com_erro(ax, x, medias, desvios, cores, log=False):
    medias = np.asarray(medias, dtype=float)
    desvios = np.asarray(desvios, dtype=float)
    inferior = np.minimum(desvios, medias * 0.999) if log else desvios
    ax.bar(x, medias, yerr=[inferior, desvios], capsize=4,
           color=cores, edgecolor="black", linewidth=0.6)


def configurar_eixos(ax, titulo, ylabel, algoritmos, ylim, log=True):
    ax.set_title(titulo)
    ax.set_ylabel(ylabel)
    if log:
        ax.set_yscale("log")
    ax.set_ylim(*ylim)
    ax.set_xticks(range(len(algoritmos)))
    ax.set_xticklabels(algoritmos, rotation=30)
    ax.grid(axis="y", linestyle="--", alpha=0.4)


def gerar_grafico(df_lambda, lam, pasta_saida, lim_dist, lim_tempo):
    algoritmos = df_lambda["algoritmo"].tolist()
    x = np.arange(len(algoritmos))
    cores = plt.cm.tab10(x)

    fig, axes = plt.subplots(1, 3, figsize=(17, 5.5))
    fig.suptitle(f"λ = {lam}", fontsize=14, fontweight="bold")

    # 1) Distância média com baseline (Dijkstra/A*)
    ax = axes[0]
    baseline = calcular_baseline(df_lambda)
    validas = mascara_validas(df_lambda, baseline).to_numpy()
    barras_com_erro(ax, x[validas],
                    df_lambda["distancia_media"].to_numpy()[validas],
                    df_lambda["distancia_desvio"].to_numpy()[validas],
                    cores[validas], log=True)
    configurar_eixos(ax, "Distância média (± desvio)", "Distância (escala log)",
                     algoritmos, lim_dist)
    if baseline is not None:
        ax.axhline(baseline, color="red", linestyle="--", linewidth=1.5,
                   label=f"Ótimo: {baseline:.1f}")
        ax.legend(loc="upper right", fontsize=9)
    omitidos = [a for a, v in zip(algoritmos, validas) if not v]

    # 2) Tempo médio
    ax = axes[1]
    barras_com_erro(ax, x, df_lambda["tempo_medio_us"], df_lambda["tempo_desvio_us"],
                    cores, log=True)
    configurar_eixos(ax, "Tempo médio (± desvio)", "Tempo em µs (escala log)",
                     algoritmos, lim_tempo)

    # 3) Taxa de sucesso
    ax = axes[2]
    taxa = 100 * df_lambda["sucessos"] / df_lambda["buscas"]
    barras = ax.bar(x, taxa, color=cores, edgecolor="black", linewidth=0.6)
    configurar_eixos(ax, "Taxa de sucesso", "Sucesso (%)", algoritmos, (0, 110), log=False)
    for b, s, n in zip(barras, df_lambda["sucessos"], df_lambda["buscas"]):
        ax.text(b.get_x() + b.get_width() / 2, b.get_height() + 1.5, f"{s}/{n}",
                ha="center", va="bottom", fontsize=9)

    fig.tight_layout(rect=[0, 0, 1, 0.94])
    arquivo = pasta_saida / f"grafico_lambda_{lam}.png"
    fig.savefig(arquivo, dpi=150)
    print(f"Salvo: {arquivo}")
    return fig


def main():
    parser = argparse.ArgumentParser(description="Gera um gráfico por lambda.")
    parser.add_argument("csv", help="Caminho do arquivo CSV")
    parser.add_argument("--saida", default="graficos", help="Pasta de saída (padrão: graficos)")
    parser.add_argument("--mostrar", action="store_true", help="Exibe os gráficos na tela")
    args = parser.parse_args()

    pasta = Path(args.saida)
    pasta.mkdir(parents=True, exist_ok=True)
    df = pd.read_csv(args.csv)

    lim_dist, lim_tempo = limites_globais(df)
    for lam, grupo in df.groupby("lambda", sort=True):
        gerar_grafico(grupo, lam, pasta, lim_dist, lim_tempo)

    if args.mostrar:
        plt.show()


if __name__ == "__main__":
    main()