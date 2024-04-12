// Autor: Williams Chan Pecador
// Algorithm: Algoritmo Evolutivo
// Description: Implementación de un algoritmo evolutivo para optimizar los parámetros de la colonia de hormigas
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>

// #include <windows.h>

// Definir el comando para ejecutar el programa de la colonia de hormigas en Windows y Linux
#ifdef _WIN32
#define COMANDO "Colonia_Hormigas.exe"
#else
#define COMANDO "./Colonia_Hormigas"
#endif

// Estructura de los parámetros
typedef struct
{
    float Alpha;         // Alpha
    float Beta;          // Beta
    float Gamma;         // Gamma
    float Rho;           // Rho
    int N;               // Número de iteraciones
    float FO;            // Función objetivo
    int Tipo_Evaluacion; // Tipo de evaluación
} Parametros;

// Funcion para evaluar la función objetivo
void Evaluar_FO(Parametros *Vector, int Tam_Poblacion, int Tipo_Evaluacion)
{
    float FO_Mejor = 1000000000.0; // Inicializar la función objetivo mejor con un valor muy grande
    // Bucle para evaluar la función objetivo
    for (int i = 0; i < Tam_Poblacion; i++)
    {
        // Ejecutar el programa de la colonia de hormigas
        char buffer[128];
        /* 1.- Numero de iteraciones
           2.- Numero de hormigas
           3.- Alpha
           4.- Beta
           5.- Gamma
           6.- Rho
           7.- Tipo de evaluacion
           8.- Numero de archivo
        */
        snprintf(buffer, sizeof(buffer), "%s %d %d %f %f %f %f %d %d", COMANDO, 3, 10, Vector[i].Alpha, Vector[i].Beta, Vector[i].Gamma, Vector[i].Rho, Tipo_Evaluacion, i); // i es igual al N de iteraciones, servira para detectar a que archivo meterlo
        // Ejecutar el comando en la terminal o consola de comandos del sistema operativo (Windows o Linux) para ejecutar el programa de la colonia de hormigas con los parámetros correspondientes 
        // y guardar el valor de la función objetivo en un archivo de texto llamado FO.txt 
        system(buffer);
        // Leer el valor de la función objetivo

        FILE *FO;
        float valor;
        
        char mensaje[100];

        FO = fopen("FuncionObjetivo/FO.txt", "r");
        if (FO == NULL)
        {
            printf("Error al abrir el archivo.\n");
            //return 1;
        }

        if (fscanf(FO, "%f", &valor) == 1)
        {
            // Si se leyó un número flotante correctamente
            Vector[i].FO = valor;
            if (valor < FO_Mejor)
            {
                FO_Mejor = valor;
            }
            
            printf("\nValor FO: %f\n", Vector[i].FO);
            printf("Tipo Evaluacion: %d\n", Tipo_Evaluacion);
            printf("Numero de iteracion: %d\n", i);
        }
        else
        {
            // Si no se pudo leer un número flotante, intenta leer una cadena
            fseek(FO, 0, SEEK_SET); // Reiniciar la posición del archivo
            if (fscanf(FO, "%99[^\n]", mensaje) == 1)
            {
                // Si se leyó una cadena correctamente
                if (strcmp(mensaje, "Error: Violacion de segmento. El programa se detuvo inesperadamente.") == 0)
                {
                    printf("Error: Violacion de segmento. El programa se detuvo inesperadamente.\n");
                    i--;
                    //system("pause");
                }
            }
            else
            {
                printf("Error al leer el archivo.\n");
            }
        }

        fclose(FO);
        // Guardar el FO_Mejor en un archivo de texto
        FILE *FO_Mejor_Archivo;
        FO_Mejor_Archivo = fopen("FuncionObjetivo/FO_Mejor.txt", "w");
        if (FO_Mejor_Archivo == NULL)
        {
            printf("Error al abrir el archivo.\n");
            //return 1;
        }
        fprintf(FO_Mejor_Archivo, "%f", FO_Mejor);
        fclose(FO_Mejor_Archivo);
        

        // system("pause");
    }
}

