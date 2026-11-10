"""
Gera um único gráfico de linhas (3 painéis) comparando todos os lambdas.
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
TOLERANCIA = 1e-6   # tolerância relativa ao comparar com a baseline
DESLOCAMENTO = 0.07  # pequeno deslocamento horizontal entre lambdas (evita sobrepor barras de erro)


def calcular_baseline(df_lambda):
    """Distância média ótima (Dijkstra/A*) de um lambda."""
    otimos = df_lambda[df_lambda["algoritmo"].isin(ALGORITMOS_OTIMOS)]
    return None if otimos.empty else otimos["distancia_media"].min()


def erro_assimetrico(medias, desvios, log):
    """Em escala log, impede que a barra de erro inferior chegue a zero/negativo."""
    inferior = np.minimum(desvios, medias * 0.999) if log else desvios
    return np.vstack([inferior, desvios])


def plotar_linha(ax, x, medias, desvios, cor, rotulo, log=True):
    x = np.asarray(x, dtype=float)
    medias = np.asarray(medias, dtype=float)
    desvios = np.asarray(desvios, dtype=float)
    # Remove pontos omitidos (NaN) para que os pontos adjacentes sejam ligados
    validos = ~np.isnan(medias)
    x, medias, desvios = x[validos], medias[validos], desvios[validos]
    ax.errorbar(x, medias, yerr=erro_assimetrico(medias, desvios, log),
                fmt="o-", color=cor, capsize=3, linewidth=1.8, markersize=6, label=rotulo)


def main():
    parser = argparse.ArgumentParser(description="Gráfico de linhas com todos os lambdas.")
    parser.add_argument("csv", help="Caminho do arquivo CSV")
    parser.add_argument("--saida", default="graficos", help="Pasta de saída (padrão: graficos)")
    parser.add_argument("--mostrar", action="store_true", help="Exibe o gráfico na tela")
    args = parser.parse_args()

    pasta = Path(args.saida)
    pasta.mkdir(parents=True, exist_ok=True)
    df = pd.read_csv(args.csv)

    lambdas = sorted(df["lambda"].unique())
    algoritmos = list(dict.fromkeys(df["algoritmo"]))  # mantém a ordem do CSV
    x = np.arange(len(algoritmos))
    cores = plt.cm.tab10(np.arange(len(lambdas)))

    fig, axes = plt.subplots(1, 3, figsize=(18, 6))
    fig.suptitle("Comparação dos algoritmos por λ", fontsize=14, fontweight="bold")
    ax_dist, ax_tempo, ax_suc = axes
    omitidos = []

    for i, (lam, cor) in enumerate(zip(lambdas, cores)):
        g = df[df["lambda"] == lam].set_index("algoritmo").reindex(algoritmos)
        xs = x + (i - (len(lambdas) - 1) / 2) * DESLOCAMENTO
        rotulo = f"λ = {lam}"

        # Distância: pontos abaixo da baseline viram NaN e são removidos em plotar_linha
        dist_m = g["distancia_media"].to_numpy(dtype=float)
        dist_d = g["distancia_desvio"].to_numpy(dtype=float)
        baseline = calcular_baseline(df[df["lambda"] == lam])
        if baseline is not None:
            abaixo = dist_m < baseline * (1 - TOLERANCIA)
            omitidos += [f"λ={lam}: {a}" for a in np.array(algoritmos)[abaixo]]
            dist_m = np.where(abaixo, np.nan, dist_m)
            dist_d = np.where(abaixo, np.nan, dist_d)
            ax_dist.axhline(baseline, color=cor, linestyle="--", linewidth=1.2, alpha=0.8)
        plotar_linha(ax_dist, xs, dist_m, dist_d, cor, rotulo)

        # Tempo
        plotar_linha(ax_tempo, xs, g["tempo_medio_us"], g["tempo_desvio_us"], cor, rotulo)

        # Taxa de sucesso (%)
        taxa = 100 * g["sucessos"] / g["buscas"]
        ax_suc.plot(x, taxa, "o-", color=cor, linewidth=1.8, markersize=6, label=rotulo)

    # Distância
    ax_dist.set_title("Distância média (± desvio)")
    ax_dist.set_ylabel("Distância (escala log)")
    ax_dist.set_yscale("log")
    handles, labels = ax_dist.get_legend_handles_labels()
    handles.append(plt.Line2D([], [], color="gray", linestyle="--"))
    ax_dist.legend(handles, labels, fontsize=9, loc="upper right")

    # Tempo
    ax_tempo.set_title("Tempo médio (± desvio)")
    ax_tempo.set_ylabel("Tempo em µs (escala log)")
    ax_tempo.set_yscale("log")
    ax_tempo.legend(fontsize=9)

    # Sucesso
    ax_suc.set_title("Taxa de sucesso")
    ax_suc.set_ylabel("Sucesso (%)")
    ax_suc.set_ylim(0, 105)
    ax_suc.legend(fontsize=9, loc="lower left")

    for ax in axes:
        ax.set_xticks(x)
        ax.set_xticklabels(algoritmos, rotation=30)
        ax.grid(True, linestyle="--", alpha=0.4)

    fig.tight_layout(rect=[0, 0, 1, 0.94])
    arquivo = pasta / "grafico_lambdas.png"
    fig.savefig(arquivo, dpi=150)
    print(f"Salvo: {arquivo}")

    if args.mostrar:
        plt.show()


if __name__ == "__main__":
    main()