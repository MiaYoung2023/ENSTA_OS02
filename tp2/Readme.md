# TD n° 2 - 27 Janvier 2025

##  1. Parallélisation ensemble de Mandelbrot

L'ensensemble de Mandebrot est un ensemble fractal inventé par Benoit Mandelbrot permettant d'étudier la convergence ou la rapidité de divergence dans le plan complexe de la suite récursive suivante :
$$
\left\{
\begin{array}{l}
    c\,\,\textrm{valeurs\,\,complexe\,\,donnée}\\
    z_{0} = 0 \\
    z_{n+1} = z_{n}^{2} + c
\end{array}
\right.
$$
dépendant du paramètre $c$.

Il est facile de montrer que si il existe un $N$ tel que $\mid z_{N} \mid > 2$, alors la suite $z_{n}$ diverge. Cette propriété est très utile pour arrêter le calcul de la suite puisqu'on aura détecter que la suite a divergé. La rapidité de divergence est le plus petit $N$ trouvé pour la suite tel que $\mid z_{N} \mid > 2$.

On fixe un nombre d'itérations maximal $N_{\textrm{max}}$. Si jusqu'à cette itération, aucune valeur de $z_{N}$ ne dépasse en module 2, on considère que la suite converge.

L'ensemble de Mandelbrot sur le plan complexe est l'ensemble des valeurs de $c$ pour lesquels la suite converge.

Pour l'affichage de cette suite, on calcule une image de $W\times H$ pixels telle qu'à chaque pixel $(p_{i},p_{j})$, de l'espace image, on associe une valeur complexe  $c = x_{min} + p_{i}.\frac{x_{\textrm{max}}-x_{\textrm{min}}}{W} + i.\left(y_{\textrm{min}} + p_{j}.\frac{y_{\textrm{max}}-y_{\textrm{min}}}{H}\right)$. Pour chacune des valeurs $c$ associées à chaque pixel, on teste si la suite converge ou diverge.

- Si la suite converge, on affiche le pixel correspondant en noir
- Si la suite diverge, on affiche le pixel avec une couleur correspondant à la rapidité de divergence.

1. À partir du code séquentiel `mandelbrot.py`, faire une partition équitable par bloc suivant les lignes de l'image pour distribuer le calcul sur `nbp` processus  puis rassembler l'image sur le processus zéro pour la sauvegarder. Calculer le temps d'exécution pour différents nombre de tâches et calculer le speedup. Comment interpréter les résultats obtenus ?

Réponse:
<br>Le code séquentiel `mandelbrot.py` est séquentiel, donc il faut modifier le code par *mpi4py* et ajouter des lignes de code pour mesurer le temps d'exécution et calculer le speedup.
<br>Il convient de noter que *comm.Gather* ne garantit pas automatiquement que les données sont concaténées dans le bon ordre.
<br>Solution : utilisez *comm.Gatherv* pour garantir un ordre positif qui nous permet de spécifier la taille des données envoyées par chaque processus et où elles sont stockées, garantissant ainsi que la convergence globale finale est correcte.

Nbp | Temps d'exécution (s) | Speedup
:--:|:---------------------:|:------:
1   |2.4895                 | 1
2   |1.7747                 | 1.40
3   |1.2447                 | 2.00
4   |1.8398                 | 1.35
5   |0.8976                 | 2.77
6   |0.8573                 | 2.90
7   |0.8864                 | 2.82
8   |0.8613                 | 2.90

<div style="text-align: center;">
  <img src="mandelbrot_parallel_8.png" width="500"/>
</div>

2. Réfléchissez à une meilleur répartition statique des lignes au vu de l'ensemble obtenu sur notre exemple et mettez la en œuvre. Calculer le temps d'exécution pour différents nombre de tâches et calculer le speedup et comparez avec l'ancienne répartition. Quel problème pourrait se poser avec une telle stratégie ?

Nbp | Temps d'exécution optimisé(s) | Speedup | 
:--:|:-----------------------------:|:-------:|
1   |4.5163                         | 1                
2   |2.8842                         | 1.57
3   |2.6774                         | 1.69
4   |3.2317                         | 1.40
5   |2.5697                         | 1.75
6   |3.0948                         | 1.46
7   |3.2870                         | 1.37
8   |3.2505                         | 1.38

En supposant que la région centrale de l'ensemble de Mandelbrot est plus coûteuse en termes de calcul, je rends les lignes de la région centrale denses et les lignes des régions de bord clairsemées. Mais cela conduit finalement à une dégradation des performances, ce qui est pire qu’une distribution uniforme.
<br>C'est parce que je n'ai pas vraiment équilibré la charge de calcul des processus, mais je les ai divisés en fonction du nombre de lignes plutôt que de la quantité de calcul. Certains processus peuvent encore être surchargés de calcul, tandis que d'autres peuvent ne pas être en mesure de calculer suffisamment, ce qui entraîne un gaspillage de ressources.

3. Mettre en œuvre une stratégie maître-esclave pour distribuer les différentes lignes de l'image à calculer. Calculer le speedup avec cette approche et comparez  avec les solutions différentes. Qu'en concluez-vous ?

Nbp | Temps d'exécution optimisé(s) | Speedup | 
:--:|:-----------------------------:|:-------:|
1   |4.1728                         | 1                
2   |4.5343                         | 0.92
3   |4.8591                         | 0.86
4   |3.4304                         | 1.21
5   |3.4380                         | 1.21
6   |3.6173                         | 1.15
7   |2.9360                         | 1.42
8   |3.3399                         | 1.25

Le modèle maître-esclave (maître-travailleur) est utilisé pour allouer dynamiquement des tâches de calcul en ligne afin d'optimiser l'utilisation des ressources de calcul.
<br>Voici les principales améliorations :
<br>Le processus principal (rang = 0) est responsable de l'allocation dynamique des tâches de calcul de Mandelbrot (par ligne), de la collecte des résultats de calcul de tous les processus et de leur regroupement en une image complète.
<br>Les processus de travail (rang> 0) demandent en continu des tâches (équilibrage de charge dynamique basé sur l'extraction), calculent la ligne de Mandelbrot et renvoient les résultats.
<br>Théoriquement, il peut réaliser un équilibrage de charge dynamique, car les tâches de calcul sont allouées à la demande, ce qui convient au calcul d'ensembles de Mandelbrot avec une densité inégale ; il n'y a pas de nombre fixe de lignes allouées, ce qui évite le problème que certains processus prennent beaucoup de temps à calculer tandis que d'autres processus sont inactifs ; en même temps, il réduit le temps d'attente des processus, et les processus demandent de nouvelles tâches immédiatement après avoir terminé une tâche, gardant le CPU pleinement utilisé.
<br>Mais en fait, dans MPI, **la communication entre les processus** représente une charge importante, en particulier lors de l’allocation de tâches parallèles et de l’agrégation des résultats. Par exemple, les processus maître et travailleur communiquent fréquemment entre eux (par exemple, via comm.send et comm.recv). Chaque fois qu'un processus de travail termine une tâche, il doit renvoyer le résultat au processus principal via le réseau. Des communications fréquentes entraînent une augmentation des frais généraux, en particulier lorsque la quantité de données est importante ou qu'il existe de nombreux processus. Les frais généraux de communication peuvent dépasser le temps de calcul lui-même, affectant ainsi l'amélioration des performances.
## 2. Produit matrice-vecteur

On considère le produit d'une matrice carrée $A$ de dimension $N$ par un vecteur $u$ de même dimension dans $\mathbb{R}$. La matrice est constituée des cœfficients définis par $A_{ij} = (i+j) \mod N$. 

Par soucis de simplification, on supposera $N$ divisible par le nombre de tâches `nbp` exécutées.

### a - Produit parallèle matrice-vecteur par colonne

Afin de paralléliser le produit matrice–vecteur, on décide dans un premier temps de partitionner la matrice par un découpage par bloc de colonnes. Chaque tâche contiendra $N_{\textrm{loc}}$ colonnes de la matrice. 

- Calculer en fonction du nombre de tâches la valeur de Nloc
- Paralléliser le code séquentiel `matvec.py` en veillant à ce que chaque tâche n’assemble que la partie de la matrice utile à sa somme partielle du produit matrice-vecteur. On s’assurera que toutes les tâches à la fin du programme contiennent le vecteur résultat complet.
- Calculer le speed-up obtenu avec une telle approche

nbp |   temps   | speed-up |
:--:|:---------:|:--------:|
1   | 0.0029    | 1
2   | 0.0040    | 0.725
3   | 0.0020    | 1.45
4   | 0.0020    | 1.45
5   | 0.0083    | 0.35
6   | 0.0105    | 0.277
7   | 0.0122    | 0.241
8   | 0.0020    | 1.45


### b - Produit parallèle matrice-vecteur par ligne

Afin de paralléliser le produit matrice–vecteur, on décide dans un deuxième temps de partitionner la matrice par un découpage par bloc de lignes. Chaque tâche contiendra $N_{\textrm{loc}}$ lignes de la matrice.

- Calculer en fonction du nombre de tâches la valeur de Nloc
- paralléliser le code séquentiel `matvec.py` en veillant à ce que chaque tâche n’assemble que la partie de la matrice utile à son produit matrice-vecteur partiel. On s’assurera que toutes les tâches à la fin du programme contiennent le vecteur résultat complet.
- Calculer le speed-up obtenu avec une telle approche

nbp |   temps   | speed-up |
:--:|:---------:|:--------:|
1   | 0.0029    | 1
2   | 0.0020    | 1.45
3   | 0.0015    | 1.93
4   | 0.0038    | 0.75
5   | 0.0022    | 1.3
6   | 0.0077    | 0.38
7   | 0.0100    | 0.29
8   | 0.0040    | 0.73

#### Conclusion de question 2
<br>Il existe deux principales approches pour paralléliser le produit matrice-vecteur : la partition par colonnes et la partition par lignes.
<br>**Partition par colonnes** : Chaque processus traite un bloc de colonnes de la matrice et doit effectuer une réduction globale (somme des résultats partiels). Cette méthode nécessite que chaque processus possède le vecteur u complet, mais entraîne un coût de communication plus élevé.
<br>**Partition par lignes** : Chaque processus effectue directement le produit de certaines lignes avec le vecteur u. Le résultat est naturellement divisé entre les processus, ce qui réduit la communication et améliore l'efficacité en mémoire cache.
<br>En général, la partition par lignes est préférable en calcul distribué, car elle minimise la communication et améliore les performances. 

## 3. Entraînement pour l'examen écrit

Alice a parallélisé en partie un code sur machine à mémoire distribuée. Pour un jeu de données spécifiques, elle remarque que la partie qu’elle exécute en parallèle représente en temps de traitement 90% du temps d’exécution du programme en séquentiel.

En utilisant la loi d’Amdhal, pouvez-vous prédire l’accélération maximale que pourra obtenir Alice avec son code (en considérant n ≫ 1) ?

### Réponse 1:
Alice observe que 90% du programme est parallélisable, donc $P=0.9$ et l’accélération maximale que l’on peut obtenir en supposant n≫1 (nombre de nœuds très grand) est donnée par la limite de la loi d’Amdahl :
$S_{max} = \frac{1}{1-P} = 10$

À votre avis, pour ce jeu de donné spécifique, quel nombre de nœuds de calcul semble-t-il raisonnable de prendre pour ne pas trop gaspiller de ressources CPU ?

### Réponse 2：
L’idée est de choisir un nombre de nœuds qui se rapproche de l’accélération maximale sans gaspiller de ressources. On cherche donc un n tel que l’accélération soit proche de 10 mais sans saturation excessive.
<br>Selon la loi d’Amdahl: $S(n)=\frac{1}{(1-p)+\frac{p}{n}}$ ,donc ,
- pour n=4: $S(4)=3.08$
- pour n=8: $S(8)=4.71$
- pour n=16: $S(16)=6.41$
<br>Donc un choix raisonnable serait n = 4 à 8 nœuds, car au-delà de 8, le gain devient marginal et on gaspille des ressources.



En effectuant son cacul sur son calculateur, Alice s’aperçoit qu’elle obtient une accélération maximale de quatre en augmentant le nombre de nœuds de calcul pour son jeu spécifique de données.

En doublant la quantité de donnée à traiter, et en supposant la complexité de l’algorithme parallèle linéaire, quelle accélération maximale peut espérer Alice en utilisant la loi de Gustafson ?
### Réponse 3：
**La loi de Gustafson** corrige la limite de la loi d’Amdahl en supposant que lorsque plus de processeurs sont utilisés, la taille du problème augmente (au lieu de rester constante) :
<br>$S(n)=n-(1-p)(n-1)$
- pour n=4,$S(4)=4-(1-0.9)(4-1)=3.7$
- pour n=8,$S(8)=8-(1-0.9)(8-1)=7.3$
<br>On peut voir que lorsque n=8, n=4 double. 
<br>Selon la loi de Gustafson, doubler la quantité de données signifie généralement que la quantité de calcul dans la partie parallèle augmente également, améliorant encore le taux d'accélération parallèle. Par conséquent, à mesure que l’échelle de calcul augmente, les avantages du calcul parallèle deviennent plus évidents.



