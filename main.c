#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TASKS 10000
#define NUM_TECH 4
#define MAX_LINE 1024
#define PAGE_SIZE 20
#define MAX_TIME_INPUT 100000000L

/* limits.h yerine kendi sabitlerimiz */
#define INT_MAX_VAL 2147483647
#define INT_MIN_VAL (-2147483647 - 1)

/*
 * PDF ornek tablosu (SJF=9, Priority=8, RR=8) konum maliyeti KAPALI iken
 * (quantum=2) elde edilir. Proje kurali geregi varsayilan maliyet 1'dir.
 * Derleme: gcc -DLOCATION_SWITCH_COST=0 veya self-test icinde dinamik kontrol.
 */
#ifndef LOCATION_SWITCH_COST
#define LOCATION_SWITCH_COST 1
#endif

/* --- DURUM SABITLERI --- */
typedef enum {
    NOT_ARRIVED = 0,   /* Henuz gelmedi */
    WAITING,           /* Bekliyor */
    RUNNING,           /* Devam ediyor */
    COMPLETED          /* Tamamlandi */
} TaskStatus;

typedef enum {
    IDLE = 0,          /* Bos */
    IN_TRANSIT,        /* Geciste */
    BUSY               /* Mesgul */
} TechStatus;

/* --- VERI YAPILARI --- */
typedef struct {
    int id;
    int arrival_time;
    int duration;
    int remaining_time;
    int priority;
    char type[40];
    char location[20];
    TaskStatus status;
    int first_start_time;
    int finish_time;
    int assigned_tech_id;
    int last_tech_id;       /* Gorevi en son calistiran/bitiren teknisyen */
} Task;

typedef struct {
    int id;                 /* 1, 2, 3, 4 */
    char current_location[20];
    char target_location[20];
    TechStatus status;
    int transition_left;    /* kalan gecis suresi (birim) */
    int total_worked;       /* toplam calisilan birim */
    int current_quantum;    /* Round Robin dilim sayaci */
    Task *current_task;
} Technician;

typedef struct QueueNode {
    Task *task;
    struct QueueNode *next;
} QueueNode;

/* --- GLOBAL SIMULASYON DURUMU --- */
Task master_tasks[MAX_TASKS];
int total_task_count = 0;

Technician technicians[NUM_TECH];
QueueNode *waiting_queue = NULL;

int current_time = 0;
int selected_algorithm = 0; /* 1: SJF, 2: Priority, 3: RR, 4: Custom */
int rr_quantum = 0;
int g_location_switch_cost = LOCATION_SWITCH_COST;

/* Yardimci global durum */
int arrival_order[MAX_TASKS];   /* master_tasks indeksleri: (gelis, ID) sirali */
int next_arrival_idx = 0;       /* siradaki gelmemis gorevin arrival_order indeksi */
int completed_count = 0;

/* =========================================================
 * Girdi yardimcilari (ctype.h / errno.h / limits.h kullanilmaz)
 * ========================================================= */

int is_space(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

int read_line(char *buf, size_t n) {
    size_t len, start = 0;
    if (!fgets(buf, (int)n, stdin)) return 0;
    len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[--len] = '\0';
    } else {
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) {}
    }
    while (len > 0 && is_space(buf[len - 1])) buf[--len] = '\0';
    while (buf[start] && is_space(buf[start])) start++;
    if (start > 0) memmove(buf, buf + start, strlen(buf + start) + 1);
    return 1;
}

/* Gecerli bir int ise 1 dondurur ve *out'a yazar, degilse 0 */
int parse_int_strict(const char *s, int *out) {
    long long value = 0;
    int negative = 0, i = 0;

    if (s == NULL || s[0] == '\0') return 0;
    if (s[0] == '-' || s[0] == '+') {
        negative = (s[0] == '-');
        i++;
    }
    if (s[i] == '\0') return 0;

    for (; s[i] != '\0'; i++) {
        if (s[i] < '0' || s[i] > '9') return 0;
        value = value * 10 + (s[i] - '0');
        if (value > 2147483648LL) return 0;
    }
    if (negative) value = -value;
    if (value < INT_MIN_VAL || value > INT_MAX_VAL) return 0;

    *out = (int)value;
    return 1;
}

long ask_int(const char *prompt, long min_val, long max_val) {
    char buf[128];
    for (;;) {
        int value;
        printf("%s", prompt);
        fflush(stdout);
        if (!read_line(buf, sizeof(buf))) {
            printf("\nGiris sonlandi, cikiliyor.\n");
            exit(0);
        }
        if (parse_int_strict(buf, &value) && value >= min_val && value <= max_val)
            return value;
        printf("Gecersiz giris. %ld ile %ld arasinda bir tam sayi giriniz.\n", min_val, max_val);
    }
}

/* =========================================================
 * 1. Dosya okuma ve kuyruk islemleri
 * ========================================================= */

const char *VALID_LOCATIONS[] = {"LAB_A", "LAB_B", "DERSLIK_1", "DERSLIK_2", "DERSLIK_3"};

