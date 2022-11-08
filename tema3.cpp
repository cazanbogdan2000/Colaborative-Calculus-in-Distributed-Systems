#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <unistd.h>
#include <string>

using namespace std;


//functie care afiseaza rezultatul final
void print_result(int result_size, int* result) {
    string s = "";
    s += "Rezultat: ";
    for (int i = 0; i < result_size; i++) {
        s = s + to_string(result[i]) + " ";
    }
    s += "\n";
    int n = s.length();
    char char_array[n+1];
    strcpy(char_array, s.c_str());
    printf("%s", char_array);
}

// functie care afiseaza topologia pentru fiecare proces, dupa ce acesta
// termina de aflat toata topologia
void print_topology(int* rank, int n0, int* c0, int n1, int* c1, int n2,
        int* c2) {
    cout << *rank << " -> ";
    cout << "0:";
    for (int i = 0; i < n0 - 1; i++) {
        cout << c0[i] << ",";
    }
    cout << c0[n0 - 1] << " ";
    cout << "1:";
    for (int i = 0; i < n1 - 1; i++) {
        cout << c1[i] << ",";
    }
    cout << c1[n1 - 1] << " ";
    cout << "2:";
    for (int i = 0; i < n2 - 1; i++) {
        cout << c2[i] << ",";
    }
    cout << c2[n2 - 1] << endl;
}

// actiunile pe care le face coordonatorul cluster-ului 0, pentru a aflarea
// topologiei
void topology_coordinator0(int* rank, int nrWorkers, int* workers,
        MPI_Status *status, int* nrCoord0, int* nrCoord1, int* nrCoord2,
        int** cluster0, int** cluster1, int** cluster2) {
    // trimit workerii pentru fiecare cluster coordonatorului clusterului 2
    MPI_Send(&nrWorkers, 1, MPI_INT, 2, *rank, MPI_COMM_WORLD);
    cout << "M(" << *rank << "," << 2 << ")\n";
    MPI_Send(workers, nrWorkers, MPI_INT, 2, *rank, MPI_COMM_WORLD);
    cout << "M(" << *rank << "," << 2 << ")\n";

    // primesc de la coordonatorul clusterului 2 topologia clusterului 1
    int workersOfCoordinator_1, workersOfCoordinator_2;
    MPI_Recv(&workersOfCoordinator_1, 1, MPI_INT, 2, MPI_ANY_TAG, 
        MPI_COMM_WORLD, status);
    int *second_coordinator = (int *)malloc(workersOfCoordinator_1 * sizeof(int));
    MPI_Recv(second_coordinator, workersOfCoordinator_1, MPI_INT, 2,
        MPI_ANY_TAG, MPI_COMM_WORLD, status);

    // primesc de la coordonatorul clusterului 2 topologia clusterului 2
    MPI_Recv(&workersOfCoordinator_2, 1, MPI_INT, 2, MPI_ANY_TAG,
        MPI_COMM_WORLD, status);
    int *last_coordinator = (int *)malloc(workersOfCoordinator_2 * sizeof(int));
    MPI_Recv(last_coordinator, workersOfCoordinator_2, MPI_INT, 2, MPI_ANY_TAG,
        MPI_COMM_WORLD, status);
    // se afiseaza topologia
    print_topology(rank, nrWorkers, workers, workersOfCoordinator_1,
            second_coordinator, workersOfCoordinator_2, last_coordinator);

    for (int i = 0; i < nrWorkers; i++) {
        // trimit workerii lui 0 catre workers[i]
        MPI_Send(&nrWorkers, 1, MPI_INT, workers[i], *rank, MPI_COMM_WORLD);
        cout << "M(" << *rank << "," << workers[i] << ")\n";
        MPI_Send(workers, nrWorkers, MPI_INT, workers[i], *rank, MPI_COMM_WORLD);
        cout << "M(" << *rank << "," << workers[i] << ")\n";
        // trimit workerii lui 1 catre workers[i]
        MPI_Send(&workersOfCoordinator_1, 1, MPI_INT, workers[i], *rank,
            MPI_COMM_WORLD);
        cout << "M(" << *rank << "," << workers[i] << ")\n";
        MPI_Send(second_coordinator, workersOfCoordinator_1, MPI_INT,
            workers[i], *rank, MPI_COMM_WORLD);
        cout << "M(" << *rank << "," << workers[i] << ")\n";
        // trimit workerii lui 2 catre workers[i]
        MPI_Send(&workersOfCoordinator_2, 1, MPI_INT, workers[i], *rank,
            MPI_COMM_WORLD);
        cout << "M(" << *rank << "," << workers[i] << ")\n";
        MPI_Send(last_coordinator, workersOfCoordinator_2, MPI_INT,
            workers[i], *rank, MPI_COMM_WORLD);
        cout << "M(" << *rank << "," << workers[i] << ")\n";   
    }
    // salvez topologia, pentru ca de acum incolo o stiu
    *nrCoord0 = nrWorkers;
    *nrCoord1 = workersOfCoordinator_1;
    *nrCoord2 = workersOfCoordinator_2;
    *cluster0 = (int *)calloc(*nrCoord0, sizeof(int));
    *cluster1 = (int *)calloc(*nrCoord1, sizeof(int));
    *cluster2 = (int *)calloc(*nrCoord2, sizeof(int));
    memcpy(*cluster0, workers, *nrCoord0 * sizeof(int));
    memcpy(*cluster1, second_coordinator, *nrCoord1 * sizeof(int));            
    memcpy(*cluster2, last_coordinator, *nrCoord2 * sizeof(int));
}

