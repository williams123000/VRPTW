// Autor: Williams Chan Pescador
// Algorithm: Colonias de Hormigas para el VRP con ventanas de tiempo
// Description: Este programa implementa el algoritmo de colonia de hormigas para resolver el problema de rutas de vehículos con capacidad y tiempo de servicio en cada cliente
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>
#include <limits.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

// Definir la semilla aleatoria dependiendo del sistema operativo
#ifdef _WIN32
#include <windows.h>
#include <wincrypt.h>

// Función para obtener una semilla aleatoria en Windows
unsigned int obtener_semilla_aleatoria()
{
    HCRYPTPROV hProv;
    unsigned int semilla;

    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
    {
        fprintf(stderr, "Error al adquirir el contexto de criptografía: %d\n", GetLastError());
        exit(EXIT_FAILURE);
    }

    if (!CryptGenRandom(hProv, sizeof(semilla), (BYTE *)&semilla))
    {
        fprintf(stderr, "Error al generar números aleatorios: %d\n", GetLastError());
        CryptReleaseContext(hProv, 0);
        exit(EXIT_FAILURE);
    }

    CryptReleaseContext(hProv, 0);

    return semilla;
}

#else // Si no es Windows, asumimos que es un sistema basado en Unix/Linux

// Función para obtener una semilla aleatoria en sistemas basados en Unix/Linux
unsigned int obtener_semilla_aleatoria()
{
    FILE *urandom = fopen("/dev/urandom", "rb");
    if (urandom == NULL)
    {
        perror("Error abriendo /dev/urandom");
        exit(EXIT_FAILURE);
    }
    unsigned int semilla;
    if (fread(&semilla, sizeof(semilla), 1, urandom) != 1)
    {
        perror("Error leyendo desde /dev/urandom");
        exit(EXIT_FAILURE);
    }
    fclose(urandom);
    return semilla;
}

#endif

// Definir el numero maximo de clientes
#define MAX_CUSTOMERS 101

// Parámetros de la colonia de hormigas
double Alpha;
double Beta;
double Rho;
double Gamma;

// Estructura para representar un vehículo
typedef struct
{
    int number;              // Número identificador del vehículo
    int capacity;            // Capacidad del vehículo
    int capacity_restant;    // Capacidad restante del vehiculo
    double Tiempo_Consumido; // Tiempo que ha consumido en recorrer los cliente y atenderlos
    double Tiempo_Maximo;    // Maximo de tiempo para regresar al deposito
} Vehicle;

typedef struct
{
    int Cliente;              // Número identificador del cliente
    double xCoord;            // Coordenada X del cliente en el plano
    double yCoord;            // Coordenada Y del cliente en el plano
    int Demanda;              // Cantidad demandada por el cliente
    double Tiempo_Inicio;     // Tiempo de inicio permitido para el servicio
    double Fecha_Vencimiento; // Fecha límite para el servicio
    double Tiempo_Servicio;   // Tiempo necesario para atender al cliente
} Customer;

struct Nodo
{
    int dato;
    struct Nodo *siguiente;
};

// Función para insertar un nuevo nodo al final de la lista
void insertarAlFinal(struct Nodo **cabeza, int dato)
{
    // Crear un nuevo nodo
    struct Nodo *nuevoNodo = (struct Nodo *)malloc(sizeof(struct Nodo));
    nuevoNodo->dato = dato;
    nuevoNodo->siguiente = NULL;

    // Si la lista está vacía, el nuevo nodo se convierte en la cabeza de la lista
    if (*cabeza == NULL)
    {
        *cabeza = nuevoNodo;
        return;
    }

    // Recorrer la lista hasta el último nodo
    struct Nodo *ultimo = *cabeza;
    while (ultimo->siguiente != NULL)
    {
        ultimo = ultimo->siguiente;
    }

    // Enlazar el nuevo nodo al final de la lista
    ultimo->siguiente = nuevoNodo;
}

// Función para imprimir los elementos de la lista
void imprimirLista(struct Nodo *cabeza)
{
    while (cabeza != NULL)
    {
        printf(" %d -> ", cabeza->dato);
        cabeza = cabeza->siguiente;
    }
    printf("NULL\n");
}
// Función para imprimir la lista
void imprimirListasCombinadas(struct Nodo *cabeza)
{
    struct Nodo *actual = cabeza;
    while (actual != NULL)
    {
        printf("%d ", actual->dato);
        actual = actual->siguiente;
    }
}

// Función para imprimir la lista en formato CSV y guardarla en un archivo
void guardarListaCSV(struct Nodo *cabeza, const char *nombreArchivo)
{
    // Abrir el archivo en modo de escritura
    FILE *archivo = fopen(nombreArchivo, "a");
    if (archivo == NULL)
    {
        printf("No se pudo abrir el archivo.\n");
        return;
    }

    // Recorrer la lista y escribir en el archivo
    while (cabeza != NULL)
    {
        fprintf(archivo, "%d", cabeza->dato); // Escribir el dato

        if (cabeza->siguiente != NULL)
        {
            fprintf(archivo, ",");
        }
        else
        {
            fprintf(archivo, "\n");
        }

        cabeza = cabeza->siguiente;
    }

    // Cerrar el archivo
    fclose(archivo);
}

// Matrices para almacenar feromonas y visibilidad entre clientes
double feromonas[MAX_CUSTOMERS][MAX_CUSTOMERS];
double visibilidad[MAX_CUSTOMERS][MAX_CUSTOMERS];

// Funcion para inizializar la matriz de feromonas
void inicializar_feromonas(int size)
{
    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++)
        {
            if (i != j)
            {
                feromonas[i][j] = 1.0; // 1 en toda la matriz excepto la diagonal
            }
            else
            {
                feromonas[i][j] = 0.0; // La diagonal debe ser cero
            }
        }
    }
}

void Guardar_Visibilidad()
{
    FILE *archivo;
    bool existe = false;

    // Verifica si el archivo ya existe
    if ((archivo = fopen("MatricesF&V/Matriz_Visibilidad.csv", "r")) != NULL)
    {
        existe = true;
        fclose(archivo);
    }

    // Abre el archivo en modo de escritura (creará uno nuevo si no existe)
    archivo = fopen("MatricesF&V/Matriz_Visibilidad.csv", "w");

    if (archivo == NULL)
    {
        printf("Error al crear el archivo.\n");
        return;
    }

    // Escribe la matriz de feromonas en el archivo CSV
    for (int i = 0; i < MAX_CUSTOMERS; i++)
    {
        for (int j = 0; j < MAX_CUSTOMERS; j++)
        {
            fprintf(archivo, "%lf", visibilidad[i][j]);
            if (j != MAX_CUSTOMERS - 1)
            {
                fprintf(archivo, ",");
            }
        }
        fprintf(archivo, "\n");
    }

    fclose(archivo);

    if (existe)
    {
        printf("Matriz_Visibilidad actualizada exitosamente en Matriz_Feromonas.csv.\n");
    }
    else
    {
        printf("Matriz_Visibilidad guardada exitosamente en Matriz_Feromonas.csv.\n");
    }
}