int is_valid_location(const char *s) {
    size_t i;
    for (i = 0; i < sizeof(VALID_LOCATIONS) / sizeof(VALID_LOCATIONS[0]); i++)
        if (strcmp(s, VALID_LOCATIONS[i]) == 0) return 1;
    return 0;
}

int find_task_index(int id) {
    int i;
    for (i = 0; i < total_task_count; i++)
        if (master_tasks[i].id == id) return i;
    return -1;
}

int cmp_task_id(const void *a, const void *b) {
    const Task *left = (const Task *)a, *right = (const Task *)b;
    return (left->id > right->id) - (left->id < right->id);
}

int cmp_arrival_idx(const void *a, const void *b) {
    const Task *left = &master_tasks[*(const int *)a];
    const Task *right = &master_tasks[*(const int *)b];
    if (left->arrival_time != right->arrival_time)
        return (left->arrival_time > right->arrival_time) - (left->arrival_time < right->arrival_time);
    return (left->id > right->id) - (left->id < right->id);
}

void reject_line(int line_no, const char *reason) {
    printf("  [REDDEDILDI] Satir %d: %s\n", line_no, reason);
}

int load_tasks(const char *filename) {
    FILE *fp;
    char line[MAX_LINE];
    int line_no = 0, rejected = 0, i;

    total_task_count = 0;
    fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("HATA: '%s' dosyasi acilamadi.\n", filename);
        return -1;
    }

    while (fgets(line, sizeof(line), fp)) {
        char *p, *token, *tokens[8];
        int token_count = 0, line_too_long = 0;
        int id, arrival, duration, priority;
        size_t len = strlen(line);

        line_no++;
        /* UTF-8 BOM Temizligi */
        if (line_no == 1 && len >= 3 &&
            (unsigned char)line[0] == 0xEF &&
            (unsigned char)line[1] == 0xBB &&
            (unsigned char)line[2] == 0xBF) {
            memmove(line, line + 3, len - 2);
            len -= 3;
        }
        if (len > 0 && line[len - 1] != '\n' && !feof(fp)) {
            int ch;
            line_too_long = 1;
            while ((ch = fgetc(fp)) != '\n' && ch != EOF) {}
        }

        p = line;
        while (*p && is_space(*p)) p++;
        if (*p == '\0' || *p == '#') continue;

        if (line_too_long) { reject_line(line_no, "satir cok uzun"); rejected++; continue; }

        token = strtok(p, " \t\r\n");
        while (token && token_count < 7) {
            tokens[token_count++] = token;
            token = strtok(NULL, " \t\r\n");
        }
        if (token_count < 6) { reject_line(line_no, "eksik alan (6 alan gerekli)"); rejected++; continue; }
        if (token_count > 6) { reject_line(line_no, "fazla alan (6 alan gerekli)"); rejected++; continue; }

        if (!parse_int_strict(tokens[0], &id) || id <= 0)
            { reject_line(line_no, "ID pozitif tam sayi olmali"); rejected++; continue; }
        if (!parse_int_strict(tokens[1], &arrival) || arrival < 0)
            { reject_line(line_no, "GELIS_ZAMANI sifir veya daha buyuk tam sayi olmali"); rejected++; continue; }
        if (!parse_int_strict(tokens[2], &duration) || duration <= 0)
            { reject_line(line_no, "SURE sifirdan buyuk tam sayi olmali"); rejected++; continue; }
        if (!parse_int_strict(tokens[3], &priority) || priority < 1 || priority > 5)
            { reject_line(line_no, "ONCELIK 1-5 arasinda olmali"); rejected++; continue; }
        if (strlen(tokens[4]) > 39)
            { reject_line(line_no, "GOREV_TURU en fazla 39 karakter olabilir"); rejected++; continue; }
        if (!is_valid_location(tokens[5]))
            { reject_line(line_no, "gecersiz KONUM"); rejected++; continue; }
        if (find_task_index(id) >= 0)
            { reject_line(line_no, "yinelenen ID (ilk gecerli kayit korundu)"); rejected++; continue; }
        if (total_task_count >= MAX_TASKS)
            { reject_line(line_no, "kapasite asildi (MAX_TASKS)"); rejected++; continue; }

        {
            Task *task = &master_tasks[total_task_count++];
            memset(task, 0, sizeof(Task));
            task->id = id;
            task->arrival_time = arrival;
            task->duration = duration;
            task->remaining_time = duration;
            task->priority = priority;
            strcpy(task->type, tokens[4]);
            strcpy(task->location, tokens[5]);
            task->status = NOT_ARRIVED;
            task->first_start_time = -1;
            task->finish_time = -1;
        }
    }
    fclose(fp);

    qsort(master_tasks, (size_t)total_task_count, sizeof(Task), cmp_task_id);
    for (i = 0; i < total_task_count; i++) arrival_order[i] = i;
    qsort(arrival_order, (size_t)total_task_count, sizeof(int), cmp_arrival_idx);

    printf("Dosya okundu: %d gecerli gorev, %d reddedilen satir.\n", total_task_count, rejected);
    return total_task_count;
}

