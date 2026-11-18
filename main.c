/*
 * Project 56: Hospital Bed and Resource Allocation System
 *
 * Features:
 *  - Ward records (ward_id, capacity, occupied_beds, department)
 *  - Patient records (name, age, severity, ward_assigned)
 *  - Priority-based allocation (higher severity is served first)
 *  - Functions for admission / discharge, arrays for ward listing
 *  - Dynamic memory using pointers, malloc() and realloc()
 *  - Reports: occupancy, vacant beds, department statistics
 *
 * Compile: gcc hospital_allocation.c -o hospital
 * Run:     ./hospital
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define NAME_LEN   50
#define DEPT_LEN   30
#define WAITING    -1      /* ward_assigned value for patients on waiting list */
#define MIN_SEV     1
#define MAX_SEV    10      /* 10 = most critical */

/* ---------------------------- Data structures ---------------------------- */

typedef struct {
    int  ward_id;
    int  capacity;
    int  occupied_beds;
    char department[DEPT_LEN];
} Ward;

typedef struct {
    char name[NAME_LEN];
    int  age;
    int  severity;                 /* 1 (mild) .. 10 (critical) */
    int  ward_assigned;            /* ward_id, or WAITING */
    char department[DEPT_LEN];     /* department the patient needs */
} Patient;

typedef struct {
    Ward    *wards;                /* dynamic array of wards */
    int      ward_count;
    Patient *patients;             /* dynamic array of patients */
    int      patient_count;
    int      patient_cap;
} Hospital;

/* ------------------------------ Input helpers ---------------------------- */

static int readLine(char *buf, int size)
{
    if (fgets(buf, size, stdin) == NULL)
        return 0;
    buf[strcspn(buf, "\n")] = '\0';
    return 1;
}

static int readInt(const char *prompt, int min, int max)
{
    char line[64];
    char *end;
    long v;

    while (1) {
        printf("%s", prompt);
        if (!readLine(line, sizeof line))
            exit(0);                       /* EOF: quit cleanly */
        v = strtol(line, &end, 10);
        if (end != line && *end == '\0' && v >= min && v <= max)
            return (int)v;
        printf("  Invalid input. Enter a number between %d and %d.\n", min, max);
    }
}

static void readString(const char *prompt, char *buf, int size)
{
    do {
        printf("%s", prompt);
        if (!readLine(buf, size))
            exit(0);
    } while (buf[0] == '\0');
}

static int equalsIgnoreCase(const char *a, const char *b)
{
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
            return 0;
        a++;
        b++;
    }
    return *a == *b;
}

/* --------------------------- Hospital lifecycle -------------------------- */

static void initHospital(Hospital *h)
{
    h->ward_count    = 0;
    h->patient_count = 0;
    h->patient_cap   = 4;
    h->wards         = NULL;
    h->patients      = (Patient *)malloc(h->patient_cap * sizeof(Patient));
    if (h->patients == NULL) {
        printf("Memory allocation failed.\n");
        exit(1);
    }
}

static void freeHospital(Hospital *h)
{
    free(h->wards);
    free(h->patients);
    h->wards = NULL;
    h->patients = NULL;
    h->ward_count = h->patient_count = h->patient_cap = 0;
}

/* ------------------------------ Ward functions --------------------------- */

static Ward *findWardById(const Hospital *h, int id)
{
    int i;
    for (i = 0; i < h->ward_count; i++)
        if (h->wards[i].ward_id == id)
            return &h->wards[i];
    return NULL;
}

static int addWard(Hospital *h, int id, int capacity, const char *dept)
{
    Ward *tmp;

    if (findWardById(h, id) != NULL) {
        printf("  Ward ID %d already exists.\n", id);
        return 0;
    }
    tmp = (Ward *)realloc(h->wards, (h->ward_count + 1) * sizeof(Ward));
    if (tmp == NULL) {
        printf("  Memory allocation failed.\n");
        return 0;
    }
    h->wards = tmp;

    h->wards[h->ward_count].ward_id       = id;
    h->wards[h->ward_count].capacity      = capacity;
    h->wards[h->ward_count].occupied_beds = 0;
    strncpy(h->wards[h->ward_count].department, dept, DEPT_LEN - 1);
    h->wards[h->ward_count].department[DEPT_LEN - 1] = '\0';
    h->ward_count++;
    return 1;
}