// actiunile pe care le face coordonatorul cluster-ului 1, pentru a aflarea
// topologiei
void topology_coordinator1(int* rank, int nrWorkers, int* workers,
        MPI_Status *status, int* nrCoord0, int* nrCoord1, int* nrCoord2,
        int** cluster0, int** cluster1, int** cluster2) {

    int workersOfCoordinator_0;
    int workersOfCoordinator_2;
    
    // primesc workerii de la 2
    MPI_Recv(&workersOfCoordinator_2, 1, MPI_INT, 2, MPI_ANY_TAG,
        MPI_COMM_WORLD, status);
    int* last_coordinator = (int *)malloc(workersOfCoordinator_2 * sizeof(int));
    MPI_Recv(last_coordinator, workersOfCoordinator_2, MPI_INT, 2, MPI_ANY_TAG,
        MPI_COMM_WORLD, status);

    // primesc workerii de la 0
    MPI_Recv(&workersOfCoordinator_0, 1, MPI_INT, 2, MPI_ANY_TAG,
        MPI_COMM_WORLD, status);
    int* first_coordinator = (int *)malloc(workersOfCoordinator_0 * sizeof(int));
    MPI_Recv(first_coordinator, workersOfCoordinator_0, MPI_INT, 2,
        MPI_ANY_TAG, MPI_COMM_WORLD, status);

    // Afisez pentru coordonatorul 1 topologia:
    print_topology(rank, workersOfCoordinator_0,    first_coordinator, nrWorkers,
            workers, workersOfCoordinator_2, last_coordinator);

    for (int i = 0; i < nrWorkers; i++) {
        // trimit workerii lui 0 catre workers[i]
        MPI_Send(&workersOfCoordinator_0, 1, MPI_INT, workers[i], *rank,
            MPI_COMM_WORLD);
        cout << "M(" << *rank << "," << workers[i] << ")\n";
        MPI_Send(first_coordinator, workersOfCoordinator_0, MPI_INT,
            workers[i], *rank, MPI_COMM_WORLD);
        cout << "M(" << *rank << "," << workers[i] << ")\n";
        // trimit workerii lui 1 catre workers[i]
        MPI_Send(&nrWorkers, 1, MPI_INT, workers[i], *rank, MPI_COMM_WORLD);
        cout << "M(" << *rank << "," << workers[i] << ")\n";
        MPI_Send(workers, nrWorkers, MPI_INT, workers[i], *rank, MPI_COMM_WORLD);
        cout << "M(" << *rank << "," << workers[i] << ")\n";   
        // trimit workerii lui 2 catre workers[i]
        MPI_Send(&workersOfCoordinator_2, 1, MPI_INT, workers[i],
            *rank, MPI_COMM_WORLD);
        cout << "M(" << *rank << "," << workers[i] << ")\n";
        MPI_Send(last_coordinator, workersOfCoordinator_2, MPI_INT,
            workers[i], *rank, MPI_COMM_WORLD);
        cout << "M(" << *rank << "," << workers[i] << ")\n";
    }
    // trimit workerii lui 1 catre 2
    MPI_Send(&nrWorkers, 1, MPI_INT, 2, *rank, MPI_COMM_WORLD);
    cout << "M(" << *rank << "," << 2 << ")\n";
    MPI_Send(workers, nrWorkers, MPI_INT, 2, *rank, MPI_COMM_WORLD);
    cout << "M(" << *rank << "," << 2 << ")\n";

    // salvez topologia, pentru ca de acum incolo o stiu
    *nrCoord0 = workersOfCoordinator_0;
    *nrCoord1 = nrWorkers;
    *nrCoord2 = workersOfCoordinator_2;
    *cluster0 = (int *)calloc(*nrCoord0, sizeof(int));
    *cluster1 = (int *)calloc(*nrCoord1, sizeof(int));
    *cluster2 = (int *)calloc(*nrCoord2, sizeof(int));
    memcpy(*cluster0, first_coordinator, *nrCoord0 * sizeof(int));
    memcpy(*cluster1, workers, *nrCoord1 * sizeof(int));   
    memcpy(*cluster2, last_coordinator, *nrCoord2 * sizeof(int));
}

