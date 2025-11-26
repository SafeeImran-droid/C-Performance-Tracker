// main.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>

#include "attendance.h"
#include "grades.h"
#include "random_forest.h"
#include "gui.h"
#include "utils.h"

#define ATTFILE "attendance_db.csv"
#define GRDFILE "grades_db.csv"

CourseAttendance courses[MAX_COURSES];
int course_count = 0;

CourseGrades gcourses[MAX_COURSES];
int gcourse_count = 0;

RandomForest rf_class;
RandomForest rf_reg;


void console_menu();
void add_attendance_interactive();
void add_grades_interactive();
void show_summary();
void train_and_predict_student();
void view_student_summary();

double compute_course_percentage_student(CourseGrades *g, int student_idx);
double compute_cgpa_student(CourseGrades courses[], int course_count, int student_idx);

void clear_all_data() {

    for (int i = 0; i < course_count; i++) {
        for (int s = 0; s < courses[i].student_count; s++) {
            for (int d = 0; d < courses[i].days; d++) {
                courses[i].matrix[s][d] = 0;
            }
        }
        courses[i].days = 0;
    }
    course_count = 0;

    
    for (int i = 0; i < gcourse_count; i++) {
        for (int s = 0; s < MAX_STUDENTS; s++) {
            for (int c = 0; c < gcourses[i].component_count; c++) {
                gcourses[i].scores[s][c] = 0.0;
            }
        }
        gcourses[i].component_count = 0;
    }
    gcourse_count = 0;

    remove(ATTFILE);
    remove(GRDFILE);

    gui_set_status("Cleared all records.");
    printf("All attendance and grade data cleared.\n");
}

void *console_thread(void *arg){
    console_menu();
    return NULL;
}


int main(int argc, char **argv){
    printf("Starting GUI Performance Tracker & Predictor\n");

    if (file_exists(ATTFILE)) {
        load_attendance_csv(ATTFILE, courses, &course_count);
        printf("Loaded %d attendance courses from %s\n", course_count, ATTFILE);
    }
    if (file_exists(GRDFILE)) {
        load_grades_csv(GRDFILE, gcourses, &gcourse_count);
        printf("Loaded %d grade courses from %s\n", gcourse_count, GRDFILE);
    }

    rf_init(&rf_class, 7, 3, 12345);
    rf_init(&rf_reg, 7, 3, 54321);

    pthread_t tid;
    pthread_create(&tid, NULL, console_thread, NULL);

    gui_init(&argc, &argv);
    gui_mainloop_start();

    rf_free(&rf_class);
    rf_free(&rf_reg);
    return 0;
}

void console_menu(){
    while (1){
        printf("\n--- MENU ---\n");
        printf("1) Add Attendance\n");
        printf("2) Add Grades\n");
        printf("3) Show Summary\n");
        printf("4) Train RF & Predict Student Performance\n");
        printf("5) Save Data\n");
        printf("6) Exit (saves then quit)\n");
        printf("7) Clear all records (attendance + grades)\n");
        printf("9) View summary of a student (attendance + GPA)\n");

        printf("Choose: ");

        int c;
        if (scanf("%d", &c) != 1){
            while (getchar() != '\n');
            continue;
        }
        while (getchar() != '\n');

        switch(c){
            case 1: add_attendance_interactive(); break;
            case 2: add_grades_interactive(); break;
            case 3: show_summary(); break;
            case 4: train_and_predict_student(); break;
            case 5:
                save_attendance_csv(ATTFILE, courses, course_count);
                save_grades_csv(GRDFILE, gcourses, gcourse_count);
                printf("Saved data.\n");
                break;
            case 6:
                save_attendance_csv(ATTFILE, courses, course_count);
                save_grades_csv(GRDFILE, gcourses, gcourse_count);
                printf("Files saved. Exiting.\n");
                exit(0);
            case 7:
                clear_all_data();
                break;
            case 9:
                view_student_summary();
                break;
            default:
                printf("Invalid option.\n");
        }
    }
}


void add_attendance_interactive(){
    char code[32];
    int students;
    printf("Enter course code: ");
    if (!fgets(code, sizeof(code), stdin)) return;
    trim_newline(code);

    printf("Number of students: ");
    if (scanf("%d", &students) != 1) return;
    while(getchar() != '\n');

    if (students <= 0) {
        printf("Invalid number of students.\n");
        return;
    }
    if (students > MAX_STUDENTS) {
        printf("Limiting students to MAX_STUDENTS (%d).\n", MAX_STUDENTS);
        students = MAX_STUDENTS;
    }

    init_course_attendance(&courses[course_count], code, students);
    printf("Recording one day of attendance. Enter 1 for present, 0 for absent for each student.\n");

    int arr[MAX_STUDENTS];
    for (int i=0;i<students;i++){
        printf("Student %d present (1/0): ", i+1);
        if (scanf("%d", &arr[i]) != 1) arr[i] = 0;
        while(getchar() != '\n');
    }
    record_attendance_day(&courses[course_count], 0, arr);
    course_count++;
    printf("Course added.\n");
    gui_set_status("Added course attendance.");
}

