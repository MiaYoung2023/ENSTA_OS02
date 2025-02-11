# une implémentation du Mandelbrot avec un modèle maître-esclave en MPI
import numpy as np
from dataclasses import dataclass
from PIL import Image
from math import log
from time import time
import matplotlib.cm
from typing import Union
from mpi4py import MPI

@dataclass
class MandelbrotSet:
    max_iterations: int
    escape_radius:  float = 2.0

    def convergence(self, c: complex, smooth=False, clamp=True) -> float:
        value = self.count_iterations(c, smooth) / self.max_iterations
        return max(0.0, min(value, 1.0)) if clamp else value

    def count_iterations(self, c: complex, smooth=False) -> Union[int, float]:
        if c.real * c.real + c.imag * c.imag < 0.0625:
            return self.max_iterations
        if (c.real + 1) * (c.real + 1) + c.imag * c.imag < 0.0625:
            return self.max_iterations
        if -0.75 < c.real < 0.5:
            ct = c.real - 0.25 + 1.j * c.imag
            ctnrm2 = abs(ct)
            if ctnrm2 < 0.5 * (1 - ct.real / max(ctnrm2, 1.E-14)):
                return self.max_iterations
        z = 0
        for iter in range(self.max_iterations):
            z = z * z + c
            if abs(z) > self.escape_radius:
                if smooth:
                    return iter + 1 - log(log(abs(z))) / log(2)
                return iter
        return self.max_iterations

# Initialisation MPI
comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()

# Paramètres de l'image
mandelbrot_set = MandelbrotSet(max_iterations=50, escape_radius=10)
width, height = 1024, 1024
scaleX = 3.0 / width
scaleY = 2.25 / height

# Début du chrono
deb_total = time()

if size == 1:
    # --- Cas particulier : exécution en 1 seul processus ---
    print("Mode séquentiel (1 seul processus)...")
    global_convergence = np.empty((height, width), dtype=np.double)

    for y in range(height):
        for x in range(width):
            c = complex(-2.0 + scaleX * x, -1.125 + scaleY * y)
            global_convergence[y, x] = mandelbrot_set.convergence(c, smooth=True)

    # Sauvegarde de l’image
    image = Image.fromarray(np.uint8(matplotlib.cm.plasma(global_convergence) * 255))
    image.save("mandelbrot_sequentiel.png")

    fin = time()
    print(f"Image sauvegardée sous 'mandelbrot_sequentiel.png'")
    print(f"Temps total d'exécution : {fin - deb_total:.4f} secondes")
else:
    if rank == 0:
        # --- Processus maître ---
        pending_rows = list(range(height))  # Liste des lignes à calculer
        num_workers = size - 1
        tasks_in_progress = {}

        # Stocker les résultats finaux
        global_convergence = np.empty((height, width), dtype=np.double)

        # Envoi initial des tâches
        for worker in range(1, min(num_workers + 1, len(pending_rows) + 1)):
            row = pending_rows.pop(0)
            comm.send(row, dest=worker, tag=1)  # Envoie une ligne à calculer
            tasks_in_progress[worker] = row

        # Gestion dynamique des tâches
        while tasks_in_progress:
            status = MPI.Status()
            line_data = comm.recv(source=MPI.ANY_SOURCE, tag=2, status=status)
            worker = status.Get_source()
            row_completed = tasks_in_progress.pop(worker)

            # Stocke la ligne calculée
            global_convergence[row_completed, :] = line_data

            # Attribuer une nouvelle ligne si disponible
            if pending_rows:
                new_row = pending_rows.pop(0)
                comm.send(new_row, dest=worker, tag=1)
                tasks_in_progress[worker] = new_row
            else:
                comm.send(None, dest=worker, tag=0)  # Fin du travail

        # Sauvegarde de l’image finale
        image = Image.fromarray(np.uint8(matplotlib.cm.plasma(global_convergence) * 255))
        image.save(f"mandelbrot_master_worker_{size}.png")

        fin = time()
        print(f"Image sauvegardée sous 'mandelbrot_master_worker_{size}.png'")
        print(f"Temps total d'exécution : {fin - deb_total:.4f} secondes")

    else:
        # --- Processus esclaves ---
        while True:
            row = comm.recv(source=0, tag=MPI.ANY_TAG, status=MPI.Status())
            if row is None:
                break  # Fin du travail

            # Calculer la ligne du Mandelbrot
            local_result = np.empty(width, dtype=np.double)
            for x in range(width):
                c = complex(-2.0 + scaleX * x, -1.125 + scaleY * row)
                local_result[x] = mandelbrot_set.convergence(c, smooth=True)

            comm.send(local_result, dest=0, tag=2)  # Retourne la ligne calculée