// actiunile pe care le face coordonatorul cluster-ului 2, pentru a aflarea
// topologiei
void topology_coordinator2(int* rank, int nrWorkers, int* workers,
        MPI_Status *status, int* nrCoord0, int* nrCoord1, int* nrCoord2,
        int** cluster0, int** cluster1, int** cluster2) {

    int workersOfCoordinator_0;

    // primesc workerii de la coordonatorul 0 
    MPI_Recv(&workersOfCoordinator_0, 1, MPI_INT, 0, MPI_ANY_TAG, 
        MPI_COMM_WORLD, status);
    int* first_coordinator = (int *)malloc(workersOfCoordinator_0 * sizeof(int));
    MPI_Recv(first_coordinator, workersOfCoordinator_0, MPI_INT, 0,
        MPI_ANY_TAG, MPI_COMM_WORLD, status);

    // trimit workerii lui 2 catre 1
    MPI_Send(&nrWorkers, 1, MPI_INT, 1, *rank, MPI_COMM_WORLD);
    cout << "M(" << *rank << "," << 1 << ")\n";
    MPI_Send(workers, nrWorkers, MPI_INT, 1, *rank, MPI_COMM_WORLD);
    cout << "M(" << *rank << "," << 1 << ")\n";
    // trimit workerii lui 0 catre 1
    MPI_Send(&workersOfCoordinator_0, 1, MPI_INT, 1, *rank, MPI_COMM_WORLD);
    cout << "M(" << *rank << "," << 1 << ")\n";
    MPI_Send(first_coordinator, workersOfCoordinator_0, MPI_INT, 1,
        *rank, MPI_COMM_WORLD);
    cout << "M(" << *rank << "," << 1 << ")\n";

    int workersOfCoordinator_1;
    MPI_Recv(&workersOfCoordinator_1, 1, MPI_INT, 1, MPI_ANY_TAG,
        MPI_COMM_WORLD, status);
    int *second_coordinator = (int *)malloc(workersOfCoordinator_1 * sizeof(int));
    MPI_Recv(second_coordinator, workersOfCoordinator_1, MPI_INT, 1,
        MPI_ANY_TAG, MPI_COMM_WORLD, status);

    // Afisez pentru coordonatorul 2 topologia:
    print_topology(rank, workersOfCoordinator_0, first_coordinator,
            workersOfCoordinator_1, second_coordinator, nrWorkers,  workers);
    
    for (int i = 0; i < nrWorkers; i++) {
        // trimit workerii lui 0 catre workers[i]
        MPI_Send(&workersOfCoordinator_0, 1, MPI_INT, workers[i],
            *rank, MPI_COMM_WORLD);
        cout << "M(" << *rank << "," << workers[i] << ")\n";
        MPI_Send(first_coordinator, workersOfCoordinator_0, MPI_INT, 
            workers[i], *rank, MPI_COMM_WORLD);
        cout << "M(" << *rank << "," << workers[i] << ")\n";
        // trimit workerii lui 1 catre workers[i]
        MPI_Send(&workersOfCoordinator_1, 1, MPI_INT, workers[i], 
            *rank, MPI_COMM_WORLD);
        cout << "M(" << *rank << "," << workers[i] << ")\n";
        MPI_Send(second_coordinator, workersOfCoordinator_1, MPI_INT, 
            workers[i], *rank, MPI_COMM_WORLD);
        cout << "M(" << *rank << "," << workers[i] << ")\n";
        // trimit workerii lui 2 catre workers[i]
        MPI_Send(&nrWorkers, 1, MPI_INT, workers[i], *rank, MPI_COMM_WORLD);
        cout << "M(" << *rank << "," << workers[i] << ")\n";
        MPI_Send(workers, nrWorkers, MPI_INT, workers[i], *rank, MPI_COMM_WORLD);
        cout << "M(" << *rank << "," << workers[i] << ")\n";   
    }

    // trimit workerii lui 1 catre 0
    MPI_Send(&workersOfCoordinator_1, 1, MPI_INT, 0, *rank, MPI_COMM_WORLD);
    cout << "M(" << *rank << "," << 0 << ")\n";
    MPI_Send(second_coordinator, workersOfCoordinator_1, MPI_INT, 0,
        *rank, MPI_COMM_WORLD);
    cout << "M(" << *rank << "," << 0 << ")\n";
    // trimit workerii lui 2 catre 0
    MPI_Send(&nrWorkers, 1, MPI_INT, 0, *rank, MPI_COMM_WORLD);
    cout << "M(" << *rank << "," << 0 << ")\n";
    MPI_Send(workers, nrWorkers, MPI_INT, 0, *rank, MPI_COMM_WORLD);
    cout << "M(" << *rank << "," << 0 << ")\n";
    
    // salvez topologia, pentru ca de acum incolo o stiu
    *nrCoord0 = workersOfCoordinator_0;
    *nrCoord1 = workersOfCoordinator_1;
    *nrCoord2 = nrWorkers;
    *cluster0 = (int *)calloc(*nrCoord0, sizeof(int));
    *cluster1 = (int *)calloc(*nrCoord1, sizeof(int));
    *cluster2 = (int *)calloc(*nrCoord2, sizeof(int));
    memcpy(*cluster0, first_coordinator, *nrCoord0 * sizeof(int));
    memcpy(*cluster1, second_coordinator, *nrCoord1 * sizeof(int));            
    memcpy(*cluster2, workers, *nrCoord2 * sizeof(int));
}

