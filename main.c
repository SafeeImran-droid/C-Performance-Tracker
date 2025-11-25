// main.c (SDL2 GUI version)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "attendance.h"
#include "grades.h"
#include "random_forest.h"
#include "utils.h"
#include "sdl_gui.h"

#define ATTFILE "attendance_db.csv"
#define GRDFILE "grades_db.csv"

CourseAttendance courses[MAX_COURSES];
int course_count = 0;

CourseGrades gcourses[MAX_COURSES];
int gcourse_count = 0;

RandomForest rf_class;
RandomForest rf_reg;

// Forward declarations used by GUI module
void add_attendance_interactive();
void add_grades_interactive();
void show_summary();
void train_and_predict();

int main(int argc, char **argv){
    printf("Starting SDL2 GUI Performance Tracker & Predictor\n");
    // Load existing data if available
    if (file_exists(ATTFILE)) {
        load_attendance_csv(ATTFILE, courses, &course_count);
        printf("Loaded %d attendance courses from %s\n", course_count, ATTFILE);
    }
    if (file_exists(GRDFILE)) {
        load_grades_csv(GRDFILE, gcourses, &gcourse_count);
        printf("Loaded %d grade courses from %s\n", gcourse_count, GRDFILE);
    }

    // initialize RFs
    rf_init(&rf_class, 7, 3, 12345);
    rf_init(&rf_reg, 7, 3, 54321);

    // Initialize SDL GUI
    if (!sdl_gui_init("GUI Performance Tracker and Predictor", 900, 520)){
        fprintf(stderr, "Failed to initialize SDL GUI. Exiting.\n");
        return 1;
    }

    // Enter GUI loop (buttons will call interactive functions)
    sdl_gui_loop();

    // Shutdown
    sdl_gui_shutdown();
    rf_free(&rf_class);
    rf_free(&rf_reg);
    printf("Goodbye.\n");
    return 0;
}

// Interactive functions - reuse the same logic as before (console prompts)
// Note: these functions block the GUI loop while waiting for input (simple approach).
// For non-blocking multi-threaded GUI you'd need threads; this keeps it simple.

void add_attendance_interactive(){
    char code[64];
    int students;
    printf("Enter course code: ");
    if (!fgets(code, sizeof(code), stdin)) return;
    // trim newline
    int L = strlen(code); while (L>0 && (code[L-1]=='\n' || code[L-1]=='\r')) code[--L]=0;
    printf("Number of students: ");
    if (scanf("%d", &students) != 1) { while (getchar()!='\n'); return; }
    while (getchar()!='\n');
    init_course_attendance(&courses[course_count], code, students);
    printf("Recording one day of attendance. Enter 1 for present, 0 for absent for each student.\n");
    int arr[MAX_STUDENTS];
    for (int i=0;i<students;i++){
        printf("Student %d present (1/0): ", i+1);
        if (scanf("%d", &arr[i]) != 1) arr[i] = 0;
        while (getchar() != '\n');
    }
    record_attendance_day(&courses[course_count], 0, arr);
    course_count++;
    printf("Course added.\n");
}

void add_grades_interactive(){
    char code[64];
    int credits, comp;
    printf("Enter course code: ");
    if (!fgets(code, sizeof(code), stdin)) return;
    int L = strlen(code); while (L>0 && (code[L-1]=='\n' || code[L-1]=='\r')) code[--L]=0;
    printf("Credits (integer): ");
    if (scanf("%d", &credits) != 1) { while (getchar()!='\n'); return; }
    printf("Number of components (assignments/exams): ");
    if (scanf("%d", &comp) != 1) { while (getchar()!='\n'); return; }
    while (getchar() != '\n');
    init_course_grades(&gcourses[gcourse_count], code, credits);
    gcourses[gcourse_count].component_count = comp;
    for (int i=0;i<comp;i++){
        printf("Weight for component %d (e.g. 0.2): ", i+1);
        if (scanf("%lf", &gcourses[gcourse_count].weights[i]) != 1) gcourses[gcourse_count].weights[i] = 1.0/comp;
        while (getchar() != '\n');
    }
    for (int i=0;i<comp;i++){
        printf("Score (percentage 0-100) for component %d: ", i+1);
        if (scanf("%lf", &gcourses[gcourse_count].scores[i]) != 1) gcourses[gcourse_count].scores[i] = 0.0;
        while (getchar() != '\n');
    }
    gcourse_count++;
}

void show_summary(){
    printf("\n--- Attendance Summary ---\n");
    for (int i=0;i<course_count;i++){
        printf("Course: %s, Students: %d, Days recorded: %d\n", courses[i].course_code, courses[i].student_count, courses[i].days);
        for (int s=0;s<courses[i].student_count;s++){
            double pct = compute_attendance_pct(&courses[i], s);
            int abs = compute_absences(&courses[i], s);
            printf("  Student %d -> %.2f%%, Absences: %d\n", s+1, pct, abs);
        }
    }
    printf("\n--- Grades Summary ---\n");
    for (int i=0;i<gcourse_count;i++){
        double pct = compute_course_percentage(&gcourses[i]);
        double gpa = percentage_to_gpa(pct);
        printf("Course: %s Credits: %d -> %.2f%% -> GPA: %.2f\n", gcourses[i].course_code, gcourses[i].credits, pct, gpa);
    }
    double cgpa = compute_cgpa(gcourses, gcourse_count);
    printf("Current CGPA (estimated): %.3f\n", cgpa);
}

void train_and_predict(){
    // Build dataset: each student is a sample. For simplicity, use first N students and courses
    int n_samples = 0;
    if (course_count == 0) { printf("No attendance data to train.\n"); return; }
    int chosen_course = 0;
    CourseAttendance *ca = &courses[chosen_course];
    int features = course_count < 10 ? course_count : 10;
    double X[512][32];
    int y_class[512];
    double y_reg[512]; // target CGPA estimation from current grades
    double cgpa = compute_cgpa(gcourses, gcourse_count);
    for (int s=0; s<ca->student_count && s < 512; s++){
        for (int f=0; f<features; f++){
            X[n_samples][f] = compute_attendance_pct(&courses[f], s);
        }
        y_class[n_samples] = (compute_attendance_pct(ca, s) >= 80.0) ? 1 : 0;
        y_reg[n_samples] = cgpa;
        n_samples++;
    }
    if (n_samples == 0) { printf("No student records found.\n"); return; }
    printf("Training RF classifier with %d samples and %d features...\n", n_samples, features);
    rf_train_classification(&rf_class, X, n_samples, features, y_class);
    rf_train_regression(&rf_reg, X, n_samples, features, y_reg);
    double oob_err = rf_oob_classification_error(&rf_class, X, n_samples, features, y_class);
    double oob_rmse = rf_oob_regression_rmse(&rf_reg, X, n_samples, features, y_reg);
    printf("OOB classification error (approx): %.3f\n", oob_err);
    printf("OOB regression RMSE (approx): %.3f\n", oob_rmse);

    printf("\nPredictions per student:\n");
    for (int i=0;i<n_samples;i++){
        int pred_class = rf_predict_classification(&rf_class, X[i], features);
        double pred_reg = rf_predict_regression(&rf_reg, X[i], features);
        printf("Student %d -> Attend>=80? %s, Pred CGPA %.2f\n", i+1, pred_class ? "YES" : "NO", pred_reg);
    }
}
