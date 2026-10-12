#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TASKS 10000
#define NUM_TECH 4
#define PAGE_SIZE 20
#define MAX_TIME_INPUT 100000000L

#define INT_MAX_VAL 2147483647
#define INT_MIN_VAL (-2147483647 - 1)

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
    int last_tech_id;
} Task;

typedef struct {
    int id;                 /* 1, 2, 3, 4 */
    char current_location[20];
    char target_location[20];
    TechStatus status;
    int transition_left;    /* kalan gecis suresi */
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
int completed_count = 0;

/* =========================================================
 * 1. SADE DOSYA OKUMA VE KUYRUK ISLEMLERI (project_v1.1 mantigi)
 * ========================================================= */

int is_valid_location(const char *s) {
    return (strcmp(s, "LAB_A") == 0 || strcmp(s, "LAB_B") == 0 ||
            strcmp(s, "DERSLIK_1") == 0 || strcmp(s, "DERSLIK_2") == 0 ||
            strcmp(s, "DERSLIK_3") == 0);
}

int find_task_index(int id) {
    int i;
    for (i = 0; i < total_task_count; i++) {
        if (master_tasks[i].id == id) return i;
    }
    return -1;
}

int load_tasks(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Hata: Dosya acilamadi: %s\n", filename);
        return -1;
    }

    char line[512];
    int line_number = 0;
    int rejected = 0;
    total_task_count = 0;

    while (fgets(line, sizeof(line), fp) != NULL) {
        line_number++;

        /* 1. Bosluklari atla */
        char *ptr = line;
        while (*ptr == ' ' || *ptr == '\t') ptr++;

        /* Yorum ve bos satirlari gec */
        if (*ptr == '\0' || *ptr == '\r' || *ptr == '\n' || *ptr == '#') {
            continue;
        }

        int id, arrival, duration, priority;
        char type[40], location[20], extra[50];

        /* 2. sscanf ile 6 veri ve ekstra tespiti */
        int length = sscanf(ptr, "%d %d %d %d %39s %19s %49s",
                            &id, &arrival, &duration, &priority, type, location, extra);

        if (length < 6) {
            printf("  [REDDEDILDI] Satir %d: Eksik alan bulundu.\n", line_number);
            rejected++;
            continue;
        }
        if (length > 6) {
            printf("  [REDDEDILDI] Satir %d: Fazla alan bulundu.\n", line_number);
            rejected++;
            continue;
        }

        /* 3. Sayisal deger kontrolleri */
        if (id <= 0 || arrival < 0 || duration <= 0 || priority < 1 || priority > 5) {
            printf("  [REDDEDILDI] Satir %d: Gecersiz veri araligi (ID:%d).\n", line_number, id);
            rejected++;
            continue;
        }

        /* 4. Konum kontrolu */
        if (!is_valid_location(location)) {
            printf("  [REDDEDILDI] Satir %d: Gecersiz konum (%s).\n", line_number, location);
            rejected++;
            continue;
        }

        /* 5. Yinelenen ID kontrolu */
        if (find_task_index(id) >= 0) {
            printf("  [REDDEDILDI] Satir %d: Yinelenen ID (%d).\n", line_number, id);
            rejected++;
            continue;
        }

        /* Kapasite kontrolu */
        if (total_task_count >= MAX_TASKS) {
            printf("Hata: Maksimum gorev kapasitesi (%d) asildi!\n", MAX_TASKS);
            break;
        }

        /* 6. Gorevi diziye ilk haliyle kaydet */
        Task *t = &master_tasks[total_task_count];
        memset(t, 0, sizeof(Task));
        t->id = id;
        t->arrival_time = arrival;
        t->duration = duration;
        t->remaining_time = duration;
        t->priority = priority;
        strncpy(t->type, type, sizeof(t->type) - 1);
        strncpy(t->location, location, sizeof(t->location) - 1);
        t->status = NOT_ARRIVED;
        t->first_start_time = -1;
        t->finish_time = -1;
        t->assigned_tech_id = 0;
        t->last_tech_id = 0;

        total_task_count++;
    }

    fclose(fp);
    printf("Dosya okundu: %d gecerli gorev, %d reddedilen satir.\n", total_task_count, rejected);
    return total_task_count;
}

