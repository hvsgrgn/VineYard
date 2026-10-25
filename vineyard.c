#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>

#define MAX_RECORD_LEN 50
#define INITIAL_CAPACITY 8
#define BENCHMARK 30

// necessary for enabling simple adaptation if in the future we modify max record length
// using preprocessor to format macros into strings so the scanf can use it
#define STRINGIFY(x) #x
#define FORMAT(x) " %" STRINGIFY(x) "[^\n]"

typedef struct
{
    char location[MAX_RECORD_LEN + 1]; // +1 so the \0 terminating char has space
    char site[MAX_RECORD_LEN + 1];
    char type[MAX_RECORD_LEN + 1];
    double spread;
    double damage;
} Vineyard;

typedef struct
{
    Vineyard *list;
    int count;
    int capacity;
} VineyardList;

// reallocates the dataset if its filled up to the capacity
// returns 0 on success, 1 on failure to reallocate
int grow_if_needed(VineyardList *list)
{
    if (list->capacity <= list->count)
    {
        list->capacity *= 2;
        Vineyard *temp = list->list;
        list->list = realloc(list->list, list->capacity * sizeof(Vineyard));
        if (!list->list)
        {
            list->list = temp;
            list->capacity /= 2;
            printf("\nThe dataset growth request has failed.\n");
            return 1;
        }
    }
    return 0;
}

// reads all the content of the given file into the given array
void load_from_file(const char *filename, VineyardList *dataset)
{
    int fd = open(filename, O_RDONLY);

    if (fd == -1)
    {
        printf("\nThe dataset file does not exist yet. The dataset has been initialized and is ready to be saved into file upon modification.\n");
        return;
    }

    ssize_t bytes;
    while (!grow_if_needed(dataset))
    {
        bytes = read(fd, &dataset->list[dataset->count], sizeof(Vineyard));
        if (bytes == sizeof(Vineyard))
            dataset->count++;
        else
            break;
    }

    close(fd);
}

// saves our dataset to the file, overwrites it if it exists, creates it if it doesnt
void save_to_file(const char *filename, VineyardList *dataset)
{
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (fd == -1)
    {
        printf("\nThe modification has been administered in the program but persisting into file has failed.\n");
        return;
    }

    for (int i = 0; i < dataset->count; i++)
    {
        write(fd, &dataset->list[i], sizeof(Vineyard));
    }

    close(fd);
}

// adds the new record to the database
// save_mode 1: persist to file, save_mode 0: in-memory only
void add_record(int save_mode, const char *filename, VineyardList *dataset, Vineyard new_record)
{
    if (grow_if_needed(dataset) == 1)
        return;

    dataset->list[dataset->count] = new_record;
    dataset->count++;

    if (save_mode)
        save_to_file(filename, dataset);
}

// attempts finding a record in the dataset, returns -1 if not found, otherwise the correct index
int find_record(VineyardList *dataset, char location[MAX_RECORD_LEN + 1], char site[MAX_RECORD_LEN + 1], char type[MAX_RECORD_LEN + 1])
{
    for (int i = 0; i < dataset->count; i++)
    {
        if (!strcmp(dataset->list[i].location, location) &&
            !strcmp(dataset->list[i].site, site) &&
            !strcmp(dataset->list[i].type, type))
            return i;
    }
    return -1;
}

void clear_input_buffer()
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ;
}

// prompts the user to input data for creation of a new record, then returns the new struct
Vineyard create_record()
{
    Vineyard new_record;

    do
    {
        printf("Location: ");
        scanf(FORMAT(MAX_RECORD_LEN), new_record.location);
        clear_input_buffer();
        if (strlen(new_record.location) == 0)
            printf("This field cannot be empty!\n");
    } while (strlen(new_record.location) == 0);

    do
    {
        printf("Site: ");
        scanf(FORMAT(MAX_RECORD_LEN), new_record.site);
        clear_input_buffer();
        if (strlen(new_record.site) == 0)
            printf("This field cannot be empty!\n");
    } while (strlen(new_record.site) == 0);

    do
    {
        printf("Type: ");
        scanf(FORMAT(MAX_RECORD_LEN), new_record.type);
        clear_input_buffer();
        if (strlen(new_record.type) == 0)
            printf("This field cannot be empty!\n");
    } while (strlen(new_record.type) == 0);

    printf("Spread: ");
    while (scanf("%lf", &new_record.spread) != 1 || new_record.spread <= 0)
    {
        clear_input_buffer();
        printf("Invalid input, please enter a positive number!\n");
        printf("Spread: ");
    }
    clear_input_buffer();

    do
    {
        printf("Damage (0-100): ");
        while (scanf("%lf", &new_record.damage) != 1)
        {
            clear_input_buffer();
            printf("Invalid input, please enter a number!\n");
            printf("Damage (0-100): ");
        }
        clear_input_buffer();
        if (new_record.damage < 0 || new_record.damage > 100)
            printf("Please enter a value between 0 and 100!\n");
    } while (new_record.damage < 0 || new_record.damage > 100);

    return new_record;
}