static void addWardInteractive(Hospital *h)
{
    int  id, cap;
    char dept[DEPT_LEN];

    id  = readInt("Ward ID: ", 1, 100000);
    cap = readInt("Capacity (beds): ", 1, 1000);
    readString("Department: ", dept, sizeof dept);

    if (addWard(h, id, cap, dept))
        printf("  Ward %d added to %s.\n", id, dept);
}

/* Returns the ward in `dept` with the most vacant beds, or NULL if all full. */
static Ward *findBestWard(const Hospital *h, const char *dept)
{
    Ward *best = NULL;
    int   i, bestFree = 0, freeBeds;

    for (i = 0; i < h->ward_count; i++) {
        if (!equalsIgnoreCase(h->wards[i].department, dept))
            continue;
        freeBeds = h->wards[i].capacity - h->wards[i].occupied_beds;
        if (freeBeds > bestFree) {
            bestFree = freeBeds;
            best = &h->wards[i];
        }
    }
    return best;
}

static int departmentExists(const Hospital *h, const char *dept)
{
    int i;
    for (i = 0; i < h->ward_count; i++)
        if (equalsIgnoreCase(h->wards[i].department, dept))
            return 1;
    return 0;
}

/* ---------------------------- Patient functions -------------------------- */

static Patient *addPatientRecord(Hospital *h, const char *name, int age,
                                 int severity, const char *dept)
{
    Patient *p;

    if (h->patient_count == h->patient_cap) {      /* grow the array */
        int      newCap = h->patient_cap * 2;
        Patient *tmp = (Patient *)realloc(h->patients, newCap * sizeof(Patient));
        if (tmp == NULL) {
            printf("  Memory allocation failed.\n");
            return NULL;
        }
        h->patients   = tmp;
        h->patient_cap = newCap;
    }

    p = &h->patients[h->patient_count++];
    strncpy(p->name, name, NAME_LEN - 1);
    p->name[NAME_LEN - 1] = '\0';
    strncpy(p->department, dept, DEPT_LEN - 1);
    p->department[DEPT_LEN - 1] = '\0';
    p->age           = age;
    p->severity      = severity;
    p->ward_assigned = WAITING;
    return p;
}

/*
 * Priority logic: whenever a bed becomes free in `ward`, repeatedly admit the
 * WAITING patient of that department with the HIGHEST severity.
 * Ties are broken by arrival order (earlier record wins).
 */
static void admitFromWaitingList(Hospital *h, Ward *ward)
{
    int i, best;

    while (ward->occupied_beds < ward->capacity) {
        best = -1;
        for (i = 0; i < h->patient_count; i++) {
            if (h->patients[i].ward_assigned != WAITING)
                continue;
            if (!equalsIgnoreCase(h->patients[i].department, ward->department))
                continue;
            if (best == -1 || h->patients[i].severity > h->patients[best].severity)
                best = i;
        }
        if (best == -1)
            break;                                  /* nobody waiting */

        h->patients[best].ward_assigned = ward->ward_id;
        ward->occupied_beds++;
        printf("  Waiting-list patient %s (severity %d) admitted to ward %d.\n",
               h->patients[best].name, h->patients[best].severity, ward->ward_id);
    }
}

static void admitPatient(Hospital *h)
{
    char     name[NAME_LEN], dept[DEPT_LEN];
    int      age, severity;
    Patient *p;
    Ward    *w;

    if (h->ward_count == 0) {
        printf("  No wards exist yet. Add a ward first.\n");
        return;
    }

    readString("Patient name: ", name, sizeof name);
    age      = readInt("Age: ", 0, 120);
    severity = readInt("Severity (1 = mild ... 10 = critical): ", MIN_SEV, MAX_SEV);
    readString("Department needed: ", dept, sizeof dept);

    if (!departmentExists(h, dept)) {
        printf("  No ward exists for department '%s'.\n", dept);
        return;
    }

    p = addPatientRecord(h, name, age, severity, dept);
    if (p == NULL)
        return;

    w = findBestWard(h, dept);
    if (w != NULL) {
        p->ward_assigned = w->ward_id;
        w->occupied_beds++;
        printf("  %s admitted to ward %d (%s).\n", p->name, w->ward_id, w->department);
    } else {
        printf("  All %s beds are full. %s placed on the waiting list "
               "(severity %d).\n", dept, p->name, severity);
    }
}

