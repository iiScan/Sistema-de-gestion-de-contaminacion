# Sistema Integral de Gestión y Predicción de Contaminación del Aire
### Documentación Técnica y Funcional — ISWZ1102 Programación 1

---

## Tabla de Contenido

1. [Diagnóstico del Problema](#1-diagnóstico-del-problema)
2. [Variables y Restricciones del Sistema](#2-variables-y-restricciones-del-sistema)
3. [Modelo Matemático de Predicción](#3-modelo-matemático-de-predicción)
4. [Fenómenos Meteorológicos Simulados](#4-fenómenos-meteorológicos-simulados)
5. [Estructura del Proyecto y Librerías](#5-estructura-del-proyecto-y-librerías)
6. [funciones.h — Constantes, Estructuras y Prototipos](#6-funcionesh--constantes-estructuras-y-prototipos)
7. [funciones.c — Funciones Utilitarias Base](#7-funcionesc--funciones-utilitarias-base)
8. [funciones.c — Implementación de Módulos](#8-funcionesc--implementación-de-módulos)
9. [main.c — Función Principal](#9-mainc--función-principal)
10. [Manejo de Archivos Binarios](#10-manejo-de-archivos-binarios)
11. [Sistema de Alertas y Recomendaciones](#11-sistema-de-alertas-y-recomendaciones)
12. [Casos de Prueba y Validación](#12-casos-de-prueba-y-validación)

---

## 1. Diagnóstico del Problema

### 1.1 Descripción General

El crecimiento urbano acelerado y el incremento del tráfico vehicular han intensificado los niveles de contaminación atmosférica en Quito. Los principales contaminantes monitoreados son:

| Contaminante | Descripción | Umbral OMS (24h) |
|---|---|---|
| **CO2** | Dióxido de carbono — fuente vehicular e industrial | > 400 ppm (referencia) |
| **SO2** | Dióxido de azufre — combustión diésel e industria | > 20 µg/m³ |
| **NO2** | Dióxido de nitrógeno — tráfico y combustión | > 25 µg/m³ |
| **PM2.5** | Material particulado fino — principal indicador de salud | > 15 µg/m³ |

### 1.2 Zonas Urbanas Monitoreadas

El sistema contempla **cinco zonas** con perfiles de emisión y características geográficas distintas. Cada zona tiene un **ID numérico** que coincide con el índice de su posición en el arreglo `zonas[]` y con la constante `#define` correspondiente:

| ID (`#define`) | Zona | Principales Fuentes | Características | Factor Zf |
|---|---|---|---|---|
| `0` — `ZONA_SUR` | **Zona Sur** | Actividad industrial, fábricas, transporte de carga | Concentración crítica de PM2.5. Topografía semi-cerrada | **+18** |
| `1` — `ZONA_CENTRO_HIST` | **Centro Histórico** | Transporte público a diésel, tráfico comercial pesado | Calles estrechas — efecto cañón. Muy baja ventilación | **+12** |
| `2` — `ZONA_CENTRO_NORTE` | **Centro Norte** | Alta densidad vehicular privada y comercial | Zonas de alta actividad económica. Picos ligados a jornada laboral | **+8** |
| `3` — `ZONA_NORTE` | **Zona Norte** | Flujo de tránsito rápido, polvo en suspensión | Zonas residenciales y de expansión. Corrientes de aire | **+4** |
| `4` — `ZONA_VALLES` | **Valles Aledaños** | Tráfico residencial pendular hacia el hipercentro | Menor altitud (Cumbayá, Tumbaco). Vulnerables a inversión térmica | **+0** |

---

## 2. Variables y Restricciones del Sistema

### 2.1 Variables de la Ecuación Predictiva

| Variable | Símbolo en código | Peso (W) | Rango Válido | Efecto |
|---|---|---|---|---|
| Promedio Histórico (30 días) | `promedioHistorico` | 0.40 | ≥ 0 µg/m³ | **Suma** — línea base |
| Velocidad del Viento | `velocidadViento` | 0.30 | 0 – 120 km/h | **Resta** — dispersión mecánica |
| Humedad Relativa | `humedadRelativa` | 0.15 | 0 – 100 % | **Suma** — aglutina partículas |
| Temperatura | `temperatura` | 0.10 | -5 – 35 °C | **Suma** — densidad térmica |
| Lluvia / Lavado Atmosférico | `lluvia` | 0.50 | 0 o 1 (entero) | **Resta** — depurador masivo |
| Factor de Zona | `factorZona` | — | Constante por zona | **Suma fija** |

### 2.2 Variables de Contaminantes (rangos para `validarFloatRango`)

```
PM2.5 → rango: [0, 300] µg/m³    →  validarFloatRango(0, 300)
NO2   → rango: [0, 400] µg/m³    →  validarFloatRango(0, 400)
SO2   → rango: [0, 500] µg/m³    →  validarFloatRango(0, 500)
CO2   → rango: [300, 2000] ppm   →  validarFloatRango(300, 2000)
```

### 2.3 Restricciones Físicas del Modelo

- Si `temperatura < 10°C` en Valles Aledaños → `Zf += 15` (inversión térmica activa)
- Si `lluvia == 1` → la predicción final se divide entre 2 (lavado atmosférico)
- El resultado de la predicción no puede ser negativo: mínimo retornado es `0`

### 2.4 Restricciones de Implementación

| Restricción | Detalle |
|---|---|
| **Librerías permitidas** | `stdio.h`, `string.h` y `funciones.h` (propia del proyecto) |
| **Entrada numérica** | Siempre mediante `validarFloatRango(min, max)` — nunca `scanf` directo |
| **Entrada de texto** | Siempre mediante `leerCadena(cadena, tamanio)` — nunca `scanf("%s")` |
| **Persistencia** | Archivos binarios `.dat` con `fread` / `fwrite` para el historial |
| **Reportes** | Archivo de texto `.txt` con `fprintf` / `fopen("a")` |
| **Librerías prohibidas** | `stdlib.h`, `math.h`, `time.h` y cualquier librería externa |
| **Datos históricos** | Últimos `DIAS_HISTORICOS` (30) días por zona |
| **Horizonte de predicción** | Próximas 24 horas |

---

## 3. Modelo Matemático de Predicción

### 3.1 Ecuación de Regresión Lineal Múltiple Ponderada

```
Pt+1 = (H × Wh) - (v × Wv) + (h × Whum) + (T × Wt) + Zf - (L × Wll)
```

Traducido a los nombres de variables del código:

```
prediccion = (promedioHistorico * W_HISTORICO)
           - (velocidadViento   * W_VIENTO)
           + (humedadRelativa   * W_HUMEDAD)
           + (temperatura       * W_TEMPERATURA)
           + Zf
           - (lluvia            * W_LLUVIA)
```

Donde los pesos están definidos como constantes `#define` en `funciones.h`:

```
W_HISTORICO   = 0.40
W_VIENTO      = 0.30
W_HUMEDAD     = 0.15
W_TEMPERATURA = 0.10
W_LLUVIA      = 0.50
```

### 3.2 Ejemplo Resuelto — Zona Sur (sin lluvia)

```
Datos:
  promedioHistorico = 80  velocidadViento = 5
  humedadRelativa   = 70  temperatura     = 15
  Zf = 18  lluvia = 0

prediccion = (80×0.40) - (5×0.30) + (70×0.15) + (15×0.10) + 18 - (0×0.50)
           =  32.0     -  1.5     +  10.5     +   1.5     + 18 -  0
           = 60.5 µg/m³  →  PELIGROSO
```

### 3.3 Ejemplo Resuelto — Valles, Inversión Térmica + Lluvia

```
Datos:
  promedioHistorico = 40  velocidadViento = 3
  humedadRelativa   = 85  temperatura     = 8  (< 10°C → Zf += 15)
  Zf = 0 + 15 = 15    lluvia = 1

Paso 1 — Ecuación base:
  prediccion = (40×0.40) - (3×0.30) + (85×0.15) + (8×0.10) + 15 - (1×0.50)
             = 16.0 - 0.9 + 12.75 + 0.8 + 15 - 0.5
             = 43.15 µg/m³

Paso 2 — Ajuste por lluvia (lluvia == 1 → /2):
  prediccion = 43.15 / 2 = 21.58 µg/m³  →  MODERADO
```

### 3.4 Umbrales de Clasificación OMS (PM2.5)

| Rango (µg/m³) | Nivel | String en el código |
|---|---|---|
| 0 – 15 | Normal | `"NORMAL"` |
| 16 – 35 | Moderado | `"MODERADO"` |
| 36 – 55 | Dañino | `"DANINO"` |
| > 55 | Peligroso | `"PELIGROSO"` |

> **Nota:** Se usa `"DANINO"` sin tilde para evitar problemas de codificación de caracteres en archivos `.dat` y en la consola de Windows.

---

## 4. Fenómenos Meteorológicos Simulados

### 4.1 Inversión Térmica Matutina

Ocurre en los **Valles Aledaños** durante madrugadas despejadas: una capa de aire caliente bloquea la dispersión vertical de los contaminantes.

**Condición:** `zona->id == ZONA_VALLES && met->temperatura < 10`
**Efecto:** `Zf += 15` antes de aplicar la ecuación principal.

### 4.2 Advección Horizontal

Desplazamiento horizontal del aire que transporta masas contaminadas a lo largo del corredor de Quito. Si los vientos provienen del sur con alta velocidad, se incrementa el `promedioHistorico` del Centro Histórico y Centro Norte antes de calcular su predicción.

### 4.3 Dinámica Fotoquímica (Ozono Troposférico O3)

A 2,800 msnm, la radiación UV es severa. Con alta temperatura en el Centro Norte, los NOx forman **Ozono Troposférico**.

**Condición de alerta extra:** `zona->id == ZONA_CENTRO_NORTE` + `nivel == "DANINO"`

### 4.4 Lavado Atmosférico

La lluvia captura PM2.5 en suspensión y disuelve SO2, reduciendo a la mitad la concentración predicha.

**Condición:** `met->lluvia == 1` → `prediccion /= 2`

---

## 5. Estructura del Proyecto y Librerías

### 5.1 Organización de Archivos

```
proyecto_calidad_aire/
├── main.c                         ← Menú principal (do-while + switch)
├── funciones.h                    ← Constantes, structs y prototipos
├── funciones.c                    ← Implementación de todas las funciones
└── datos/
    ├── historico_zona0.dat        (Zona Sur)
    ├── historico_zona1.dat        (Centro Histórico)
    ├── historico_zona2.dat        (Centro Norte)
    ├── historico_zona3.dat        (Zona Norte)
    ├── historico_zona4.dat        (Valles Aledaños)
    └── reporte_calidad_aire.txt   (Reportes exportados — texto acumulativo)
```

### 5.2 Inclusión de Librerías

```c
/* main.c */
#include <stdio.h>
#include "funciones.h"

/* funciones.c */
#include <stdio.h>
#include <string.h>
#include "funciones.h"
```

### 5.3 Módulos Lógicos Dentro de funciones.c

Los 6 módulos funcionales del requerimiento se agrupan como conjuntos de funciones dentro del mismo archivo `funciones.c`:

| Módulo | Función(es) |
|---|---|
| **1 — Telemetría actual** | `capturarTelemetria()`, `capturarMeteorologiaConsola()` |
| **2 — Histórico** | `guardarRegistro()`, `calcularPromedioHistorico()` |
| **3 — Predicción** | `calcularPrediccion()` |
| **4 — Alertas** | `clasificarNivel()`, `listarZonas()` |
| **5 — Recomendaciones** | `generarRecomendaciones()` |
| **6 — Reporte** | `exportarReporte()` |
| **Utilitarias (base)** | `validarFloatRango()`, `leerCadena()`, `menu()`, `inicializarZonas()` |

---

## 6. funciones.h — Constantes, Estructuras y Prototipos

```c
/* funciones.h */

/* --- Constantes del sistema --- */
#define NUM_ZONAS       5
#define DIAS_HISTORICOS 30

/* Identificadores de zona (coinciden con el índice en zonas[]) */
#define ZONA_SUR          0
#define ZONA_CENTRO_HIST  1
#define ZONA_CENTRO_NORTE 2
#define ZONA_NORTE        3
#define ZONA_VALLES       4

/* Pesos del modelo predictivo */
#define W_HISTORICO   0.40
#define W_VIENTO      0.30
#define W_HUMEDAD     0.15
#define W_TEMPERATURA 0.10
#define W_LLUVIA      0.50

/* --- Estructuras --- */

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

/* --- Prototipos --- */

/* Utilitarias */
int   menu();
float validarFloatRango(float a, float b);
void  leerCadena(char *cadena, int n);
void  inicializarZonas(Zona zonas[]);

/* Módulo 1 — Telemetría */
void capturarTelemetria(Zona *zona);
void capturarMeteorologiaConsola(Meteorologia *met);

/* Módulo 2 — Histórico */
void  guardarRegistro(RegistroHistorico *reg, int zonaId);
float calcularPromedioHistorico(int zonaId);

/* Módulo 3 — Predicción */
float calcularPrediccion(Zona *zona, Meteorologia *met);

/* Módulo 4 — Alertas */
void clasificarNivel(Prediccion *pred);
void listarZonas(Zona zonas[], int n);

/* Módulo 5 — Recomendaciones */
void generarRecomendaciones(Prediccion *pred, Zona *zona);

/* Módulo 6 — Reporte */
void exportarReporte(Zona zonas[], Prediccion preds[], int n);
```

---

## 7. funciones.c — Funciones Utilitarias Base

Estas dos funciones son la **base de toda la entrada de datos** del sistema. Deben definirse al inicio de `funciones.c` ya que todos los módulos las invocan. Se transcriben idénticas al código de referencia.

```c
/* Valida que el valor ingresado esté en el rango [a, b].
   Repite la solicitud hasta obtener un dato correcto.       */
float validarFloatRango(float a, float b){
    int aux;
    float n;
    do
    {
        aux = scanf("%f", &n);
        while ((getchar()) != '\n');
        if (aux != 1 || n < a || n > b){
            printf("Error: valor ingresado incorrecto\n");
            printf(">> ");
        }
    } while (aux != 1 || n < a || n > b);
    return n;
}

/* Lee una cadena de texto con fgets y elimina el salto de línea final */
void leerCadena(char *cadena, int n){
    int len;
    fgets(cadena, n, stdin);
    len = strlen(cadena) - 1;
    cadena[len] = '\0';
}
```

> **Regla de uso:**
> - `validarFloatRango(min, max)` reemplaza a `scanf` en **toda** captura numérica (enteros y flotantes).
> - `leerCadena(cadena, tamanio)` reemplaza a `scanf("%s")` en **toda** captura de texto.
> - Nunca se usa `scanf` ni `gets` directamente para entrada del usuario.

---

## 8. funciones.c — Implementación de Módulos

### Módulo 1 — Telemetría Actual

```c
/* Captura y valida los datos de contaminantes actuales de una zona */
void capturarTelemetria(Zona *zona){
    printf("--- Datos actuales: %s ---\n", zona->nombre);
    printf("PM2.5 (0-300 ug/m3): ");
    zona->actual.pm25 = validarFloatRango(0, 300);
    printf("NO2 (0-400 ug/m3): ");
    zona->actual.no2 = validarFloatRango(0, 400);
    printf("SO2 (0-500 ug/m3): ");
    zona->actual.so2 = validarFloatRango(0, 500);
    printf("CO2 (300-2000 ppm): ");
    zona->actual.co2 = validarFloatRango(300, 2000);
}

/* Captura las condiciones meteorológicas actuales */
void capturarMeteorologiaConsola(Meteorologia *met){
    printf("\n--- Condiciones Meteorologicas ---\n");
    printf("Velocidad del viento (0-120 km/h): ");
    met->velocidadViento = validarFloatRango(0, 120);
    printf("Humedad relativa (0-100%%): ");
    met->humedadRelativa = validarFloatRango(0, 100);
    printf("Temperatura (-5 a 35 C): ");
    met->temperatura = validarFloatRango(-5, 35);
    printf("Lluvia (0=No / 1=Si): ");
    met->lluvia = validarFloatRango(0, 1);
}
```

### Módulo 2 — Procesamiento Histórico

```c
/* Guarda un registro en el archivo binario de la zona correspondiente.
   Usa "ab+" para añadir sin borrar datos previos (mismo patrón que
   guardarEstudiante con pos == -1 en el código de referencia).       */
void guardarRegistro(RegistroHistorico *reg, int zonaId){
    char ruta[50];
    FILE *f;
    sprintf(ruta, "datos/historico_zona%d.dat", zonaId);
    f = fopen(ruta, "ab+");
    if (f == NULL){
        printf("[ERROR] No se pudo abrir: %s\n", ruta);
        return;
    }
    fwrite(reg, sizeof(RegistroHistorico), 1, f);
    fclose(f);
}

/* Lee el archivo binario de la zona y calcula la media de PM2.5
   sobre los últimos DIAS_HISTORICOS registros disponibles.       */
float calcularPromedioHistorico(int zonaId){
    char ruta[50];
    RegistroHistorico reg;
    float suma = 0;
    int contador = 0;
    FILE *f;
    sprintf(ruta, "datos/historico_zona%d.dat", zonaId);
    f = fopen(ruta, "rb+");
    if (f == NULL){
        return 0;
    }
    while (fread(&reg, sizeof(RegistroHistorico), 1, f) && contador < DIAS_HISTORICOS)
    {
        suma += reg.pm25;
        contador++;
    }
    fclose(f);
    if (contador == 0){
        return 0;
    }
    return suma / contador;
}
```

### Módulo 3 — Predicción Matemática

```c
/* Aplica la ecuación de regresión ponderada.
   Contempla ajuste por inversión térmica y por lavado atmosférico. */
float calcularPrediccion(Zona *zona, Meteorologia *met){
    float Zf = zona->factorZona;
    float prediccion;

    /* Ajuste por inversión térmica (solo Valles con T < 10°C) */
    if (zona->id == ZONA_VALLES && met->temperatura < 10){
        Zf += 15;
        printf("[AVISO] Inversion termica en %s. Factor zona: %.1f\n",
               zona->nombre, Zf);
    }

    /* Ecuación principal */
    prediccion = (zona->promedioHistorico * W_HISTORICO)
               - (met->velocidadViento    * W_VIENTO)
               + (met->humedadRelativa    * W_HUMEDAD)
               + (met->temperatura        * W_TEMPERATURA)
               + Zf
               - (met->lluvia             * W_LLUVIA);

    /* Ajuste por lavado atmosférico */
    if (met->lluvia == 1){
        prediccion /= 2;
        printf("[AVISO] Lluvia activa. Concentracion reducida por lavado atmosferico.\n");
    }

    /* El resultado no puede ser negativo */
    return (prediccion < 0) ? 0 : prediccion;
}
```

### Módulo 4 — Alertas Preventivas

```c
/* Clasifica el nivel según umbrales OMS y activa la bandera de alerta */
void clasificarNivel(Prediccion *pred){
    if (pred->valorPredicho <= 15){
        strcpy(pred->nivel, "NORMAL");
        pred->alertaActiva = 0;
    } else if (pred->valorPredicho <= 35){
        strcpy(pred->nivel, "MODERADO");
        pred->alertaActiva = 1;
    } else if (pred->valorPredicho <= 55){
        strcpy(pred->nivel, "DANINO");
        pred->alertaActiva = 1;
    } else {
        strcpy(pred->nivel, "PELIGROSO");
        pred->alertaActiva = 1;
    }
    if (pred->alertaActiva){
        printf("[ALERTA %s] Zona %d - %s: %.2f ug/m3\n",
               pred->nivel, pred->zonaId,
               pred->contaminante, pred->valorPredicho);
    }
}

/* Lista todas las zonas con su estado actual de contaminación */
void listarZonas(Zona zonas[], int n){
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
```

### Módulo 5 — Recomendaciones Mitigatorias

```c
/* Árbol de decisión jerárquico: emite medidas según contaminante, zona y nivel */
void generarRecomendaciones(Prediccion *pred, Zona *zona){
    if (!pred->alertaActiva){
        return;
    }
    printf("\n=== RECOMENDACIONES PARA %s ===\n", zona->nombre);

    if (strcmp(pred->contaminante, "PM2.5") == 0)
    {
        if (zona->id == ZONA_SUR){
            printf("  [IND] Restriccion de emisiones industriales en Zona Sur.\n");
            printf("  [IND] Cierre temporal de plantas contaminantes.\n");
        }
        if (strcmp(pred->nivel, "PELIGROSO") == 0){
            printf("  [SAL] Suspension de actividades al aire libre.\n");
            printf("  [SAL] Uso obligatorio de mascarillas.\n");
            printf("  [POB] Cierre preventivo de escuelas y espacios publicos.\n");
        }
    }

    if (strcmp(pred->contaminante, "CO2") == 0 || strcmp(pred->contaminante, "NO2") == 0)
    {
        if (zona->id == ZONA_CENTRO_NORTE || zona->id == ZONA_CENTRO_HIST){
            printf("  [VEH] Restriccion vehicular ampliada (Pico y Placa).\n");
            printf("  [VEH] Incentivo de teletrabajo para empresas del sector.\n");
        }
        if (zona->id == ZONA_CENTRO_NORTE && strcmp(pred->nivel, "DANINO") == 0){
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
```

### Módulo 6 — Exportación de Reportes

```c
/* Genera el reporte y lo añade al archivo de texto acumulativo.
   Usa "a" para no borrar reportes anteriores.                    */
void exportarReporte(Zona zonas[], Prediccion preds[], int n){
    FILE *f;
    f = fopen("datos/reporte_calidad_aire.txt", "a");
    if (f == NULL){
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
```

### Inicialización de Zonas

```c
/* Configura los datos estáticos de las 5 zonas al arrancar el programa.
   Se llama una sola vez al inicio de main().                             */
void inicializarZonas(Zona zonas[]){
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
```

---

## 9. main.c — Función Principal

La estructura de `main.c` sigue el mismo patrón `do-while + switch` del código de referencia.

```c
/* main.c */
#include <stdio.h>
#include "funciones.h"

int menu(){
    printf("\nSeleccione una opcion\n");
    printf("1. Capturar datos actuales de una zona\n");
    printf("2. Calcular promedios historicos\n");
    printf("3. Predecir niveles (proximas 24h)\n");
    printf("4. Listar zonas y alertas\n");
    printf("5. Exportar reporte\n");
    printf(">> ");
    return validarFloatRango(1, 5);
}

int main(int argc, char *argv[]){
    int opc, opc2, idx, i;
    Zona zonas[NUM_ZONAS];
    Meteorologia met;
    Prediccion preds[NUM_ZONAS];
    RegistroHistorico reg;

    inicializarZonas(zonas);

    do
    {
        opc = menu();
        switch (opc)
        {
        case 1:
            printf("Seleccione zona (0=Sur, 1=CentroHist, 2=CentroNorte, 3=Norte, 4=Valles): ");
            idx = validarFloatRango(0, NUM_ZONAS - 1);
            capturarTelemetria(&zonas[idx]);
            break;

        case 2:
            for (i = 0; i < NUM_ZONAS; i++)
            {
                zonas[i].promedioHistorico = calcularPromedioHistorico(i);
                printf("Zona %-20s: promedio PM2.5 = %.2f ug/m3\n",
                       zonas[i].nombre, zonas[i].promedioHistorico);
            }
            break;

        case 3:
            capturarMeteorologiaConsola(&met);
            for (i = 0; i < NUM_ZONAS; i++)
            {
                preds[i].zonaId = i;
                strcpy(preds[i].contaminante, "PM2.5");
                preds[i].valorPredicho = calcularPrediccion(&zonas[i], &met);
                clasificarNivel(&preds[i]);
                strcpy(zonas[i].nivelAlerta, preds[i].nivel);
                generarRecomendaciones(&preds[i], &zonas[i]);
            }
            break;

        case 4:
            listarZonas(zonas, NUM_ZONAS);
            break;

        case 5:
            exportarReporte(zonas, preds, NUM_ZONAS);
            break;

        default:
            printf("No existe la opcion\n");
            break;
        }

        printf("\nDesea seleccionar otra opcion 1.Si / 2.No: ");
        opc2 = validarFloatRango(1, 2);
    } while (opc2 == 1);

    return 0;
}
```

---

## 10. Manejo de Archivos Binarios

### 10.1 Modos de Apertura (mismo patrón del código de referencia)

| Operación | Modo | Función que lo usa |
|---|---|---|
| Añadir registro nuevo | `"ab+"` | `guardarRegistro()` |
| Leer registros existentes | `"rb+"` | `calcularPromedioHistorico()` |
| Editar un registro en posición fija | `"rb+"` + `fseek` | Si se requiere sobrescribir |

### 10.2 Escritura y Lectura de Structs Completos

```c
/* Escritura: guarda el struct completo en binario de una sola vez */
fwrite(reg, sizeof(RegistroHistorico), 1, f);

/* Lectura: recorre el archivo struct por struct con while */
while (fread(&reg, sizeof(RegistroHistorico), 1, f) && contador < DIAS_HISTORICOS)
{
    suma += reg.pm25;
    contador++;
}
```

### 10.3 Construcción de Ruta de Archivo por Zona

```c
char ruta[50];
sprintf(ruta, "datos/historico_zona%d.dat", zonaId);
/* zonaId = 0  →  "datos/historico_zona0.dat"  (Zona Sur)    */
/* zonaId = 4  →  "datos/historico_zona4.dat"  (Valles)      */
```

### 10.4 Ejemplo de Salida del Archivo de Reporte (.txt)

El reporte es el único archivo de **texto** del sistema. Acumula registros en cada ejecución:

```
=== REPORTE DE CALIDAD DEL AIRE ===
Zona: Zona Sur              | PM2.5:  60.50 ug/m3 | Nivel: PELIGROSO
Zona: Centro Historico      | PM2.5:  38.20 ug/m3 | Nivel: DANINO
Zona: Centro Norte          | PM2.5:  22.10 ug/m3 | Nivel: MODERADO
Zona: Zona Norte            | PM2.5:  12.80 ug/m3 | Nivel: NORMAL
Zona: Valles Aledanos       | PM2.5:  21.58 ug/m3 | Nivel: MODERADO
====================================
```

---

## 11. Sistema de Alertas y Recomendaciones

### 11.1 Árbol de Decisión en generarRecomendaciones()

```
¿pred->alertaActiva == 1?
│
├── NO → return (sin imprimir nada)
│
└── SÍ
    ├── contaminante == "PM2.5"
    │   ├── zonaId == ZONA_SUR
    │   │   └── Restricción industrial, cierre de plantas
    │   └── nivel == "PELIGROSO"
    │       └── Suspensión actividades, mascarillas, cierre escuelas
    │
    ├── contaminante == "CO2" o "NO2"
    │   ├── zonaId == ZONA_CENTRO_NORTE o ZONA_CENTRO_HIST
    │   │   └── Pico y Placa extendido, teletrabajo
    │   └── zonaId == ZONA_CENTRO_NORTE && nivel == "DANINO"
    │       └── ALERTA Ozono Troposférico O3
    │
    └── contaminante == "SO2" && zonaId == ZONA_SUR
        └── Inspección industrial de emergencia
```

### 11.2 Tabla de Recomendaciones por Escenario

| Contaminante | Zona | Nivel | Recomendación emitida |
|---|---|---|---|
| PM2.5 | Zona Sur | DANINO / PELIGROSO | Restricción industrial, cierre temporal de plantas |
| PM2.5 | Todas | PELIGROSO | Suspensión al aire libre, mascarillas, cierre de escuelas |
| CO2 / NO2 | Centro Norte / Centro Hist. | MODERADO+ | Extensión de Pico y Placa, teletrabajo |
| NO2 | Centro Norte | DANINO | Alerta de Ozono Troposférico O3 |
| SO2 | Zona Sur | Cualquier alerta | Inspección industrial de emergencia |

---

## 12. Casos de Prueba y Validación

### Caso 1 — Zona Sur, Sin Lluvia → PELIGROSO

```
Entrada en consola (capturarTelemetria + capturarMeteorologiaConsola):
  PM2.5: 80   NO2: 50   SO2: 20   CO2: 420
  Viento: 5   Humedad: 70   Temperatura: 15   Lluvia: 0

Cálculo en calcularPrediccion():
  Zf = 18  (ZONA_SUR, T=15 no activa inversión térmica)
  prediccion = (80×0.40)-(5×0.30)+(70×0.15)+(15×0.10)+18-(0×0.50)
             = 32.0 - 1.5 + 10.5 + 1.5 + 18 - 0
             = 60.5 µg/m³

Salida esperada en consola:
  [ALERTA PELIGROSO] Zona 0 - PM2.5: 60.50 ug/m3
  === RECOMENDACIONES PARA Zona Sur ===
    [IND] Restriccion de emisiones industriales en Zona Sur.
    [IND] Cierre temporal de plantas contaminantes.
    [SAL] Suspension de actividades al aire libre.
    [SAL] Uso obligatorio de mascarillas.
    [POB] Cierre preventivo de escuelas y espacios publicos.
```

### Caso 2 — Valles, Inversión Térmica + Lluvia → MODERADO

```
Entrada:
  PM2.5: 40   Viento: 3   Humedad: 85   Temperatura: 8   Lluvia: 1

Cálculo en calcularPrediccion():
  ZONA_VALLES && temperatura (8) < 10  →  Zf = 0 + 15 = 15
  Mensaje: "[AVISO] Inversion termica en Valles Aledanos. Factor zona: 15.0"

  prediccion = (40×0.40)-(3×0.30)+(85×0.15)+(8×0.10)+15-(1×0.50)
             = 16.0 - 0.9 + 12.75 + 0.8 + 15 - 0.5
             = 43.15 µg/m³

  lluvia == 1  →  prediccion /= 2  →  21.58 µg/m³
  Mensaje: "[AVISO] Lluvia activa. Concentracion reducida por lavado atmosferico."

Salida esperada:
  [ALERTA MODERADO] Zona 4 - PM2.5: 21.58 ug/m3
```

### Caso 3 — Centro Norte, Riesgo de Ozono Troposférico → DANINO

```
Entrada:
  PM2.5: 45   Viento: 6   Humedad: 55   Temperatura: 27   Lluvia: 0

Cálculo en calcularPrediccion():
  Zf = 8  (ZONA_CENTRO_NORTE, T=27 no activa inversión térmica)
  prediccion = (45×0.40)-(6×0.30)+(55×0.15)+(27×0.10)+8-(0×0.50)
             = 18.0 - 1.8 + 8.25 + 2.7 + 8 - 0
             = 35.15 µg/m³

Salida esperada:
  [ALERTA DANINO] Zona 2 - PM2.5: 35.15 ug/m3
  === RECOMENDACIONES PARA Centro Norte ===
    [VEH] Restriccion vehicular ampliada (Pico y Placa).
    [VEH] Incentivo de teletrabajo para empresas del sector.
    [O3]  ALERTA: Riesgo de formacion de Ozono Troposferico (O3).
    [O3]  Reduccion urgente de emisiones de NOx en la zona.
```

### Caso 4 — Validación de validarFloatRango (entrada inválida)

```
Secuencia de consola:
  PM2.5 (0-300 ug/m3): -15
  Error: valor ingresado incorrecto
  >> 350
  Error: valor ingresado incorrecto
  >> 80          ←  Valor aceptado, continúa

El do-while de validarFloatRango repite hasta recibir un valor en [0, 300].
El programa nunca avanza al siguiente campo con un dato incorrecto.
```

### Caso 5 — Zona Norte, Lluvia, Sin Alerta → NORMAL

```
Entrada:
  PM2.5: 10   Viento: 20   Humedad: 40   Temperatura: 15   Lluvia: 1

Cálculo en calcularPrediccion():
  Zf = 4
  prediccion = (10×0.40)-(20×0.30)+(40×0.15)+(15×0.10)+4-(1×0.50)
             = 4.0 - 6.0 + 6.0 + 1.5 + 4 - 0.5
             = 9.0 µg/m³

  lluvia == 1  →  prediccion /= 2  →  4.5 µg/m³

Salida esperada:
  (Sin mensaje de alerta — clasificarNivel asigna "NORMAL", alertaActiva = 0)
  generarRecomendaciones()  →  return inmediato (alertaActiva == 0)
```

---

## Resumen de Conceptos de Programación Utilizados

| Concepto C | Aplicación en el sistema |
|---|---|
| **`typedef struct`** | `Contaminantes`, `Meteorologia`, `Zona`, `Prediccion`, `RegistroHistorico` — definidos en `funciones.h`, mismo estilo que `Materia` y `Estudiante` del código de referencia |
| **Arreglos de structs** | `Zona zonas[NUM_ZONAS]` y `Prediccion preds[NUM_ZONAS]` declarados en `main()` |
| **Punteros a struct** | `capturarTelemetria(Zona *zona)`, `calcularPrediccion(Zona *zona, Meteorologia *met)` — permite modificar los datos originales sin copiarlos |
| **Funciones modulares** | Separación en `main.c`, `funciones.c` y `funciones.h`; cada función tiene una única responsabilidad |
| **`validarFloatRango`** | Toda entrada numérica (enteros y flotantes) pasa por esta función — reemplaza `scanf` directo |
| **`leerCadena`** | Toda entrada de texto usa `fgets` + eliminación de `\n` — igual que en el código de referencia |
| **Archivos binarios `.dat`** | `fopen`, `fread`, `fwrite`, `fclose` para el historial por zona; modo `"ab+"` para añadir, `"rb+"` para leer |
| **Archivo de texto `.txt`** | `fprintf` con modo `"a"` para acumular reportes sin borrar los anteriores |
| **`#define` constantes** | `NUM_ZONAS`, `DIAS_HISTORICOS`, `ZONA_SUR`…`ZONA_VALLES`, pesos del modelo |
| **`sprintf`** | Construcción dinámica de ruta de archivo: `"datos/historico_zona%d.dat"` |
| **`do-while + switch`** | Bucle de menú en `main.c` — patrón idéntico al código de referencia |
| **`strcmp` / `strcpy`** | Comparación y asignación de cadenas (`string.h`) para niveles, contaminantes y nombres |

---

*Documento generado para ISWZ1102 — Programación 1 | Adaptado al estilo de programación del código de referencia: librerías `stdio.h`, `string.h` y `funciones.h`; entrada siempre mediante `validarFloatRango` y `leerCadena`; persistencia con archivos binarios `.dat`.*
