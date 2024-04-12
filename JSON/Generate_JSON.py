"""
Autor: Williams Chan Pescador
Descripción: Este script se encarga de leer un archivo CSV que contiene los datos de una instancia y 
convertirlo a un diccionario. Luego, lee un archivo CSV que contiene la mejor ruta y lo convierte a un diccionario. 
Finalmente, fusiona ambos diccionarios en un solo diccionario y lo convierte a un formato JSON.
"""
import json
import sys
import csv

# Función para leer el archivo CSV y convertirlo a un diccionario (Instancia)
def Dict_Instance (filename : str):
    # Ruta del archivo CSV
    Base_Name = "../Instancias/Csv/" + filename
    # Lee el archivo y divide las líneas
    with open(Base_Name, 'r') as file:
        lines = file.readlines()

    # Obtén el nombre de la instancia
    Name_Instance = lines[0].strip()

    # Obtén el número de vehículos y la capacidad
    Number_Vehicles, Capacity = map(int, lines[1].strip().split(','))

    # Crea un diccionario para almacenar las ciudades
    cities = {}
    for line in lines[2:]:
        data = line.strip().split(',')
        city_id = int(data[0])
        x, y = map(int, data[1:3])
        cities[city_id] = {"x": x, "y": y}

    # Crea el diccionario final
    Data_Dict_Instance = {
        "Name_Instance": Name_Instance,
        "Data_Vehicle": {
            "Number_Vehicles": Number_Vehicles,
            "Capacity": Capacity
        },
        "Cities": cities
    }
    #Retorna el diccionario
    return Data_Dict_Instance

# Función para leer el archivo CSV y convertirlo a un diccionario (Mejor Ruta)
def Dic_Best_Route ():
    # Abre el archivo CSV y lee los datos
    with open('../Vectores/V_Prueba/6.csv', newline='') as csvfile:
        # Lee el archivo CSV y convierte los datos a una lista
        csv_reader = csv.reader(csvfile)
        routes = list(csv_reader)

    # Elimina las filas y columnas vacías (si existen)
    routes = [list(filter(None, route)) for route in routes if route]

    # Crea el diccionario final con el formato deseado
    Dict_Best_Route = {}
    for i, route in enumerate(routes, start=1):
        route_name = f"Route {i}"
        Dict_Best_Route[route_name] = list(map(int, route))



    # Retorna el diccionario
    return Dict_Best_Route

# Función para fusionar la instancia y la mejor ruta en un solo diccionario
def Fusion (Instance, Best_Route):
    # Crea una lista vacía para almacenar los datos fusionados
    Data_Fusion = []
    # Crea un contador para asignar un ID a cada ruta
    Counter_Route = 1
    
    # Recorre las rutas y las ciudades de la instancia
    for Route in Best_Route:
        Point = {
            "id" : Counter_Route,
            "name" : Route,
            "points_" : [Instance["Cities"][i] for i in Best_Route[Route]],
            "points" : []
        }
        
        # Convierte los puntos a un formato JSON
        for i in Point["points_"]:
            Point["points"].append({"x":( i["x"] ) , "y": i["y"] })

        # Elimina la clave "points_" del diccionario 
        Point.pop("points_")

        # Añade el diccionario a la lista
        Data_Fusion.append(Point)
        # Incrementa el contador
        Counter_Route += 1

    # Convierte la lista a un formato JSON y lo imprime
    json_string = json.dumps(Data_Fusion, indent=4)
    print(json_string)



File_Instance = sys.argv[1] # Nombre del archivo de la instancia
Instance = Dict_Instance(File_Instance) # Llama a la función Dict_Instance




# Crea un diccionario para almacenar la mejor ruta (en formato de diccionario)
Best_Route = Dic_Best_Route()

# Llama a la función Fusion para fusionar la instancia y la mejor ruta en un solo diccionario y convertirlo a un formato JSON 
Fusion(Instance, Best_Route)