static void dischargePatient(Hospital *h)
{
    char  name[NAME_LEN];
    int   i, found = -1;
    Ward *w;

    if (h->patient_count == 0) {
        printf("  No patients on record.\n");
        return;
    }

    readString("Name of patient to discharge: ", name, sizeof name);

    for (i = 0; i < h->patient_count; i++) {
        if (equalsIgnoreCase(h->patients[i].name, name) &&
            h->patients[i].ward_assigned != WAITING) {
            found = i;
            break;
        }
    }
    if (found == -1) {
        printf("  No admitted patient named '%s'.\n", name);
        return;
    }

    w = findWardById(h, h->patients[found].ward_assigned);
    if (w != NULL)
        w->occupied_beds--;

    printf("  %s discharged from ward %d.\n", h->patients[found].name,
           h->patients[found].ward_assigned);

    /* remove the record by shifting the array left */
    for (i = found; i < h->patient_count - 1; i++)
        h->patients[i] = h->patients[i + 1];
    h->patient_count--;

    /* the freed bed goes to the most severe waiting patient */
    if (w != NULL)
        admitFromWaitingList(h, w);
}

/* -------------------------------- Listings ------------------------------- */

static void listWards(const Hospital *h)
{
    int i;

    if (h->ward_count == 0) {
        printf("  No wards.\n");
        return;
    }
    printf("\n%-8s %-18s %-9s %-9s %-7s\n",
           "WardID", "Department", "Capacity", "Occupied", "Vacant");
    printf("-------------------------------------------------------\n");
    for (i = 0; i < h->ward_count; i++) {
        const Ward *w = &h->wards[i];
        printf("%-8d %-18s %-9d %-9d %-7d\n", w->ward_id, w->department,
               w->capacity, w->occupied_beds, w->capacity - w->occupied_beds);
    }
}

static void listPatients(const Hospital *h)
{
    int i;

    if (h->patient_count == 0) {
        printf("  No patients on record.\n");
        return;
    }
    printf("\n%-20s %-5s %-9s %-16s %-10s\n",
           "Name", "Age", "Severity", "Department", "Ward");
    printf("-----------------------------------------------------------------\n");
    for (i = 0; i < h->patient_count; i++) {
        const Patient *p = &h->patients[i];
        if (p->ward_assigned == WAITING)
            printf("%-20s %-5d %-9d %-16s %-10s\n", p->name, p->age,
                   p->severity, p->department, "WAITING");
        else
            printf("%-20s %-5d %-9d %-16s %-10d\n", p->name, p->age,
                   p->severity, p->department, p->ward_assigned);
    }
}

/* Waiting list printed in priority order (highest severity first). */
static void listWaiting(const Hospital *h)
{
    int *idx, n = 0, i, j, key;

    idx = (int *)malloc((h->patient_count > 0 ? h->patient_count : 1) * sizeof(int));
    if (idx == NULL) {
        printf("  Memory allocation failed.\n");
        return;
    }

    for (i = 0; i < h->patient_count; i++)
        if (h->patients[i].ward_assigned == WAITING)
            idx[n++] = i;

    if (n == 0) {
        printf("  Waiting list is empty.\n");
        free(idx);
        return;
    }

    /* stable insertion sort by severity (descending) */
    for (i = 1; i < n; i++) {
        key = idx[i];
        j = i - 1;
        while (j >= 0 && h->patients[idx[j]].severity < h->patients[key].severity) {
            idx[j + 1] = idx[j];
            j--;
        }
        idx[j + 1] = key;
    }

    printf("\nWaiting list (highest priority first):\n");
    printf("%-4s %-20s %-5s %-9s %-16s\n", "#", "Name", "Age", "Severity", "Department");
    printf("---------------------------------------------------------\n");
    for (i = 0; i < n; i++) {
        const Patient *p = &h->patients[idx[i]];
        printf("%-4d %-20s %-5d %-9d %-16s\n", i + 1, p->name, p->age,
               p->severity, p->department);
    }
    free(idx);
}

/* --------------------------------- Reports ------------------------------- */

