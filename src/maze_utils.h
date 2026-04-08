#ifndef MAZE_UTILS_H
#define MAZE_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Platform‑specific sleep functions
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

// Maximum maze dimensions (40x40)
#define MAX 40
// Delay in milliseconds between animation frames
#define DELAY_MS 300

// -----------------------------------------------------------------------------
// Backpack (sorted linked list of treasure values)
// -----------------------------------------------------------------------------

// A single node storing a treasure value
typedef struct TreasureNode {
    int value;                  // monetary value of the treasure
    struct TreasureNode* next;  // pointer to the next node (sorted order)
} TreasureNode;

// Backpack structure: maintains a sorted linked list of collected treasures
typedef struct {
    TreasureNode* head;   // pointer to the smallest treasure
    int size;             // number of treasures currently in the backpack
} Backpack;

// Initialise an empty backpack
void initBackpack(Backpack* bp) {
    bp->head = NULL;
    bp->size = 0;
}

// Insert a new treasure value into the backpack, keeping the list sorted
void insertSorted(Backpack* bp, int value) {
    // Create the new node
    TreasureNode* newNode = (TreasureNode*)malloc(sizeof(TreasureNode));
    newNode->value = value;
    newNode->next = NULL;

    // Insert at the beginning if the list is empty or the new value is smallest
    if (bp->head == NULL || value < bp->head->value) {
        newNode->next = bp->head;
        bp->head = newNode;
    } else {
        // Find the correct position (maintaining ascending order)
        TreasureNode* curr = bp->head;
        while (curr->next != NULL && curr->next->value < value) {
            curr = curr->next;
        }
        newNode->next = curr->next;
        curr->next = newNode;
    }
    bp->size++;
}

// Remove the smallest treasure from the backpack (the head of the list)
void removeSmallest(Backpack* bp) {
    if (bp->head == NULL) {
        printf("Mochila vazia, nada a remover.\n");
        return;
    }
    TreasureNode* temp = bp->head;
    bp->head = bp->head->next;
    free(temp);
    bp->size--;
}

// Calculate the total value of all treasures in the backpack
int getTotalValue(Backpack* bp) {
    int total = 0;
    TreasureNode* curr = bp->head;
    while (curr != NULL) {
        total += curr->value;
        curr = curr->next;
    }
    return total;
}

// Display the current contents of the backpack and the total value
void printBackpack(Backpack* bp) {
    printf("\nMochila [");
    TreasureNode* curr = bp->head;
    while (curr != NULL) {
        printf("%d", curr->value);
        if (curr->next != NULL) printf(", ");
        curr = curr->next;
    }
    printf("] (total: %d)\n", getTotalValue(bp));
}

// Free all dynamically allocated memory used by the backpack
void freeBackpack(Backpack* bp) {
    TreasureNode* curr = bp->head;
    while (curr != NULL) {
        TreasureNode* temp = curr;
        curr = curr->next;
        free(temp);
    }
    bp->head = NULL;
    bp->size = 0;
}

// -----------------------------------------------------------------------------
// Screen and animation utilities
// -----------------------------------------------------------------------------

// Clear the terminal screen (platform independent)
void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// Pause execution for a given number of milliseconds
void delayMs(int ms) {
#ifdef _WIN32
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

// Print the current state of the maze, with the player shown as 'P'
void printMap(char map[MAX][MAX], int rows, int cols, int px, int py) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (i == py && j == px)
                printf("P");      // player position
            else
                printf("%c", map[i][j]);   // wall, path, treasure, trap, or exit
        }
        printf("\n");
    }
}

// -----------------------------------------------------------------------------
// Maze reading and solving
// -----------------------------------------------------------------------------

// Read a maze from a text file.
// Format: first line "rowsxcols", then the grid with:
//   '#' = wall, ' ' = path, 'P' = start, 'S' = exit,
//   'T' = treasure, 'A' = trap (armadilha)
// Returns 1 on success, 0 on error.
int readMap(const char* filename, char grid[MAX][MAX], int* rows, int* cols,
            int* startX, int* startY, int* exitX, int* exitY) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Erro: arquivo %s nao encontrado.\n", filename);
        return 0;
    }

    char line[256];
    // Read dimensions (e.g., "10x15")
    if (fgets(line, sizeof(line), file) == NULL) {
        printf("Erro: arquivo vazio.\n");
        fclose(file);
        return 0;
    }
    sscanf(line, "%dx%d", rows, cols);
    if (*rows <= 0 || *rows > MAX || *cols <= 0 || *cols > MAX) {
        printf("Erro: dimensoes invalidas (max %dx%d).\n", MAX, MAX);
        fclose(file);
        return 0;
    }

    // --- FIX: Initialize entire grid with walls ('#') to avoid garbage ---
    for (int i = 0; i < MAX; i++)
        for (int j = 0; j < MAX; j++)
            grid[i][j] = '#';

    // Read each row of the maze
    int i = 0;
    while (i < *rows && fgets(line, sizeof(line), file)) {
        // Remove trailing newline
        line[strcspn(line, "\n")] = '\0';
        int len = (int)strlen(line);

        // Fill exactly 'cols' characters; if line is shorter, pad with spaces
        for (int j = 0; j < *cols; j++) {
            if (j < len)
                grid[i][j] = line[j];
            else
                grid[i][j] = ' ';   // fill missing columns with open path
            // Record start and exit coordinates when encountered
            if (grid[i][j] == 'P') {
                *startX = j;
                *startY = i;
            } else if (grid[i][j] == 'S') {
                *exitX = j;
                *exitY = i;
            }
        }
        i++;
    }
    fclose(file);
    return 1;
}