// Función para generar valores aleatorios
float Generar_Valores(double Limite_Inferior, double Limite_Superior)
{
    // Generar un valor aleatorio
    float Valor;
    Valor = Limite_Inferior + ((float)rand() / RAND_MAX) * (Limite_Superior - Limite_Inferior);
    return Valor;
}

// Función para inicializar el vector objetivo
void Inicializar_Objetivo(Parametros *Vector_Objetivo, int Tam_Poblacion)
{
    srand(time(NULL)); // Inicializar la semilla aleatoria
    // Bucle para inicializar el vector objetivo
    for (int i = 0; i < Tam_Poblacion; i++)
    {
        // Inicializar los valores del vector objetivo
        Vector_Objetivo[i].Alpha = Generar_Valores(0.2, 1.5);
        Vector_Objetivo[i].Beta = Generar_Valores(0.2, 1.5);
        Vector_Objetivo[i].Gamma = Generar_Valores(0.2, 2);
        Vector_Objetivo[i].Rho = Generar_Valores(0.000001, 0.999999);
        Vector_Objetivo[i].N = Generar_Valores(10, 50);
        Vector_Objetivo[i].Tipo_Evaluacion = 1;
    }
}

// Función para inicializar el vector ruidoso
void Inicializar_Ruidoso(Parametros *Vector_Ruidoso, Parametros *Vector_Objetivo, int Tam_Poblacion, float Factor_Mutacion)
{
    srand(time(NULL)); // Inicializar la semilla aleatoria
    // Variables para los números aleatorios
    int Numero_Aleatorio_A = 0;
    int Numero_Aleatorio_B = 0;
    int Numero_Aleatorio_C = 0;

    // Bucle para inicializar el vector ruidoso
    for (int i = 0; i < Tam_Poblacion; i++)
    {

        Numero_Aleatorio_A = 0;
        Numero_Aleatorio_B = 0;
        Numero_Aleatorio_C = 0;

        // Generar tres números aleatorios diferentes entre 0 y el tamaño de la población sin que se repitan
        while (Numero_Aleatorio_A == Numero_Aleatorio_B || Numero_Aleatorio_A == Numero_Aleatorio_C || Numero_Aleatorio_B == Numero_Aleatorio_C)
        {
            Numero_Aleatorio_A = rand() % Tam_Poblacion;
            Numero_Aleatorio_B = rand() % Tam_Poblacion;
            Numero_Aleatorio_C = rand() % Tam_Poblacion;
        }
        // Calcular los valores del vector ruidoso en base a los valores del vector objetivo y los números aleatorios generados anteriormente y el factor de mutación
        Vector_Ruidoso[i].Alpha = Vector_Objetivo[Numero_Aleatorio_C].Alpha + (Factor_Mutacion * (Vector_Objetivo[Numero_Aleatorio_B].Alpha - Vector_Objetivo[Numero_Aleatorio_A].Alpha));
        Vector_Ruidoso[i].Beta = Vector_Objetivo[Numero_Aleatorio_C].Beta + (Factor_Mutacion * (Vector_Objetivo[Numero_Aleatorio_B].Beta - Vector_Objetivo[Numero_Aleatorio_A].Beta));
        Vector_Ruidoso[i].Gamma = Vector_Objetivo[Numero_Aleatorio_C].Gamma + (Factor_Mutacion * (Vector_Objetivo[Numero_Aleatorio_B].Gamma - Vector_Objetivo[Numero_Aleatorio_A].Gamma));
        Vector_Ruidoso[i].Rho = Vector_Objetivo[Numero_Aleatorio_C].Rho + (Factor_Mutacion * (Vector_Objetivo[Numero_Aleatorio_B].Rho - Vector_Objetivo[Numero_Aleatorio_A].Rho));
        Vector_Ruidoso[i].N = Vector_Objetivo[Numero_Aleatorio_C].N + (Factor_Mutacion * (Vector_Objetivo[Numero_Aleatorio_B].N - Vector_Objetivo[Numero_Aleatorio_A].N));

        Vector_Ruidoso[i].Tipo_Evaluacion = 2;

        // Verificar que los valores del vector ruidoso estén dentro de los límites establecidos
        if (Vector_Ruidoso[i].Alpha < 0.2)
        {
            Vector_Ruidoso[i].Alpha = 0.2;
        }
        else if (Vector_Ruidoso[i].Alpha > 1.5)
        {
            Vector_Ruidoso[i].Alpha = 1.5;
        }

        if (Vector_Ruidoso[i].Beta < 0.2)
        {
            Vector_Ruidoso[i].Beta = 0.2;
        }
        else if (Vector_Ruidoso[i].Beta > 1.5)
        {
            Vector_Ruidoso[i].Beta = 1.5;
        }

        if (Vector_Ruidoso[i].Gamma < 0.2)
        {
            Vector_Ruidoso[i].Gamma = 0.2;
        }
        else if (Vector_Ruidoso[i].Gamma > 2)
        {
            Vector_Ruidoso[i].Gamma = 2;
        }

        if (Vector_Ruidoso[i].Rho < 0)
        {
            
            Vector_Ruidoso[i].Rho = 0.000001;
        }
        else if (Vector_Ruidoso[i].Rho > 1)
        {
            Vector_Ruidoso[i].Rho = 0.999999;
        }

        if (Vector_Ruidoso[i].N < 10)
        {
            Vector_Ruidoso[i].N = 10;
        }
        else if (Vector_Ruidoso[i].N > 50)
        {
            Vector_Ruidoso[i].N = 50;
        }
    }
}