void enqueue(QueueNode **head, Task *task) {
    QueueNode *new_node = (QueueNode *)malloc(sizeof(QueueNode));
    if (new_node == NULL) {
        fprintf(stderr, "HATA: bellek yetersiz (enqueue).\n");
        exit(1);
    }
    new_node->task = task;
    new_node->next = NULL;

    if (*head == NULL) {
        *head = new_node;
        return;
    }

    {
        QueueNode *tail = *head;
        while (tail->next != NULL) tail = tail->next;
        tail->next = new_node;
    }
}

int sjf_comes_before(const Task *a, const Task *b) {
    if (a->duration != b->duration) return a->duration < b->duration;
    if (a->arrival_time != b->arrival_time) return a->arrival_time < b->arrival_time;
    return a->id < b->id;
}

int priority_comes_before(const Task *a, const Task *b) {
    if (a->priority != b->priority) return a->priority < b->priority;
    if (a->arrival_time != b->arrival_time) return a->arrival_time < b->arrival_time;
    return a->id < b->id;
}

void insert_sorted(QueueNode **head, Task *task,
                   int (*comes_before)(const Task *, const Task *)) {
    QueueNode *new_node = (QueueNode *)malloc(sizeof(QueueNode));
    if (new_node == NULL) {
        fprintf(stderr, "HATA: bellek yetersiz (insert_sorted).\n");
        exit(1);
    }
    new_node->task = task;
    new_node->next = NULL;

    if (*head == NULL || comes_before(task, (*head)->task)) {
        new_node->next = *head;
        *head = new_node;
        return;
    }

    {
        QueueNode *prev = *head;
        while (prev->next != NULL && !comes_before(task, prev->next->task))
            prev = prev->next;
        new_node->next = prev->next;
        prev->next = new_node;
    }
}

void insert_sorted_sjf(QueueNode **head, Task *task) {
    insert_sorted(head, task, sjf_comes_before);
}

void insert_sorted_priority(QueueNode **head, Task *task) {
    insert_sorted(head, task, priority_comes_before);
}

Task *dequeue(QueueNode **head) {
    QueueNode *front;
    Task *task;

    if (head == NULL || *head == NULL) return NULL;

    front = *head;
    task = front->task;
    *head = (*head)->next;
    free(front);
    return task;
}

void clear_queue(QueueNode **head) {
    if (head == NULL) return;
    while (*head != NULL) dequeue(head);
}

/* =========================================================
 * 2. Simulasyon ve atama motoru
 * ========================================================= */

int needs_transition(const Technician *tech, const Task *task) {
    if (g_location_switch_cost <= 0) return 0;
    if (tech->current_location[0] == '\0') return 0; /* henuz atanmis bir konumu yok */
    return strcmp(tech->current_location, task->location) != 0;
}

void enqueue_by_algorithm(Task *task) {
    switch (selected_algorithm) {
        case 1:  insert_sorted_sjf(&waiting_queue, task); break;
        case 2:  insert_sorted_priority(&waiting_queue, task); break;
        default: enqueue(&waiting_queue, task); break; /* RR ve Ozel: FIFO */
    }
}

void admit_arrivals(void) {
    while (next_arrival_idx < total_task_count &&
           master_tasks[arrival_order[next_arrival_idx]].arrival_time <= current_time) {
        Task *task = &master_tasks[arrival_order[next_arrival_idx]];
        task->status = WAITING;
        enqueue_by_algorithm(task);
        next_arrival_idx++;
    }
}

/*
 * Ozel algoritma: Konum Duyarli SJF (Location-Aware Shortest Job)
 * Efektif Sure = Kalan Sure + (Gecis gerekiyorsa Konum Maliyeti)
 * En kucuk efektif sureye oncelik verilir; esitlikte erken gelis, sonra kucuk ID.
 */
Task *dequeue_best_for_tech(QueueNode **head, const Technician *tech) {
    QueueNode *current, *previous = NULL, *best = NULL, *best_previous = NULL;
    int best_penalty = INT_MAX_VAL;

    if (head == NULL || *head == NULL) return NULL;

    for (current = *head; current != NULL; previous = current, current = current->next) {
        int penalty = current->task->remaining_time;
        if (needs_transition(tech, current->task)) {
            penalty += g_location_switch_cost;
        }

        if (best == NULL || penalty < best_penalty ||
            (penalty == best_penalty &&
             (current->task->arrival_time < best->task->arrival_time ||
              (current->task->arrival_time == best->task->arrival_time &&
               current->task->id < best->task->id)))) {
            best = current;
            best_previous = previous;
            best_penalty = penalty;
        }
    }

    if (best_previous == NULL) *head = best->next;
    else best_previous->next = best->next;

    {
        Task *task = best->task;
        free(best);
        return task;
    }
}

