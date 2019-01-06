#include "digitalbrain.h"

#define TITLE_ELEMENT  "*ELEMENT_SOLID"
#define TITLE_NODE     "*NODE"
#define TITLE_END      "*END"

//-------------------------------------------------------------------------------------------
static int  CmpFunc(const void *item1, const void *item2) {
    const double **val1 = (const double **)item1;
    const double **val2 = (const double **)item2;
    return int(**val1) - int(**val2);
}
//-------------------------------------------------------------------------------------------
bool ReadLsDyna(const char *FileName) {
    // Checking if mesh file can be opened or not
    FILE *File;
    if ((File = fopen(FileName, "rb")) == NULL) {
        printf("\nERROR( proc %d ): Cannot open input file.\n", world_rank);
        return false;
    }

    // Counting number of elements in elements section of mesh file
    // And fixing elements and nodes sections' positions in the file
    const char *Delim = " \t";
    char Line[MAX_FILE_LINE];
    long ElementsSectionPos = -1, NodesSectionPos = -1;
    int AllNodesCount = 0;
    ndim = 0;
    bool Ended = false;
    while (!Ended && fgets(Line, sizeof(Line), File) != NULL) {
        if (strncmp(Line, TITLE_ELEMENT, strlen(TITLE_ELEMENT)) == 0) {
            ElementsSectionPos = ftell(File);
            while (fgets(Line, sizeof(Line), File) != NULL) {
                if (LineToArray(true, false, 2, 1, Line) > 0 && LineToArray(true, true, 3, 0, Line) > 0) {
                    nallelements++;
                }
                if (strncmp(Line, TITLE_NODE, strlen(TITLE_NODE)) == 0) {
                    NodesSectionPos = ftell(File);
                    break;
                }
            }
        }
        
        if (NodesSectionPos != -1) {
            while (ndim == 0 && fgets(Line, sizeof(Line), File) != NULL) {
                if (strstr(Line, "nid") != NULL) {
                    if (strstr(Line, "x") != NULL) {
                        ndim = ndim + 1;
                    }
                    if (strstr(Line, "y") != NULL) {
                        ndim = ndim + 1;
                    }
                    if (strstr(Line, "z") != NULL) {
                        ndim = ndim + 1;
                    }
                }
            }
            if (ndim == 2 || ndim == 3) {
                const int nCols = ndim + 1;
                while (!Ended && fgets(Line, sizeof(Line), File) != NULL) {
                    if (LineToArray(false, false, 1, nCols, Line) == nCols) {
                        AllNodesCount++;
                    }
                    else {
                        Ended = strncmp(Line, TITLE_END, strlen(TITLE_END)) == 0;
                    }
                }
            }
        }
    }

    // Checking if elements and nodes were found in mesh file
    // And checking if we can be back to elements section in the file
    int fseeked = 0;
    if (nallelements == 0 || AllNodesCount == 0 || (fseeked = fseek(File, ElementsSectionPos, SEEK_SET)) != 0) {
        fclose(File);
        if (nallelements == 0) {
            printf("\nERROR( proc %d ): No element found. This means input file is empty or contains invalid data.\n", world_rank);
        }
        if (AllNodesCount == 0) {
            printf("\nERROR( proc %d ): No node found. This means input file is empty or contains invalid data.\n", world_rank);
        }
        if (fseeked != 0) {
            printf("\nERROR( proc %d ): 'fseek()' call for ElementsSectionPos failed.\n", world_rank);
        }
        return false;
    }
    
    // Determining number/count of elements processor will hold
    nelements = nallelements / world_size;
    if (world_rank == world_size - 1) {
        nelements += (nallelements % world_size);
    }

    // Fixing range of lines (in elements section of mesh file) from which processor will read its own elements
    const int From = world_rank * (nallelements / world_size);
    const int To = From + nelements - 1;
    
    // Creating and initializing "eptr" array, and determining size of "connectivity" array of processor
    eptr = (int *)calloc(nelements + 1, sizeof(int));
    int i = 0, j = 0, ConnectivitySize = 0;
    while (fgets(Line, sizeof(Line), File) != NULL) {
        const int np = LineToArray(true, false, 2, 1, Line);
        const int nn = LineToArray(true, true, 3, 0, Line);
        if (np == 0 || nn == 0) {
            continue;
        }
        else if (i >= From && i <= To) {
            j = j + 1;
            eptr[j] = eptr[j - 1] + nn;
            ConnectivitySize = ConnectivitySize + nn;
        }
        if (strncmp(Line, TITLE_NODE, strlen(TITLE_NODE)) == 0) {
            break;
        }
        i = i + 1;
    }

    // Checking if calculated size of connectivity array is valid
    // And checking if we can be back to elements section of mesh file
    if (ConnectivitySize != eptr[nelements] || fseek(File, ElementsSectionPos, SEEK_SET) != 0) {
        fclose(File);
        FreeArrays();
        if (ConnectivitySize != eptr[nelements]) {
            printf("\nERROR( proc %d ): Size of 'eind' array is not vald.\n", world_rank);
        }
        else {
            printf("\nERROR( proc %d ): 'fseek()' call failed.\n", world_rank);
        }
        return false;
    }

    // Creating and initializing "connectivity", "pid" and "ElementType" arrays for processor
    connectivity = (int *)calloc(ConnectivitySize, sizeof(int));
    pid = (int *)calloc(nelements, sizeof(int));
    ElementType = (char **)calloc(nelements, sizeof(char *));
    for (int i = 0; i < nelements; i++) {
        ElementType[i] = (char *)calloc(MAX_ELEMENT_TYPE_SIZE, sizeof(char));
    }
    i = 0;
    int ci = 0, pi = 0;
    while (fgets(Line, sizeof(Line), File) != NULL) {
        int *PIDs, *Nodes;
        const int np = LineToArray(true, false, 2, 1, Line, Delim, (void**)&PIDs);
        const int nn = LineToArray(true, true, 3, 0, Line, Delim, (void**)&Nodes);
        if (np == 0 || nn == 0) {
            if (np > 0) {
                free(PIDs);
            }
            if (nn > 0) {
                free(Nodes);
            }
            continue;
        }
        else if (i >= From && i <= To) {
            pid[pi] = PIDs[0];
            
            if (nn == 8) {
                strcpy(ElementType[pi], "C3D8");
            }
            else if (nn == 4) {
                strcpy(ElementType[pi], "C3D4");
            }
            pi = pi + 1;
             
            for (int j = 0; j < nn; j++) {
                connectivity[ci] = Nodes[j] - 1;
                ci = ci + 1;
            }
        }
        free(PIDs);
        free(Nodes);
        if (strncmp(Line, TITLE_NODE, strlen(TITLE_NODE)) == 0) {
            break;
        }
        i = i + 1;
    }

    // Checking if we can go to nodes section of mesh file
    if (fseek(File, NodesSectionPos, SEEK_SET) != 0) {
        fclose(File);
        FreeArrays();
        printf("\nERROR( proc %d ): 'fseek()' call failed.\n", world_rank);
        return false;
    }
    
    // Initializing "coordinates" array
    i = 0;
    Ended = false;
    double **Nodes = (double **)malloc(AllNodesCount * sizeof(double *));
    const int nCols = ndim + 1;
    while (!Ended && fgets(Line, sizeof(Line), File) != NULL) {
        if (LineToArray(false, false, 1, nCols, Line, Delim, (void**)&Nodes[i]) == nCols) {
            i = i + 1;
        }
        else {
            Ended = strncmp(Line, TITLE_END, strlen(TITLE_END)) == 0;
        }
    }
    qsort(Nodes, AllNodesCount, sizeof(double *), CmpFunc);
    nnodes = 0;
    const int csize = ndim * sizeof(double);
    coordinates = (double *)malloc(ConnectivitySize * csize);
    for (int i = 0; i < ConnectivitySize; i++) {
        const double Key = connectivity[i] + 1;
        const double *PKey = &Key;
        const double **bs = (const double **)bsearch(&PKey, Nodes, AllNodesCount, sizeof(double *), CmpFunc);
        if (bs != NULL) {
            const double *NodeData = *bs;
            memcpy(&coordinates[i * ndim], &NodeData[1], csize);
            nnodes = nnodes + 1;
        }
    }
    for (int i = 0; i < AllNodesCount; i++) {
        free(Nodes[i]);
    }
    free(Nodes);

    // Checking if "coordinates" array is OK
    if (nnodes != ConnectivitySize) {
        FreeArrays();
        printf("\nERROR( proc %d ): Failed to initialize 'coordinates' array.\n", world_rank);
        printf("\nnnodes = %d, ConnectivitySize = %d, ndim = %d\n", nnodes, ConnectivitySize, ndim);
    }

#if 0
    // Printing local arrays of processor (this section can be removed)
    printf("\neptr array in processor %d before partitioning = ", world_rank);
    for (int i = 0; i <= nelements ; i++) {
        printf("%d ", eptr[i]);
    }
    printf("\n");
    printf("\neind array in processor %d before partitioning =", world_rank);
    for (int i = 0; i < nelements; i++) {
        printf(" (%d)  ", i);
        for (int j = eptr[i]; j < eptr[i + 1]; j++) {
            printf("%d ", connectivity[j]);
        }
    }
    printf("\n");
    printf("\nType/PartID of element in processor %d before partitioning = ", world_rank);
    for (int i = 0; i < nelements ; i++) {
        printf("%s/%d  ", ElementType[i], pid[i]);
    }
    printf("\n");
    printf("\nSize of coordinates array in processor %d before partitioning = %d\n", world_rank, nnodes * ndim);
    printf("\nCoordinates array in processor %d before partitioning =", world_rank);
    for (int i = 0; i < nnodes; i++) {
        printf(" (%d)  ", i);
        for (int j = 0; j < ndim; j++) {
            printf("%.*f ", 1, coordinates[ndim * i + j]);
        }
    }
    printf("\n");
#endif
    // Everything is OK for now. Closing the mesh file, returning TRUE.
    fclose(File);
    return nnodes == ConnectivitySize;
}