// Función para inicializar el vector de prueba
void Inicializar_Prueba(Parametros *Vector_Prueba, Parametros *Vector_Objetivo, Parametros *Vector_Ruidoso, int Tam_Poblacion, float Factor_Cuza)
{
    srand(time(NULL)); // Inicializar la semilla aleatoria

    // Variable para el valor aleatorio
    float Valor_Aleatorio = 0;

    // Bucle para inicializar el vector de prueba
    for (int i = 0; i < Tam_Poblacion; i++)
    {
        // Generar un valor aleatorio entre 0 y 1
        Valor_Aleatorio = Generar_Valores(0, 1);

        // PREGUNTAR SI EL VALOR ALEATORIO DEBE SER MENOR O IGUAL A 0.5 Y SI ES ASI ELEGIR EL VALOR DE VECTOR_RUIDOSO O VECTOR_OBJETIVO

        // Si el valor aleatorio es menor o igual al factor de cuza entonces el valor de alpha del vector de prueba es el valor de alpha del vector ruidoso en la posición i
        // en caso contrario el valor de alpha del vector de prueba es el valor de alpha del vector objetivo en la posición i
        if (Valor_Aleatorio <= Factor_Cuza)
        {
            Vector_Prueba[i].Alpha = Vector_Ruidoso[i].Alpha;
        }
        else
        {
            Vector_Prueba[i].Alpha = Vector_Objetivo[i].Alpha;
        }

        // Generar un valor aleatorio entre 0 y 1
        Valor_Aleatorio = Generar_Valores(0, 1);
        // Si el valor aleatorio es menor o igual al factor de cuza entonces el valor de beta del vector de prueba es el valor de beta del vector ruidoso en la posición i
        // en caso contrario el valor de beta del vector de prueba es el valor de beta del vector objetivo en la posición i
        if (Valor_Aleatorio <= Factor_Cuza)
        {
            Vector_Prueba[i].Beta = Vector_Ruidoso[i].Beta;
        }
        else
        {
            Vector_Prueba[i].Beta = Vector_Objetivo[i].Beta;
        }

        // Generar un valor aleatorio entre 0 y 1
        Valor_Aleatorio = Generar_Valores(0, 1);
        // Si el valor aleatorio es menor o igual al factor de cuza entonces el valor de gamma del vector de prueba es el valor de gamma del vector ruidoso en la posición i
        // en caso contrario el valor de gamma del vector de prueba es el valor de gamma del vector objetivo en la posición i
        if (Valor_Aleatorio <= Factor_Cuza)
        {
            Vector_Prueba[i].Gamma = Vector_Ruidoso[i].Gamma;
        }
        else
        {
            Vector_Prueba[i].Gamma = Vector_Objetivo[i].Gamma;
        }

        // Generar un valor aleatorio entre 0 y 1
        Valor_Aleatorio = Generar_Valores(0.0000001, 0.9999999);
        // Si el valor aleatorio es menor o igual al factor de cuza entonces el valor de rho del vector de prueba es el valor de rho del vector ruidoso en la posición i
        // en caso contrario el valor de rho del vector de prueba es el valor de rho del vector objetivo en la posición i
        if (Valor_Aleatorio <= Factor_Cuza)
        {
            Vector_Prueba[i].Rho = Vector_Ruidoso[i].Rho;
        }
        else
        {
            Vector_Prueba[i].Rho = Vector_Objetivo[i].Rho;
        }

        // Generar un valor aleatorio entre 0 y 1
        Valor_Aleatorio = Generar_Valores(0, 1);
        // Si el valor aleatorio es menor o igual al factor de cuza entonces el valor de n del vector de prueba es el valor de n del vector ruidoso en la posición i
        // en caso contrario el valor de n del vector de prueba es el valor de n del vector objetivo en la posición i
        if (Valor_Aleatorio <= Factor_Cuza)
        {
            Vector_Prueba[i].N = Vector_Ruidoso[i].N;
        }
        else
        {
            Vector_Prueba[i].N = Vector_Objetivo[i].N;
        }
        Vector_Prueba[i].Tipo_Evaluacion = 3;
    }
}