void start_task_on(Technician *tech, Task *task) {
    int transition = needs_transition(tech, task);

    tech->current_task = task;
    tech->current_quantum = 0;
    task->status = RUNNING;
    task->assigned_tech_id = tech->id;
    task->last_tech_id = tech->id;

    if (transition) {
        tech->status = IN_TRANSIT;
        tech->transition_left = g_location_switch_cost;
        strcpy(tech->target_location, task->location);
    } else {
        tech->status = BUSY;
        tech->transition_left = 0;
        strcpy(tech->current_location, task->location);
        if (task->first_start_time < 0) {
            task->first_start_time = current_time;
        }
    }
}

void assign_idle_technicians(void) {
    int i;
    if (selected_algorithm == 0) return;

    for (i = 0; i < NUM_TECH; i++) {
        Technician *tech = &technicians[i];
        Task *task;
        if (tech->status != IDLE) continue;

        task = (selected_algorithm == 4)
                   ? dequeue_best_for_tech(&waiting_queue, tech)
                   : dequeue(&waiting_queue);
        if (task == NULL) break;
        start_task_on(tech, task);
    }
}

void step_simulation_1_unit(void) {
    Task *quantum_expired[NUM_TECH];
    int quantum_expired_count = 0, i, k;

    if (total_task_count > 0 && completed_count == total_task_count) {
        current_time++;
        return;
    }
    if (total_task_count == 0) {
        current_time++;
        return;
    }

    /* 1. Calisanlarin surelerini veya gecis sayacini eksilt */
    for (i = 0; i < NUM_TECH; i++) {
        Technician *tech = &technicians[i];
        if (tech->status == IN_TRANSIT) {
            tech->transition_left--;
            if (tech->transition_left <= 0) {
                tech->transition_left = 0;
                tech->status = BUSY;
                strcpy(tech->current_location, tech->target_location);
            }
        } else if (tech->status == BUSY && tech->current_task != NULL) {
            if (tech->current_task->first_start_time < 0) {
                tech->current_task->first_start_time = current_time;
            }
            tech->current_task->remaining_time--;
            tech->total_worked++;
            tech->current_quantum++;
        }
    }

    /* 2. Zaman ilerler */
    current_time++;

    /* 3. Biten gorevler / 4. RR dilimi dolanlar */
    for (i = 0; i < NUM_TECH; i++) {
        Technician *tech = &technicians[i];
        Task *task = tech->current_task;
        if (tech->status != BUSY || task == NULL) continue;

        if (task->remaining_time <= 0) {
            task->remaining_time = 0;
            task->status = COMPLETED;
            task->finish_time = current_time;
            task->assigned_tech_id = 0;
            task->last_tech_id = tech->id;
            completed_count++;
            tech->status = IDLE;
            tech->current_task = NULL;
            tech->current_quantum = 0;
        } else if (selected_algorithm == 3 && tech->current_quantum >= rr_quantum) {
            task->status = WAITING;
            task->assigned_tech_id = 0;
            quantum_expired[quantum_expired_count++] = task;
            tech->status = IDLE;
            tech->current_task = NULL;
            tech->current_quantum = 0;
        }
    }

    /* 5. Once yeni gelenler (ID sirali), ardindan dilimi dolanlar kuyruga eklenir */
    admit_arrivals();
    for (k = 0; k < quantum_expired_count; k++)
        enqueue(&waiting_queue, quantum_expired[k]);

    /* 6. Bos teknisyenlere atama yapilir (T1..T4) */
    assign_idle_technicians();
}

void advance_time_n_units(int n) {
    int i;
    if (n > INT_MAX_VAL - 1 - current_time) n = INT_MAX_VAL - 1 - current_time;
    for (i = 0; i < n; i++) step_simulation_1_unit();
}

void reset_simulation(void) {
    int i;
    clear_queue(&waiting_queue);
    current_time = 0;
    completed_count = 0;
    next_arrival_idx = 0;

    for (i = 0; i < total_task_count; i++) {
        Task *task = &master_tasks[i];
        task->remaining_time = task->duration;
        task->status = NOT_ARRIVED;
        task->first_start_time = -1;
        task->finish_time = -1;
        task->assigned_tech_id = 0;
        task->last_tech_id = 0;
    }
    for (i = 0; i < NUM_TECH; i++) {
        Technician *tech = &technicians[i];
        tech->id = i + 1;
        tech->current_location[0] = '\0';
        tech->target_location[0] = '\0';
        tech->status = IDLE;
        tech->transition_left = 0;
        tech->total_worked = 0;
        tech->current_quantum = 0;
        tech->current_task = NULL;
    }
    admit_arrivals();
    assign_idle_technicians();
}

void jump_to_time(int target_time) {
    if (target_time > INT_MAX_VAL - 1) target_time = INT_MAX_VAL - 1;
    if (target_time < current_time) reset_simulation();
    while (current_time < target_time) step_simulation_1_unit();
}