// functia care va face citirea din fisier, va initializa datele, iar apoi se
// va face, asa cum se si cere in enunt, legatura intre cele 3 clustere, astfel
// incat fiecare proces sa stie intreaga topologie
// Mecanism de functionare: procesul coordonator 0 va comunica cu workerii sai,
// si in plus va comunica si cu procesul coordonator 2, caruia ii va trasmite
// topologia sa; coordonatorul 2 va comunica cu workerii sai, si va da mai
// departe topologia sa, precum si topologia lui 0, catre procesul coordonator 1
// Procesul 1 va comunica cu workerii sai, si apoi va intoarce lui 2 topologia
// finala, care va da si la workerii sai, si, in final, coordonatorul 2 va duce
// informatia mai departe catre 0, acesta din urma dand topologia finala si 
// workerilor sai
void topology_coordinators(int *rank, int *coordinator, int *nrCoord0,
        int *nrCoord1, int *nrCoord2, int** cluster0, int** cluster1,
        int** cluster2, MPI_Status *status) {
    char numeFisier[50] = "clusterX.txt";
    numeFisier[7] = '0' + *rank;
    ifstream fin(numeFisier);
    int nrWorkers;
    fin >> nrWorkers;
    // workers -> workerii procesului curent
    int* workers = (int *)malloc(nrWorkers * sizeof(int));
    for (int i = 0; i < nrWorkers; i++) {
        int worker;
        fin >> worker;
        workers[i] = worker;
    }
    fin.close();
    for (int i = 0; i < nrWorkers; i++) {
        MPI_Send(rank, 1, MPI_INT, workers[i], *rank, MPI_COMM_WORLD);
        cout << "M(" << *rank << "," << workers[i] << ")\n";
    }
    // sunt in cluster-ul 0
    if (*rank == 0) {
        topology_coordinator0(rank, nrWorkers, workers, status, nrCoord0,
            nrCoord1, nrCoord2, cluster0, cluster1, cluster2);
    }
    // sunt in cluster-ul 1
    else if (*rank == 1) {
        topology_coordinator1(rank, nrWorkers, workers, status, nrCoord0,
            nrCoord1, nrCoord2, cluster0, cluster1, cluster2);
    }
    // sunt in cluster-ul 2
    else {
        topology_coordinator2(rank, nrWorkers, workers, status, nrCoord0,
            nrCoord1, nrCoord2, cluster0, cluster1, cluster2);
    }    
}