// Función para imprimir el vector
void Test_Imprimir_Vector(Parametros *Vector, int Tam_Poblacion)
{
    // Bucle para imprimir el vector de parámetros
    for (int i = 0; i < Tam_Poblacion; i++)
    {
        printf("\nPosicion: %d\n", i);
        printf("Alpha: %f\n", Vector[i].Alpha);
        printf("Beta: %f\n", Vector[i].Beta);
        printf("Gamma: %f\n", Vector[i].Gamma);
        printf("Rho: %f\n", Vector[i].Rho);
        printf("N: %d\n", Vector[i].N);
        printf("FO: %f\n", Vector[i].FO);
        printf("Tipo Evaluacion: %d\n", Vector[i].Tipo_Evaluacion);
    }
}

// Función para actualizar el vector objetivo con el vector de prueba
void Actualizar_Vector_Objetivo(Parametros *Vector_Objetivo, Parametros *Vector_Prueba, int Tam_Poblacion)
{
    // Bucle para actualizar el vector objetivo
    for (int i = 0; i < Tam_Poblacion; i++)
    {
        // Si la función objetivo del vector de prueba es menor que la función objetivo del vector objetivo entonces el vector objetivo en la posición i toma los valores del vector de prueba en la posición i
        if (Vector_Objetivo[i].FO > Vector_Prueba[i].FO)
        {
            // Actualizar el vector objetivo con los nuevos valores
            Vector_Objetivo[i].Alpha = Vector_Prueba[i].Alpha;
            Vector_Objetivo[i].Beta = Vector_Prueba[i].Beta;
            Vector_Objetivo[i].Gamma = Vector_Prueba[i].Gamma;
            Vector_Objetivo[i].Rho = Vector_Prueba[i].Rho;
            Vector_Objetivo[i].N = Vector_Prueba[i].N;
            Vector_Objetivo[i].FO = Vector_Prueba[i].FO;
            Vector_Objetivo[i].Tipo_Evaluacion = Vector_Prueba[i].Tipo_Evaluacion;
        }
    }
}

