#include <stdio.h>
#include <string.h>
#include "attendance.h"
#include "utils.h"

void init_course_attendance(CourseAttendance *c, const char *code, int students){
    strncpy(c->course_code, code, 15);
    c->course_code[15] = 0;
    c->days = 0;
    c->student_count = students;
    for (int i=0;i<students;i++)
        for (int j=0;j<MAX_DAYS;j++)
            c->matrix[i][j] = 0;
}

int record_attendance_day(CourseAttendance *c, int day_index, int present_array[]){
    if (day_index < 0 || day_index >= MAX_DAYS) return 0;
    if (c->days <= day_index) c->days = day_index+1;
    for (int i=0;i<c->student_count;i++){
        c->matrix[i][day_index] = present_array[i] ? 1 : 0;
    }
    return 1;
}

double compute_attendance_pct(CourseAttendance *c, int student_index){
    if (student_index < 0 || student_index >= c->student_count) return 0.0;
    int present = 0;
    for (int d=0; d<c->days; d++) present += c->matrix[student_index][d];
    if (c->days == 0) return 0.0;
    return (100.0 * present) / (double)c->days;
}

int compute_absences(CourseAttendance *c, int student_index){
    if (student_index < 0 || student_index >= c->student_count) return 0;
    int absent = 0;
    for (int d=0; d<c->days; d++) if (!c->matrix[student_index][d]) absent++;
    return absent;
}

// Simple CSV format:
// course_code,student_count,days
// then rows = student_count lines each with days entries 0/1 separated by commas
int load_attendance_csv(const char *filename, CourseAttendance courses[], int *course_count){
    FILE *f = fopen(filename, "r");
    if (!f) return 0;
    char line[4096];
    int idx = 0;
    while (fgets(line, sizeof(line), f)){
        if (line[0] == '#' || strlen(line) < 2) continue;
        char course[32];
        int students, days;
        if (sscanf(line, "%31[^,],%d,%d", course, &students, &days) != 3) continue;
        init_course_attendance(&courses[idx], course, students);
        courses[idx].days = days;
        // read students lines
        for (int s=0; s<students; s++){
            if (!fgets(line, sizeof(line), f)) break;
            trim_newline(line);
            int j = 0;
            char *tok = strtok(line, ",");
            while (tok && j < days && j < MAX_DAYS){
                courses[idx].matrix[s][j] = atoi(tok) ? 1 : 0;
                tok = strtok(NULL, ",");
                j++;
            }
        }
        idx++;
        if (idx >= MAX_COURSES) break;
    }
    fclose(f);
    *course_count = idx;
    return 1;
}

int save_attendance_csv(const char *filename, CourseAttendance courses[], int course_count){
    FILE *f = fopen(filename, "w");
    if (!f) return 0;
    for (int i=0;i<course_count;i++){
        fprintf(f, "%s,%d,%d\n", courses[i].course_code, courses[i].student_count, courses[i].days);
        for (int s=0;s<courses[i].student_count;s++){
            for (int d=0; d<courses[i].days; d++){
                fprintf(f, "%d", courses[i].matrix[s][d] ? 1 : 0);
                if (d+1 < courses[i].days) fprintf(f, ",");
            }
            fprintf(f, "\n");
        }
    }
    fclose(f);
    return 1;
}