// actiunile pe care le face un worker
void topology_workers(int *rank, int *coordinator, int *nrCoord0,
        int *nrCoord1, int *nrCoord2, int** cluster0, int** cluster1,
        int** cluster2, MPI_Status *status)  {
    // Worker-ul va primi numele procesului coordonator; de acum incolo, procesul
    // worker va sti cine este coordonatorul
    MPI_Recv(coordinator, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, 
        MPI_COMM_WORLD, status);

    int workersOfCoordinator_0, workersOfCoordinator_1, workersOfCoordinator_2;
    
    // primeste topologia pentru cluster-ul 0 de la coordonatorul sau
    MPI_Recv(&workersOfCoordinator_0, 1, MPI_INT, *coordinator,
         MPI_ANY_TAG, MPI_COMM_WORLD, status);
    int *first_coordinator = (int *)malloc(workersOfCoordinator_0 * sizeof(int));
    MPI_Recv(first_coordinator, workersOfCoordinator_0, MPI_INT,
        *coordinator, MPI_ANY_TAG, MPI_COMM_WORLD, status);
    
    // primeste topologia pentru cluster-ul 1 de la coordonatorul sau
    MPI_Recv(&workersOfCoordinator_1, 1, MPI_INT, *coordinator,
        MPI_ANY_TAG, MPI_COMM_WORLD, status);
    int *second_coordinator = (int *)malloc(workersOfCoordinator_1 * sizeof(int));
    MPI_Recv(second_coordinator, workersOfCoordinator_1, MPI_INT,
        *coordinator, MPI_ANY_TAG, MPI_COMM_WORLD, status);

    // primeste topologia pentru cluster-ul 2 de la coordonatorul sau
    MPI_Recv(&workersOfCoordinator_2, 1, MPI_INT, *coordinator,
        MPI_ANY_TAG, MPI_COMM_WORLD, status);
    int *last_coordinator = (int *)malloc(workersOfCoordinator_2 * sizeof(int));
    MPI_Recv(last_coordinator, workersOfCoordinator_2, MPI_INT,
        *coordinator, MPI_ANY_TAG, MPI_COMM_WORLD, status);

     // Afisez pentru worker-ul curent topologia:
    print_topology(rank, workersOfCoordinator_0, first_coordinator,
            workersOfCoordinator_1, second_coordinator, workersOfCoordinator_2,
            last_coordinator);

    // salvez topologia, pentru ca de acum incolo o stiu
    *nrCoord0 = workersOfCoordinator_0;
    *nrCoord1 = workersOfCoordinator_1;
    *nrCoord2 = workersOfCoordinator_2;
    *cluster0 = (int *)calloc(*nrCoord0, sizeof(int));
    *cluster1 = (int *)calloc(*nrCoord1, sizeof(int));
    *cluster2 = (int *)calloc(*nrCoord2, sizeof(int));
    memcpy(*cluster0, first_coordinator, *nrCoord0 * sizeof(int));
    memcpy(*cluster1, second_coordinator, *nrCoord1 * sizeof(int));            
    memcpy(*cluster2, last_coordinator, *nrCoord2 * sizeof(int));
}

