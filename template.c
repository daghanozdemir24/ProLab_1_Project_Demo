#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TASKS 2000

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

// =========================================================
// 1. KİŞİNİN DOLDURACAĞI METOTLAR (File I/O & Queue)
// =========================================================

int load_tasks(const char *filename) {
    // TODO: 1. Kişi dolduracak
    // - Dosyayı aç, satır satır oku.
    // - # ve boş satırları atla.
    // - Hatalı ve yinelenen ID'leri reddedip ekrana yaz.
    // - Geçerli verileri master_tasks dizisine yaz, total_task_count döndür.
    return 0;
}

void enqueue(QueueNode **head, Task *task) {
    // TODO: 1. Kişi dolduracak (Kuyruk sonuna ekleme)
}

void insert_sorted_sjf(QueueNode **head, Task *task) {
    // TODO: 1. Kişi dolduracak (Süre -> Erken Geliş -> Küçük ID)
}

void insert_sorted_priority(QueueNode **head, Task *task) {
    // TODO: 1. Kişi dolduracak (Öncelik -> Erken Geliş -> Küçük ID)
}

Task* dequeue(QueueNode **head) {
    // TODO: 1. Kişi dolduracak (Baştan eleman çıkarma ve free işlemi)
    return NULL;
}

void clear_queue(QueueNode **head) {
    // TODO: 1. Kişi dolduracak
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
    for (int i = 0; i < n; i++) {
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