void Guardar_Feromonas()
{
    FILE *archivo;
    bool existe = false;

    // Verifica si el archivo ya existe
    if ((archivo = fopen("MatricesF&V/Matriz_Feromonas.csv", "r")) != NULL)
    {
        existe = true;
        fclose(archivo);
    }

    // Abre el archivo en modo de escritura (creará uno nuevo si no existe)
    archivo = fopen("MatricesF&V/Matriz_Feromonas.csv", "w");

    if (archivo == NULL)
    {
        printf("Error al crear el archivo.\n");
        return;
    }

    // Escribe la matriz de feromonas en el archivo CSV
    for (int i = 0; i < MAX_CUSTOMERS; i++)
    {
        for (int j = 0; j < MAX_CUSTOMERS; j++)
        {
            fprintf(archivo, "%lf", feromonas[i][j]);
            if (j != MAX_CUSTOMERS - 1)
            {
                fprintf(archivo, ",");
            }
        }
        fprintf(archivo, "\n");
    }

    fclose(archivo);

    if (existe)
    {
        printf("Matriz de feromonas actualizada exitosamente en Matriz_Feromonas.csv.\n");
    }
    else
    {
        printf("Matriz de feromonas guardada exitosamente en Matriz_Feromonas.csv.\n");
    }
}

// Función para imprimir la matriz de feromonas
void Visualizar_Feromonas()
{
    printf("*******************************************Feromonas*******************************************\n");
    for (int i = 0; i < MAX_CUSTOMERS; i++)
    {
        for (int j = 0; j < MAX_CUSTOMERS; j++)
        {
            printf("%.2lf\t", feromonas[i][j]);
        }
        printf("\n");
    }
}

// Función para truncar un número a un solo decimal
double TruncarAUnDecimal(double valor)
{
    return floor(valor * 10.0) / 10.0;
}

// Función para inicializar la visibilidad entre clientes
void inicializar_visibilidad(int size, Customer clientes[])
{
    // Inicialización de la matriz de visibilidad
    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++)
        {
            if (i != j)
            {
                // Cálculo de la distancia euclidiana entre dos clientes
                double distancia = sqrt(pow(clientes[i].xCoord - clientes[j].xCoord, 2) +
                                        pow(clientes[i].yCoord - clientes[j].yCoord, 2));
                visibilidad[i][j] = 1.0 / distancia; // Inversa de la distancia como visibilidad
            }
            else
            {
                visibilidad[i][j] = 0.0; // La diagonal debe ser cero
            }
        }
    }
}

// Función para retornar la distancia con respecto a los clientes que nos llegue, truncada a un decimal
double Calcular_Distancia(Customer Origen, Customer Destino)
{
    double distancia = sqrt(pow(Origen.xCoord - Destino.xCoord, 2) + pow(Origen.yCoord - Destino.yCoord, 2));
    return distancia;
}

double Calcular_Tiempo_Recorrido(double Distancia)
{
    // printf("Distancia calcula: %lf\n", Distancia);
    double Velocidad = 1;
    double Tiempo = Distancia / Velocidad;
    // printf("Tiempo calculado: %lf\n", Tiempo);
    return Tiempo;
}

// Función para imprimir la matriz de visibilidad
void Visualiar_Visibilidad()
{
    printf("*******************************************Visibilidad*******************************************\n");
    for (int i = 0; i < MAX_CUSTOMERS; i++)
    {
        for (int j = 0; j < MAX_CUSTOMERS; j++)
        {
            printf("%.4lf\t", visibilidad[i][j]);
        }
        printf("\n");
    }
}
// Función para verificar si la capacidad del vehículo permite atender al cliente de destino sin exceder la capacidad máxima
bool Calculo_Capacidad(Customer Destino, Vehicle Vehiculo)
{
    if (Vehiculo.capacity_restant + Destino.Demanda > Vehiculo.capacity)
    {
        return false;
    }
    return true;
}
// Función para calcular el numerador necesario para determinar la probabilidad de seleccionar un destino en la construcción de la ruta
// Función para calcular el numerador necesario para determinar la probabilidad de seleccionar un destino en la construcción de la ruta
double Calcular_Numerador(Customer Origen, Customer Destino, double Alpha, double Beta, double gamma)
{
    int Origen_Indice = Origen.Cliente;
    int Destino_Indice = Destino.Cliente;

    //(Origen,destino) de la matriz inversa de la distancia
    double Valor_Visibilidad = visibilidad[Origen_Indice][Destino_Indice];
    //(Origen,destino) de la matriz de feromonas
    double Valor_Feromona = feromonas[Origen_Indice][Destino_Indice];

    double tiempo_hasta_vencimiento = Destino.Fecha_Vencimiento;

    // Ajusta la importancia de los tiempos
    double Valor_Tiempos = (tiempo_hasta_vencimiento > 0) ? 1.0 / tiempo_hasta_vencimiento : 0.0;
    // (Origen,destino)^Alpha*(1/(Origen,destino))^Beta = Numerador
    double Numerador = pow(Valor_Feromona, Alpha) * pow(Valor_Visibilidad, Beta) * pow(Valor_Tiempos, gamma);
    // printf("Numerador del cliente: %lf\n", Numerador);
    // printf("Origen_Indice: %d\n", Origen_Indice);
    // printf("Destino_Indice: %d\n", Destino_Indice);
    // printf("Valor_Visibilidad: %lf\n", Valor_Visibilidad);
    // printf("Valor_Feromona: %lf\n", Valor_Feromona);
    return Numerador;
}

float Generar_Valores(double Limite_Inferior, double Limite_Superior)
{
    //srand(obtener_semilla_aleatoria());
    float Valor;
    Valor = Limite_Inferior + ((float)rand() / RAND_MAX) * (Limite_Superior - Limite_Inferior);
    return Valor;
}