/* Standart Kuyruk Ekleme (FIFO) */
void enqueue(QueueNode **head, Task *task) {
    QueueNode *new_node = (QueueNode *)malloc(sizeof(QueueNode));
    if (!new_node) {
        printf("Hata: Yetersiz bellek!\n");
        exit(1);
    }
    new_node->task = task;
    new_node->next = NULL;

    if (*head == NULL) {
        *head = new_node;
        return;
    }

    QueueNode *temp = *head;
    while (temp->next != NULL) {
        temp = temp->next;
    }
    temp->next = new_node;
}

/* Sade SJF Sirali Ekleme */
void insert_sorted_sjf(QueueNode **head, Task *task) {
    QueueNode *new_node = (QueueNode *)malloc(sizeof(QueueNode));
    if (!new_node) {
        printf("Hata: Yetersiz bellek!\n");
        exit(1);
    }
    new_node->task = task;
    new_node->next = NULL;

    if (*head == NULL) {
        *head = new_node;
        return;
    }

    /* Basa ekleme kurali: Once kisa sure, esitlikte erken gelis, esitlikte kucuk ID */
    int basa_ekle = 0;
    if (task->duration < (*head)->task->duration)
        basa_ekle = 1;
    else if (task->duration == (*head)->task->duration && task->arrival_time < (*head)->task->arrival_time)
        basa_ekle = 1;
    else if (task->duration == (*head)->task->duration && task->arrival_time == (*head)->task->arrival_time && task->id < (*head)->task->id)
        basa_ekle = 1;

    if (basa_ekle) {
        new_node->next = *head;
        *head = new_node;
        return;
    }

    /* Araya veya sona ekleme */
    QueueNode *temp = *head;
    while (temp->next != NULL) {
        int araya_ekle = 0;
        Task *sonraki = temp->next->task;

        if (task->duration < sonraki->duration)
            araya_ekle = 1;
        else if (task->duration == sonraki->duration && task->arrival_time < sonraki->arrival_time)
            araya_ekle = 1;
        else if (task->duration == sonraki->duration && task->arrival_time == sonraki->arrival_time && task->id < sonraki->id)
            araya_ekle = 1;

        if (araya_ekle) {
            new_node->next = temp->next;
            temp->next = new_node;
            return;
        }
        temp = temp->next;
    }

    temp->next = new_node;
}

/* Sade Priority Sirali Ekleme */
void insert_sorted_priority(QueueNode **head, Task *task) {
    QueueNode *new_node = (QueueNode *)malloc(sizeof(QueueNode));
    if (!new_node) {
        printf("Hata: Yetersiz bellek!\n");
        exit(1);
    }
    new_node->task = task;
    new_node->next = NULL;

    if (*head == NULL) {
        *head = new_node;
        return;
    }

    /* Basa ekleme kurali: Once kucuk oncelik sayisi (yuksek oncelik), esitlikte gelis, sonra ID */
    int basa_ekle = 0;
    if (task->priority < (*head)->task->priority)
        basa_ekle = 1;
    else if (task->priority == (*head)->task->priority && task->arrival_time < (*head)->task->arrival_time)
        basa_ekle = 1;
    else if (task->priority == (*head)->task->priority && task->arrival_time == (*head)->task->arrival_time && task->id < (*head)->task->id)
        basa_ekle = 1;

    if (basa_ekle) {
        new_node->next = *head;
        *head = new_node;
        return;
    }

    /* Araya veya sona ekleme */
    QueueNode *temp = *head;
    while (temp->next != NULL) {
        int araya_ekle = 0;
        Task *sonraki = temp->next->task;

        if (task->priority < sonraki->priority)
            araya_ekle = 1;
        else if (task->priority == sonraki->priority && task->arrival_time < sonraki->arrival_time)
            araya_ekle = 1;
        else if (task->priority == sonraki->priority && task->arrival_time == sonraki->arrival_time && task->id < sonraki->id)
            araya_ekle = 1;

        if (araya_ekle) {
            new_node->next = temp->next;
            temp->next = new_node;
            return;
        }
        temp = temp->next;
    }

    temp->next = new_node;
}

