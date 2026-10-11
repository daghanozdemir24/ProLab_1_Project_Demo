#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TASKS 10000

// --- DURUM SABİTLERİ ---
typedef enum {
    HENUZ_GELMEDI,
    BEKLIYOR,
    DEVAM_EDIYOR,
    TAMAMLANDI
} TaskStatus;

typedef enum {
    BOS,
    GECISTE,
    MESGUL
} TechStatus;

// --- VERİ YAPILARI ---
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
} Task;

typedef struct {
    int id;                 // 1, 2, 3, 4
    char current_location[20];
    TechStatus status;
    int transition_left;    // 1 birimlik geçiş süresi sayacı
    int total_worked;       // Toplam çalışılan birim
    int current_quantum;    // Round Robin için dilim sayacı
    Task *current_task;
} Technician;

typedef struct QueueNode {
    Task *task;
    struct QueueNode *next;
} QueueNode;

// --- GLOBAL SİMÜLASYON DURUMU ---
Task master_tasks[MAX_TASKS];
int total_task_count = 0;

Technician technicians[4];
QueueNode *waiting_queue = NULL;

int current_time = 0;
int selected_algorithm = 0; // 1: SJF, 2: Priority, 3: RR, 4: Custom
int rr_quantum = 0;


int load_tasks(const char *filename) {
    
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Dosya acilamadi.\n");
        return 0;
    }

    char line[512];
    int line_number = 0;
    total_task_count = 0;

    while (fgets(line, sizeof(line), fp) != NULL) {
        line_number++;

        
        char *ptr = line;
        while (*ptr == ' ' || *ptr == '\t') ptr++;

        
        if (*ptr == '\0' || *ptr == '\r' || *ptr == '\n' || *ptr == '#') {
            continue;
        }

        int id, arrival, duration, priority;
        char type[40], location[20], extra[50];

        
        int length = sscanf(ptr, "%d %d %d %d %39s %19s %49s", 
                            &id, &arrival, &duration, &priority, type, location, extra);

        if (length < 6) {
            printf("Satir %d eksik alan bulundu. \n", line_number);
            continue;
        }
        if (length > 6) {
            printf("Satir %d fazla alan bulundu. \n", line_number);
            continue;
        }

        
        if (id <= 0 || arrival < 0 || duration <= 0 || priority < 1 || priority > 5) {
            printf("Satir %d gecersiz veri araligi (ID:%d) \n", line_number, id);
            continue;
        }

        
        int id_control = 0;
        int i;
        for (i = 0; i < total_task_count; i++) {
            if (master_tasks[i].id == id) {
                id_control = 1;
                break;
            }
        }
        if (id_control) {
            printf("Satir %d yinelenen ID (%d). \n", line_number, id);
            continue;
        }

        
        if (total_task_count >= MAX_TASKS) {
            printf("Maksimum gorev kapasitesi (%d) asildi.\n", MAX_TASKS);
            break;
        }

        
        Task *t = &master_tasks[total_task_count];
        t->id = id;
        t->arrival_time = arrival;
        t->duration = duration;
        t->remaining_time = duration;
        t->priority = priority;
        strncpy(t->type, type, sizeof(t->type) - 1);
        t->type[sizeof(t->type) - 1] = '\0';
        strncpy(t->location, location, sizeof(t->location) - 1);
        t->location[sizeof(t->location) - 1] = '\0';

        t->status = HENUZ_GELMEDI;
        t->first_start_time = -1;
        t->finish_time = -1;
        t->assigned_tech_id = 0;

        total_task_count++;
    }

    fclose(fp);
    return total_task_count;	
}

void enqueue(QueueNode **head, Task *task) {
	
	QueueNode* new_node = (QueueNode*)malloc(sizeof(QueueNode));
	new_node->task = task;
	new_node->next = NULL;
	
	if(*head==NULL){
		*head = new_node;
		return;
		
	}
	
	QueueNode* temp = *head;
	while(temp->next != NULL){
		temp = temp->next;
	}
	temp->next = new_node; 
	
}