/* ---------------------------------------------------------
 * Ekran ve istatistik fonksiyonlari
 * --------------------------------------------------------- */

const char *status_name(TaskStatus s) {
    switch (s) {
        case NOT_ARRIVED: return "Henuz gelmedi";
        case WAITING:     return "Bekliyor";
        case RUNNING:     return "Devam ediyor";
        default:          return "Tamamlandi";
    }
}

const char *tech_status_name(TechStatus s) {
    switch (s) {
        case IDLE:       return "Bos";
        case IN_TRANSIT: return "Geciste";
        default:         return "Mesgul";
    }
}

const char *algorithm_name(int a) {
    switch (a) {
        case 1: return "Shortest Job";
        case 2: return "Priority";
        case 3: return "Round Robin";
        case 4: return "Konum Duyarli SJF (Ozel)";
        default: return "Secilmedi";
    }
}

void print_task_header(void) {
    printf("%-6s %-7s %-6s %-8s %-14s %-10s %-6s\n",
           "ID", "Gelis", "Sure", "Oncelik", "Durum", "Teknisyen", "Kalan");
}

void print_task_row(const Task *task) {
    char tech_label[16];
    if (task->assigned_tech_id > 0) sprintf(tech_label, "T%d", task->assigned_tech_id);
    else strcpy(tech_label, "-");
    printf("%-6d %-7d %-6d %-8d %-14s %-10s %-6d\n",
           task->id, task->arrival_time, task->duration, task->priority,
           status_name(task->status), tech_label, task->remaining_time);
}

void list_tasks(int filter) {
    int i, shown = 0;
    char buf[32];
    print_task_header();
    for (i = 0; i < total_task_count; i++) {
        if (filter >= 0 && (int)master_tasks[i].status != filter) continue;
        if (shown > 0 && shown % PAGE_SIZE == 0) {
            printf("-- Devam icin Enter, durdurmak icin q: ");
            fflush(stdout);
            if (!read_line(buf, sizeof(buf)) || buf[0] == 'q' || buf[0] == 'Q') break;
            print_task_header();
        }
        print_task_row(&master_tasks[i]);
        shown++;
    }
    printf("Listelenen gorev sayisi: %d\n", shown);
}

void query_task(int id) {
    int idx = find_task_index(id);
    const Task *task;
    if (idx < 0) { printf("ID %d ile gorev bulunamadi.\n", id); return; }
    task = &master_tasks[idx];
    printf("--- Gorev %d ---\n", task->id);
    printf("Gorev turu        : %s\n", task->type);
    printf("Konum             : %s\n", task->location);
    printf("Gelis zamani      : %d\n", task->arrival_time);
    printf("Toplam sure       : %d\n", task->duration);
    printf("Oncelik           : %d\n", task->priority);
    printf("Durum             : %s\n", status_name(task->status));
    if (task->assigned_tech_id > 0) printf("Guncel teknisyen  : T%d\n", task->assigned_tech_id);
    else                            printf("Guncel teknisyen  : -\n");
    printf("Kalan sure        : %d\n", task->remaining_time);
    if (task->first_start_time >= 0) printf("Ilk baslama       : %d\n", task->first_start_time);
    else                             printf("Ilk baslama       : -\n");
    if (task->status == COMPLETED) {
        printf("Tamamlanma        : %d\n", task->finish_time);
        printf("Son bitiren       : T%d\n", task->last_tech_id);
        printf("Toplam bekleme    : %d\n",
               task->finish_time - task->arrival_time - task->duration);
    } else {
        printf("Tamamlanma        : -\n");
    }
}

void show_technicians(void) {
    int i;
    printf("%-5s %-9s %-10s %-7s %-6s %-10s %s\n",
           "Tek.", "Durum", "Konum", "Gorev", "Kalan", "Calisilan", "Dilimde kalan (RR)");
    for (i = 0; i < NUM_TECH; i++) {
        const Technician *tech = &technicians[i];
        char task_s[16], rem_s[16], quantum_left_s[16];
        if (tech->current_task) {
            sprintf(task_s, "%d", tech->current_task->id);
            sprintf(rem_s, "%d", tech->current_task->remaining_time);
        } else {
            strcpy(task_s, "-");
            strcpy(rem_s, "-");
        }
        if (selected_algorithm == 3 && tech->current_task)
            sprintf(quantum_left_s, "%d", rr_quantum - tech->current_quantum);
        else
            strcpy(quantum_left_s, "-");
        printf("T%-4d %-9s %-10s %-7s %-6s %-10d %s\n",
               tech->id, tech_status_name(tech->status),
               tech->current_location[0] ? tech->current_location : "-",
               task_s, rem_s, tech->total_worked, quantum_left_s);
    }
}

typedef struct {
    int makespan;
    int all_completed;
    double avg_wait;
    double avg_turnaround;
    double overall_utilization;
    int status_counts[4];
} SimStats;

