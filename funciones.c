#include <stdio.h>
#include <string.h>
#include "funciones.h"

static int abrirArchivoConFallback(const char *rutaPreferida, const char *rutaAlternativa, const char *modo, FILE **archivo)
{
    *archivo = fopen(rutaPreferida, modo);
    if (*archivo != NULL)
    {
        return 1;
    }

    *archivo = fopen(rutaAlternativa, modo);
    return (*archivo != NULL);
}

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

int seleccionarZona(void)
{
    printf("\n--- Seleccione una zona ---\n");
    printf("0. Zona Sur\n");
    printf("1. Centro Historico\n");
    printf("2. Centro Norte\n");
    printf("3. Zona Norte\n");
    printf("4. Valles Aledanos\n");
    printf(">> ");
    return validarIntRango(0, NUM_ZONAS - 1);
}

float validarFloatRango(float a, float b)
{
    int aux;
    float n;

    do
    {
        printf(">> ");
        aux = scanf("%f", &n);
        while (getchar() != '\n');

        if (aux != 1 || n < a || n > b)
        {
            printf("Error: valor ingresado incorrecto\n");
        }
    } while (aux != 1 || n < a || n > b);

    return n;
}

int validarIntRango(int a, int b)
{
    int aux;
    int n;

    do
    {
        printf(">> ");
        aux = scanf("%d", &n);
        while (getchar() != '\n');

        if (aux != 1 || n < a || n > b)
        {
            printf("Error: valor ingresado incorrecto\n");
        }
    } while (aux != 1 || n < a || n > b);

    return n;
}

void leerCadena(char *cadena, int n)
{
    if (fgets(cadena, n, stdin) == NULL)
    {
        cadena[0] = '\0';
        return;
    }

    int len = (int)strlen(cadena);
    while (len > 0 && (cadena[len - 1] == '\n' || cadena[len - 1] == '\r'))
    {
        cadena[--len] = '\0';
    }
}

void inicializarZonas(Zona zonas[])
{
    for (int i = 0; i < NUM_ZONAS; i++)
    {
        zonas[i].actual.pm25 = 0;
        zonas[i].actual.no2 = 0;
        zonas[i].actual.so2 = 0;
        zonas[i].actual.co2 = 0;
        zonas[i].promedioHistorico = 0;
    }

    zonas[ZONA_SUR].id = ZONA_SUR;
    zonas[ZONA_SUR].factorZona = 18;
    strcpy(zonas[ZONA_SUR].nombre, "Zona Sur");
    strcpy(zonas[ZONA_SUR].nivelAlerta, "NORMAL");

    zonas[ZONA_CENTRO_HIST].id = ZONA_CENTRO_HIST;
    zonas[ZONA_CENTRO_HIST].factorZona = 12;
    strcpy(zonas[ZONA_CENTRO_HIST].nombre, "Centro Historico");
    strcpy(zonas[ZONA_CENTRO_HIST].nivelAlerta, "NORMAL");

    zonas[ZONA_CENTRO_NORTE].id = ZONA_CENTRO_NORTE;
    zonas[ZONA_CENTRO_NORTE].factorZona = 8;
    strcpy(zonas[ZONA_CENTRO_NORTE].nombre, "Centro Norte");
    strcpy(zonas[ZONA_CENTRO_NORTE].nivelAlerta, "NORMAL");

    zonas[ZONA_NORTE].id = ZONA_NORTE;
    zonas[ZONA_NORTE].factorZona = 4;
    strcpy(zonas[ZONA_NORTE].nombre, "Zona Norte");
    strcpy(zonas[ZONA_NORTE].nivelAlerta, "NORMAL");

    zonas[ZONA_VALLES].id = ZONA_VALLES;
    zonas[ZONA_VALLES].factorZona = 0;
    strcpy(zonas[ZONA_VALLES].nombre, "Valles Aledanos");
    strcpy(zonas[ZONA_VALLES].nivelAlerta, "NORMAL");
}