// Función donde selecciona un índice basado en las probabilidades acumuladas y genera un número aleatorio para tomar una decisión
int Seleccion_Parte(double *Probabilidades, int Num_Probabilidades)
{
    // Inicializa la semilla para la función rand() con el tiempo actual
    // srand(obtener_semilla_aleatoria());
    srand((unsigned int)time(NULL));

    // Genera un número aleatorio en el rango [0, RAND_MAX]
    int numeroAleatorio = rand() % 100;

    // Normaliza el número aleatorio en el rango [0, 1)
    // double Aleatorio = Generar_Valores(0, 1);
    double Aleatorio = (double)numeroAleatorio / 100;

    // printf("Numero Aleatorio: %lf\n", Aleatorio);
    //  system("pause");

    // Variable para guardar el valor siguiente al aleatorio
    double Sig_Mayor;
    // Pos del cliente elegido
    int Posicion_Cliente_Elegido;

    for (size_t i = 0; i < Num_Probabilidades; ++i)
    {
        if (Probabilidades[i] > Aleatorio)
        {
            Sig_Mayor = Probabilidades[i];
            // Guarda la pos del cliente que se eligio
            Posicion_Cliente_Elegido = i;
            break;
        }
    }
    // printf("Numero Aleatorio: %lf\n", Aleatorio);
    // printf("Numero Mayor: %lf\n", Sig_Mayor);
    // printf("Indice del cliente elegido: %d\n", Posicion_Cliente_Elegido);
    return Posicion_Cliente_Elegido;
}

// Función donde calcula las probabilidades acumuladas para seleccionar un destino durante la construcción de la ruta
double *Probabilidad(double *Numeradores, int Num_Numeradores, double Denominador)
{
    double *Probabilidades = malloc(Num_Numeradores * sizeof(double));

    if (Probabilidades == NULL)
    {
        // Manejo de error en caso de que no se pueda asignar memoria
        fprintf(stderr, "Error al asignar memoria\n");
        exit(EXIT_FAILURE);
    }

    // Cálculo de probabilidades acumuladas
    for (size_t i = 0; i < Num_Numeradores; ++i)
    {
        if (i == 0)
        {
            Probabilidades[i] = (Numeradores[i] / Denominador);
        }
        Probabilidades[i] = (Numeradores[i] / Denominador) + Probabilidades[i - 1];
        // printf("Probabilidades: %lf\n", Probabilidades[i]);
        // printf("Numerador: %lf\n", (Numeradores[i] / Denominador));
    }

    return Probabilidades;
}

// Función para imprimr datos del cliente
void Imprimir_Customer(Customer cliente)
{
    printf("Cliente: %d\n", cliente.Cliente);
    printf("Coordenadas: (%lf, %lf)\n", cliente.xCoord, cliente.yCoord);
    printf("Demanda: %d\n", cliente.Demanda);
    printf("Tiempo de inicio permitido: %lf\n", cliente.Tiempo_Inicio);
    printf("Fecha limite para el servicio: %lf\n", cliente.Fecha_Vencimiento);
    printf("Tiempo de servicio necesario: %lf\n", cliente.Tiempo_Servicio);
    printf("\n");
}

// Funcion verifica si un índice específico está presente en la lista Tabú.
bool Validar_Tabu_Indice(struct Nodo *cabeza, int Indice)
{
    // Recorre la lista Tabú
    while (cabeza != NULL)
    {
        // Compara el índice actual con el índice proporcionado
        if (cabeza->dato == Indice)
        {
            // El índice está presente en la lista Tabú
            return true;
        }
        // Avanza al siguiente nodo en la lista Tabú
        cabeza = cabeza->siguiente;
    }

    return false;
}

// Función devuelve el último índice presente en la lista Tabú.
int Ultimo_Indice(struct Nodo *cabeza)
{
    // Recorre la lista Tabú hasta el último nodo
    while (cabeza->siguiente != NULL)
    {
        // Avanza al siguiente nodo en la lista Tabú
        cabeza = cabeza->siguiente;
    }
    // Devuelve el último índice en la lista Tabú
    return cabeza->dato;
}

int SiguienteAleatorioEnteroModN(long *semilla, int n)
// DEVUELVE UN ENTERO ENTRE 0 Y n-1
{
    double a;
    int v;
    long double zi, mhi31 = 2147483648u, ahi31 = 314159269u, chi31 = 453806245u;
    long int dhi31;
    zi = *semilla;
    zi = (ahi31 * zi) + chi31;
    if (zi > mhi31)
    {
        dhi31 = (long int)(zi / mhi31);
        zi = zi - (dhi31 * mhi31);
    }
    *semilla = (long int)zi;
    zi = zi / mhi31;
    a = zi;
    v = (int)(a * n);
    if (v == n)
        return (v - 1);
    return (v);
}