Task *dequeue(QueueNode **head) {
    if (head == NULL || *head == NULL) return NULL;

    QueueNode *temp = *head;
    Task *task = temp->task;
    *head = (*head)->next;
    free(temp);
    return task;
}

void clear_queue(QueueNode **head) {
    if (head == NULL) return;
    while (*head != NULL) {
        dequeue(head);
    }
}

/* =========================================================
 * 2. SIMULASYON MOTORU (main.c'nin calisan cekirdegi)
 * ========================================================= */

int needs_transition(const Technician *tech, const Task *task) {
    if (g_location_switch_cost <= 0) return 0;
    if (tech->current_location[0] == '\0') return 0;
    return strcmp(tech->current_location, task->location) != 0;
}

void enqueue_by_algorithm(Task *task) {
    switch (selected_algorithm) {
        case 1:  insert_sorted_sjf(&waiting_queue, task); break;
        case 2:  insert_sorted_priority(&waiting_queue, task); break;
        default: enqueue(&waiting_queue, task); break;
    }
}

/* Simülasyon zamanında gelen görevleri ana havuzdan alıp kuyruğa ekler */
void admit_arrivals(void) {
    int i;
    for (i = 0; i < total_task_count; i++) {
        Task *task = &master_tasks[i];
        if (task->status == NOT_ARRIVED && task->arrival_time <= current_time) {
            task->status = WAITING;
            enqueue_by_algorithm(task);
        }
    }
}