// Odata aflata topologia, coordonatorul 0 incepe sa imparta catre celelalte
// procese bucati din vector, pentru a putea dupa computate
// Se realizeaza astfel: trimite direct catre workerii sai prima parte a
// vectorului si va primi noua valoare, salvata in result, va trimite direct
// catre  coordonatorul 2, restul vectorului initial
void calculus_coordinator0(int** result, int* result_size, int numtasks,
        int rank, int nrCoord0, int nrCoord1, int nrCoord2, int* cluster0,
        int* cluster1, int* cluster2, MPI_Status *status, char** argv) {
    int vector_size = atoi(argv[1]);
    *result_size = vector_size;
    int* vector = (int *)malloc(vector_size * sizeof(int));
    for (int i = 0; i < vector_size; i++) {
        vector[i] = i;
    }
    // trimit prima parte a vectorului catre workerii lui 0
    // numtasks - 3 reprezinta numarul total de workeri din toata
    // topologia
    int nrOperationsPerWorker = vector_size / (numtasks - 3);
    int sparsedTasks = vector_size % (numtasks - 3);
    int nrAddedSparsedTasks = 0;

    // trimite bucati din vector ce vor trebui computate (send)
    for (int i = 0; i < nrCoord0; i++) {
        // cazul in care trebuie sa dau un numar inegala a subvectorului (cu unul
        // mai mult ca in rest)
        if (sparsedTasks > nrAddedSparsedTasks) {
            nrOperationsPerWorker++;        
            MPI_Send(&nrOperationsPerWorker, 1, MPI_INT, cluster0[i],
                rank, MPI_COMM_WORLD);
            cout << "M(" << rank << "," << cluster0[i] << ")\n";
            MPI_Send(vector + i * nrOperationsPerWorker + nrAddedSparsedTasks,
                nrOperationsPerWorker, MPI_INT, cluster0[i], rank, MPI_COMM_WORLD);
            cout << "M(" << rank << "," << cluster0[i] << ")\n";
            MPI_Recv(vector + i * nrOperationsPerWorker + nrAddedSparsedTasks,
                nrOperationsPerWorker, MPI_INT, cluster0[i], MPI_ANY_TAG,
                MPI_COMM_WORLD, status);
            nrAddedSparsedTasks++;
            nrOperationsPerWorker--;
        }
        // cazul in care dau o lungime egala
        else {
            MPI_Send(&nrOperationsPerWorker, 1, MPI_INT, cluster0[i],
                rank, MPI_COMM_WORLD);
            cout << "M(" << rank << "," << cluster0[i] << ")\n";

            MPI_Send(vector + i * nrOperationsPerWorker + nrAddedSparsedTasks,
                nrOperationsPerWorker, MPI_INT, cluster0[i], rank, MPI_COMM_WORLD);
            cout << "M(" << rank << "," << cluster0[i] << ")\n";

            MPI_Recv(vector + i * nrOperationsPerWorker + nrAddedSparsedTasks,
                nrOperationsPerWorker, MPI_INT, cluster0[i], MPI_ANY_TAG,
                MPI_COMM_WORLD, status);
        }           
    }

    // trimite acum catre nodul 2 restul vectorului
    int nrTasksDone = nrCoord0 * nrOperationsPerWorker + nrAddedSparsedTasks;
    int vector_left_size = vector_size - nrTasksDone;
    MPI_Send(&nrOperationsPerWorker, 1, MPI_INT, 2, rank, MPI_COMM_WORLD);
    cout << "M(" << rank << "," << 2 << ")\n";
    MPI_Send(&vector_left_size, 1, MPI_INT, 2, rank, MPI_COMM_WORLD);
    cout << "M(" << rank << "," << 2 << ")\n";
    MPI_Send(vector + nrTasksDone, vector_left_size, MPI_INT, 2,
        rank, MPI_COMM_WORLD);
    cout << "M(" << rank << "," << 2 << ")\n";

    MPI_Recv(vector + nrTasksDone, vector_left_size, MPI_INT, 2,
        MPI_ANY_TAG, MPI_COMM_WORLD, status);

    *result = (int *)calloc(vector_size, sizeof(int));
    memcpy(*result, vector, vector_size * sizeof(int));
}