// Find a path from start to exit using iterative depth‑first search (DFS).
// The path is stored as an array of (x, y) coordinates in order from start to exit.
// Returns 1 if a path exists, 0 otherwise.
int findPath(char grid[MAX][MAX], int rows, int cols,
             int startX, int startY, int exitX, int exitY,
             int path[MAX * MAX][2], int* pathLen) {
    // Directions: up, right, down, left
    int dx[] = {0, 1, 0, -1};
    int dy[] = {-1, 0, 1, 0};

    // Visited matrix to avoid revisiting cells
    int visited[MAX][MAX] = {0};
    // Parent arrays to reconstruct the path after the search
    int parentX[MAX][MAX], parentY[MAX][MAX];

    // Stack frame for iterative DFS: stores current cell and next direction to try
    typedef struct {
        int x, y, dir;
    } Frame;
    Frame stack[MAX * MAX];
    int top = -1;

    // Push the start cell onto the stack
    stack[++top] = (Frame){startX, startY, 0};
    visited[startY][startX] = 1;
    parentX[startY][startX] = -1;
    parentY[startY][startX] = -1;

    while (top >= 0) {
        Frame* cur = &stack[top];
        int x = cur->x, y = cur->y;

        // If we reached the exit, reconstruct the path
        if (x == exitX && y == exitY) {
            // Walk backwards from exit to start using parent pointers
            int px = x, py = y;
            int idx = 0;
            while (px != -1 && py != -1) {
                path[idx][0] = px;
                path[idx][1] = py;
                idx++;
                int tmpX = parentX[py][px];
                int tmpY = parentY[py][px];
                px = tmpX;
                py = tmpY;
            }
            *pathLen = idx;
            // Reverse the list to get order from start to exit
            for (int i = 0; i < idx / 2; i++) {
                int tx = path[i][0], ty = path[i][1];
                path[i][0] = path[idx - 1 - i][0];
                path[i][1] = path[idx - 1 - i][1];
                path[idx - 1 - i][0] = tx;
                path[idx - 1 - i][1] = ty;
            }
            return 1;
        }

        // If there are still directions to explore from the current cell
        if (cur->dir < 4) {
            int nx = x + dx[cur->dir];
            int ny = y + dy[cur->dir];
            cur->dir++;   // move to next direction for next time this frame is visited

            // Check bounds, not visited, and not a wall
            if (nx >= 0 && nx < cols && ny >= 0 && ny < rows &&
                !visited[ny][nx] && grid[ny][nx] != '#') {
                visited[ny][nx] = 1;
                parentX[ny][nx] = x;
                parentY[ny][nx] = y;
                stack[++top] = (Frame){nx, ny, 0};   // push new cell
            }
        } else {
            // All four directions have been tried – backtrack
            top--;
        }
    }
    // No path found
    return 0;
}

// -----------------------------------------------------------------------------
// Simulation and display
// -----------------------------------------------------------------------------

// Simulate walking along the found path step by step.
// At each step, handle treasures (T) and traps (A) by updating the backpack.
// Animate the player movement with a delay.
void simulateAndDisplay(char original[MAX][MAX], int rows, int cols,
                        int path[MAX * MAX][2], int pathLen,
                        Backpack* backpack) {
    // Create a mutable copy of the original maze for display
    char display[MAX][MAX];
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            display[i][j] = original[i][j];

    // Start at the first cell of the path (should be the start position)
    int curX = path[0][0], curY = path[0][1];

    // Show initial state
    clearScreen();
    printMap(display, rows, cols, curX, curY);
    printBackpack(backpack);
    delayMs(DELAY_MS);

    // Walk through the remaining steps of the path
    for (int step = 1; step < pathLen; step++) {
        int nx = path[step][0], ny = path[step][1];
        char cell = original[ny][nx];   // what is in the cell we are about to enter

        // Process special cells (except exit, which ends the simulation)
        if (cell == 'T') {
            int valor = rand() % 100 + 1;   // random treasure value between 1 and 100
            insertSorted(backpack, valor);
            printf("Tesouro encontrado! +%d moedas.\n", valor);
            display[ny][nx] = ' ';          // treasure is consumed, remove from display
        } else if (cell == 'A') {
            int lostCount = 0;
            if (backpack->head != NULL) {
                removeSmallest(backpack);   // lose the smallest treasure
                lostCount = 1;
            }
            printf("Armadilha! Perdeu %d tesouro(s)!!!\n", lostCount);
            display[ny][nx] = ' ';          // trap is consumed
        } else if (cell == 'S') {
            // Exit reached – finish simulation and show final score
            curX = nx;
            curY = ny;
            clearScreen();
            printMap(display, rows, cols, curX, curY);
            printBackpack(backpack);
            printf("\n*** Saida encontrada! ***\n");
            printf("Valor total acumulado: %d moedas.\n", getTotalValue(backpack));
            return;
        }

        // Update current position
        curX = nx;
        curY = ny;

        // Refresh screen
        clearScreen();
        printMap(display, rows, cols, curX, curY);
        printBackpack(backpack);
        delayMs(DELAY_MS);
    }

    // If the path ends but we never saw an 'S', something is wrong
    printf("Fim do caminho, mas saida nao alcançada? Verifique o labirinto.\n");
}

// -----------------------------------------------------------------------------
// Path saving
// -----------------------------------------------------------------------------

// Write the found path to a text file.
// Each line contains "row column" (y x) in order from start to exit.
void savePathToFile(const char* filename, int path[MAX * MAX][2], int pathLen) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        printf("Erro ao criar arquivo de saida %s.\n", filename);
        return;
    }
    for (int i = 0; i < pathLen; i++) {
        fprintf(file, "%d %d\n", path[i][1], path[i][0]);   // (row, column)
    }
    fclose(file);
    printf("Caminho salvo em %s\n", filename);
}

#endif // MAZE_UTILS_H