void compute_statistics(SimStats *stats) {
    int i;
    long wait_sum = 0, turnaround_sum = 0, worked_sum = 0;

    memset(stats, 0, sizeof(*stats));
    stats->makespan = 0;
    stats->all_completed = (total_task_count > 0 && completed_count == total_task_count);

    for (i = 0; i < total_task_count; i++) {
        const Task *task = &master_tasks[i];
        stats->status_counts[task->status]++;
        if (task->status == COMPLETED) {
            wait_sum += task->finish_time - task->arrival_time - task->duration;
            turnaround_sum += task->finish_time - task->arrival_time;
            if (task->finish_time > stats->makespan) stats->makespan = task->finish_time;
        }
    }

    if (completed_count > 0) {
        stats->avg_wait = (double)wait_sum / completed_count;
        stats->avg_turnaround = (double)turnaround_sum / completed_count;
    }
    if (current_time > 0) {
        for (i = 0; i < NUM_TECH; i++) worked_sum += technicians[i].total_worked;
        stats->overall_utilization = 100.0 * worked_sum / (4.0 * current_time);
    }
}

void show_statistics(void) {
    SimStats stats;
    int i;

    compute_statistics(&stats);

    printf("=== Istatistikler (zaman %d, algoritma: %s) ===\n",
           current_time, algorithm_name(selected_algorithm));
    printf("Toplam gorev: %d | Henuz gelmedi: %d | Bekliyor: %d | Devam ediyor: %d | Tamamlandi: %d\n",
           total_task_count,
           stats.status_counts[0], stats.status_counts[1],
           stats.status_counts[2], stats.status_counts[3]);

    if (total_task_count == 0)
        printf("Makespan (son bitis zamani): hesaplanamaz (gorev yok)\n");
    else if (stats.all_completed)
        printf("Makespan (son bitis zamani): %d\n", stats.makespan);
    else
        printf("Makespan (son bitis zamani): henuz tum gorevler tamamlanmadi\n");

    if (completed_count > 0) {
        printf("Ortalama bekleme      : %.3f\n", stats.avg_wait);
        printf("Ortalama sistemde kalma: %.3f\n", stats.avg_turnaround);
    } else {
        printf("Ortalama bekleme      : hesaplanamaz\n");
        printf("Ortalama sistemde kalma: hesaplanamaz\n");
    }

    if (current_time > 0) {
        for (i = 0; i < NUM_TECH; i++) {
            printf("T%d kullanim orani     : %%%.3f (calisilan: %d)\n",
                   technicians[i].id,
                   100.0 * technicians[i].total_worked / current_time,
                   technicians[i].total_worked);
        }
        printf("Genel kullanim orani  : %%%.3f\n", stats.overall_utilization);
    } else {
        printf("Kullanim oranlari     : hesaplanamaz (zaman 0)\n");
    }
}

void run_until_all_completed(void) {
    int guard = 0;
    while (completed_count < total_task_count && guard < 10000000) {
        step_simulation_1_unit();
        guard++;
    }
}

void print_menu_header(void) {
    printf("\n==============================================\n");
    printf(" Guncel zaman: %d | Algoritma: %s", current_time, algorithm_name(selected_algorithm));
    if (selected_algorithm == 3) printf(" | Quantum: %d", rr_quantum);
    printf(" | Konum maliyeti: %d", g_location_switch_cost);
    printf("\n==============================================\n");
    printf(" 1 - Algoritma sec veya degistir\n");
    printf(" 2 - Tum gorevleri listele\n");
    printf(" 3 - Gorevleri durumuna gore listele\n");
    printf(" 4 - Gorev ID ile sorgula\n");
    printf(" 5 - Teknisyenlerin durumunu goster\n");
    printf(" 6 - Zamani 1 birim ilerlet\n");
    printf(" 7 - Zamani N birim ilerlet\n");
    printf(" 8 - Belirli bir zamana git\n");
    printf(" 9 - Istatistikleri goster\n");
    printf("10 - Simulasyonu sifirla\n");
    printf(" 0 - Cikis\n");
}

int choose_algorithm(int allow_cancel) {
    long choice;
    printf("\n--- Algoritma Secimi ---\n");
    printf("1 - Shortest Job\n2 - Priority\n3 - Round Robin\n4 - Kendi Algoritmaniz (Konum Duyarli SJF)\n");
    if (allow_cancel) printf("0 - Geri\n");
    choice = ask_int("Seciminiz: ", allow_cancel ? 0 : 1, 4);
    if (choice == 0) return 0;
    if (choice == 3) rr_quantum = (int)ask_int("Zaman dilimi (quantum) degeri: ", 1, 1000000);
    selected_algorithm = (int)choice;
    reset_simulation();
    printf("%s secildi; simulasyon zaman 0'dan baslatildi.\n", algorithm_name(selected_algorithm));
    return 1;
}

void print_progress(void) {
    printf("Guncel zaman: %d | Tamamlanan: %d/%d\n",
           current_time, completed_count, total_task_count);
}

/* =========================================================
 * Test Fonksiyonlari (Static kaldirildi)
 * ========================================================= */