static void generateReport(const Hospital *h)
{
    int i, j, k, seen;
    int totalCap = 0, totalOcc = 0, waiting = 0;

    if (h->ward_count == 0) {
        printf("  No wards to report on.\n");
        return;
    }

    printf("\n================ HOSPITAL REPORT ================\n");

    /* 1. Ward-wise occupancy and vacant beds */
    printf("\n[Ward occupancy]\n");
    printf("%-8s %-18s %-9s %-9s %-7s %-10s\n",
           "WardID", "Department", "Capacity", "Occupied", "Vacant", "Occupancy");
    for (i = 0; i < h->ward_count; i++) {
        const Ward *w = &h->wards[i];
        double pct = 100.0 * w->occupied_beds / w->capacity;
        printf("%-8d %-18s %-9d %-9d %-7d %6.1f%%\n", w->ward_id, w->department,
               w->capacity, w->occupied_beds, w->capacity - w->occupied_beds, pct);
        totalCap += w->capacity;
        totalOcc += w->occupied_beds;
    }

    /* 2. Department statistics (wards sharing a department are combined) */
    printf("\n[Department statistics]\n");
    printf("%-18s %-6s %-9s %-9s %-7s %-8s %-8s\n",
           "Department", "Wards", "Capacity", "Occupied", "Vacant", "Waiting", "AvgSev");
    for (i = 0; i < h->ward_count; i++) {
        const char *dept = h->wards[i].department;
        int wards = 0, cap = 0, occ = 0, wait = 0, cnt = 0, sevSum = 0;

        seen = 0;                                   /* already processed? */
        for (k = 0; k < i; k++)
            if (equalsIgnoreCase(h->wards[k].department, dept)) { seen = 1; break; }
        if (seen)
            continue;

        for (j = 0; j < h->ward_count; j++) {
            if (equalsIgnoreCase(h->wards[j].department, dept)) {
                wards++;
                cap += h->wards[j].capacity;
                occ += h->wards[j].occupied_beds;
            }
        }
        for (j = 0; j < h->patient_count; j++) {
            if (equalsIgnoreCase(h->patients[j].department, dept)) {
                cnt++;
                sevSum += h->patients[j].severity;
                if (h->patients[j].ward_assigned == WAITING)
                    wait++;
            }
        }
        printf("%-18s %-6d %-9d %-9d %-7d %-8d %-8.1f\n", dept, wards, cap, occ,
               cap - occ, wait, cnt ? (double)sevSum / cnt : 0.0);
    }

    /* 3. Hospital totals */
    for (i = 0; i < h->patient_count; i++)
        if (h->patients[i].ward_assigned == WAITING)
            waiting++;

    printf("\n[Hospital totals]\n");
    printf("Total beds     : %d\n", totalCap);
    printf("Occupied beds  : %d\n", totalOcc);
    printf("Vacant beds    : %d\n", totalCap - totalOcc);
    printf("Overall usage  : %.1f%%\n", totalCap ? 100.0 * totalOcc / totalCap : 0.0);
    printf("Patients waiting: %d\n", waiting);
    printf("=================================================\n");
}

/* ------------------------------- Sample data ----------------------------- */

static void loadSampleData(Hospital *h)
{
    addWard(h, 101, 3, "Cardiology");
    addWard(h, 102, 2, "Emergency");
    addWard(h, 103, 4, "Orthopedics");
    addWard(h, 104, 2, "Cardiology");
    printf("  Sample wards loaded.\n");
}

/* ---------------------------------- Menu --------------------------------- */

static void printMenu(void)
{
    printf("\n====== Hospital Bed & Resource Allocation ======\n");
    printf(" 1. Add ward\n");
    printf(" 2. Admit patient\n");
    printf(" 3. Discharge patient\n");
    printf(" 4. List wards\n");
    printf(" 5. List patients\n");
    printf(" 6. Show waiting list (by priority)\n");
    printf(" 7. Generate report\n");
    printf(" 8. Load sample wards\n");
    printf(" 0. Exit\n");
    printf("================================================\n");
}

int main(void)
{
    Hospital hospital;
    int      choice;

    initHospital(&hospital);

    do {
        printMenu();
        choice = readInt("Enter choice: ", 0, 8);
        switch (choice) {
            case 1: addWardInteractive(&hospital); break;
            case 2: admitPatient(&hospital);       break;
            case 3: dischargePatient(&hospital);   break;
            case 4: listWards(&hospital);          break;
            case 5: listPatients(&hospital);       break;
            case 6: listWaiting(&hospital);        break;
            case 7: generateReport(&hospital);     break;
            case 8: loadSampleData(&hospital);     break;
            case 0: printf("Exiting. Goodbye!\n"); break;
        }
    } while (choice != 0);

    freeHospital(&hospital);
    return 0;
}