// Función para la construcción de rutas utilizando el algoritmo de colonia de hormigas
bool Calculo_Probabilidad(Customer *Destinos, struct Nodo **Tabu, struct Nodo **Tabu_Vehiculo, int num_hormigas, int num_destino_x_hormiga, Vehicle *Vehiculo)
{
    // Imprime la lista Tabú del vehículo para fines de depuración
    // imprimirLista(*Tabu_Vehiculo);
    // Obtiene el último índice de la lista Tabú del vehículo
    int indice_ult = Ultimo_Indice(*Tabu_Vehiculo);

    // Crea un objeto Customer 'Origen' con la información del cliente correspondiente al último índice en la lista Tabú del vehículo
    Customer Origen;
    Origen.Cliente = Destinos[indice_ult].Cliente;
    Origen.Demanda = Destinos[indice_ult].Demanda;
    Origen.Fecha_Vencimiento = Destinos[indice_ult].Fecha_Vencimiento;
    Origen.Tiempo_Inicio = Destinos[indice_ult].Tiempo_Inicio;
    Origen.Tiempo_Servicio = Destinos[indice_ult].Tiempo_Servicio;
    Origen.xCoord = Destinos[indice_ult].xCoord;
    Origen.yCoord = Destinos[indice_ult].yCoord;

    // Imprime información del cliente Origen
    // printf("Origen: %d\n", Origen.Cliente);
    // printf("Demanda: %d\n", Origen.Demanda);
    // printf("Fecha de vencimiento: %lf\n", Origen.Fecha_Vencimiento);
    // printf("Tiempo de inicio: %lf\n", Origen.Tiempo_Inicio);
    // printf("Tiempo de servicio: %lf\n", Origen.Tiempo_Servicio);
    // printf("Coordenadas: (%lf, %lf)\n", Origen.xCoord, Origen.yCoord);

    // Inicializa un arreglo de clientes 'Destinos_Posibles' para almacenar los destinos que aún no han sido seleccionados
    int Numero_DestinosPosibles = 1;
    Customer *Destinos_Posibles = malloc(Numero_DestinosPosibles * sizeof(Customer));

    // Manejo de error en caso de que no se pueda asignar memoria
    if (Destinos_Posibles == NULL)
    {
        fprintf(stderr, "Error al asignar memoria\n");
    }
    
    // Itera sobre todos los clientes para identificar los destinos que aún no han sido seleccionados
    for (int k = 0; k < MAX_CUSTOMERS; k++)
    {
        // Verifica si el índice del cliente no está en la lista Tabú general
        if (!Validar_Tabu_Indice(*Tabu, k))
        {
            double distancia_recorrida = Calcular_Distancia(Origen, Destinos[k]);
            double tiempo_del_recorrido = Calcular_Tiempo_Recorrido(distancia_recorrida);
            // printf("Distancia recorrida: %lf\n", distancia_recorrida);
            // printf("Tiempo del recorrido: %lf\n", tiempo_del_recorrido);
            // printf("Tiempo calculado del vehiculo: %lf\n", Vehiculo->Tiempo_Consumido + tiempo_del_recorrido);
            // printf("Vehiculo: %d\n", Vehiculo->number);
            // printf("Tiempo calculado del destino Inicio: %lf\n", Destinos[k].Tiempo_Inicio);
            // printf("Tiempo calculado del destino Vencimiento: %lf\n", Destinos[k].Fecha_Vencimiento);
            // printf("Destino: %d\n", k);
            //  system("pause");
            //   printf("Tiempo calculado del vehiculo: %lf\n", Vehiculo->Tiempo_Consumido + tiempo_del_recorrido);
            //   printf("Vehiculo: %d\n", Vehiculo->number);
            //   printf("Tiempo calculado del destino Inicio: %lf\n", Destinos[k].Tiempo_Inicio);
            //   printf("Tiempo calculado del destino Vencimiento: %lf\n", Destinos[k].Fecha_Vencimiento);
            //    system("pause");

            if (Vehiculo->Tiempo_Consumido + tiempo_del_recorrido >= Destinos[k].Tiempo_Inicio && Vehiculo->Tiempo_Consumido + tiempo_del_recorrido <= Destinos[k].Fecha_Vencimiento)
            {
                if (Vehiculo->capacity_restant + Destinos[k].Demanda <= Vehiculo->capacity)
                {
                    // printf("Es posible el destino: %d\n", k);
                    //  Almacena información del destino posible
                    Destinos_Posibles[Numero_DestinosPosibles - 1].Cliente = Destinos[k].Cliente;
                    Destinos_Posibles[Numero_DestinosPosibles - 1].Demanda = Destinos[k].Demanda;
                    Destinos_Posibles[Numero_DestinosPosibles - 1].Fecha_Vencimiento = Destinos[k].Fecha_Vencimiento;
                    Destinos_Posibles[Numero_DestinosPosibles - 1].Tiempo_Inicio = Destinos[k].Tiempo_Inicio;
                    Destinos_Posibles[Numero_DestinosPosibles - 1].Tiempo_Servicio = Destinos[k].Tiempo_Servicio;
                    Destinos_Posibles[Numero_DestinosPosibles - 1].xCoord = Destinos[k].xCoord;
                    Destinos_Posibles[Numero_DestinosPosibles - 1].yCoord = Destinos[k].yCoord;

                    Numero_DestinosPosibles++;
                    // Reasigna memoria para almacenar más destinos posibles
                    Destinos_Posibles = realloc(Destinos_Posibles, Numero_DestinosPosibles * sizeof(Customer));
                    // Manejo de error en caso de que no se pueda asignar memoria
                    if (Destinos_Posibles == NULL)
                    {
                        fprintf(stderr, "Error al aumentar el tamaño del array de personas\n");
                    }
                }
            }

            /*if (Vehiculo->capacity_restant + Destinos[k].Demanda <= Vehiculo->capacity)
            {

            }

        }

        */

            // printf("Es posible el destino: %d\n", k);
            //  Almacena información del destino posible
            /*
            Destinos_Posibles[Numero_DestinosPosibles - 1].Cliente = Destinos[k].Cliente;
            Destinos_Posibles[Numero_DestinosPosibles - 1].Demanda = Destinos[k].Demanda;
            Destinos_Posibles[Numero_DestinosPosibles - 1].Fecha_Vencimiento = Destinos[k].Fecha_Vencimiento;
            Destinos_Posibles[Numero_DestinosPosibles - 1].Tiempo_Inicio = Destinos[k].Tiempo_Inicio;
            Destinos_Posibles[Numero_DestinosPosibles - 1].Tiempo_Servicio = Destinos[k].Tiempo_Servicio;
            Destinos_Posibles[Numero_DestinosPosibles - 1].xCoord = Destinos[k].xCoord;
            Destinos_Posibles[Numero_DestinosPosibles - 1].yCoord = Destinos[k].yCoord;

            Numero_DestinosPosibles++;
            // Reasigna memoria para almacenar más destinos posibles
            Destinos_Posibles = realloc(Destinos_Posibles, Numero_DestinosPosibles * sizeof(Customer));
            // Manejo de error en caso de que no se pueda asignar memoria
            if (Destinos_Posibles == NULL)
            {
                fprintf(stderr, "Error al aumentar el tamaño del array de personas\n");
            }
            */
        }
    }
    // Ajusta el número total de destinos posibles
    Numero_DestinosPosibles--;

    if (Numero_DestinosPosibles != 0)
    {
        // printf("Numero de Destinos Posibles: %d\n", Numero_DestinosPosibles);
        // printf("Alpha: %lf\n", Alpha);
        // printf("Beta: %lf\n\n\n", Beta);

        // Inicializa un arreglo de 'Numeradores' para almacenar los resultados de los cálculos de numerador para cada destino posible
        double Numeradores[Numero_DestinosPosibles];
        double Denominador = 0;
        int cont = 0;

        // Itera sobre todos los destinos posibles para calcular los numeradores y el denominador
        for (int i = 0; i < Numero_DestinosPosibles; i++)
        {
            Numeradores[cont] = Calcular_Numerador(Origen, Destinos_Posibles[i], Alpha, Beta, Gamma);
            Denominador = Denominador + Numeradores[cont];
            cont++;
        }

        // printf("Denominador: %lf\n\n\n", Denominador);

        // Calcula las probabilidades acumuladas utilizando los numeradores y el denominador
        double *Probabilidades = Probabilidad(Numeradores, Numero_DestinosPosibles, Denominador);

        // Selecciona un destino basado en las probabilidades acumuladas y genera un número aleatorio para tomar una decisión
        int Posicion_Probabilidades_Elegida = Seleccion_Parte(Probabilidades, Numero_DestinosPosibles);

        // printf("Lista global\n");
        insertarAlFinal(Tabu, Destinos_Posibles[Posicion_Probabilidades_Elegida].Cliente);
        // imprimirLista(*Tabu);

        // printf("Lista vehiculo\n");
        insertarAlFinal(Tabu_Vehiculo, Destinos_Posibles[Posicion_Probabilidades_Elegida].Cliente);
        // imprimirLista(*Tabu_Vehiculo);

        double distancia_recorrida = Calcular_Distancia(Origen, Destinos_Posibles[Posicion_Probabilidades_Elegida]);
        double tiempo_del_recorrido = Calcular_Tiempo_Recorrido(distancia_recorrida);
        Vehiculo->Tiempo_Consumido = Vehiculo->Tiempo_Consumido + Destinos_Posibles[Posicion_Probabilidades_Elegida].Tiempo_Servicio + tiempo_del_recorrido;
        Vehiculo->capacity_restant = Vehiculo->capacity_restant + Destinos[Posicion_Probabilidades_Elegida].Demanda;
        // system("pause");
        free(Destinos_Posibles);
        free(Probabilidades);
        return true;
    }
    else
    {
        free(Destinos_Posibles);
        return false;
    }
}