static int leerUltimosRegistros(int zonaId, RegistroHistorico registros[], int maxRegistros)
{
    char rutaPrincipal[80];
    char rutaAlternativa[80];
    FILE *f = NULL;
    RegistroHistorico reg;
    int contador = 0;

    sprintf(rutaPrincipal, "datos/historico_zona%d.dat", zonaId);
    sprintf(rutaAlternativa, "historico_zona%d.dat", zonaId);

    if (!abrirArchivoConFallback(rutaPrincipal, rutaAlternativa, "rb", &f))
    {
        return 0;
    }

    while (fread(&reg, sizeof(RegistroHistorico), 1, f))
    {
        if (contador < maxRegistros)
        {
            registros[contador++] = reg;
        }
        else
        {
            for (int j = 1; j < maxRegistros; j++)
            {
                registros[j - 1] = registros[j];
            }
            registros[maxRegistros - 1] = reg;
        }
    }

    fclose(f);
    return contador;
}

void capturarTelemetria(Zona *zona)
{
    printf("--- Datos actuales: %s ---\n", zona->nombre);
    printf("PM2.5 (Material particulado fino) [0-300 ug/m3]: ");
    zona->actual.pm25 = validarFloatRango(0, 300);

    printf("NO2 (Dioxido de nitrogeno) [0-400 ug/m3]: ");
    zona->actual.no2 = validarFloatRango(0, 400);

    printf("SO2 (Dioxido de azufre) [0-500 ug/m3]: ");
    zona->actual.so2 = validarFloatRango(0, 500);

    printf("CO2 (Dioxido de carbono) [300-2000 ppm]: ");
    zona->actual.co2 = validarFloatRango(300, 2000);
}

void capturarMeteorologiaConsola(Meteorologia *met)
{
    printf("\n--- Condiciones Meteorologicas ---\n");
    printf("Velocidad del viento (0-120 km/h): ");
    met->velocidadViento = validarFloatRango(0, 120);

    printf("Humedad relativa (0-100%%): ");
    met->humedadRelativa = validarFloatRango(0, 100);

    printf("Temperatura (-5 a 35 C): ");
    met->temperatura = validarFloatRango(-5, 35);

    printf("Lluvia (0=No / 1=Si): ");
    met->lluvia = validarIntRango(0, 1);
}

void guardarRegistro(RegistroHistorico *reg, int zonaId)
{
    char rutaPrincipal[80];
    char rutaAlternativa[80];
    FILE *f = NULL;

    sprintf(rutaPrincipal, "datos/historico_zona%d.dat", zonaId);
    sprintf(rutaAlternativa, "historico_zona%d.dat", zonaId);

    if (!abrirArchivoConFallback(rutaPrincipal, rutaAlternativa, "ab+", &f))
    {
        printf("[!] No se pudo abrir el archivo de historial de la zona %d\n", zonaId);
        return;
    }

    fwrite(reg, sizeof(RegistroHistorico), 1, f);
    fclose(f);
}

float calcularPromedioHistorico(int zonaId)
{
    RegistroHistorico registros[DIAS_HISTORICOS];
    int registrosLeidos = leerUltimosRegistros(zonaId, registros, DIAS_HISTORICOS);

    if (registrosLeidos == 0)
    {
        return 0;
    }

    float suma = 0;
    for (int i = 0; i < registrosLeidos; i++)
    {
        suma += registros[i].pm25;
    }

    return suma / registrosLeidos;
}

float calcularPrediccion(Zona *zona, Meteorologia *met)
{
    float Zf = zona->factorZona;
    float prediccion;

    if (zona->id == ZONA_VALLES && met->temperatura < 10)
    {
        Zf += 15;
        printf("[!] Inversion termica en %s. Factor zona: %.1f\n", zona->nombre, Zf);
    }

    prediccion = (zona->promedioHistorico * W_HISTORICO)
              - (met->velocidadViento * W_VIENTO)
              + (met->humedadRelativa * W_HUMEDAD)
              + (met->temperatura * W_TEMPERATURA)
              + Zf
              - (met->lluvia * W_LLUVIA);

    if (met->lluvia == 1)
    {
        prediccion /= 2;
    }

    return (prediccion < 0) ? 0 : prediccion;
}