/* Ozel Algoritma: Konum Duyarli SJF */
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

    Task *task = best->task;
    free(best);
    return task;
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
        strncpy(tech->target_location, task->location, sizeof(tech->target_location) - 1);
    } else {
        tech->status = BUSY;
        tech->transition_left = 0;
        strncpy(tech->current_location, task->location, sizeof(tech->current_location) - 1);
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

    /* 1. Calisanlarin surelerini azalt */
    for (i = 0; i < NUM_TECH; i++) {
        Technician *tech = &technicians[i];
        if (tech->status == IN_TRANSIT) {
            tech->transition_left--;
            if (tech->transition_left <= 0) {
                tech->transition_left = 0;
                tech->status = BUSY;
                strncpy(tech->current_location, tech->target_location, sizeof(tech->current_location) - 1);
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

    /* 2. Simülasyon saati ilerler */
    current_time++;

    /* 3. Biten gorevler ve kuantumu dolanlar */
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

    /* 4. Yeni gelenleri kuyruga ekle, ardindan RR dilimi bitenleri geri koy */
    admit_arrivals();
    for (k = 0; k < quantum_expired_count; k++) {
        enqueue(&waiting_queue, quantum_expired[k]);
    }

    /* 5. Bosta olan teknisyenlere yeni gorevleri ata */
    assign_idle_technicians();
}

void advance_time_n_units(int n) {
    int i;
    for (i = 0; i < n; i++) {
        step_simulation_1_unit();
    }
}

void reset_simulation(void) {
    int i;
    clear_queue(&waiting_queue);
    current_time = 0;
    completed_count = 0;

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
    if (target_time < current_time) {
        reset_simulation();
    }
    while (current_time < target_time) {
        step_simulation_1_unit();
    }
}

/* =========================================================
 * 3. EKRAN, ISTATISTIK VE RAPORLAMA FONKSIYONLARI
 * ========================================================= */

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
            if (fgets(buf, sizeof(buf), stdin) && (buf[0] == 'q' || buf[0] == 'Q')) break;
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
        printf("Toplam bekleme    : %d\n", task->finish_time - task->arrival_time - task->duration);
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
           total_task_count, stats.status_counts[0], stats.status_counts[1],
           stats.status_counts[2], stats.status_counts[3]);

    if (total_task_count == 0)
        printf("Makespan: hesaplanamaz\n");
    else if (stats.all_completed)
        printf("Makespan (son bitis zamani): %d\n", stats.makespan);
    else
        printf("Makespan: henuz tum gorevler tamamlanmadi\n");

    if (completed_count > 0) {
        printf("Ortalama bekleme        : %.3f\n", stats.avg_wait);
        printf("Ortalama sistemde kalma : %.3f\n", stats.avg_turnaround);
    }
    if (current_time > 0) {
        for (i = 0; i < NUM_TECH; i++) {
            printf("T%d kullanim orani       : %%%.3f (calisilan: %d)\n",
                   technicians[i].id,
                   100.0 * technicians[i].total_worked / current_time,
                   technicians[i].total_worked);
        }
        printf("Genel kullanim orani    : %%%.3f\n", stats.overall_utilization);
    }
}

/* =========================================================
 * 4. KULLANICI GIRISI VE MENU
 * ========================================================= */

int ask_int_simple(const char *prompt, int min_val, int max_val) {
    int val;
    char line[128];
    while (1) {
        printf("%s", prompt);
        if (!fgets(line, sizeof(line), stdin)) exit(0);
        if (sscanf(line, "%d", &val) == 1 && val >= min_val && val <= max_val) {
            return val;
        }
        printf("Gecersiz giris! (%d ile %d arasinda giriniz)\n", min_val, max_val);
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
    printf("\n--- Algoritma Secimi ---\n");
    printf("1 - Shortest Job (SJF)\n2 - Priority\n3 - Round Robin\n4 - Konum Duyarli SJF (Ozel)\n");
    if (allow_cancel) printf("0 - Geri\n");

    int choice = ask_int_simple("Seciminiz: ", allow_cancel ? 0 : 1, 4);
    if (choice == 0) return 0;
    if (choice == 3) rr_quantum = ask_int_simple("Zaman dilimi (quantum): ", 1, 1000000);

    selected_algorithm = choice;
    reset_simulation();
    printf("%s secildi; simulasyon baslatildi.\n", algorithm_name(selected_algorithm));
    return 1;
}

int main(void) {
    char path[256];
    int running = 1, i;

    printf("Simulasyon baslatiliyor...\n");

    /* Dosya yolu al */
    while (1) {
        printf("Gorev dosyasinin yolunu giriniz (orn. gorevler.txt): ");
        if (!fgets(path, sizeof(path), stdin)) return 0;
        path[strcspn(path, "\r\n")] = '\0';
        if (strlen(path) == 0) continue;

        if (load_tasks(path) >= 0) break;
    }

    for (i = 0; i < NUM_TECH; i++) technicians[i].id = i + 1;
    reset_simulation();

    /* Ilk algoritma secimi */
    while (!choose_algorithm(0)) {}

    /* Ana Menü Döngüsü */
    while (running) {
        print_menu_header();
        int menu_choice = ask_int_simple("Seciminiz: ", 0, 10);
        switch (menu_choice) {
            case 1: choose_algorithm(1); break;
            case 2: list_tasks(-1); break;
            case 3: {
                printf("1 - Henuz gelmedi\n2 - Bekliyor\n3 - Devam ediyor\n4 - Tamamlandi\n");
                int status_choice = ask_int_simple("Durum: ", 1, 4);
                list_tasks(status_choice - 1);
                break;
            }
            case 4: {
                int id = ask_int_simple("Gorev ID: ", 1, INT_MAX_VAL);
                query_task(id);
                break;
            }
            case 5: show_technicians(); break;
            case 6:
                step_simulation_1_unit();
                printf("Guncel zaman: %d | Tamamlanan: %d/%d\n", current_time, completed_count, total_task_count);
                break;
            case 7: {
                int n = ask_int_simple("N birim: ", 1, 1000000);
                advance_time_n_units(n);
                printf("Guncel zaman: %d | Tamamlanan: %d/%d\n", current_time, completed_count, total_task_count);
                break;
            }
            case 8: {
                int target = ask_int_simple("Hedef zaman (>= 0): ", 0, 10000000);
                jump_to_time(target);
                printf("Guncel zaman: %d | Tamamlanan: %d/%d\n", current_time, completed_count, total_task_count);
                break;
            }
            case 9: show_statistics(); break;
            case 10:
                reset_simulation();
                printf("Simulasyon sifirlandi.\n");
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