// Función para la impresion de los vehiculos
void Imprimir_Vehiculos(Vehicle *Vehiculos, int Numero_Vehiculos)
{
    // Imprimir información de los vehículos
    for (int i = 0; i < Numero_Vehiculos; i++)
    {
        printf(" * Vehiculo: %d - Capacidad: %d - Tiempo Consumido: %lf - Tiempo Maximo: %lf\n", Vehiculos[i].number, Vehiculos[i].capacity, Vehiculos[i].Tiempo_Consumido, Vehiculos[i].Tiempo_Maximo);
    }
}

// Función para la impresion de los clientes
void Imprimir_Informacion_Clientes(Customer *clientes)
{
    // Imprimir información de los clientes
    for (int i = 0; i < MAX_CUSTOMERS; i++)
    {
        printf("Cliente: %d, Coordenadas: (%lf, %lf), Demanda: %d, Tiempo de inicio: %lf, Fecha de vencimiento: %lf, Tiempo de servicio: %lf\n",
        clientes[i].Cliente, clientes[i].xCoord, clientes[i].yCoord,
        clientes[i].Demanda, clientes[i].Tiempo_Inicio,
        clientes[i].Fecha_Vencimiento, clientes[i].Tiempo_Servicio);
    }
}

/*
// Función para calcular la distancia total recorrida al seguir la lista Tabu de clientes.
double Recorrer_Tabu_Distancia(struct Nodo *cabeza, Customer *Clientes)
{
    double Distancia = 0.0;        // Variable para almacenar la distancia total recorrida
    double Distancia_Actual = 0.0; // Variable para almacenar la distancia entre dos clientes consecutivos
    // Imprime información de control
    // printf("\nIndices Tabu: \n");
    // Itera sobre la lista Tabu
    while (cabeza != NULL)
    {
        if (cabeza->siguiente == NULL)
        {
            break;
        }
        // Imprime información de los clientes consecutivos en la lista Tabu
        //printf(" %d - %d       Origen: %d        Destino: %d\n", cabeza->dato, cabeza->siguiente->dato, Clientes[cabeza->dato].Cliente, Clientes[cabeza->siguiente->dato].Cliente);

        // Calcula la distancia entre dos clientes consecutivos
        Distancia_Actual = Calcular_Distancia(Clientes[cabeza->dato], Clientes[cabeza->siguiente->dato]);
        //printf("Distancia entre estos puntos: %lf\n", Distancia_Actual);

        //  Suma la distancia actual a la distancia total
        Distancia = Distancia + Distancia_Actual;
        //printf("Distancia global del Tabu: %lf\n", Distancia);
        //  Avanza al siguiente nodo en la lista Tabu
        cabeza = cabeza->siguiente;
 
    }
    // Devuelve la distancia total recorrida
    return Distancia;
}

// Función para actualizar la matriz de feromonas según la lista Tabu.
void Actualizar_Feromonas(double Delta, double Rho, struct Nodo *cabeza)
{
    // Recorrer la matriz de feromonas
    for (int i = 0; i < MAX_CUSTOMERS; i++)
    {
        for (int j = 0; j < MAX_CUSTOMERS; j++)
        {
            if (i != j)
            {
                // Evapora las feromonas existentes según el factor Rho
                feromonas[i][j] = feromonas[i][j] * (1 - Rho);
            }
            else
            {
                feromonas[i][j] = 0.0; // La diagonal debe ser cero
            }
        }
    }
    // Itera sobre la lista Tabu
    while (cabeza != NULL)
    {
        if (cabeza->siguiente == NULL)
        {
            break;
        }
        // Imprime información de los clientes consecutivos en la lista Tabu
        //printf(" %d - %d\n", cabeza->dato, cabeza->siguiente->dato);
        // Actualiza las feromonas entre clientes consecutivos en la lista Tabu
        feromonas[cabeza->dato][cabeza->siguiente->dato] = feromonas[cabeza->dato][cabeza->siguiente->dato] + Delta;

        feromonas[cabeza->siguiente->dato][cabeza->dato] = feromonas[cabeza->siguiente->dato][cabeza->dato] + Delta;
        // feromonas[cabeza->dato][cabeza->siguiente->dato] = feromonas[cabeza->dato][cabeza->siguiente->dato] * (1-Rho) + Delta;
        //  Avanza al siguiente nodo en la lista Tabu
        cabeza = cabeza->siguiente;
    }

    feromonas[0][0] = 0;
}*/

// Función para calcular la distancia total recorrida al seguir la lista Tabu de clientes.
double Recorrer_Tabu_Distancia(struct Nodo *cabeza, Customer *Clientes)
{
    if (cabeza == NULL || cabeza->siguiente == NULL) {
        return 0.0; // Manejar el caso de lista vacía o un solo elemento
    }

    double Distancia = 0.0;
    double Distancia_Actual = 0.0;
    
    while (cabeza != NULL && cabeza->siguiente != NULL)
    {
        Distancia_Actual = Calcular_Distancia(Clientes[cabeza->dato], Clientes[cabeza->siguiente->dato]);
        Distancia += Distancia_Actual;
        cabeza = cabeza->siguiente;
    }
    
    return Distancia;
}