void add_grades_interactive(){
    char code[32];
    int credits, comp;
    int students;
    printf("Enter course code: ");
    if(!fgets(code,sizeof(code),stdin)) return;
    trim_newline(code);

    printf("Credits (integer): ");
    if(scanf("%d",&credits)!=1) return;

    printf("Number of students: ");
    if(scanf("%d",&students)!=1) return;

    printf("Number of components (assignments/exams): ");
    if(scanf("%d",&comp)!=1) return;
    while(getchar()!='\n');

    init_course_grades(&gcourses[gcourse_count], code, credits);
    gcourses[gcourse_count].component_count = comp;

    for(int i=0;i<comp;i++){
        printf("Weight for component %d (e.g., 0.2): ",i+1);
        if(scanf("%lf",&gcourses[gcourse_count].weights[i])!=1) 
            gcourses[gcourse_count].weights[i]=1.0/comp;
        while(getchar()!='\n');
    }

    for(int s=0;s<students;s++){
        printf("Entering scores for student %d:\n", s+1);
        for(int i=0;i<comp;i++){
            printf("Score for component %d (0-100): ",i+1);
            if(scanf("%lf",&gcourses[gcourse_count].scores[s][i])!=1)
                gcourses[gcourse_count].scores[s][i]=0.0;
            while(getchar()!='\n');
        }
    }
    gcourse_count++;
    gui_set_status("Added course grades.");
}

void show_summary(){
    printf("\n--- Attendance Summary ---\n");
    for (int i=0;i<course_count;i++){
        printf("Course: %s, Students: %d, Days recorded: %d\n",
               courses[i].course_code, courses[i].student_count, courses[i].days);
        for (int s=0;s<courses[i].student_count;s++){
            double pct = compute_attendance_pct(&courses[i], s);
            int abs = compute_absences(&courses[i], s);
            printf("  Student %d -> %.2f%%, Absences: %d\n", s+1, pct, abs);
        }
    }

    printf("\n--- Grades Summary (per-course) ---\n");
    for (int i=0;i<gcourse_count;i++){
        
        double avg_pct = 0.0;
        int counted = 0;
        for (int s=0; s<MAX_STUDENTS; s++){
            
            int any = 0;
            for (int j=0;j<gcourses[i].component_count;j++){
                if (gcourses[i].scores[s][j] != 0.0) { any = 1; break; }
            }
            if (!any) continue;
            double pct = compute_course_percentage_student(&gcourses[i], s);
            avg_pct += pct;
            counted++;
        }
        if (counted) avg_pct /= counted;
        double gpa = percentage_to_gpa(avg_pct);
        printf("Course: %s Credits: %d -> Avg%%: %.2f -> Avg GPA: %.2f (based on %d students)\n",
               gcourses[i].course_code, gcourses[i].credits, avg_pct, gpa, counted);
    }
}

