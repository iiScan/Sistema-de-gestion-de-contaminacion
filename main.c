#include <stdio.h>
#include <string.h>
#include <time.h>
#include "funciones.h"

void mostrarAlertasRecomendaciones(Zona zonas[], Prediccion preds[], int n)
{
    int idx = seleccionarZona();

    if (preds[idx].zonaId != idx)
    {
        printf("[!] No hay predicciones registradas para %s.\n", zonas[idx].nombre);
        return;
    }

    printf("\n--- Alertas y recomendaciones para %s ---\n", zonas[idx].nombre);
    printf("Contaminante crítico: %s\n", preds[idx].contaminante);
    printf("Valor estimado: %.2f\n", preds[idx].valorPredicho);
    printf("Nivel: %s\n", preds[idx].nivel);
    printf("Alerta activa: %s\n", preds[idx].alertaActiva ? "SI" : "NO");

    generarRecomendaciones(&preds[idx], &zonas[idx]);
}


int menu()
{
    printf("\n=== Sistema de Calidad del Aire de Quito ===\n");
    printf("Seleccione una opcion\n");
    printf("1. Capturar datos actuales de una zona\n");
    printf("2. Consultar / administrar historial\n");
    printf("3. Predecir niveles (proximas 24h)\n");
    printf("4. Listar zonas y alertas\n");
    printf("5. Exportar reporte\n");
    printf("6. Ver alertas y recomendaciones por zona\n");
    printf(">> ");
    return validarIntRango(1, 6);
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
            idx = seleccionarZona();

            {
                time_t ahora = time(NULL);
                struct tm *fecha = localtime(&ahora);
                if (fecha != NULL)
                {
                    reg.dia = fecha->tm_mday;
                    reg.mes = fecha->tm_mon + 1;
                    reg.anio = fecha->tm_year + 1900;
                }
                else
                {
                    reg.dia = reg.mes = reg.anio = 0;
                }

                printf("\nFecha del registro: %02d/%02d/%04d\n",
                       reg.dia, reg.mes, reg.anio);
            }

            capturarTelemetria(&zonas[idx]);
            capturarMeteorologiaConsola(&met);

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
            printf("\n--- Historial ---\n");
            printf("1. Ver historial de una zona\n");
            printf("2. Reiniciar historial de una zona\n");
            printf("3. Editar registro historico por fecha\n");
            printf(">> ");
            opc2 = validarIntRango(1, 3);

            idx = seleccionarZona();
            if (opc2 == 1)
            {
                verHistoricoZona(idx);
            }
            else if (opc2 == 2)
            {
                reiniciarHistorialZona(idx);
            }
            else
            {
                editarRegistroHistorico(idx);
            }
            break;

        case 3:
            printf("\n--- Prediccion de niveles ---\n");
            printf("1. Predecir para una zona especifica\n");
            printf("2. Predecir para todas las zonas\n");
            printf(">> ");
            opc2 = validarIntRango(1, 2);

            if (opc2 == 1)
            {
                idx = seleccionarZona();
                capturarMeteorologiaConsola(&met);
                zonas[idx].promedioHistorico = calcularPromedioHistorico(idx);

                preds[idx].zonaId = idx;
                strcpy(preds[idx].contaminante, seleccionarContaminanteCritico(&zonas[idx]));
                preds[idx].valorPredicho = calcularPrediccion(&zonas[idx], &met);
                clasificarNivel(&preds[idx]);
                generarRecomendaciones(&preds[idx], &zonas[idx]);
                prediccionesDisponibles = 1;
            }
            else
            {
                capturarMeteorologiaConsola(&met);
                for (i = 0; i < NUM_ZONAS; i++)
                {
                    zonas[i].promedioHistorico = calcularPromedioHistorico(i);
                    preds[i].zonaId = i;
                    strcpy(preds[i].contaminante, seleccionarContaminanteCritico(&zonas[i]));
                    preds[i].valorPredicho = calcularPrediccion(&zonas[i], &met);
                    clasificarNivel(&preds[i]);
                }
                prediccionesDisponibles = 1;
            }
            break;

        case 4:
            listarZonas(zonas, NUM_ZONAS);
            break;

        case 5:
            if (!prediccionesDisponibles)
            {
                printf("[!] Primero calcule las predicciones.\n");
                break;
            }
            exportarReporte(zonas, preds, NUM_ZONAS, -1);
            break;

        case 6:
            if (!prediccionesDisponibles)
            {
                printf("[!] Primero calcule las predicciones.\n");
                break;
            }
            mostrarAlertasRecomendaciones(zonas, preds, NUM_ZONAS);
            break;

        default:
            printf("[!] Opcion no valida.\n");
            break;
        }

        printf("\nDesea seleccionar otra opcion 1.Si / 2.No: ");
        opc2 = validarIntRango(1, 2);
    } while (opc2 == 1);

    return 0;
}