void clasificarNivel(Prediccion *pred)
{
    if (pred->valorPredicho <= 15)
    {
        strcpy(pred->nivel, "NORMAL");
        pred->alertaActiva = 0;
    }
    else if (pred->valorPredicho <= 35)
    {
        strcpy(pred->nivel, "MODERADO");
        pred->alertaActiva = 1;
    }
    else if (pred->valorPredicho <= 55)
    {
        strcpy(pred->nivel, "DANINO");
        pred->alertaActiva = 1;
    }
    else
    {
        strcpy(pred->nivel, "PELIGROSO");
        pred->alertaActiva = 1;
    }

    if (pred->alertaActiva)
    {
        printf("[!] Zona %d - %s: %.2f ug/m3\n",
               pred->zonaId,
               pred->contaminante,
               pred->valorPredicho);
    }
}

void listarZonas(Zona zonas[], int n)
{
    printf("#\t\tNombre\t\t\t\tPM2.5\t\tNivel\n");
    for (int i = 0; i < n; i++)
    {
        printf("%d\t\t%-25s\t%.2f\t\t%s\n",
               i,
               zonas[i].nombre,
               zonas[i].actual.pm25,
               zonas[i].nivelAlerta);
    }
}

void generarRecomendaciones(Prediccion *pred, Zona *zona)
{
    if (!pred->alertaActiva)
    {
        return;
    }

    printf("\n=== RECOMENDACIONES PARA %s ===\n", zona->nombre);

    if (strcmp(pred->contaminante, "PM2.5") == 0)
    {
        if (zona->id == ZONA_SUR)
        {
            printf("  [!] Restriccion de emisiones industriales en Zona Sur.\n");
            printf("  [!] Cierre temporal de plantas contaminantes.\n");
        }

        if (strcmp(pred->nivel, "PELIGROSO") == 0)
        {
            printf("  [!] Suspension de actividades al aire libre.\n");
            printf("  [!] Uso obligatorio de mascarillas.\n");
            printf("  [!] Cierre preventivo de escuelas y espacios publicos.\n");
        }
    }

    if (strcmp(pred->contaminante, "CO2") == 0 || strcmp(pred->contaminante, "NO2") == 0)
    {
        if (zona->id == ZONA_CENTRO_NORTE || zona->id == ZONA_CENTRO_HIST)
        {
            printf("  [!] Restriccion vehicular ampliada (Pico y Placa).\n");
            printf("  [!] Incentivo de teletrabajo para empresas del sector.\n");
        }

        if (zona->id == ZONA_CENTRO_NORTE && strcmp(pred->nivel, "DANINO") == 0)
        {
            printf("  [!]  ALERTA: Riesgo de formacion de Ozono Troposferico (O3).\n");
            printf("  [!]  Reduccion urgente de emisiones de NOx en la zona.\n");
        }
    }

    if (strcmp(pred->contaminante, "SO2") == 0 && zona->id == ZONA_SUR)
    {
        printf("  [!] Inspeccion industrial de emergencia por SO2.\n");
        printf("  [!] Coordinacion con autoridades ambientales distritales.\n");
    }
}

