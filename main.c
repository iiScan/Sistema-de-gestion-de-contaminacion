#include <stdio.h>
#include <string.h>
#include "funciones.h"

int menu()
{
    printf("\n=== Sistema de Calidad del Aire de Quito ===\n");
    printf("Seleccione una opcion\n");
    printf("1. Capturar datos actuales de una zona\n");
    printf("2. Calcular promedios historicos\n");
    printf("3. Predecir niveles (proximas 24h)\n");
    printf("4. Listar zonas y alertas\n");
    printf("5. Exportar reporte\n");
    printf(">> ");
    return (int)validarFloatRango(1, 5);
}

int main(int argc, char *argv[])
{
    int opc, opc2, idx, i;
    int prediccionesDisponibles = 0;

    Zona zonas[NUM_ZONAS];
    Meteorologia met;
    Prediccion preds[NUM_ZONAS];
    RegistroHistorico reg;

    inicializarZonas(zonas);

    for (i = 0; i < NUM_ZONAS; i++)
    {
        preds[i].zonaId = -1;
        strcpy(preds[i].contaminante, "PM2.5");
        preds[i].valorPredicho = 0;
        strcpy(preds[i].nivel, "NORMAL");
        preds[i].alertaActiva = 0;
    }

    do
    {
        opc = menu();

        switch (opc)
        {
        case 1:
            printf("Seleccione zona (0=Sur, 1=CentroHist, 2=CentroNorte, 3=Norte, 4=Valles): ");
            idx = (int)validarFloatRango(0, NUM_ZONAS - 1);

            capturarTelemetria(&zonas[idx]);
            capturarMeteorologiaConsola(&met);

            reg.dia = 28;
            reg.mes = 6;
            reg.anio = 2026;
            reg.zonaId = idx;
            reg.pm25 = zonas[idx].actual.pm25;
            reg.no2 = zonas[idx].actual.no2;
            reg.so2 = zonas[idx].actual.so2;
            reg.co2 = zonas[idx].actual.co2;
            reg.temperatura = met.temperatura;
            reg.humedad = met.humedadRelativa;
            reg.viento = met.velocidadViento;
            reg.lluvia = met.lluvia;

            guardarRegistro(&reg, idx);
            break;

        case 2:
            for (i = 0; i < NUM_ZONAS; i++)
            {
                zonas[i].promedioHistorico = calcularPromedioHistorico(i);
                printf("Zona %-20s: promedio PM2.5 = %.2f ug/m3\n",
                       zonas[i].nombre,
                       zonas[i].promedioHistorico);
            }
            break;

        case 3:
            capturarMeteorologiaConsola(&met);

            if (met.lluvia == 1)
            {
                printf("[AVISO] Lluvia activa. Concentracion reducida por lavado atmosferico.\n");
            }

            for (i = 0; i < NUM_ZONAS; i++)
            {
                zonas[i].promedioHistorico = calcularPromedioHistorico(i);

                preds[i].zonaId = i;
                strcpy(preds[i].contaminante, "PM2.5");
                preds[i].valorPredicho = calcularPrediccion(&zonas[i], &met);
                clasificarNivel(&preds[i]);
                strcpy(zonas[i].nivelAlerta, preds[i].nivel);
                generarRecomendaciones(&preds[i], &zonas[i]);
            }

            prediccionesDisponibles = 1;
            break;

        case 4:
            listarZonas(zonas, NUM_ZONAS);
            break;

        case 5:
            if (!prediccionesDisponibles)
            {
                printf("[INFO] Primero debe ejecutar la opcion 3 para generar predicciones.\n");
                break;
            }
            exportarReporte(zonas, preds, NUM_ZONAS);
            break;

        default:
            printf("No existe la opcion\n");
            break;
        }

        printf("\nDesea seleccionar otra opcion 1.Si / 2.No: ");
        opc2 = (int)validarFloatRango(1, 2);
    } while (opc2 == 1);

    return 0;
}