// prints a single record formatted in an easy-to-read way
void print_record(Vineyard v)
{
    printf("\nLocation: %s\n", v.location);
    printf("Site:     %s\n", v.site);
    printf("Type:     %s\n", v.type);
    printf("Spread:   %.2f\n", v.spread);
    printf("Damage:   %.2f%%\n", v.damage);
    printf("__________________________________________________\n");
}

// prints all records, or optionally filters either by location or type
void print_records(VineyardList *dataset, char *location_filter, char *type_filter)
{
    if (dataset->count == 0)
    {
        printf("\nThere are no entries in the database yet.\n");
        return;
    }

    printf("\nThe following entries have been found:\n__________________________________________________\n");

    if (!location_filter && !type_filter)
    {
        for (int i = 0; i < dataset->count; i++)
            print_record(dataset->list[i]);
    }
    else if (!location_filter)
    {
        for (int i = 0; i < dataset->count; i++)
            if (!strcmp(dataset->list[i].type, type_filter))
                print_record(dataset->list[i]);
    }
    else
    {
        for (int i = 0; i < dataset->count; i++)
            if (!strcmp(dataset->list[i].location, location_filter))
                print_record(dataset->list[i]);
    }
}

// initializes a VineyardList with allocated memory and returns it by value
VineyardList initialize()
{
    VineyardList data;
    data.capacity = INITIAL_CAPACITY;
    data.list = malloc(data.capacity * sizeof(Vineyard));
    data.count = 0;
    return data;
}

// empty handler so SIGUSR1 doesn't terminate the parent by default
void signal_handler(int sig) {}

// type 1 = destruction commissioner, type 0 = defense commissioner
void run_commissioner(int pipe_read_end, int type, pid_t parent_pid)
{
    Vineyard record;

    while (read(pipe_read_end, &record, sizeof(Vineyard)) == sizeof(Vineyard))
    {
        if (type == 1)
        {
            printf("\nDear Chief Engineer of the %s wine community, %s, %s plot!\n"
       "By order of the National Mountain Judge, as part of the Golden Yellowness\n"
       "disease control measures, you are hereby requested to immediately\n"
       "destroy the plot.\n",
       record.location, record.site, record.type);
        }
        else
        {
            printf("\nDear Chief Engineer of the %s wine community, %s, %s plot!\n"
       "Please be advised that the national spring chemical defense campaign\n"
       "is about to commence. You are kindly requested not to initiate any\n"
       "spraying or defensive measures of your own at this time.\n",
       record.location, record.site, record.type);
        }
    }

    close(pipe_read_end);
    kill(parent_pid, SIGUSR1);
}