int nearly_equal(double a, double b, double eps) {
    double d = a - b;
    if (d < 0) d = -d;
    return d <= eps;
}

int run_algo_to_end(int algo, int quantum, int loc_cost, SimStats *out) {
    int saved_cost = g_location_switch_cost;
    g_location_switch_cost = loc_cost;
    selected_algorithm = algo;
    rr_quantum = quantum;
    reset_simulation();
    run_until_all_completed();
    if (completed_count != total_task_count) {
        g_location_switch_cost = saved_cost;
        return 0;
    }
    compute_statistics(out);
    g_location_switch_cost = saved_cost;
    return 1;
}

int run_self_test(const char *filename) {
    SimStats sjf, pri, rr, custom;
    int fails = 0;
    int makespan_step, makespan_jump;
    int i;

    printf("=== SELF-TEST basliyor: %s ===\n", filename);
    if (load_tasks(filename) < 0) return 1;
    for (i = 0; i < NUM_TECH; i++) technicians[i].id = i + 1;

    {
        int duration_sum = 0;
        int is_pdf_sample;
        for (i = 0; i < total_task_count; i++) duration_sum += master_tasks[i].duration;
        is_pdf_sample = (total_task_count == 8 && duration_sum == 29);

        printf("\n[1] Algoritma kosumu (LOCATION_SWITCH_COST=0, RR quantum=2)\n");
        if (!run_algo_to_end(1, 0, 0, &sjf)) { printf("  FAIL: SJF tamamlanamadi\n"); fails++; }
        else {
            printf("  SJF: makespan=%d avg_wait=%.3f util=%.3f%%\n",
                   sjf.makespan, sjf.avg_wait, sjf.overall_utilization);
            if (is_pdf_sample) {
                if (sjf.makespan != 9 || !nearly_equal(sjf.avg_wait, 0.375, 0.001) ||
                    !nearly_equal(sjf.overall_utilization, 80.556, 0.01)) {
                    printf("  FAIL: SJF beklenen 9 / 0.375 / 80.556\n");
                    fails++;
                } else printf("  OK: SJF (PDF tablo)\n");
            } else printf("  OK: SJF tamamlandi\n");
        }

        if (!run_algo_to_end(2, 0, 0, &pri)) { printf("  FAIL: Priority tamamlanamadi\n"); fails++; }
        else {
            printf("  Priority: makespan=%d avg_wait=%.3f util=%.3f%%\n",
                   pri.makespan, pri.avg_wait, pri.overall_utilization);
            if (is_pdf_sample) {
                if (pri.makespan != 8 || !nearly_equal(pri.avg_wait, 0.750, 0.001) ||
                    !nearly_equal(pri.overall_utilization, 90.625, 0.01)) {
                    printf("  FAIL: Priority beklenen 8 / 0.750 / 90.625\n");
                    fails++;
                } else printf("  OK: Priority (PDF tablo)\n");
            } else printf("  OK: Priority tamamlandi\n");
        }

        if (!run_algo_to_end(3, 2, 0, &rr)) { printf("  FAIL: RR tamamlanamadi\n"); fails++; }
        else {
            printf("  RR(q=2): makespan=%d avg_wait=%.3f util=%.3f%%\n",
                   rr.makespan, rr.avg_wait, rr.overall_utilization);
            if (is_pdf_sample) {
                if (rr.makespan != 8 || !nearly_equal(rr.avg_wait, 0.875, 0.001) ||
                    !nearly_equal(rr.overall_utilization, 90.625, 0.01)) {
                    printf("  FAIL: RR beklenen 8 / 0.875 / 90.625\n");
                    fails++;
                } else printf("  OK: Round Robin (PDF tablo)\n");
            } else printf("  OK: RR tamamlandi\n");
        }
    }

    printf("\n[2] Adim adim vs jump_to_time tutarliligi (Priority, maliyet=1)\n");
    g_location_switch_cost = 1;
    selected_algorithm = 2;
    reset_simulation();
    while (completed_count < total_task_count) step_simulation_1_unit();
    makespan_step = 0;
    for (i = 0; i < total_task_count; i++)
        if (master_tasks[i].finish_time > makespan_step)
            makespan_step = master_tasks[i].finish_time;

    reset_simulation();
    jump_to_time(makespan_step);
    makespan_jump = 0;
    for (i = 0; i < total_task_count; i++)
        if (master_tasks[i].finish_time > makespan_jump)
            makespan_jump = master_tasks[i].finish_time;

    if (makespan_step != makespan_jump || completed_count != total_task_count) {
        printf("  FAIL: step=%d jump=%d completed=%d/%d\n",
               makespan_step, makespan_jump, completed_count, total_task_count);
        fails++;
    } else {
        printf("  OK: makespan=%d (step == jump)\n", makespan_step);
    }

    printf("\n[3] Konum maliyeti=1 ile tamamlanma\n");
    g_location_switch_cost = 1;
    if (!run_algo_to_end(1, 0, 1, &sjf)) { printf("  FAIL: SJF (cost=1)\n"); fails++; }
    else printf("  OK: SJF makespan=%d avg_wait=%.3f\n", sjf.makespan, sjf.avg_wait);
    if (!run_algo_to_end(2, 0, 1, &pri)) { printf("  FAIL: Priority (cost=1)\n"); fails++; }
    else printf("  OK: Priority makespan=%d avg_wait=%.3f\n", pri.makespan, pri.avg_wait);
    if (!run_algo_to_end(3, 2, 1, &rr)) { printf("  FAIL: RR (cost=1)\n"); fails++; }
    else printf("  OK: RR makespan=%d avg_wait=%.3f\n", rr.makespan, rr.avg_wait);
    if (!run_algo_to_end(4, 0, 1, &custom)) { printf("  FAIL: Custom (cost=1)\n"); fails++; }
    else printf("  OK: Custom makespan=%d avg_wait=%.3f\n", custom.makespan, custom.avg_wait);

    printf("\n[4] Durum sayilari tutarliligi\n");
    {
        SimStats st;
        int sum = 0;
        compute_statistics(&st);
        sum = st.status_counts[0] + st.status_counts[1] + st.status_counts[2] + st.status_counts[3];
        if (sum != total_task_count) {
            printf("  FAIL: status sum %d != %d\n", sum, total_task_count);
            fails++;
        } else printf("  OK: durum toplami = %d\n", sum);
    }

    printf("\n=== SELF-TEST bitti: %s ===\n", fails == 0 ? "TUM TESTLER GECTI" : "HATALAR VAR");
    return fails == 0 ? 0 : 1;
}