static int buscarRegistroPorFecha(int zonaId, int dia, int mes, int anio, RegistroHistorico *registro, long *pos)
{
    char rutaPrincipal[80];
    char rutaAlternativa[80];
    FILE *f = NULL;

    sprintf(rutaPrincipal, "datos/historico_zona%d.dat", zonaId);
    sprintf(rutaAlternativa, "historico_zona%d.dat", zonaId);

    if (!abrirArchivoConFallback(rutaPrincipal, rutaAlternativa, "rb", &f))
    {
        return 0;
    }

    while (fread(registro, sizeof(RegistroHistorico), 1, f))
    {
        *pos = ftell(f) - (long)sizeof(RegistroHistorico);
        if (registro->dia == dia && registro->mes == mes && registro->anio == anio)
        {
            fclose(f);
            return 1;
        }
    }

    fclose(f);
    return 0;
}

void reiniciarHistorialZona(int zonaId)
{
    char rutaPrincipal[80];
    char rutaAlternativa[80];
    FILE *f = NULL;

    sprintf(rutaPrincipal, "datos/historico_zona%d.dat", zonaId);
    sprintf(rutaAlternativa, "historico_zona%d.dat", zonaId);

    if (!abrirArchivoConFallback(rutaPrincipal, rutaAlternativa, "wb", &f))
    {
        printf("[!] No se pudo reiniciar el historial de la zona %d\n", zonaId);
        return;
    }

    fclose(f);
    printf("[!] Historial de la zona %d reiniciado correctamente.\n", zonaId);
}

int editarRegistroHistorico(int zonaId)
{
    RegistroHistorico reg;
    long posicion;
    int dia, mes, anio;

    printf("\n--- Editar registro historico ---\n");
    printf("Dia [1-31]: ");
    dia = validarIntRango(1, 31);
    printf("Mes [1-12]: ");
    mes = validarIntRango(1, 12);
    printf("Anio [2024-2030]: ");
    anio = validarIntRango(2024, 2030);

    if (!buscarRegistroPorFecha(zonaId, dia, mes, anio, &reg, &posicion))
    {
        printf("[!] No se encontro un registro con esa fecha en la zona %d.\n", zonaId);
        return 0;
    }

    printf("Registro encontrado: PM2.5=%.2f NO2=%.2f SO2=%.2f CO2=%.2f TEMP=%.1f HUM=%.1f VIENTO=%.1f LLUVIA=%s\n",
           reg.pm25, reg.no2, reg.so2, reg.co2, reg.temperatura, reg.humedad, reg.viento,
           reg.lluvia ? "Si" : "No");

    printf("\nIngrese los nuevos datos:\n");
    printf("PM2.5 (0-300 ug/m3): ");
    reg.pm25 = validarFloatRango(0, 300);
    printf("NO2 (0-400 ug/m3): ");
    reg.no2 = validarFloatRango(0, 400);
    printf("SO2 (0-500 ug/m3): ");
    reg.so2 = validarFloatRango(0, 500);
    printf("CO2 (300-2000 ppm): ");
    reg.co2 = validarFloatRango(300, 2000);
    printf("Temperatura (-5 a 35 C): ");
    reg.temperatura = validarFloatRango(-5, 35);
    printf("Humedad relativa (0-100%%): ");
    reg.humedad = validarFloatRango(0, 100);
    printf("Velocidad del viento (0-120 km/h): ");
    reg.viento = validarFloatRango(0, 120);
    printf("Lluvia (0=No / 1=Si): ");
    reg.lluvia = validarIntRango(0, 1);

    char rutaPrincipal[80];
    char rutaAlternativa[80];
    FILE *f = NULL;

    sprintf(rutaPrincipal, "datos/historico_zona%d.dat", zonaId);
    sprintf(rutaAlternativa, "historico_zona%d.dat", zonaId);

    if (!abrirArchivoConFallback(rutaPrincipal, rutaAlternativa, "rb+", &f))
    {
        printf("[!] No se pudo abrir el archivo para editar.\n");
        return 0;
    }

    fseek(f, posicion, SEEK_SET);
    fwrite(&reg, sizeof(RegistroHistorico), 1, f);
    fclose(f);

    printf("[!] Registro actualizado correctamente.\n");
    return 1;
}

