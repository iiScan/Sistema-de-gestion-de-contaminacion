#ifndef FUNCIONES_H
#define FUNCIONES_H

#include <stdio.h>
#include <string.h>

#define NUM_ZONAS       5
#define DIAS_HISTORICOS 30

#define ZONA_SUR          0
#define ZONA_CENTRO_HIST  1
#define ZONA_CENTRO_NORTE 2
#define ZONA_NORTE        3
#define ZONA_VALLES       4

#define W_HISTORICO   0.40
#define W_VIENTO      0.30
#define W_HUMEDAD     0.15
#define W_TEMPERATURA 0.10
#define W_LLUVIA      0.50

typedef struct
{
    float pm25;
    float no2;
    float so2;
    float co2;
} Contaminantes;

typedef struct
{
    float velocidadViento;
    float humedadRelativa;
    float temperatura;
    int   lluvia;
} Meteorologia;

typedef struct
{
    char          nombre[30];
    int           id;
    float         factorZona;
    Contaminantes actual;
    float         promedioHistorico;
    char          nivelAlerta[20];
} Zona;

typedef struct
{
    int   zonaId;
    char  contaminante[10];
    float valorPredicho;
    char  nivel[20];
    int   alertaActiva;
} Prediccion;

typedef struct
{
    int   dia;
    int   mes;
    int   anio;
    int   zonaId;
    float pm25;
    float no2;
    float so2;
    float co2;
    float temperatura;
    float humedad;
    float viento;
    int   lluvia;
} RegistroHistorico;

int menu();
float validarFloatRango(float a, float b);
void leerCadena(char *cadena, int n);

void inicializarZonas(Zona zonas[]);

void capturarTelemetria(Zona *zona);
void capturarMeteorologiaConsola(Meteorologia *met);

void guardarRegistro(RegistroHistorico *reg, int zonaId);
float calcularPromedioHistorico(int zonaId);

float calcularPrediccion(Zona *zona, Meteorologia *met);

void clasificarNivel(Prediccion *pred);
void listarZonas(Zona zonas[], int n);

void generarRecomendaciones(Prediccion *pred, Zona *zona);

void exportarReporte(Zona zonas[], Prediccion preds[], int n);

#endif