void insert_sorted_sjf(QueueNode **head, Task *task) {
    
    QueueNode* new_node = (QueueNode*)malloc(sizeof(QueueNode));
    new_node->task = task;
    new_node->next = NULL;
    
    if (*head == NULL) {
        *head = new_node;
        return;
    }
    
    int control = 0;
    int control2 = 0;
    
    if(new_node->task->duration < (*head)->task->duration)
    control = 1;
    
    if(new_node->task->duration == (*head)->task->duration 
	&& new_node->task->arrival_time < (*head)->task->arrival_time)
    control = 1;
    
    if(new_node->task->duration == (*head)->task->duration 
	&& new_node->task->arrival_time == (*head)->task->arrival_time 
	&&new_node->task->id < (*head)->task->id)
	control = 1;
    
    
    
    if(control==1){
    	new_node->next = *head;
    	*head = new_node;
    	return;
	}
	
	QueueNode* temp = *head;
	while(temp->next!=NULL){
	
		control2 = 0;
	
		if(new_node->task->duration < temp->next->task->duration)
    	control2 = 1;
    
    	if(new_node->task->duration == temp->next->task->duration 
		&& new_node->task->arrival_time < temp->next->task->arrival_time)
    	control2 = 1;
    
    	if(new_node->task->duration == temp->next->task->duration
		&& new_node->task->arrival_time == temp->next->task->arrival_time 
		&&new_node->task->id < temp->next->task->id)
		control2 = 1;
		
		if(control2 == 1){
			new_node->next = temp->next;
			temp->next = new_node;
			return;
		}
		temp = temp->next;
		
	}
	
	temp->next = new_node;
	
}

void insert_sorted_priority(QueueNode **head, Task *task) {
    
    QueueNode* new_node = (QueueNode*)malloc(sizeof(QueueNode));
    new_node->task = task;
    new_node->next = NULL;
    
    if(*head==NULL){
    	*head=new_node;
    	return;
	}
	
	int control = 0;
	int control2 = 0;
	
	if(new_node->task->priority < (*head)->task->priority)
	control = 1;
	if(new_node->task->priority == (*head)->task->priority
	&& new_node->task->arrival_time < (*head)->task->arrival_time)
	control = 1;
	if(new_node->task->priority == (*head)->task->priority
	&& new_node->task->arrival_time == (*head)->task->arrival_time
	&& new_node->task->id < (*head)->task->id)
	control = 1;
	
	if(control == 1){
		new_node->next = *head;
		*head = new_node;
		return;
	}
	
	QueueNode* temp = *head;
	while(temp->next!=NULL){
		control2 = 0;
		if(new_node->task->priority < temp->next->task->priority)
		control2 = 1;
		if(new_node->task->priority == temp->next->task->priority
		&& new_node->task->arrival_time < temp->next->task->arrival_time)
		control2 = 1;
		if(new_node->task->priority == temp->next->task->priority
		&& new_node->task->arrival_time == temp->next->task->arrival_time
		&& new_node->task->id < temp->next->task->id)
		control2 = 1;
		
		if(control2==1){
			new_node->next = temp->next;
			temp->next = new_node;
			return;
		}
		
		
		
		temp = temp->next;
	}
	temp->next = new_node;
	
	
}

Task* dequeue(QueueNode **head) {

	if(head == NULL || *head==NULL){
		return NULL;
	}

	QueueNode* temp = *head;
	Task* task = temp->task;
	*head = (*head)->next;
	free(temp);
	
    return task;
}

void clear_queue(QueueNode **head) {
    
    if(head == NULL || *head == NULL){
    	return;
	}
	while(*head!=NULL){
		dequeue(head);
	}
}

// =========================================================
// 2. KİŞİNİN DOLDURACAĞI METOTLAR (Simülasyon & Ekran)
// =========================================================

void step_simulation_1_unit() {
    // TODO: 2. Kişi dolduracak
    // 1. Çalışanların sürelerini azalt (Geçişte olanların transition_left azalt)
    // 2. current_time++
    // 3. Kalan süresi 0 olan görevleri TAMAMLANDI yap
    // 4. RR ise süresi dolanları kuyruğa alma sırasına hazırla
    // 5. Yeni gelen görevleri kuyruğa ekle
    // 6. Boş teknisyenlere (T1..T4) atama yap, konum farklıysa GECISTE durumuna al
}

void advance_time_n_units(int n) {
	int i;
    for ( i = 0; i < n; i++) {
        step_simulation_1_unit();
    }
}

void reset_simulation() {
    // TODO: 2. Kişi dolduracak
    // current_time = 0 yap, görevleri ilk haline çek, teknisyenleri sıfırla
}

void jump_to_time(int target_time) {
    if (target_time < current_time) {
        reset_simulation();
    }
    while (current_time < target_time) {
        step_simulation_1_unit();
    }
}

void show_statistics() {
    // TODO: 2. Kişi dolduracak
    // Makespan, ortalama bekleme, kullanım oranlarını 3 ondalık basamakla bas
}

// =========================================================
// ORTAK / ANA ÇALIŞTIRMA (MAIN)
// =========================================================

int main() {
    // Dosya yolu alma, menü döngüsü ve kullanıcı girişleri burada çalışır.
    printf("Simulasyon baslatiliyor...\n");
    return 0;
}
