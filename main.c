#include "src/maze_utils.h"  // Contains maze reading, solving, and display utilities

int main(int argc, char* argv[]) {
    // Seed the random number generator for any stochastic elements (e.g., random events)
    srand(time(NULL));

    // Ask the user for the maze file name
    char filename[100];
    printf("Digite o nome do arquivo do labirinto: ");
    scanf("%99s", filename);   // Read at most 99 characters to avoid buffer overflow

    // Grid to store the maze (maximum size defined in maze_utils.h)
    char grid[MAX][MAX];
    int rows, cols;            // Dimensions of the maze
    int startX, startY;        // Coordinates of the starting point
    int exitX, exitY;          // Coordinates of the exit

    // Load the maze from the file; if it fails, exit with error
    if (!readMap(filename, grid, &rows, &cols, &startX, &startY, &exitX, &exitY)) {
        return 1;
    }

    // Find a path from start to exit using backtracking
    int path[MAX * MAX][2];    // Array to store the sequence of (x, y) positions
    int pathLen = 0;           // Number of steps in the found path

    if (!findPath(grid, rows, cols, startX, startY, exitX, exitY, path, &pathLen)) {
        printf("Nenhum caminho encontrado do início a saida.\n");
        return 1;
    }
    printf("Caminho encontrado com %d passos.\n", pathLen);

    // Save the found path to a text file (e.g., "solution.txt")
    savePathToFile("solution.txt", path, pathLen);

    // Prepare a backpack structure that may hold items collected during simulation
    Backpack backpack;
    initBackpack(&backpack);

    // Run an interactive or animated simulation showing the path and backpack usage
    simulateAndDisplay(grid, rows, cols, path, pathLen, &backpack);

    // Free any dynamically allocated resources inside the backpack
    freeBackpack(&backpack);

    return 0;
}