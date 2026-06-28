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
    met->lluvia = (int)validarFloatRango(0, 1);
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
        printf("[ERROR] No se pudo abrir el archivo de historial de la zona %d\n", zonaId);
        return;
    }

    fwrite(reg, sizeof(RegistroHistorico), 1, f);
    fclose(f);
}

float calcularPromedioHistorico(int zonaId)
{
    char rutaPrincipal[80];
    char rutaAlternativa[80];
    RegistroHistorico reg;
    float suma = 0;
    int contador = 0;
    FILE *f = NULL;

    sprintf(rutaPrincipal, "datos/historico_zona%d.dat", zonaId);
    sprintf(rutaAlternativa, "historico_zona%d.dat", zonaId);

    if (!abrirArchivoConFallback(rutaPrincipal, rutaAlternativa, "rb+", &f))
    {
        return 0;
    }

    while (fread(&reg, sizeof(RegistroHistorico), 1, f) && contador < DIAS_HISTORICOS)
    {
        suma += reg.pm25;
        contador++;
    }

    fclose(f);

    if (contador == 0)
    {
        return 0;
    }

    return suma / contador;
}

float calcularPrediccion(Zona *zona, Meteorologia *met)
{
    float Zf = zona->factorZona;
    float prediccion;

    if (zona->id == ZONA_VALLES && met->temperatura < 10)
    {
        Zf += 15;
        printf("[AVISO] Inversion termica en %s. Factor zona: %.1f\n", zona->nombre, Zf);
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
        printf("[ALERTA %s] Zona %d - %s: %.2f ug/m3\n",
               pred->nivel,
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
            printf("  [IND] Restriccion de emisiones industriales en Zona Sur.\n");
            printf("  [IND] Cierre temporal de plantas contaminantes.\n");
        }

        if (strcmp(pred->nivel, "PELIGROSO") == 0)
        {
            printf("  [SAL] Suspension de actividades al aire libre.\n");
            printf("  [SAL] Uso obligatorio de mascarillas.\n");
            printf("  [POB] Cierre preventivo de escuelas y espacios publicos.\n");
        }
    }

    if (strcmp(pred->contaminante, "CO2") == 0 || strcmp(pred->contaminante, "NO2") == 0)
    {
        if (zona->id == ZONA_CENTRO_NORTE || zona->id == ZONA_CENTRO_HIST)
        {
            printf("  [VEH] Restriccion vehicular ampliada (Pico y Placa).\n");
            printf("  [VEH] Incentivo de teletrabajo para empresas del sector.\n");
        }

        if (zona->id == ZONA_CENTRO_NORTE && strcmp(pred->nivel, "DANINO") == 0)
        {
            printf("  [O3]  ALERTA: Riesgo de formacion de Ozono Troposferico (O3).\n");
            printf("  [O3]  Reduccion urgente de emisiones de NOx en la zona.\n");
        }
    }

    if (strcmp(pred->contaminante, "SO2") == 0 && zona->id == ZONA_SUR)
    {
        printf("  [IND] Inspeccion industrial de emergencia por SO2.\n");
        printf("  [AMB] Coordinacion con autoridades ambientales distritales.\n");
    }
}

void exportarReporte(Zona zonas[], Prediccion preds[], int n)
{
    char rutaPrincipal[80];
    char rutaAlternativa[80];
    FILE *f = NULL;

    sprintf(rutaPrincipal, "datos/reporte_calidad_aire.txt");
    sprintf(rutaAlternativa, "reporte_calidad_aire.txt");

    if (!abrirArchivoConFallback(rutaPrincipal, rutaAlternativa, "a", &f))
    {
        printf("[ERROR] No se pudo crear el archivo de reporte.\n");
        return;
    }

    fprintf(f, "\n=== REPORTE DE CALIDAD DEL AIRE ===\n");
    for (int i = 0; i < n; i++)
    {
        fprintf(f, "Zona: %-25s | %s: %6.2f ug/m3 | Nivel: %s\n",
                zonas[preds[i].zonaId].nombre,
                preds[i].contaminante,
                preds[i].valorPredicho,
                preds[i].nivel);
    }
    fprintf(f, "====================================\n");
    fclose(f);

    printf("[OK] Reporte guardado en datos/reporte_calidad_aire.txt\n");
}