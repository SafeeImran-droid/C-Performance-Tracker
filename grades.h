#ifndef GRADES_H
#define GRADES_H

#define MAX_COMPONENTS 20
#define MAX_STUDENTS 200   
#define MAX_COURSES 20

typedef struct {
    char course_code[16];

    int component_count;                
    double weights[MAX_COMPONENTS];     

    int student_count;                  
    double scores[MAX_STUDENTS][MAX_COMPONENTS]; 

    int credits;
} CourseGrades;

void init_course_grades(CourseGrades *g, const char *code, int credits);

double compute_course_percentage_student(CourseGrades *g, int student_idx);

double percentage_to_gpa(double pct);

double compute_cgpa_student(CourseGrades courses[], int course_count, int student_idx);

int load_grades_csv(const char *filename, CourseGrades courses[], int *course_count);
int save_grades_csv(const char *filename, CourseGrades courses[], int course_count);

#endif