// Coorodnatorul 1 primeste de la 2 subvectorul ramas, il va da worker-ilor
// sai sa faca computatii, va primi rezultatul si va incepe dupa sa intoarca
// partea sa catre 2, care va da mai departe catre 0
void calculus_coordinator1(int numtasks, int rank, int nrCoord0, int nrCoord1,
        int nrCoord2, int* cluster0, int* cluster1, int* cluster2,
        MPI_Status *status) {

    // primeste subvectorul de prelucrat
    int vector_size, nrOperationsPerWorker;
    MPI_Recv(&nrOperationsPerWorker, 1, MPI_INT, 2, MPI_ANY_TAG,
        MPI_COMM_WORLD, status);
    MPI_Recv(&vector_size, 1, MPI_INT, 2, MPI_ANY_TAG, MPI_COMM_WORLD, status);
    int* vector = (int *)calloc(vector_size, sizeof(int));
    MPI_Recv(vector, vector_size, MPI_INT, 2, MPI_ANY_TAG,
        MPI_COMM_WORLD, status);

    // trimite la workeri cate o bucata
    for (int i = 0; i < nrCoord1; i++) {
        MPI_Send(&nrOperationsPerWorker, 1, MPI_INT, cluster1[i],
            rank, MPI_COMM_WORLD);
        cout << "M(" << rank << "," << cluster1[i] << ")\n";

        MPI_Send(vector + i * nrOperationsPerWorker, nrOperationsPerWorker,
            MPI_INT, cluster1[i], rank, MPI_COMM_WORLD);
        cout << "M(" << rank << "," << cluster1[i] << ")\n";
    }

    // primeste de la workeri noile bucati, deja prelucrate
    for (int i = 0; i < nrCoord1; i++) {
        MPI_Recv(vector + i * nrOperationsPerWorker, nrOperationsPerWorker,
            MPI_INT, cluster1[i], MPI_ANY_TAG, MPI_COMM_WORLD, status);
    }

    // trimitem catre 2 rezultatul prelucrat
    MPI_Send(vector, vector_size, MPI_INT, 2, rank, MPI_COMM_WORLD);
    cout << "M(" << rank << "," << 2 << ")\n";
}

// Coordonatorul 2 primeste de la 0, da mai departe la workerii sai, da 
// restul vectorului neprelucrat lui 1, dupa care il primeste ianpoi de la 1,
// pentru a da mai departe dupa ambele bucati din vector catre 0; coordonatorul
// 0 va avea rezultatul final
void calculus_coordinator2(int numtasks, int rank, int nrCoord0, int nrCoord1,
        int nrCoord2, int* cluster0, int* cluster1, int* cluster2,
        MPI_Status *status) {

    // primesc de la coordonatorul 0 o bucata din vectorul initial
    int vector_size, nrOperationsPerWorker;
    MPI_Recv(&nrOperationsPerWorker, 1, MPI_INT, 0, MPI_ANY_TAG,
        MPI_COMM_WORLD, status);
    MPI_Recv(&vector_size, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, status);
    int* vector = (int *)calloc(vector_size, sizeof(int));
    MPI_Recv(vector, vector_size, MPI_INT, 0, MPI_ANY_TAG,
        MPI_COMM_WORLD, status);

    int sparsedTasks = vector_size % (numtasks - 3 - nrCoord0);
    int nrAddedSparsedTasks = 0;
    // workerii lui 2 vor face prelucrarile necesare
    for (int i = 0; i < nrCoord2; i++) {
        // cazul in care trebuie sa dau un numar inegala a subvectorului (cu unul
        // mai mult ca in rest)
        if (sparsedTasks > nrAddedSparsedTasks) {
            nrOperationsPerWorker++;
            nrAddedSparsedTasks++;
            MPI_Send(&nrOperationsPerWorker, 1, MPI_INT, cluster2[i],
                rank, MPI_COMM_WORLD);
            cout << "M(" << rank << "," << cluster2[i] << ")\n";

            MPI_Send(vector + i * nrOperationsPerWorker, nrOperationsPerWorker,
                MPI_INT, cluster2[i], rank, MPI_COMM_WORLD);
            cout << "M(" << rank << "," << cluster2[i] << ")\n";
            MPI_Recv(vector + i * nrOperationsPerWorker, nrOperationsPerWorker,
                MPI_INT, cluster2[i], MPI_ANY_TAG, MPI_COMM_WORLD, status);
            nrOperationsPerWorker--;
        }
        // cazul in care dau o lungime egala
        else {
            MPI_Send(&nrOperationsPerWorker, 1, MPI_INT, cluster2[i], rank,
                MPI_COMM_WORLD);
            cout << "M(" << rank << "," << cluster2[i] << ")\n";

            MPI_Send(vector + i * nrOperationsPerWorker + nrAddedSparsedTasks,
                nrOperationsPerWorker, MPI_INT, cluster2[i], rank, MPI_COMM_WORLD);
            cout << "M(" << rank << "," << cluster2[i] << ")\n";
            MPI_Recv(vector + i * nrOperationsPerWorker + nrAddedSparsedTasks,
                nrOperationsPerWorker, MPI_INT, cluster2[i], MPI_ANY_TAG,
                MPI_COMM_WORLD, status);
        }
    }
    // trimite acum catre nodul 1 restul vectorului
    int nrTasksDone = nrCoord2 * nrOperationsPerWorker + nrAddedSparsedTasks;
    int vector_left_size = vector_size - nrTasksDone;
    MPI_Send(&nrOperationsPerWorker, 1, MPI_INT, 1, rank, MPI_COMM_WORLD);
    cout << "M(" << rank << "," << 1 << ")\n";
    MPI_Send(&vector_left_size, 1, MPI_INT, 1, rank, MPI_COMM_WORLD);
    cout << "M(" << rank << "," << 1 << ")\n";
    MPI_Send(vector + nrTasksDone, vector_left_size, MPI_INT, 1,
        rank, MPI_COMM_WORLD);
    cout << "M(" << rank << "," << 1 << ")\n";

    // Primeste rezultatul prelucrat de la coordonatorul 1
    MPI_Recv(vector + nrTasksDone, vector_left_size, MPI_INT, 1,
        MPI_ANY_TAG, MPI_COMM_WORLD, status);

    // trimitem catre 0 rezultatul prelucrat
    MPI_Send(vector, vector_size, MPI_INT, 0, rank, MPI_COMM_WORLD);
    cout << "M(" << rank << "," << 0 << ")\n";
}