// Función para actualizar la matriz de feromonas según la lista Tabu.
void Actualizar_Feromonas(double Delta, double Rho, struct Nodo *cabeza)
{
  for (int i = 0; i < MAX_CUSTOMERS; i++)
    {
        for (int j = 0; j < MAX_CUSTOMERS; j++)
        {
            if (i != j)
            {
                // Evapora las feromonas existentes según el factor Rho
                feromonas[i][j] = feromonas[i][j] * (1 - Rho);
            }
            else
            {
                feromonas[i][j] = 0.0; // La diagonal debe ser cero
            }
        }
    }
    if (cabeza == NULL || cabeza->siguiente == NULL) {
        return; // No hay elementos suficientes para actualizar
    }
    
    while (cabeza != NULL && cabeza->siguiente != NULL)
    {
        feromonas[cabeza->dato][cabeza->siguiente->dato] += Delta;
        //feromonas[cabeza->siguiente->dato][cabeza->dato] += Delta;
        cabeza = cabeza->siguiente;
    }
    feromonas[0][0] = 0.0;
}


// Función copia los elementos de una lista enlazada a otra nueva lista.
void copiarLista(struct Nodo *origen, struct Nodo **destino)
{
    // Itera sobre la lista original
    while (origen != NULL)
    {
        // Inserta el elemento actual al final de la nueva lista
        insertarAlFinal(destino, origen->dato);
        // Avanza al siguiente elemento en la lista original
        origen = origen->siguiente;
    }
}

// Función para agregar un elemento a la lista
void agregarElemento(struct Nodo **cabeza, int dato)
{
    struct Nodo *nuevoNodo = (struct Nodo *)malloc(sizeof(struct Nodo));
    nuevoNodo->dato = dato;
    nuevoNodo->siguiente = *cabeza;
    *cabeza = nuevoNodo;
}

// Función para copiar una lista a otra
void copiarListas(struct Nodo *origen, struct Nodo **destino)
{
    while (origen != NULL)
    {
        agregarElemento(destino, origen->dato);
        origen = origen->siguiente;
    }
}

char *Ruta_Archivo(int Tipo_Iteracion, int Numero_Iteracion)
{
    char *Ruta_Archivo;
    char *CSV;
    char *ArchivoInstancia;

    // Tipo de Vector para actualizar en este momento CVS
    // 1 = Vector Objetivo = V_Objetivo.csv
    // 2 = Vector Ruido = V_Ruido.csv
    // 3 = Vector Prueba = V_Prueba.csv
    if (Tipo_Iteracion == 1)
    {
        CSV = "Vectores/V_Objetivo";
    }
    else if (Tipo_Iteracion == 2)
    {
        CSV = "Vectores/V_Ruido";
    }
    else if (Tipo_Iteracion == 3)
    {
        CSV = "Vectores/V_Prueba";
    }

    char Num_Ite[10];
    sprintf(Num_Ite, "%d", Numero_Iteracion);

    size_t Ruta = strlen(CSV) + strlen("/") + strlen(Num_Ite) + strlen(".csv") + 1;
    Ruta_Archivo = malloc(Ruta);
    // strcpy(Ruta_Archivo_1, "/");
    strcpy(Ruta_Archivo, CSV);
    strcat(Ruta_Archivo, "/");
    strcat(Ruta_Archivo, Num_Ite);
    strcat(Ruta_Archivo, ".csv");
    printf("Ruta Archivo: %s\n", Ruta_Archivo);
    
    return Ruta_Archivo;
}


// Función para manejar la señal SIGSEGV
void manejador_segfault(int sig) {
    fprintf(stderr, "Error: Violacion de segmento. El programa se detuvo inesperadamente.\n");
    printf("Hola");
    FILE *archivo_m;
    archivo_m = fopen("FuncionObjetivo/FO.txt", "w");

    

    // Escribe el valor en el archivo
    fprintf(archivo_m, "Error: Violacion de segmento. El programa se detuvo inesperadamente.");

    // Cierra el archivo
    fclose(archivo_m);

    // Realizar cualquier acción necesaria, como registrar el error en un archivo de registro
    exit(1); // Salir del programa con un código de error
}


