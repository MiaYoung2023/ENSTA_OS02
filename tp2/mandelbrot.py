# Calcul de l'ensemble de Mandelbrot en python (avec MPI)
import numpy as np
from dataclasses import dataclass
from PIL import Image
from math import log
from time import time
import matplotlib.cm
from typing import Union
from mpi4py import MPI  # Importer MPI pour la parallélisation

@dataclass
class MandelbrotSet:
    max_iterations: int
    escape_radius:  float = 2.0

    def convergence(self, c: complex, smooth=False, clamp=True) -> float:
        value = self.count_iterations(c, smooth) / self.max_iterations
        return max(0.0, min(value, 1.0)) if clamp else value

    def count_iterations(self, c: complex,  smooth=False) -> Union[int,float]:
        if c.real*c.real+c.imag*c.imag < 0.0625:
            return self.max_iterations
        if (c.real+1)*(c.real+1)+c.imag*c.imag < 0.0625:
            return self.max_iterations
        if (c.real > -0.75) and (c.real < 0.5):
            ct = c.real-0.25 + 1.j * c.imag
            ctnrm2 = abs(ct)
            if ctnrm2 < 0.5*(1-ct.real/max(ctnrm2, 1.E-14)):
                return self.max_iterations
        z = 0
        for iter in range(self.max_iterations):
            z = z*z + c
            if abs(z) > self.escape_radius:
                if smooth:
                    return iter + 1 - log(log(abs(z)))/log(2)
                return iter
        return self.max_iterations


# Initialisation de MPI
comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()

# Paramètres de l'image
mandelbrot_set = MandelbrotSet(max_iterations=50, escape_radius=10)
width, height = 1024, 1024
scaleX = 3.0 / width
scaleY = 2.25 / height

# Répartition équitable des lignes
rows_per_process = height // size
start_row = rank * rows_per_process
end_row = (rank + 1) * rows_per_process if rank != size - 1 else height

# Stockage local des calculs
local_convergence = np.empty((end_row - start_row, width), dtype=np.double)

# Début du calcul
deb_total = time()
for y in range(start_row, end_row):
    for x in range(width):
        c = complex(-2.0 + scaleX * x, -1.125 + scaleY * y)
        local_convergence[y - start_row, x] = mandelbrot_set.convergence(c, smooth=True)
fin = time()
print(f"Process {rank}: Temps de calcul = {fin - deb_total:.4f} secondes")

# Collecte des résultats avec MPI.Gatherv
if rank == 0:
    global_convergence = np.empty((height, width), dtype=np.double)
    recvcounts = np.array([rows_per_process * width] * size, dtype=int)
    recvcounts[-1] = (height - (size - 1) * rows_per_process) * width  # Ajustement dernier processus
    displacements = np.array([i * rows_per_process * width for i in range(size)], dtype=int)
else:
    global_convergence = None
    recvcounts = None
    displacements = None

comm.Gatherv(local_convergence.flatten(), (global_convergence, recvcounts, displacements, MPI.DOUBLE), root=0)

# Sauvegarde de l'image
if rank == 0:
    image = Image.fromarray(np.uint8(matplotlib.cm.plasma(global_convergence) * 255))
    filename = f"mandelbrot_parallel_{size}.png"
    image.save(filename)
    fin = time()
    print(f"Image sauvegardée sous '{filename}', Temps de constitution: {fin - deb_total:.4f} secondes")