// functia main, care are rolul de a initializa tot contextul MPI si de a
// face apelurile necesare de functii puse mai sus
int main (int argc, char *argv[]) {
    int numtasks, rank, len;
    char hostname[MPI_MAX_PROCESSOR_NAME];
 
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    MPI_Get_processor_name(hostname, &len);

    int nrCoord0, nrCoord1, nrCoord2;
    int* cluster0, *cluster1, *cluster2;
    int coordinator = -1; // daca ramane -1, atunci chiar nodul este coordonator
    MPI_Status status;

    // partea de topologie

    // cei trei coordonatori
    if (rank < 3) {
        topology_coordinators(&rank, &coordinator, &nrCoord0, &nrCoord1,
            &nrCoord2, &cluster0, &cluster1, &cluster2, &status);
    }
    // workeri
    else {       
        topology_workers(&rank, &coordinator, &nrCoord0, &nrCoord1,
            &nrCoord2, &cluster0, &cluster1, &cluster2, &status);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    // Partea de calculare a rezultatului
    int* result;
    int result_size;
    // coordonatorul 0
    if (rank == 0) {
        calculus_coordinator0(&result, &result_size, numtasks,  rank, nrCoord0,
            nrCoord1, nrCoord2, cluster0, cluster1, cluster2, &status, argv);
    }
    // coordonatorul 1
    else if (rank == 1) {
        calculus_coordinator1(numtasks, rank, nrCoord0, nrCoord1, nrCoord2,
            cluster0, cluster1, cluster2, &status);
    }
    // coordonatorul 2
    else if (rank == 2) {
        calculus_coordinator2(numtasks, rank, nrCoord0, nrCoord1, nrCoord2,
            cluster0, cluster1, cluster2, &status);
    }
    // fiecare worker va primi de la coordonator numarul de elemente pe care
    // trebuie sa le prelucreze si dupa sa le trimita inapoi la cluster-ul sau
    else {
        int nrOperations;
        int* subvector;
        MPI_Recv(&nrOperations, 1, MPI_INT, coordinator, MPI_ANY_TAG,
            MPI_COMM_WORLD, &status);
        subvector = (int *)calloc(nrOperations, sizeof(int));
        MPI_Recv(subvector, nrOperations, MPI_INT, coordinator, MPI_ANY_TAG,
            MPI_COMM_WORLD, &status);       
        for (int i = 0; i < nrOperations; i++) {
            subvector[i] *= 2;
        }
        MPI_Send(subvector, nrOperations, MPI_INT, coordinator, rank,
            MPI_COMM_WORLD);
        cout << "M(" << rank << "," << coordinator << ")\n";
    }

    MPI_Barrier(MPI_COMM_WORLD);

    // afisarea rezultatului doar de coordonatorul 0
    if (rank == 0) {
        print_result(result_size, result);
    }
    MPI_Finalize();
}
