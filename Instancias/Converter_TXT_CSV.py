"""
Autor: Williams Chan Pescador
Descripción: Este script se encarga de leer un archivo txt que contiene los datos de una instancia y
convertirlo a un archivo CSV.
"""

import csv
import sys
import os


# Función para convertir un archivo .txt a .csv
def txt_to_csv(input_file, output_file, name_instance):
    # Abre el archivo de entrada y salida
    with open(input_file, 'r') as infile, open(output_file, 'w', newline='') as outfile:
        lines = infile.readlines()
        lines_iter = iter(lines)
        writer = csv.writer(outfile)
        writer.writerow([name_instance])
        for line in lines_iter:
            line = line.strip()
            if not line:
                continue  # Ignore empty lines

            if line.isupper():
                current_section = line
                header = next(lines_iter).split()
                continue

            data = line.split()
            writer.writerow(data)
            

if __name__ == "__main__":
    # Especifica las rutas de los directorios aquí
    input_directory = "Instancias/Vrp-Set-Solomon/"
    output_directory = "Instancias/Csv/"

    # Nombre de la instancia
    name_instance = "C102"

    txt_file = os.path.join(input_directory, name_instance + ".txt")
    csv_file = os.path.join(output_directory, name_instance + ".csv")
    txt_to_csv(txt_file, csv_file, name_instance)
    print(f"Se ha convertido {txt_file} a {csv_file}")
