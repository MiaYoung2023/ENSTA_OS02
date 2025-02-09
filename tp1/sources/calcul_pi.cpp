# include <chrono>
# include <random>
# include <cstdlib>
# include <sstream>
# include <string>
# include <fstream>
# include <iostream>
# include <iomanip>
# include <mpi.h>

// Attention , ne marche qu'en C++ 11 ou supérieur :
double approximate_pi( unsigned long nbSamples ) 
{
    typedef std::chrono::high_resolution_clock myclock;
    myclock::time_point beginning = myclock::now();
    myclock::duration d = beginning.time_since_epoch();
    unsigned seed = d.count();
    std::default_random_engine generator(seed);
    std::uniform_real_distribution <double> distribution ( -1.0 ,1.0);
    unsigned long nbDarts = 0;
    // Throw nbSamples darts in the unit square [-1 :1] x [-1 :1]
    for ( unsigned sample = 0 ; sample < nbSamples ; ++ sample ) {
        double x = distribution(generator);
        double y = distribution(generator);
        // Test if the dart is in the unit disk
        if ( x*x+y*y<=1 ) nbDarts ++;
    }
    // Number of nbDarts throwed in the unit disk
    double ratio = double(nbDarts)/double(nbSamples);
    return 4*ratio;
}

int main( int nargs, char* argv[] )
{
	// On initialise le contexte MPI qui va s'occuper :
	//    1. Créer un communicateur global, COMM_WORLD qui permet de gérer
	//       et assurer la cohésion de l'ensemble des processus créés par MPI;
	//    2. d'attribuer à chaque processus un identifiant ( entier ) unique pour
	//       le communicateur COMM_WORLD
	//    3. etc...
	auto program_start = std::chrono::high_resolution_clock::now();
	MPI_Init( &nargs, &argv );
	// Pour des raisons de portabilité qui débordent largement du cadre
	// de ce cours, on préfère toujours cloner le communicateur global
	// MPI_COMM_WORLD qui gère l'ensemble des processus lancés par MPI.
	MPI_Comm globComm;
	MPI_Comm_dup(MPI_COMM_WORLD, &globComm);
	// On interroge le communicateur global pour connaître le nombre de processus
	// qui ont été lancés par l'utilisateur :
	int nbp;
	MPI_Comm_size(globComm, &nbp);
	// On interroge le communicateur global pour connaître l'identifiant qui
	// m'a été attribué ( en tant que processus ). Cet identifiant est compris
	// entre 0 et nbp-1 ( nbp étant le nombre de processus qui ont été lancés par
	// l'utilisateur )
	int rank;
	MPI_Comm_rank(globComm, &rank);
	// Création d'un fichier pour ma propre sortie en écriture :
	// std::stringstream fileName;
	// fileName << "Output" << std::setfill('0') << std::setw(5) << rank << ".txt";
	// std::ofstream output( fileName.str().c_str() );

	// Rajout de code....
	// 计算局部 PI 
	//calcul local PI
	unsigned long totalSamples = 10000000;
	unsigned long localSamples = totalSamples / nbp;
	double local_pi = approximate_pi(localSamples);
	double global_pi = 0.0;

	// 进程 0 收集所有进程的局部 pi 计算结果
	// Process 0 collects the local pi calculation results of all processes
	MPI_Reduce(&local_pi, &global_pi, 1, MPI_DOUBLE, MPI_SUM, 0, globComm);

	// 每个进程输出自己的计算结果
	// Each process outputs its own calculation result
	// output << std::fixed << std::setprecision(10);
	// output << "Process " << rank << " computed local_pi = " << local_pi << std::endl;

	// 进程 0 计算最终 PI 并输出
	// Process 0 calculates the final PI and outputs it
	auto program_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> program_duration = program_end - program_start;

	if (rank == 0) {
		global_pi /= nbp; // 取平均值
		std::cout << std::fixed << std::setprecision(10);
		std::cout << "Approximate value of π = " << global_pi << std::endl;
		std::cout << "Total execution time for the program: " 
                  << program_duration.count() << " seconds" << std::endl;
	}


	// output.close();
	// A la fin du programme, on doit synchroniser une dernière fois tous les processus
	// afin qu'aucun processus ne se termine pendant que d'autres processus continue à
	// tourner. Si on oublie cet instruction, on aura une plantage assuré des processus
	// qui ne seront pas encore terminés.



	MPI_Finalize();
	return EXIT_SUCCESS;
}