const char *seleccionarContaminanteCritico(Zona *zona)
{
    float ratios[4];
    ratios[0] = zona->actual.pm25 / 300.0f;
    ratios[1] = zona->actual.no2  / 400.0f;
    ratios[2] = zona->actual.so2  / 500.0f;
    ratios[3] = (zona->actual.co2 - 300.0f) / 1700.0f;

    int indiceMax = 0;
    for (int i = 1; i < 4; i++)
    {
        if (ratios[i] > ratios[indiceMax])
        {
            indiceMax = i;
        }
    }

    switch (indiceMax)
    {
        case 0: return "PM2.5";
        case 1: return "NO2";
        case 2: return "SO2";
        case 3: return "CO2";
        default: return "PM2.5";
    }
}

void exportarReporte(Zona zonas[], Prediccion preds[], int n, int zonaId)
{
    char rutaPrincipal[80];
    char rutaAlternativa[80];
    FILE *f = NULL;
    int filas = 0;

    sprintf(rutaPrincipal, "datos/reporte_calidad_aire.txt");
    sprintf(rutaAlternativa, "reporte_calidad_aire.txt");

    for (int i = 0; i < n; i++)
    {
        if (zonaId >= 0 && preds[i].zonaId != zonaId)
            continue;
        filas++;
    }

    if (filas == 0)
    {
        printf("[!] No hay predicciones disponibles para el reporte.\n");
        return;
    }

    if (!abrirArchivoConFallback(rutaPrincipal, rutaAlternativa, "a", &f))
    {
        printf("[!] No se pudo crear el archivo de reporte.\n");
        return;
    }

    fprintf(f, "\n");
    fprintf(f, "============================================================\n");
    fprintf(f, "REPORTE DE CALIDAD DEL AIRE - QUITO\n");
    fprintf(f, "============================================================\n");
    fprintf(f, "ZONA                     | CONTAMINANTE | VALOR (ug/m3) | NIVEL   \n");
    fprintf(f, "--------------------------+--------------+---------------+---------\n");

    for (int i = 0; i < n; i++)
    {
        if (zonaId >= 0 && preds[i].zonaId != zonaId)
            continue;

        fprintf(f, "%-25s | %-12s | %13.2f | %-7s\n",
                zonas[preds[i].zonaId].nombre,
                preds[i].contaminante,
                preds[i].valorPredicho,
                preds[i].nivel);
    }

    fprintf(f, "============================================================\n");
    fclose(f);

    printf("[!] Reporte guardado en datos/reporte_calidad_aire.txt\n");
}

void verHistoricoZona(int zonaId)
{
    RegistroHistorico registros[DIAS_HISTORICOS];
    int contador = leerUltimosRegistros(zonaId, registros, DIAS_HISTORICOS);

    if (contador == 0)
    {
        printf("[!] No hay registros historicos para la zona %d.\n", zonaId);
        return;
    }

    printf("\n");
    printf("+------------+--------+-------+-------+-------+--------+--------+--------+---------+\n");
    printf("|   FECHA    | PM2.5  |  NO2  |  SO2  |  CO2  | TEMP   | HUM    | VIENTO | LLUVIA  |\n");
    printf("+------------+--------+-------+-------+-------+--------+--------+--------+---------+\n");

    for (int i = 0; i < contador; i++)
    {
        RegistroHistorico reg = registros[i];
        printf("| %02d/%02d/%04d | %6.2f | %5.2f | %5.2f | %5.2f | %6.1f | %6.1f | %6.1f | %-7s |\n",
               reg.dia,
               reg.mes,
               reg.anio,
               reg.pm25,
               reg.no2,
               reg.so2,
               reg.co2,
               reg.temperatura,
               reg.humedad,
               reg.viento,
               reg.lluvia == 1 ? "Si" : "No");
    }

    printf("+------------+--------+-------+-------+-------+--------+--------+--------+---------+\n");
    printf("[!] Mostrando los ultimos %d registros de la zona %d.\n", contador, zonaId);
}