/* =========================================================
 * ANA CALISTIRMA (MAIN) - Tek ve Dogrudan Yonetim
 * ========================================================= */

int main(int argc, char **argv) {
    char path[512];
    int running = 1;
    int i;

    /* Komut satiri testi modu */
    if (argc >= 2 && (strcmp(argv[1], "--self-test") == 0 || strcmp(argv[1], "-t") == 0)) {
        const char *file = (argc >= 3) ? argv[2] : "gorevler.txt";
        return run_self_test(file);
    }

    /* Interaktif Baslangic */
    printf("Simulasyon baslatiliyor...\n");
    printf("(Konum degisim maliyeti: %d — PDF ornek tablo icin 0 gerekir)\n", g_location_switch_cost);

    /* Dosya yolu alma dongusu */
    for (;;) {
        printf("Gorev dosyasinin yolunu giriniz (orn. gorevler.txt): ");
        fflush(stdout);
        if (!read_line(path, sizeof(path))) {
            printf("\nGiris sonlandi.\n");
            return 0;
        }
        if (path[0] == '\0') continue;
        {
            size_t path_len = strlen(path);
            if (path_len >= 2 && path[0] == '"' && path[path_len - 1] == '"') {
                path[path_len - 1] = '\0';
                memmove(path, path + 1, path_len - 1);
            }
        }
        if (load_tasks(path) >= 0) break;
    }

    /* Teknisyen kimlikleri ve baslangic durumu */
    for (i = 0; i < NUM_TECH; i++) technicians[i].id = i + 1;
    reset_simulation();

    /* Ilk algoritma secimi zorunlu */
    while (!choose_algorithm(0)) {}

    /* Ana Menü Döngüsü */
    while (running) {
        long menu_choice;
        print_menu_header();
        menu_choice = ask_int("Seciminiz: ", 0, 10);
        switch (menu_choice) {
            case 1:
                choose_algorithm(1);
                break;
            case 2:
                list_tasks(-1);
                break;
            case 3: {
                long status_choice;
                printf("1 - Henuz gelmedi\n2 - Bekliyor\n3 - Devam ediyor\n4 - Tamamlandi\n");
                status_choice = ask_int("Durum: ", 1, 4);
                list_tasks((int)status_choice - 1);
                break;
            }
            case 4:
                query_task((int)ask_int("Gorev ID: ", 1, INT_MAX_VAL));
                break;
            case 5:
                show_technicians();
                break;
            case 6:
                step_simulation_1_unit();
                print_progress();
                break;
            case 7:
                advance_time_n_units((int)ask_int("N (pozitif tam sayi): ", 1, MAX_TIME_INPUT));
                print_progress();
                break;
            case 8: {
                long target = ask_int("Hedef zaman (>= 0): ", 0, MAX_TIME_INPUT);
                if (target < current_time)
                    printf("Gecmise donuluyor: simulasyon bastan yeniden hesaplanacak.\n");
                jump_to_time((int)target);
                print_progress();
                break;
            }
            case 9:
                show_statistics();
                break;
            case 10:
                reset_simulation();
                printf("Simulasyon sifirlandi (zaman 0).\n");
                break;
            case 0:
                running = 0;
                break;
        }
    }

    clear_queue(&waiting_queue);
    printf("Cikis yapildi.\n");
    return 0;
}