// Función para reiniciar el vector de parámetros
void Reiniciar_Vector(Parametros *Vector, int Tam_Poblacion)
{
    // Bucle para reiniciar el vector de parámetros
    for (int i = 0; i < Tam_Poblacion; i++)
    {
        // Reiniciar los valores del vector
        Vector[i].Alpha = 0;
        Vector[i].Beta = 0;
        Vector[i].Gamma = 0;
        Vector[i].Rho = 0;
        Vector[i].N = 0;
        Vector[i].FO = 0;
        Vector[i].Tipo_Evaluacion = 0;
    }
}

// Función para actualizar el tipo de evaluación del vector
void Actualizar_Tipo_Evaluacion(Parametros *Vector, int Tam_Poblacion, int Tipo_Evaluacion)
{
    // Bucle para actualizar el tipo de evaluación del vector
    for (int i = 0; i < Tam_Poblacion; i++)
    {
        // Actualizar el tipo de evaluación del vector
        Vector[i].Tipo_Evaluacion = Tipo_Evaluacion;
    }
}

// Función principal
int main()
{
    // Definir las variables
    int Tam_Poblacion = 10;
    float Factor_Mutacion = 0.5;
    float Factor_Cuza = 0.5;
    int Numero_Iteraciones_Max = 3;
    int Numero_Iteraciones_Actual = 0;

    // Inicializar los vectores
    Parametros Vector_Objetivo[Tam_Poblacion];
    Parametros Vector_Ruidoso[Tam_Poblacion];
    Parametros Vector_Prueba[Tam_Poblacion];

    // Bucle para realizar las iteraciones
    for (int i = 0; i < Numero_Iteraciones_Max; i++)
    {
        printf("\nIteracion: %d\n", i);
        // Reiniciar los vectores de parámetros
        Reiniciar_Vector(Vector_Ruidoso, Tam_Poblacion);
        Reiniciar_Vector(Vector_Prueba, Tam_Poblacion);

        // Inicializar el vector objetivo con valores aleatorios solo la primera vez
        if (Numero_Iteraciones_Actual == 0)
        {
            Reiniciar_Vector(Vector_Objetivo, Tam_Poblacion);
            Inicializar_Objetivo(Vector_Objetivo, Tam_Poblacion);
            // Test_Imprimir_Vector(Vector_Objetivo, Tam_Poblacion);
        }

        // Actualizar el tipo de evaluación del vector objetivo
        Actualizar_Tipo_Evaluacion(Vector_Objetivo, Tam_Poblacion, 1);

        // Inicializar el vector ruidoso y el vector de prueba
        Inicializar_Ruidoso(Vector_Ruidoso, Vector_Objetivo, Tam_Poblacion, Factor_Mutacion);
        // Test_Imprimir_Vector(Vector_Ruidoso, Tam_Poblacion);
        Inicializar_Prueba(Vector_Prueba, Vector_Objetivo, Vector_Ruidoso, Tam_Poblacion, Factor_Cuza);
        // Test_Imprimir_Vector(Vector_Prueba, Tam_Poblacion);

        // Evaluar la función objetivo de los vectores objetivo, ruidoso y de prueba
        Evaluar_FO(Vector_Objetivo, Tam_Poblacion, 1);
        Evaluar_FO(Vector_Ruidoso, Tam_Poblacion, 2);
        Evaluar_FO(Vector_Prueba, Tam_Poblacion, 3);

        // Actualizar el vector objetivo con el vector de prueba si es necesario
        Actualizar_Vector_Objetivo(Vector_Objetivo, Vector_Prueba, Tam_Poblacion);
        // Incrementar el número de iteraciones
        Numero_Iteraciones_Actual++;
    }

    // Imprimir el vector objetivo final
    Test_Imprimir_Vector(Vector_Objetivo, Tam_Poblacion);
    return 0;
}