void train_and_predict_student(){
    if (course_count == 0 || gcourse_count == 0){
        printf("No data available.\n");
        return;
    }

    int course_idx, student_idx;
    printf("Enter course index for prediction (0-%d): ", course_count-1);
    if (scanf("%d", &course_idx) != 1 || course_idx < 0 || course_idx >= course_count) { while(getchar()!='\n'); return; }
    printf("Enter student index (1-%d): ", courses[course_idx].student_count);
    if (scanf("%d", &student_idx) !=1 || student_idx < 1 || student_idx > courses[course_idx].student_count) { while(getchar()!='\n'); return; }
    student_idx--;
    while(getchar()!='\n');

    int n_samples = 0, max_features = 0;
    double X[512][32];
    int y_class[512];
    double y_reg[512];

    // Prepare dataset
    for (int s = 0; s < courses[course_idx].student_count && n_samples < 512; s++){
        int f = 0;
        for (int c = 0; c < course_count && c < 10; c++)
            X[n_samples][f++] = compute_attendance_pct(&courses[c], s);
        for (int c = 0; c < gcourse_count; c++)
            for (int j = 0; j < gcourses[c].component_count; j++)
                X[n_samples][f++] = gcourses[c].scores[s][j]; 

        max_features = f > max_features ? f : max_features;
        y_class[n_samples] = (compute_attendance_pct(&courses[course_idx], s) >= 80.0) ? 1 : 0;
        y_reg[n_samples] = compute_cgpa_student(gcourses, gcourse_count, s);
        n_samples++;
    }

    if (n_samples == 0){ printf("No student records.\n"); return; }

    
    rf_train_classification(&rf_class, X, n_samples, max_features, y_class);
    rf_train_regression(&rf_reg, X, n_samples, max_features, y_reg);

    double X_student[32];
    int f = 0;
    for (int c = 0; c < course_count && c < 10; c++)
        X_student[f++] = compute_attendance_pct(&courses[c], student_idx);
    for (int c = 0; c < gcourse_count; c++)
        for (int j = 0; j < gcourses[c].component_count; j++)
            X_student[f++] = gcourses[c].scores[student_idx][j];

    double att_preds[rf_class.tcount];
    double cgpa_preds[rf_reg.tcount];
    for (int t = 0; t < rf_class.tcount; t++){
        TreeNodeSimple node = rf_class.trees[t].nodes[0];
        att_preds[t] = (X_student[node.feature_index] <= node.threshold) ? node.left_value : node.right_value;

        node = rf_reg.trees[t].nodes[0];
        cgpa_preds[t] = (X_student[node.feature_index] <= node.threshold) ? node.left_value : node.right_value;
    }

    double att_sum = 0, att_sq = 0;
    double cgpa_sum = 0, cgpa_sq = 0;
    for (int t = 0; t < rf_class.tcount; t++){
        att_sum += att_preds[t];
        att_sq += att_preds[t]*att_preds[t];
        cgpa_sum += cgpa_preds[t];
        cgpa_sq += cgpa_preds[t]*cgpa_preds[t];
    }

    double pred_att = att_sum / rf_class.tcount;
    double pred_att_std = sqrt(att_sq/rf_class.tcount - pred_att*pred_att);

    double pred_cgpa = cgpa_sum / rf_reg.tcount;
    double pred_cgpa_std = sqrt(cgpa_sq/rf_reg.tcount - pred_cgpa*pred_cgpa);

    int better_att = 0;
    int better_cgpa = 0;
    for (int s=0; s < courses[course_idx].student_count; s++){
        double att_s = compute_attendance_pct(&courses[course_idx], s);
        double cgpa_s = compute_cgpa_student(gcourses, gcourse_count, s);
        if (pred_att*100 > att_s) better_att++;
        if (pred_cgpa > cgpa_s) better_cgpa++;
    }
    double att_percentile = (double)better_att / courses[course_idx].student_count * 100.0;
    double cgpa_percentile = (double)better_cgpa / courses[course_idx].student_count * 100.0;

    double avg_att = 0, avg_cgpa = 0;
    for (int s=0; s<courses[course_idx].student_count; s++){
        avg_att += compute_attendance_pct(&courses[course_idx], s);
        avg_cgpa += compute_cgpa_student(gcourses, gcourse_count, s);
    }
    avg_att /= courses[course_idx].student_count;
    avg_cgpa /= courses[course_idx].student_count;

    printf("\n--- Student Performance Prediction ---\n");
    printf("Predicted Attendance: %.2f%% (Percentile: %.1f%%)\n", pred_att_std*100, att_percentile);
    printf("Class Average Attendance: %.2f%%\n", avg_att);
    printf("Predicted CGPA: %.2f ; confidence boost : %.2f (Percentile: %.1f%%)\n", pred_cgpa, pred_cgpa_std, cgpa_percentile);
    printf("Class Average CGPA: %.2f\n", avg_cgpa);
    printf("Prediction Complete\n");
    gui_set_status("Prediction complete.");
}


void view_student_summary() {
    if(course_count==0 && gcourse_count==0){
        printf("No attendance or grade courses available.\n");
        return;
    }

    int student_idx;
    printf("Enter student index (1-%d): ", MAX_STUDENTS);
    if(scanf("%d",&student_idx)!=1 || student_idx<1 || student_idx>MAX_STUDENTS){
        while(getchar()!='\n');
        printf("Invalid student index.\n");
        return;
    }
    student_idx--;
    while(getchar()!='\n');

    printf("\n========== STUDENT SUMMARY (ID: %d) ==========\n", student_idx+1);

    if(course_count>0){
        printf("\n--- Attendance ---\n");
        for(int i=0;i<course_count;i++){
            if(student_idx>=courses[i].student_count) continue;
            double pct = compute_attendance_pct(&courses[i],student_idx);
            int absences = compute_absences(&courses[i],student_idx);
            printf("Course: %-12s | Attendance: %6.2f%% | Absences: %d\n",
                courses[i].course_code, pct, absences);
        }
    }

    if(gcourse_count>0){
        printf("\n--- Grades ---\n");
        for(int i=0;i<gcourse_count;i++){
            int any=0;
            for(int j=0;j<gcourses[i].component_count;j++){
                if(gcourses[i].scores[student_idx][j]!=0.0){ any=1; break; }
            }
            if(!any){
                printf("Course: %-12s | No scores recorded.\n", gcourses[i].course_code);
                continue;
            }
            double pct = compute_course_percentage_student(&gcourses[i], student_idx);
            double gpa = percentage_to_gpa(pct);
            printf("Course: %-12s | Avg. Attendance%%: %6.2f | GPA: %.2f\n", gcourses[i].course_code, pct, gpa);
        }
    }

    if(gcourse_count>0){
        double cgpa = compute_cgpa_student(gcourses, gcourse_count, student_idx);
        printf("\nOverall CGPA: %.2f\n", cgpa);
    }

    printf("============================================\n");
    gui_set_status("Displayed student summary.");
}