// Genera una lista global para las rutas de todos los vehiculos
struct Nodo *Tabu = NULL;
int main(int argc, char *argv[])
{
    clock_t inicio, fin;

    // Registrar el manejador de señales para SIGSEGV
    signal(SIGSEGV, manejador_segfault);

    // Registra el tiempo de inicio
    inicio = clock();

    double tiempo_transcurrido;

    const char *nombreArchivo = "C101.csv";

    // IMPORTANTE Verifica si se proporcionaron suficientes argumentos en la línea de comandos
    if (argc < 7)
    {
        printf("Uso: %s <num_iteraciones> <num_hormigas> <alpha> <beta> <gamma> <rho> \n", argv[0]); // gamma es la importancia de el ultimo tiempo para visitar al cliente
        return 1;
    }


    // Obtener parámetros desde la línea de comandos
    int num_iteraciones = atoi(argv[1]);
    int num_hormigas = atoi(argv[2]);
    Alpha = atof(argv[3]);
    Beta = atof(argv[4]);
    Gamma = atof(argv[5]); // este es el que añadi
    Rho = atof(argv[6]);
    int Tipo_Vector = atoi(argv[7]);
    int N_Iteracion = atoi(argv[8]);

    // Ruta del archivo a guardar dependiendo del tipo de vector y la iteración actual la mejor ruta
    char *Route_Archive = Ruta_Archivo(Tipo_Vector, N_Iteracion);

    int numeroDeLineas = 102; // Asumiendo un valor para la demostración del archivo C101.csv

    // Imprime información de la instancia y parámetros
    printf("El archivo %s tiene %d lineas.\n", nombreArchivo, numeroDeLineas);
    // system("pause"); // LOGS

    // Abrir el archivo de entrada
    FILE *archivo;
    archivo = fopen("Instancias/Csv/C101.csv", "r");
    // Manejo de errores si no se pudo abrir el archivo
    if (archivo == NULL)
    {

        fprintf(stderr, "No se pudo abrir el archivo\n");
        return 1;
    }

    // Leer información de la instancia desde el archivo
    char nombre_instancia[50];
    int Numero_Vehiculos;
    int Capacidad;

    fscanf(archivo, "%s", nombre_instancia);
    fscanf(archivo, "%d,%d", &Numero_Vehiculos, &Capacidad);

    // Imprime información de la instancia y parámetros
    printf("Nombre de la instancia: %s\n", nombre_instancia);
    printf("Numero de Vehiculos: %d\n", Numero_Vehiculos);
    printf("Capacidad de c/Vehiculo: %d\n", Capacidad);

    // Inicializa la información de los vehículos
    Vehicle Vehiculos[Numero_Vehiculos];

    for (int i = 0; i < Numero_Vehiculos; i++)
    {
        Vehiculos[i].number = i + 1;
        Vehiculos[i].capacity = Capacidad;
    }

    // Leer información de clientes desde el archivo
    Customer clientes[MAX_CUSTOMERS];

    for (int i = 0; i < MAX_CUSTOMERS; i++)
    {
        fscanf(archivo, "%d,%lf,%lf,%d,%lf,%lf,%lf",
               &clientes[i].Cliente, &clientes[i].xCoord, &clientes[i].yCoord,
               &clientes[i].Demanda, &clientes[i].Tiempo_Inicio,
               &clientes[i].Fecha_Vencimiento, &clientes[i].Tiempo_Servicio);
    }

    // Cerrar el archivo después de la lectura
    fclose(archivo);

    // Imprimir información de los clientes
    //Imprimir_Informacion_Clientes(clientes);
    //system("pause"); // LOGS

    for (int i = 0; i < Numero_Vehiculos; i++)
    {
        Vehiculos[i].Tiempo_Consumido = clientes[0].Tiempo_Inicio;  // Tiempo de inicio permitido en relacion al deposito que es Cliente [0] = 0
        Vehiculos[i].Tiempo_Maximo = clientes[0].Fecha_Vencimiento; // Fecha de vencimiento en relacion al deposito que es Cliente [0] = 1236
    }

    //Imprimir_Vehiculos(Vehiculos, Numero_Vehiculos);

    // system("pause"); // LOGS

    // Inicializar feromonas y visibilidad
    inicializar_feromonas(MAX_CUSTOMERS);

    Guardar_Feromonas();
    // system("pause"); // LOGS

    inicializar_visibilidad(MAX_CUSTOMERS, clientes);

    Guardar_Visibilidad();
    // system("pause"); // LOGS

    double Mejor_Distancia = INFINITY;     // Inicializado a infinito para asegurar que cualquier distancia sea mejor
    struct Nodo *Mejor_Ruta[num_hormigas]; // Lista para almacenar la mejor ruta
    struct Nodo *listaCombinada = NULL;    // Lista para almacenar la mejor ruta de todas las hormigas combinadas en una sola lista para calcular la feromona
    // struct Nodo *Tabu_Vehiculo[num_hormigas];

    //  Bucle principal que ejecuta el Algoritmo de Colonia de Hormigas durante un número especificado de iteraciones.
    srand(obtener_semilla_aleatoria());
    for (int x = 0; x < num_iteraciones; x++)
    {
        //printf("Iteracion: %d\n", x);

        // Inicialización de cada vehículo con el tiempo de inicio permitido, la fecha de vencimiento y la capacidad restante del vehículo en relación con el depósito (Cliente [0])
        for (int i = 0; i < Numero_Vehiculos; i++)
        {
            Vehiculos[i].Tiempo_Consumido = clientes[0].Tiempo_Inicio;
            Vehiculos[i].Tiempo_Maximo = clientes[0].Fecha_Vencimiento;
            Vehiculos[i].capacity_restant = 0;
        }

        // Imprimir_Vehiculos(Vehiculos, Numero_Vehiculos);
        // system("pause"); // LOGS

        // Inicialización de Tabu_Vehiculo para cada hormiga
        struct Nodo *Tabu_Vehiculo[num_hormigas];


        /*for (int i = 0; i < num_hormigas; i++)
        {
            Tabu_Vehiculo[i] = NULL;
        }*/

        // Asignación de memoria y configuración inicial de Tabu_Vehiculo
        for (int i = 0; i < num_hormigas; i++)
        {

            Tabu_Vehiculo[i] = (struct Nodo *)malloc(sizeof(struct Nodo));
            Tabu_Vehiculo[i] = NULL;
            insertarAlFinal(&Tabu_Vehiculo[i], 0);
            
        }
        

        /*for (int i = 0; i < num_hormigas; i++)
        {
            insertarAlFinal(&Tabu_Vehiculo[i], 0);
        }
        */

        // Inicialización de Tabu con un depósito
        insertarAlFinal(&Tabu, 0);
        

        // printf("Lista de Hormigas\n");
        //  Impresión de las listas Tabu_Vehiculo iniciales para cada hormiga

        /*for (int i = 0; i < num_hormigas; i++)
        {
            printf("Hormiga %d ", i);
            imprimirLista(Tabu_Vehiculo[i]);
        }*/

        // system("pause"); // LOGS

        // printf("Lista Tabu General\n");
        // imprimirLista(Tabu);
        // system("pause"); // LOGS

        // Bool para verificar si se superó el número máximo de intentos
        bool Prueba = false;
         int comparacion = 0;
        for (int i = 0; i < MAX_CUSTOMERS - 1; i++)
        {
            int Numero_Intentos = 0;
            // Bool para verificar si se asignó un cliente a un vehículo
            bool Se_Asigno = false;
            do
            {
                // printf("Iteracion: %d\n", i);

                // Generación de un número aleatorio para seleccionar una hormiga
                //srand(obtener_semilla_aleatoria());
                sleep(0.01);
                
                int Hormiga = Generar_Valores(0, 10);
                //no generar el mismo numero de hormiga en la misma iteracion
                /*
                while (comparacion == Hormiga)
                {
                    Hormiga = Generar_Valores(0, 10);
                }
                comparacion = Hormiga;
                */
                
                
                // printf("Hormiga seleccionada: %d\n", Hormiga);
                // system("pause"); // LOGS


                //  Cálculo de probabilidades y actualización de la lista Tabu_Vehiculo para la hormiga seleccionada
                Se_Asigno = Calculo_Probabilidad(clientes, &Tabu, &Tabu_Vehiculo[Hormiga], num_hormigas, MAX_CUSTOMERS / num_hormigas, &Vehiculos[Hormiga]);

                if (Se_Asigno)
                {
                    //  Impresión de la lista Tabu_Vehiculo actualizada para la hormiga seleccionada
                    // printf("Hormiga seleccionada %d ", Hormiga);
                    // imprimirLista(Tabu_Vehiculo[Hormiga]);

                    // system("pause"); // LOGS
                }

                //   Incremento del número de intentos
                Numero_Intentos++;
                //  Verificación de si se superó el número máximo de intentos
                if (Numero_Intentos > 100)
                {
                    Prueba = true;
                    // printf("Error: Se superó el número máximo de intentos.\n");
                    break;
                }

            } while (!Se_Asigno);
        }

        // Verificación de si se superó el número máximo de intentos y reinicio del bucle principal si es necesario 
        if (Prueba)
        {
            num_iteraciones++;
            // Reinicia la lista Tabu para la siguiente iteración
            free(Tabu);
            Tabu = NULL;
            for (int i = 0; i < num_hormigas; i++)
            {
                free(Tabu_Vehiculo[i]);
                Tabu_Vehiculo[i] = NULL;
            }
            // free(Tabu_Vehiculo);
            //  printf("Error: Se superó el número máximo de intentos.\n");
            continue;
        }

        // Actualización de Tabu_Vehiculo con un depósito al final
        for (int i = 0; i < num_hormigas; i++)
        {
            insertarAlFinal(&Tabu_Vehiculo[i], 0);
        }

        // Impresión de las listas Tabu_Vehiculo después de la actualización
        /*for (int i = 0; i < num_hormigas; i++)
        {
            printf("\n\nTabu del Vehiculo %d:", i);
            imprimirLista(Tabu_Vehiculo[i]);
        }*/
        // system("pause"); // LOGS

        // Cálculo de distancias para cada vehículo y acumulación de distancias totales
        double Distancias_Vehiculos[num_hormigas];
        double Suma_Distancias_Vehiculos = 0;

        for (int i = 0; i < num_hormigas; i++)
        {
            Distancias_Vehiculos[i] = Recorrer_Tabu_Distancia(Tabu_Vehiculo[i], clientes);
            Suma_Distancias_Vehiculos = Suma_Distancias_Vehiculos + Distancias_Vehiculos[i];
            // printf("\nVehiculo %d - Distancia global del Vehiculo: %lf", i, Distancias_Vehiculos[i]);
        }

        // Verificación y actualización de la mejor solución encontrada
        if (Suma_Distancias_Vehiculos < Mejor_Distancia)
        {
            // Actualización de Mejor_Distancia y copia de las mejores rutas encontradas
            Mejor_Distancia = Suma_Distancias_Vehiculos;

            /*for (int i = 0; i < num_hormigas; ++i)
            {
                Mejor_Ruta[i] = NULL;
            }*/
            for (int i = 0; i < num_hormigas; i++)
            {
                Mejor_Ruta[i] = (struct Nodo *)malloc(sizeof(struct Nodo));
                Mejor_Ruta[i] = NULL;
                copiarLista(Tabu_Vehiculo[i], &Mejor_Ruta[i]);
            }
        }

        // Imprime la suma total de las distancias recorridas por todos los vehículos
        //printf("\n\nDistancias Total de los Vehiculos: %lf", Suma_Distancias_Vehiculos);

        // Calcula el valor Delta para actualizar las feromonas
        double Delta = 1.0 / Suma_Distancias_Vehiculos;
        // Imprime Delta
        // printf("\n\nDelta: %lf\n", Delta);

        
        // printf("lista tabu\n");
        //imprimirLista(Tabu);

        // Copiar las listas Tabu_Vehiculo de todas las hormigas en una sola lista para calcular la feromona 
        for (int i = num_hormigas - 1; i >= 0; i--)
        {
            copiarLista(Tabu_Vehiculo[i], &listaCombinada);
        }

        //printf("lista combinada\n");
        //imprimirLista(listaCombinada);

        //Actualizar_Feromonas(Delta, Rho, Tabu);
        
        // Actualiza la matriz de feromonas 
        Actualizar_Feromonas(Delta, Rho, listaCombinada);

        // Liberar memoria de la lista donde se almacenan las rutas combinadas
        listaCombinada = NULL;

        Guardar_Feromonas();

        // Reinicia la lista Tabu para la siguiente iteración
        Tabu = NULL;


        // pausa la ejecución para visualización
        // system("pause");
        // Impresión de la mejor distancia total y las mejores rutas encontradas hasta el momento
        printf("\n\nMejor Distancia Total(FO): %lf\n", Mejor_Distancia);

        printf("Mejor Ruta:\n");

        FILE *archivo_c = fopen(Route_Archive, "w");
        if (archivo_c == NULL)
        {
            fprintf(stderr, "Error al abrir el archivo.\n");
            return 1;
        }
        fclose(archivo_c);
        for (int i = 0; i < num_hormigas; i++)
        {
            printf("Hormiga %d: ", i);
            imprimirLista(Mejor_Ruta[i]);
            guardarListaCSV(Mejor_Ruta[i], Route_Archive);
        }
        // Imprimir_Vehiculos(Vehiculos, Numero_Vehiculos);
        // system("pause"); // LOGS
        // Visualizar_Feromonas();
        //system("pause");

        // Reinicia la lista Tabu para la siguiente iteración
        free(Tabu);
        //Tabu = NULL;
        for (int i = 0; i < num_hormigas; i++)
        {
            free(Tabu_Vehiculo[i]);
            Tabu_Vehiculo[i] = NULL;
        }
        free(listaCombinada);
        // free(Tabu_Vehiculo);
       

    }
    // free(Mejor_Ruta);

    // Liberar memoria de la lista donde se almacenan las mejores rutas encontradas
    for (int i = 0; i < num_hormigas; i++)
    {
        free(Mejor_Ruta[i]);
        Mejor_Ruta[i] = NULL;
    }

    

    FILE *archivo_m;
    archivo_m = fopen("FuncionObjetivo/FO.txt", "w");

    // Verifica si el archivo se abrió correctamente
    if (archivo_m == NULL)
    {
        printf("Error al abrir el archivo.");
        return 1;
    }

    // Escribe el valor en el archivo
    fprintf(archivo_m, "%f", Mejor_Distancia);

    // Cierra el archivo
    fclose(archivo_m);

    // Registra el tiempo de finalización
    fin = clock();

    // Calcula el tiempo transcurrido en segundos
    tiempo_transcurrido = ((double)(fin - inicio)) / CLOCKS_PER_SEC;

    printf("El tiempo de ejecucion fue de %.2f segundos.\n", tiempo_transcurrido);
    

    return 0;
}