int main()
{
    char *filename = "vineyards.dat";

    VineyardList data = initialize();

    if (!data.list)
    {
        printf("\nMemory allocation for dataset failed, exiting program...\n");
        return 1;
    }

    load_from_file(filename, &data);

    int choice = 0;
    printf("\nWelcome to Mount Opsys Vineyard database!\n");

    while (1)
    {
        printf("\n1. Add record\n");
        printf("2. Modify record\n");
        printf("3. Delete record\n");
        printf("4. List all\n");
        printf("5. List by location\n");
        printf("6. List by type\n");
        printf("7. Start spring campaign\n");
        printf("8. Exit\n");

        printf("\nSelect an option to proceed with: ");
        scanf("%d", &choice);
        clear_input_buffer();
        printf("\n");

        switch (choice)
        {
        case 1:
        {
            Vineyard new_record = create_record();

            if (find_record(&data, new_record.location, new_record.site, new_record.type) != -1)
            {
                printf("\nThis record already exists, try modifying it if you wish to change its data!\n");
                break;
            }

            add_record(1, filename, &data, new_record);
            printf("\nThe new entry has successfully been saved in the database!\n");
            break;
        }
        case 2:
        {
            char location[MAX_RECORD_LEN + 1];
            char site[MAX_RECORD_LEN + 1];
            char type[MAX_RECORD_LEN + 1];

            printf("Please enter the identifier of the record you wish to modify (max %d characters each).\n", MAX_RECORD_LEN);
            printf("Location: ");
            scanf(FORMAT(MAX_RECORD_LEN), location);
            clear_input_buffer();
            printf("Site: ");
            scanf(FORMAT(MAX_RECORD_LEN), site);
            clear_input_buffer();
            printf("Type: ");
            scanf(FORMAT(MAX_RECORD_LEN), type);
            clear_input_buffer();

            int ind = find_record(&data, location, site, type);

            if (ind == -1)
            {
                printf("\nThe record you're trying to modify does not exist in the dataset.\n");
                break;
            }

            printf("\nPlease enter the modified data!\n");
            data.list[ind] = create_record();
            save_to_file(filename, &data);
            printf("\nThe modified entry has successfully been saved in the database!\n");
            break;
        }
        case 3:
        {
            char location[MAX_RECORD_LEN + 1];
            char site[MAX_RECORD_LEN + 1];
            char type[MAX_RECORD_LEN + 1];

            printf("Please enter the identifier of the record you wish to delete (max %d characters each).\n", MAX_RECORD_LEN);
            printf("Location: ");
            scanf(FORMAT(MAX_RECORD_LEN), location);
            clear_input_buffer();
            printf("Site: ");
            scanf(FORMAT(MAX_RECORD_LEN), site);
            clear_input_buffer();
            printf("Type: ");
            scanf(FORMAT(MAX_RECORD_LEN), type);
            clear_input_buffer();

            int ind = find_record(&data, location, site, type);

            if (ind == -1)
            {
                printf("\nThe record you're trying to delete does not exist in the dataset.\n");
                break;
            }

            data.list[ind] = data.list[data.count - 1];
            data.count--;
            save_to_file(filename, &data);
            printf("\nThe entry has successfully been deleted from the database!\n");
            break;
        }
        case 4:
        {
            print_records(&data, NULL, NULL);
            break;
        }
        case 5:
        {
            char location[MAX_RECORD_LEN + 1];

            printf("Enter the location to filter by (max %d characters): ", MAX_RECORD_LEN);
            scanf(FORMAT(MAX_RECORD_LEN), location);
            clear_input_buffer();

            print_records(&data, location, NULL);
            break;
        }
        case 6:
        {
            char type[MAX_RECORD_LEN + 1];

            printf("Enter the type to filter by (max %d characters): ", MAX_RECORD_LEN);
            scanf(FORMAT(MAX_RECORD_LEN), type);
            clear_input_buffer();

            print_records(&data, NULL, type);
            break;
        }
        case 7:
        {
            printf("\nThe agricultural minister ordered a thorough spring campaign to prevent further spread of Golden Yellowness!\n");

            if (!data.count)
            {
                printf("There is no data in the system, nothing to do.\n");
                break;
            }

            VineyardList to_destroy = initialize();
            VineyardList to_defend = initialize();

            if (!to_destroy.list || !to_defend.list)
            {
                printf("\nMemory allocation for campaign lists failed.\n");
                free(to_destroy.list);
                free(to_defend.list);
                break;
            }

            for (int i = 0; i < data.count; i++)
            {
                if (data.list[i].damage < BENCHMARK)
                    add_record(0, NULL, &to_defend, data.list[i]);
                else
                    add_record(0, NULL, &to_destroy, data.list[i]);
            }

            int pipe_destroy[2];
            int pipe_defend[2];

            if (pipe(pipe_destroy) < 0 || pipe(pipe_defend) < 0)
            {
                printf("\nCreating pipelines for the commissioners failed.\n");
                free(to_destroy.list);
                free(to_defend.list);
                break;
            }

            // register handler so SIGUSR1 doesn't kill the parent
            signal(SIGUSR1, signal_handler);

            pid_t parent_pid = getpid();

            pid_t pid_destroy = fork();

            if (pid_destroy < 0)
            {
                printf("Forking destruction commissioner failed.\n");
            }
            else if (pid_destroy == 0)
            {
                // child only needs to read from pipe_destroy, close everything else
                close(pipe_destroy[1]);
                close(pipe_defend[0]);
                close(pipe_defend[1]);
                run_commissioner(pipe_destroy[0], 1, parent_pid);
                exit(0);
            }
            else
            {
                close(pipe_destroy[0]);
            }

            pid_t pid_defend = fork();

            if (pid_defend < 0)
            {
                printf("Forking defense commissioner failed.\n");
            }
            else if (pid_defend == 0)
            {
                // child only needs to read from pipe_defend, close everything else
                close(pipe_defend[1]);
                close(pipe_destroy[0]);
                close(pipe_destroy[1]);
                run_commissioner(pipe_defend[0], 0, parent_pid);
                exit(0);
            }
            else
            {
                close(pipe_defend[0]);
            }

            // send destroy records, wait for that child to fully finish before moving on
            for (int i = 0; i < to_destroy.count; i++)
                write(pipe_destroy[1], &to_destroy.list[i], sizeof(Vineyard));
            close(pipe_destroy[1]);
            waitpid(pid_destroy, NULL, 0);

            // only then send defend records and wait for that child
            for (int i = 0; i < to_defend.count; i++)
                write(pipe_defend[1], &to_defend.list[i], sizeof(Vineyard));
            close(pipe_defend[1]);
            waitpid(pid_defend, NULL, 0);

            printf("\n--- Report to the Agricultural Minister ---\n");
            printf("Spring campaign completed.\n");
            printf("Plantations destroyed: %d\n", to_destroy.count);
            printf("Plantations treated chemically: %d\n", to_defend.count);
            printf("-------------------------------------------\n");

            free(to_destroy.list);
            free(to_defend.list);
            break;
        }
        case 8:
        {
            free(data.list);
            printf("\nGoodbye!\n");
            return 0;
        }
        default:
            printf("Invalid option!\n");
        }
    }

    free(data.list);
    return 0;
}
